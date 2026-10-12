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

#include <future>
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

// Transactions [first, last) are scanned against all output prefixes.
TEMPLATE
bool CLASS::scan_silent(const stopper& cancel,
    const system::scan::batch::receiver& keys, size_t first, size_t last,
    const silent_handler& callback) NOEXCEPT
{
    const auto correlate_ptr = store_.scan.correlate.get_memory();
    const auto point_ptr = store_.scan.point.get_memory();
    const auto outputs_ptr = store_.scan.outputs.get_memory();
    const auto prefix_ptr = store_.scan_prefix.get_memory();

    using correlate_t = const table::scan_correlate::span;
    using point_t = const table::scan_point::span;
    using outputs_t = const system::scan::batch::outputs_t;
    using prefix_t = const system::scan::batch::prefix;

    using namespace system;
    const auto correlate = pointer_cast<correlate_t>(correlate_ptr.data());
    const auto point = pointer_cast<point_t>(point_ptr.data());
    const auto outputs = pointer_cast<outputs_t>(outputs_ptr.data());
    const auto prefix = pointer_cast<prefix_t>(prefix_ptr.data());

    BC_ASSERT(first <= last && last <= store_.scan.count());
    const auto count = last - first;
    const auto prefixes = possible_narrow_cast<size_t>(
        store_.scan_prefix.count().value);

    const system::scan::batch batch
    {
        .correlates = { std::next(correlate, first), count },
        .points = { std::next(point, first), count },
        .outputs = { std::next(outputs, first), count },
        .prefixes = { prefix, prefixes }
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

TEMPLATE
scan_link CLASS::get_scan_start(const header_link& link) const NOEXCEPT
{
    return store_.scan.at(to_scan_start(link));
}

TEMPLATE
bool CLASS::set_scan_start(const header_link& link,
    const scan_link& start) NOEXCEPT
{
    // ========================================================================
    const auto scope = get_transactor();
    return store_.scan.push(start, to_scan_start(link));
    // ========================================================================
}

// A block's rows link above its coinbase, so the first row linked above the
// lowest coinbase at or above a height starts the rows of blocks at or above
// it. Rows of blocks archived after a mapped ancestor link above its coinbase
// and so follow its start, otherwise the seek is from the first row. Headers
// are written top down, so genesis is mapped only by a complete map.
TEMPLATE
bool CLASS::set_scan_starts(const stopper& cancel, const header_link& top,
    size_t bound, bool remap) NOEXCEPT
{
    using namespace system;
    auto link = top;
    auto floor = tx_link::terminal;
    auto lowest = tx_link::terminal;
    std::vector<std::pair<header_link, tx_link::integer>> floors{};
    while (!link.is_terminal() && (remap || get_scan_start(link).is_terminal()))
    {
        const auto coinbase = to_coinbase(link);
        if (coinbase.is_terminal())
            return false;

        lowest = std::min(lowest, coinbase.value);
        floor = std::min(floor, coinbase.value);
        floors.emplace_back(link, floor);
        link = to_parent(link);
    }

    size_t row{};
    if (!link.is_terminal() && lowest > to_coinbase(link).value)
        row = get_scan_start(link).value;

    auto last = std::min(bound, size_t{ store_.scan.count().value });
    std::vector<scan_link::integer> starts(floors.size());
    auto start = starts.rbegin();
    for (auto it = floors.crbegin(); it != floors.crend(); ++it, ++start)
    {
        if (cancel)
            return false;

        seek_scan_start(row, last, it->second);
        *start = possible_narrow_cast<scan_link::integer>(row);
    }

    for (size_t index{}; index < floors.size(); ++index)
        if (cancel || !set_scan_start(floors.at(index).first, starts.at(index)))
            return false;

    return true;
}
TEMPLATE
scan_link CLASS::get_scan_floor(size_t height) const NOEXCEPT
{
    auto index = std::min(height, get_top_confirmed());
    while (true)
    {
        const auto start = get_scan_start(to_confirmed(index));
        if (!start.is_terminal())
            return start;

        if (is_zero(index))
            return scan_link{ 0 };

        --index;
    }
}

// setters
// ----------------------------------------------------------------------------
// Caller (node) controls which txs are indexed (e.g. by confirmed height).
// The coinbase is the first tx archived for a block, so txs linked below it
// were archived before it, as pooled (and indexed) or by another block (and
// indexed only if that block was), so a tx below it is indexed if not pooled.

TEMPLATE
bool CLASS::set_silent(const header_link& link, const block& block) NOEXCEPT
{
    return set_silent_(link, *block.transactions_ptr());
}

TEMPLATE
bool CLASS::set_silent(const header_link& link,
    const block_view& block) NOEXCEPT
{
    return set_silent_(link, block.views());
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
size_t CLASS::presilent_records(bool bank) const NOEXCEPT
{
    return store_.presilent_bank(bank).count();
}

TEMPLATE
header_links CLASS::get_presilents(bool bank) const NOEXCEPT
{
    // Get all records, starting at the first position.
    const auto all = presilent_records(bank);
    const table::presilent::link first{ 0 };

    header_links links(all);
    table::presilent::get_refs out{ {}, links };
    if (store_.presilent_bank(bank).get(first, out))
        return links;

    // Empty return in case of fault is an efficiency loss, table is dropped.
    return {};
}

TEMPLATE
bool CLASS::purge_presilents(bool bank) NOEXCEPT
{
    // ========================================================================
    const auto scope = get_transactor();
    return store_.presilent_bank(bank).truncate(0);
    // ========================================================================
}

TEMPLATE
bool CLASS::set_presilent(const header_link& link, bool bank) NOEXCEPT
{
    // ========================================================================
    const auto scope = get_transactor();

    // Clean single allocation failure (e.g. disk full).
    return store_.presilent_bank(bank).put(table::presilent::record{ {}, link });
    // ========================================================================
}

TEMPLATE
bool CLASS::set_presilents(const header_links& links, bool bank) NOEXCEPT
{
    // ========================================================================
    const auto scope = get_transactor();

    // Clean single allocation failure (e.g. disk full).
    return store_.presilent_bank(bank).put(
        table::presilent::put_refs{ {}, links });
    // ========================================================================
}

TEMPLATE
bool CLASS::purge_silents(bool bank) NOEXCEPT
{
    // ========================================================================
    const auto scope = get_transactor();
    return store_.silent_bank(bank).truncate(0) &&
        store_.silent_prefix_bank(bank).truncate(0);
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

// Pooled txs linked below the coinbase are not selected (see set_silent).
TEMPLATE
bool CLASS::get_silent_prevouts(data_chunk& prevouts,
    std::vector<bool>& selected, const header_link& link,
    const block_view& block) const NOEXCEPT
{
    using namespace system;
    const auto& txs = block.views();
    const auto links = to_transactions(link);
    if (links.empty() || links.size() != txs.size())
        return false;

    selected.assign(txs.size(), false);
    stream::out::data ostream(prevouts);
    write::bytes::ostream sink(ostream);

    const auto first = links.front();
    std::vector<silent_prefix> prefixes{};
    for (auto index = one; index < txs.size(); ++index)
    {
        if (!is_silent_selected(links.at(index), first) ||
            !get_prefixes(prefixes, txs.at(index)))
            continue;

        table::transaction::get_puts record{};
        if (!store_.tx.get(links.at(index), record))
            return false;

        // Point links are contiguous (computed).
        const auto end = record.points_fk + record.ins_count;
        for (auto fk = record.points_fk; fk < end; ++fk)
        {
            // Any instance of the parent hash yields the same output.
            const auto point = get_point_key(fk);
            const auto parent = to_tx(point.hash());
            table::transaction::get_output parent_tx{ {}, point.index() };
            table::outs::get_output outs{};
            if (parent.is_terminal() || !store_.tx.get(parent, parent_tx) ||
                parent_tx.outs_fk == table::transaction::outs::terminal ||
                !store_.outs.puts.get(parent_tx.outs_fk, outs) ||
                !get_wire_output(sink, outs.out_fk))
                return false;
        }

        selected.at(index) = true;
    }

    sink.flush();
    return !!sink;
}

// The bank is quiescent (drain claimed, no writers), so it is read across the
// scan write, which is sourced directly from its columns. Scan rows are
// allocated for every bank tx and prefix, so the prefixes and outputs are
// written in parallel while the points compute on the device (or after they
// compute on the processor), and each tx is then published by its correlate
// (an invalid tx with no outputs).
TEMPLATE
code CLASS::compute_silents(const stopper& cancel, bool& device,
    bool bank) NOEXCEPT
{
    using namespace system;
    auto& bank_table = store_.silent_bank(bank);
    auto& bank_prefix = store_.silent_prefix_bank(bank);
    const auto txs = possible_narrow_cast<size_t>(bank_table.count().value);
    if (is_zero(txs))
    {
        // ====================================================================
        const auto scope = get_transactor();
        return set_scanned_(bank) ? error::success : error::integrity;
        // ====================================================================
    }

    using prefix_t = const scan::batch::prefix;
    using span_t = table::scan_prefix::put_span;
    using sum_t = const table::silent_sum::span;
    using hash_t = const table::silent_hash::span;
    using outputs_t = const scan::batch::outputs_t;
    using correlate_t = const table::silent_correlate::span;
    using word_t = table::scan_correlate::tx::integer;

    // ========================================================================
    const auto scope = get_transactor();

    const auto fk = store_.scan.allocate(
        possible_narrow_cast<scan_link::integer>(txs));
    const auto pk = store_.scan_prefix.allocate(bank_prefix.count());
    if (fk.is_terminal() || pk.is_terminal())
        return error::integrity;

    // Guard against remap (required for nomaps::put(fk)).
    const auto guard = store_.scan.guard();
    const auto point_ptr = store_.scan.point.get_memory();
    const auto prefix_ptr = bank_prefix.get_memory();
    const auto sum_ptr = bank_table.sum.get_memory();
    const auto hash_ptr = bank_table.hash.get_memory();
    const auto outputs_ptr = bank_table.outputs.get_memory();
    const auto correlate_ptr = bank_table.correlate.get_memory();

    const auto scan_prefix_ptr = store_.scan_prefix.get_memory();
    const auto prefixes = pointer_cast<prefix_t>(prefix_ptr.data());
    const auto words = pointer_cast<word_t>(guard.data());
    const auto sums = pointer_cast<sum_t>(sum_ptr.data());
    const auto hashes = pointer_cast<hash_t>(hash_ptr.data());
    const auto outputs = pointer_cast<outputs_t>(outputs_ptr.data());
    const auto correlates = pointer_cast<correlate_t>(correlate_ptr.data());

    // Points are computed directly into the allocated scan rows.
    const std::span<ec_compressed> points
    {
        std::next(pointer_cast<ec_compressed>(point_ptr.data()), fk.value), txs
    };

    data_chunk valid(txs);
    const silent::batch batch{ { sums, txs }, { hashes, txs } };
    const auto compute = [&]() NOEXCEPT
    {
        return silent::batch::compute(points, valid, device, cancel, batch);
    };

    // The device computes concurrently with the write, the processor before.
    bool success{};
    std::future<bool> computed{};
    const auto accelerated = batched::accelerated();
    if (accelerated)
        computed = std::async(std::launch::async, compute);
    else
        success = compute();

    using scan_int = scan_link::integer;
    using prefix_int = prefix_link::integer;
    using outputs_record = table::scan_outputs::record;

    // Bank prefixes are contiguous, so scan prefix rows are at the same
    // offsets from the allocation, and each tx writes only its own.
    std::atomic_bool failed{};
    std_vector<size_t> indexes(txs);
    std::iota(indexes.begin(), indexes.end(), zero);
    std::for_each(poolstl::execution::par, indexes.cbegin(), indexes.cend(),
        [&](size_t tx) NOEXCEPT
        {
            const auto& output = *std::next(outputs, tx);
            const auto first = from_little_endian<uint64_t>(output.first);
            const auto count = from_little_endian<uint32_t>(output.count);
            const auto link = possible_narrow_cast<scan_int>(fk.value + tx);
            const auto row = possible_narrow_cast<prefix_int>(pk.value + first);
            const span_t span{ {}, { std::next(prefixes, first), count } };
            const outputs_record record{ {}, row, count };

            if (!store_.scan_prefix.put(scan_prefix_ptr, row, span) ||
                !store_.scan.outputs.put(link, record))
                failed.store(true);
        });

    if (accelerated)
        success = computed.get();

    if (!success)
        return error::query_canceled;

    if (failed.load())
        return error::integrity;

    // The guard is the correlate column, published last (get_silent_frontier).
    static_assert(schema::scan_correlate::minrow == sizeof(word_t));
    for (size_t tx{}; tx < txs; ++tx)
    {
        const auto link = possible_narrow_cast<scan_int>(fk.value + tx);
        if (is_zero(valid[tx]))
        {
            points[tx] = {};
            const auto& output = *std::next(outputs, tx);
            const auto first = from_little_endian<uint64_t>(output.first);
            const auto row = possible_narrow_cast<prefix_int>(pk.value + first);
            if (!store_.scan.outputs.put(link, outputs_record{ {}, row, zero }))
                return error::integrity;
        }

        const tx_link correlate{ *std::next(correlates, tx) };
        std::atomic_ref<word_t> word{ *std::next(words, link) };
        word.store(native_to_little_end(correlate.value),
            std::memory_order_release);
    }

    store_.scan.complete(fk, possible_narrow_cast<scan_int>(txs));
    return set_scanned_(bank) ? error::success : error::integrity;
    // ========================================================================
}

// protected
// ----------------------------------------------------------------------------

// Ineligible txs have no records. The block's records and its scanned state
// are written in one transactor scope, and once both allocations succeed the
// writes cannot fail.
TEMPLATE
template <typename Transactions>
bool CLASS::set_silent_(const header_link& link,
    const Transactions& txs) NOEXCEPT
{
    struct capture
    {
        tx_link link{};
        ec_compressed point{};
        std::vector<silent_prefix> prefixes{};
    };

    const auto count = txs.size();
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
    size_t rows{};
    std::vector<capture> captures{};
    const auto first = links.front();
    for (auto index = one; index < count; ++index)
    {
        capture item{ links.at(index) };
        const auto& tx = to_tx(txs.at(index));
        if (is_silent_selected(item.link, first) &&
            get_prefixes(item.prefixes, tx) &&
            silent_payment::summarize(item.point, tx))
        {
            rows += item.prefixes.size();
            captures.push_back(std::move(item));
        }
    }

    using namespace system;
    using word_t = table::scan_correlate::tx::integer;
    using point_t = table::scan_point::put_ref;
    using prefix_t = table::scan_prefix::put_refs;
    const table::state::merge scanned{ {}, block_state::scanned };

    // ========================================================================
    const auto scope = get_transactor();

    if (is_zero(rows))
        return store_.state.set(to_state(link), scanned);

    // Allocate the block's outputs, then its txs across all columns. Outputs
    // are reached only through txs, so failure strands no unwritten tx.
    const auto pk = store_.scan_prefix.allocate(
        possible_narrow_cast<prefix_link::integer>(rows));
    if (pk.is_terminal())
        return false;

    const auto fk = store_.scan.allocate(
        possible_narrow_cast<scan_link::integer>(captures.size()));
    if (fk.is_terminal())
        return false;

    // Guard against remap (required for nomaps::put(fk)).
    const auto guard = store_.scan.guard();
    const auto words = pointer_cast<word_t>(guard.data());

    auto tx = fk;
    auto out = pk;
    for (const auto& item: captures)
    {
        const auto outputs = item.prefixes.size();
        if (!store_.scan_prefix.put(out, prefix_t{ {}, item.prefixes }) ||
            !store_.scan.point.put(tx, point_t{ {}, item.point }) ||
            !put_silent_(tx, item.link, out, outputs, words))
            return false;

        tx += one;
        out += outputs;
    }

    store_.scan.complete(fk,
        possible_narrow_cast<scan_link::integer>(captures.size()));
    return store_.state.set(to_state(link), scanned);
    // ========================================================================
}

// Caller holds the transactor.
TEMPLATE
bool CLASS::set_scanned_(bool bank) NOEXCEPT
{
    for (const auto& link: get_presilents(bank))
        if (!store_.state.set(to_state(link),
            table::state::merge{ {}, block_state::scanned }))
            return false;

    return true;
}

// Pooled txs linked below the coinbase were indexed (see set_silent).
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
        return set_block_scanned(link);

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
        if (is_silent_selected(item.link, first) &&
            get_prefixes(item.prefixes, tx) &&
            silent_payment::prepare(item.sum, item.hash, tx))
        {
            rows += item.prefixes.size();
            captures.push_back(std::move(item));
        }
    }

    if (is_zero(rows))
        return set_block_scanned(link);

    using correlate_t = table::silent_correlate::record;
    using sum_t = table::silent_sum::put_ref;
    using hash_t = table::silent_hash::put_ref;
    using outputs_t = table::silent_outputs::record;
    using prefix_t = table::silent_prefix::put_refs;

    // ========================================================================
    const auto scope = get_transactor();
    auto& bank_table = store_.silent_bank(bank);
    auto& bank_prefix = store_.silent_prefix_bank(bank);

    // The link precedes the rows, so that rows are never banked without it.
    if (!store_.presilent_bank(bank).put(table::presilent::record{ {}, link }))
        return false;

    // Allocate the block's outputs, then its txs across all columns. Outputs
    // are reached only through txs, so failure strands no unwritten tx.
    using namespace system;
    const auto pk = bank_prefix.allocate(
        possible_narrow_cast<prefix_link::integer>(rows));
    if (pk.is_terminal())
        return false;

    const auto fk = bank_table.allocate(
        possible_narrow_cast<silent_link::integer>(captures.size()));
    if (fk.is_terminal())
        return false;

    // Guard against remap (required for nomaps::put(fk)).
    const auto guard = bank_table.guard();

    auto tx = fk;
    auto out = pk;
    for (const auto& item: captures)
    {
        const auto outputs = item.prefixes.size();
        if (!bank_table.correlate.put(tx, correlate_t{ {}, item.link.value }) ||
            !bank_table.sum.put(tx, sum_t{ {}, item.sum }) ||
            !bank_table.hash.put(tx, hash_t{ {}, item.hash }) ||
            !bank_table.outputs.put(tx, outputs_t{ {}, out.value,
                possible_narrow_cast<uint32_t>(outputs) }) ||
            !bank_prefix.put(out, prefix_t{ {}, item.prefixes }))
            return false;

        tx += one;
        out += outputs;
    }

    return true;
    // ========================================================================
}

TEMPLATE
bool CLASS::is_silent_selected(const tx_link& link,
    const tx_link& coinbase) const NOEXCEPT
{
    return link >= coinbase || !is_pooled(link);
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
    using word_t = table::scan_correlate::tx::integer;
    using point_t = table::scan_point::put_ref;
    using prefix_t = table::scan_prefix::put_refs;
    const auto outputs = prefixes.size();

    // ========================================================================
    const auto scope = get_transactor();

    // Allocate the tx across all columns, and its outputs.
    const auto fk = store_.scan.allocate(one);
    const auto pk = store_.scan_prefix.allocate(
        possible_narrow_cast<prefix_link::integer>(outputs));
    if (fk.is_terminal() || pk.is_terminal())
        return false;

    // Guard against remap (required for nomaps::put(fk)).
    const auto guard = store_.scan.guard();
    const auto words = pointer_cast<word_t>(guard.data());
    if (!store_.scan_prefix.put(pk, prefix_t{ {}, prefixes }) ||
        !store_.scan.point.put(fk, point_t{ {}, point }) ||
        !put_silent_(fk, link, pk, outputs, words))
        return false;

    store_.scan.complete(fk, one);
    return true;
    // ========================================================================
}

TEMPLATE
bool CLASS::put_silent_(const scan_link& fk, const tx_link& link,
    const prefix_link& first, size_t outputs,
    table::scan_correlate::tx::integer* words) NOEXCEPT
{
    using namespace system;
    using outputs_t = table::scan_outputs::record;
    if (!store_.scan.outputs.put(fk, outputs_t{ {}, first.value,
        possible_narrow_cast<uint32_t>(outputs) }))
        return false;

    // The guard is the correlate column, published last (get_silent_frontier).
    using word_t = table::scan_correlate::tx::integer;
    static_assert(schema::scan_correlate::minrow == sizeof(word_t));
    std::atomic_ref<word_t> word{ *std::next(words, fk.value) };
    word.store(native_to_little_end(link.value), std::memory_order_release);
    return true;
}

// Rows are read in chunks so the guard is not held across a long seek.
TEMPLATE
void CLASS::seek_scan_start(size_t& row, size_t& last,
    tx_link::integer floor) const NOEXCEPT
{
    using namespace system;
    using word_t = table::scan_correlate::tx::integer;
    constexpr size_t chunk = 1 << 20;

    while (row < last)
    {
        const auto guard = store_.scan.correlate.get_memory();
        const auto words = pointer_cast<word_t>(guard.data());
        const auto end = std::min(last, row + chunk);
        for (; row < end; ++row)
        {
            std::atomic_ref<word_t> word{ *std::next(words, row) };
            const auto link = native_from_little_end(word.load(
                std::memory_order_acquire));
            if (is_zero(link))
            {
                last = row;
                return;
            }

            if (link > floor)
                return;
        }
    }
}
} // namespace database
} // namespace libbitcoin

#endif
