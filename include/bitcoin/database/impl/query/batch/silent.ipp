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
    const system::silent::batch::receiver& keys,
    const silent_handler& callback) NOEXCEPT
{
    return scan_silent(cancel, keys, zero, store_.silent.count(), callback);
}

// Rows [first, last) must begin and end on transaction boundaries.
TEMPLATE
bool CLASS::scan_silent(const stopper& cancel,
    const system::silent::batch::receiver& keys, size_t first, size_t last,
    const silent_handler& callback) NOEXCEPT
{
    const auto prefix_ptr = store_.silent.prefix.get_memory();
    const auto compressed_ptr = store_.silent.compressed.get_memory();
    const auto correlate_ptr = store_.silent.correlate.get_memory();

    using correlate_t = const table::silent_correlate::span;
    using prefix_t = const table::silent_prefix::span;
    using compressed_t = const table::silent_compressed::span;

    using namespace system;
    const auto correlate = pointer_cast<correlate_t>(correlate_ptr.data());
    const auto prefix = pointer_cast<prefix_t>(prefix_ptr.data());
    const auto compressed = pointer_cast<compressed_t>(compressed_ptr.data());

    BC_ASSERT(first <= last && last <= store_.silent.count());
    const auto count = last - first;
    const silent::batch batch
    {
        .correlates = { std::next(correlate, first), count },
        .prefixes = { std::next(prefix, first), count },
        .points = { std::next(compressed, first), count }
    };

    // False return only implies canceled.
    // Callbacks invoked on caller thread if turbo is false.
    silent::batch::scan(cancel, batch, keys, callback, store_.turbo());
    return !cancel;
}

// Rows are allocated zero-filled and a nonzero correlate publishes the row.
TEMPLATE
size_t CLASS::get_silent_frontier(size_t first) const NOEXCEPT
{
    using namespace system;
    using word_t = table::silent_correlate::tx::integer;
    const auto guard = store_.silent.guard();
    const auto words = pointer_cast<word_t>(guard.data());
    const auto count = store_.silent.count();

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

    stopper fail{};
    std::vector<size_t> it(sub1(count));
    std::iota(it.begin(), it.end(), one);
    constexpr auto parallel = poolstl::execution::par;
    constexpr auto relaxed = std::memory_order_relaxed;

    // TODO: parallel may or may not be optimal.
    // TODO: alternatively could accumulate block results and write once.
    std::for_each(parallel, it.cbegin(), it.cend(), [&](size_t index) NOEXCEPT
    {
        if (fail.load(relaxed))
            return;

        const auto& fk = links.at(index);
        if (fk >= first && !set_silent(fk, *txs->at(index)))
            fail.store(true, relaxed);
    });
    
    return !fail.load(relaxed);
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

    stopper fail{};
    std::vector<size_t> it(sub1(count));
    std::iota(it.begin(), it.end(), one);
    constexpr auto parallel = poolstl::execution::par;
    constexpr auto relaxed = std::memory_order_relaxed;

    std::for_each(parallel, it.cbegin(), it.cend(), [&](size_t index) NOEXCEPT
    {
        if (fail.load(relaxed))
            return;

        const auto& fk = links.at(index);
        if (fk >= first && !set_silent(fk, txs.at(index)))
            fail.store(true, relaxed);
    });

    return !fail.load(relaxed);
}

// Ineligible txs have no records.
TEMPLATE
bool CLASS::set_silent(const tx_link& link, const transaction& tx) NOEXCEPT
{
    using namespace system::wallet;
    ec_compressed summary{};
    silent_payment::scan_outputs outputs{};
    return !silent_payment::get_outputs(outputs, tx)
        || !silent_payment::summarize(summary, tx)
        || set_silent_(link, summary, outputs);
}

TEMPLATE
bool CLASS::set_silent(const tx_link& link,
    const transaction_view& tx) NOEXCEPT
{
    using namespace system::wallet;
    ec_compressed summary{};
    silent_payment::scan_outputs outputs{};
    return !silent_payment::get_outputs(outputs, tx)
        || !silent_payment::summarize(summary, tx)
        || set_silent_(link, summary, outputs);
}

// protected
TEMPLATE
bool CLASS::set_silent_(const tx_link& link, const ec_compressed& summary,
    const system::wallet::silent_payment::scan_outputs& outputs) NOEXCEPT
{
    if (link.is_terminal())
        return false;

    // The prefix is ec_xonly[0..7] read as little-endian.
    using namespace system;
    using prefix_t = table::silent_prefix::integral;
    std::vector<prefix_t> prefixes(outputs.size());
    std::transform(outputs.cbegin(), outputs.cend(), prefixes.begin(),
        [](const auto& output) NOEXCEPT
        {
            return unsafe_from_little_endian<prefix_t>(output.key.data());
        });

    using prefixes_t = table::silent_prefix::put_ref;
    using compressed_t = table::silent_compressed::put_ref;

    // ========================================================================
    const auto scope = get_transactor();
    auto rows = possible_narrow_cast<silent_link::integer>(prefixes.size());

    // Allocate rows across all columns.
    const auto fk = store_.silent.allocate(rows);
    if (fk.is_terminal())
        return false;

    // Guard against remap (required for nomaps::put(fk)).
    const auto guard = store_.silent.guard();

    // Write values to each column in corresponding positions.
    if (!store_.silent.prefix.put(fk, prefixes_t{ {}, prefixes }) ||
        !store_.silent.compressed.put(fk, compressed_t{ {}, rows, summary }))
        return false;

    // The guard is the correlate column, published last (get_silent_frontier).
    using word_t = table::silent_correlate::tx::integer;
    static_assert(schema::silent_correlate::minrow == sizeof(word_t));
    const auto words = pointer_cast<word_t>(guard.data());
    const auto value = native_to_little_end(link.value);

    for (auto row = fk.value; row < fk.value + rows; ++row)
    {
        std::atomic_ref<word_t> word{ *std::next(words, row) };
        word.store(value, std::memory_order_release);
    }

    return true;
    // ========================================================================
}

} // namespace database
} // namespace libbitcoin

#endif
