/**
 * Copyright (c) 2011-2026 libbitcoin developers
 *
 * This file is part of libbitcoin.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef LIBBITCOIN_DATABASE_QUERY_BATCH_SILENT_IPP
#define LIBBITCOIN_DATABASE_QUERY_BATCH_SILENT_IPP

#include <bitcoin/database/define.hpp>
#include <bitcoin/database/tables/tables.hpp>
#include <bitcoin/database/types/types.hpp>

namespace libbitcoin {
namespace database {

TEMPLATE
bool CLASS::scan_silent(const stopper& cancel,
    const system::scan::batch::receiver& keys,
    const silent_handler& callback) NOEXCEPT
{
    return scan_silent(cancel, keys, zero, store_.scan.count(), callback);
}

// Rows [first, last) must begin and end on transaction boundaries.
TEMPLATE
bool CLASS::scan_silent(const stopper& cancel,
    const system::scan::batch::receiver& keys, size_t first, size_t last,
    const silent_handler& callback) NOEXCEPT
{
    const auto correlate_ptr = store_.scan.correlate.get_memory();
    const auto row_ptr = store_.scan.row.get_memory();

    using correlate_t = const table::scan_correlate::span;
    using row_t = const system::scan::batch::row_t;

    using namespace system;
    const auto correlate = pointer_cast<correlate_t>(correlate_ptr.data());
    const auto row = pointer_cast<row_t>(row_ptr.data());

    BC_ASSERT(first <= last && last <= store_.scan.count());
    const auto count = last - first;
    const system::scan::batch batch
    {
        .correlates = { std::next(correlate, first), count },
        .rows = { std::next(row, first), count }
    };

    // False return only implies canceled.
    // Callbacks invoked on caller thread if turbo is false.
    system::scan::batch::scan(cancel, batch, keys, callback, store_.turbo());
    return !cancel;
}

// Rows are allocated zero-filled and a nonzero correlate publishes the row.
TEMPLATE
size_t CLASS::get_silent_frontier(size_t first) const NOEXCEPT
{
    using namespace system;
    using word_t = table::scan_correlate::tx::integer;
    const auto guard = store_.scan.guard();
    const auto words = pointer_cast<word_t>(guard.data());
    const auto count = store_.scan.count();

    for (auto row = first; row < count; ++row)
    {
        std::atomic_ref<word_t> word{ *std::next(words, row) };
        if (is_zero(word.load(std::memory_order_acquire)))
            return row;
    }

    return count;
}

// setters
// ----------------------------------------------------------------------------
// Caller (node) controls which txs are indexed (e.g. by confirmed height).
// The coinbase is the first tx archived for a block, so txs linked below it
// were archived (and indexed) before it, as pooled or by another block.

TEMPLATE
bool CLASS::set_silent(const header_link& link, const block& block) NOEXCEPT
{
    const auto& txs = block.transactions_ptr();
    const auto count = txs->size();
    if (is_one(count))
        return true;

    const auto links = to_transactions(link);
    if (links.size() != count)
        return false;

    const auto first = links.front();
    for (auto index = one; index < count; ++index)
    {
        const auto& fk = links.at(index);
        if (fk >= first && !set_silent(fk, *txs->at(index)))
            return false;
    }

    return true;
}

TEMPLATE
bool CLASS::set_silent(const header_link& link,
    const block_view& block) NOEXCEPT
{
    const auto& txs = block.views();
    const auto count = txs.size();
    if (is_one(count))
        return true;

    const auto links = to_transactions(link);
    if (links.size() != count)
        return false;

    const auto first = links.front();
    for (auto index = one; index < count; ++index)
    {
        const auto& fk = links.at(index);
        if (fk >= first && !set_silent(fk, txs.at(index)))
            return false;
    }

    return true;
}

// Ineligible txs have no records.
TEMPLATE
bool CLASS::set_silent(const tx_link& link, const transaction& tx) NOEXCEPT
{
    using namespace system::wallet;
    ec_compressed point{};
    std::vector<silent_prefix> prefixes{};
    return !get_prefixes(prefixes, tx)
        || !silent_payment::summarize(point, tx)
        || set_silent_(link, point, prefixes);
}

TEMPLATE
bool CLASS::set_silent(const tx_link& link,
    const transaction_view& tx) NOEXCEPT
{
    using namespace system::wallet;
    ec_compressed point{};
    std::vector<silent_prefix> prefixes{};
    return !get_prefixes(prefixes, tx)
        || !silent_payment::summarize(point, tx)
        || set_silent_(link, point, prefixes);
}

// batch
// ----------------------------------------------------------------------------

TEMPLATE
size_t CLASS::silent_records(bool bank) const NOEXCEPT
{
    return store_.silent_bank(bank).count();
}

TEMPLATE
bool CLASS::purge_silents(bool bank) NOEXCEPT
{
    // ========================================================================
    const auto scope = get_transactor();
    return store_.silent_bank(bank).truncate(0);
    // ========================================================================
}

TEMPLATE
bool CLASS::set_silents(size_t& rows, const header_link& link,
    const block& block, bool bank) NOEXCEPT
{
    return set_silents_(rows, link, *block.transactions_ptr(), bank);
}

TEMPLATE
bool CLASS::set_silents(size_t& rows, const header_link& link,
    const block_view& block, bool bank) NOEXCEPT
{
    return set_silents_(rows, link, block.views(), bank);
}

// The bank is read and computed under its guard alone, then released before
// records are set, as no two tables are guarded at once.
TEMPLATE
code CLASS::compute_silents(const stopper& cancel, bool bank) NOEXCEPT
{
    struct computed
    {
        tx_link link{};
        bool valid{};
        ec_compressed point{};
        std::vector<silent_prefix> prefixes{};
    };

    using namespace system;
    std::vector<computed> txs{};
    {
        auto& bank_table = store_.silent_bank(bank);
        const auto count = possible_narrow_cast<size_t>(
            bank_table.count().value);
        if (is_zero(count))
            return error::success;

        const auto correlate_ptr = bank_table.correlate.get_memory();
        const auto row_ptr = bank_table.row.get_memory();

        using correlate_t = const table::silent_correlate::span;
        using row_t = const silent::batch::row_t;
        const auto correlates = pointer_cast<correlate_t>(
            correlate_ptr.data());
        const auto rows = pointer_cast<row_t>(row_ptr.data());

        std_vector<ec_compressed> points{};
        data_chunk valid{};
        const silent::batch batch{ { rows, count } };
        if (!silent::batch::compute(points, valid, cancel, batch))
            return error::query_canceled;

        for (size_t row{}; row < count; ++row)
        {
            const auto& correlate = *std::next(correlates, row);
            if (is_zero(row) || correlate != *std::next(correlates, sub1(row)))
            {
                txs.push_back({ tx_link{ correlate }, is_nonzero(valid[row]),
                    points[row] });
            }

            const auto& prefix = std::next(rows, row)->prefix;
            txs.back().prefixes.push_back(
                unsafe_from_little_endian<silent_prefix>(prefix.data()));
        }
    }

    for (const auto& tx: txs)
        if (tx.valid && !set_silent_(tx.link, tx.point, tx.prefixes))
            return error::integrity;

    return error::success;
}

// protected
// ----------------------------------------------------------------------------

// Txs linked below the coinbase were indexed before it (see set_silent).
TEMPLATE
template <typename Transactions>
bool CLASS::set_silents_(size_t& rows, const header_link& link,
    const Transactions& txs, bool bank) NOEXCEPT
{
    struct capture
    {
        tx_link link{};
        ec_compressed sum{};
        ec_secret hash{};
        std::vector<silent_prefix> prefixes{};
    };

    rows = zero;
    const auto count = txs.size();
    if (is_one(count))
        return true;

    const auto links = to_transactions(link);
    if (links.size() != count)
        return false;

    // Object txs are held by pointer, views by value.
    const auto to_tx = [](const auto& tx) NOEXCEPT -> decltype(auto)
    {
        using type = std::decay_t<decltype(tx)>;
        if constexpr (is_same_type<type, transaction_view>)
            return (tx);
        else
            return (*tx);
    };

    using namespace system::wallet;
    std::vector<capture> captures{};
    const auto first = links.front();
    for (auto index = one; index < count; ++index)
    {
        capture item{ links.at(index) };
        const auto& tx = to_tx(txs.at(index));
        if (item.link >= first && get_prefixes(item.prefixes, tx) &&
            silent_payment::prepare(item.sum, item.hash, tx))
        {
            rows += item.prefixes.size();
            captures.push_back(std::move(item));
        }
    }

    if (is_zero(rows))
        return true;

    using row_t = table::silent_row::put_ref;
    using correlate_t = table::silent_correlate::records;

    // ========================================================================
    const auto scope = get_transactor();
    auto& bank_table = store_.silent_bank(bank);

    // Allocate all of the block's rows across all columns.
    using namespace system;
    const auto fk = bank_table.allocate(
        possible_narrow_cast<silent_link::integer>(rows));
    if (fk.is_terminal())
        return false;

    // Guard against remap (required for nomaps::put(fk)).
    const auto guard = bank_table.guard();

    auto row = fk;
    for (const auto& item: captures)
    {
        const auto size = item.prefixes.size();
        if (!bank_table.row.put(row,
                row_t{ {}, item.prefixes, item.sum, item.hash }) ||
            !bank_table.correlate.put(row,
                correlate_t{ {}, size, item.link.value }))
            return false;

        row += size;
    }

    return true;
    // ========================================================================
}

// The prefix is ec_xonly[0..7] read as little-endian.
TEMPLATE
template <typename Transaction>
bool CLASS::get_prefixes(std::vector<silent_prefix>& out,
    const Transaction& tx) NOEXCEPT
{
    using namespace system;
    using namespace system::wallet;
    silent_payment::scan_outputs outputs{};
    if (!silent_payment::get_outputs(outputs, tx))
        return false;

    out.resize(outputs.size());
    std::transform(outputs.cbegin(), outputs.cend(), out.begin(),
        [](const auto& output) NOEXCEPT
        {
            return unsafe_from_little_endian<silent_prefix>(output.key.data());
        });

    return true;
}

TEMPLATE
bool CLASS::set_silent_(const tx_link& link, const ec_compressed& point,
    const std::vector<silent_prefix>& prefixes) NOEXCEPT
{
    if (link.is_terminal())
        return false;

    using namespace system;
    using row_t = table::scan_row::put_ref;

    // ========================================================================
    const auto scope = get_transactor();
    auto rows = possible_narrow_cast<scan_link::integer>(prefixes.size());

    // Allocate rows across all columns.
    const auto fk = store_.scan.allocate(rows);
    if (fk.is_terminal())
        return false;

    // Guard against remap (required for nomaps::put(fk)).
    const auto guard = store_.scan.guard();

    if (!store_.scan.row.put(fk, row_t{ {}, prefixes, point }))
        return false;

    // The guard is the correlate column, published last (get_silent_frontier).
    using word_t = table::scan_correlate::tx::integer;
    static_assert(schema::scan_correlate::minrow == sizeof(word_t));
    const auto words = pointer_cast<word_t>(guard.data());
    const auto value = native_to_little_end(link.value);

    for (auto row = fk.value; row < fk.value + rows; ++row)
    {
        std::atomic_ref<word_t> word{ *std::next(words, row) };
        word.store(value, std::memory_order_release);
    }

    store_.scan.complete(fk, rows);
    return true;
    // ========================================================================
}

} // namespace database
} // namespace libbitcoin

#endif
