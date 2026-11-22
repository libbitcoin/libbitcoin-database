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
#ifndef LIBBITCOIN_DATABASE_STORE_PRUNE_IPP
#define LIBBITCOIN_DATABASE_STORE_PRUNE_IPP

#include <chrono>
#include <numeric>
#include <unordered_set>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

// public
TEMPLATE
code CLASS::prune(const event_handler& handler) NOEXCEPT
{
    // Transactor lock generally only covers writes, but here prevout reads must
    // also be guarded, as body shrinks and head is cleared.
    while (!transactor_mutex_.try_lock_for(std::chrono::seconds(1)))
    {
        handler(event_t::wait_lock, table_t::store);
    }

    code ec{ error::success };

    // Prevouts resettable if all candidates confirmed (fork is candidate top).
    if (!query<CLASS>{ *this }.is_coalesced())
    {
        ec = error::not_coalesced;
    }
    else
    {
        handler(event_t::prune_table, table_t::prevout_head);

        // nullify table head, set reference body count to zero.
        // If snapshot from this state fails previous snapshot remains valid.
        // Batch tables drop at start and under lock after verify (not here).
        if (!prevout.clear())
            ec = error::prune_table;

        // Snapshot with nullified head and zero body count.
        // 'prune' parameter signals not to reset body count.
        // Success deletes pre-prune snapshot (trim strands nothing).
        if (!ec)
            ec = snapshot(handler, true);

        // Reclaim logical extent (the snapshot remains valid on failure).
        if (!ec)
        {
            handler(event_t::prune_table, table_t::prevout_body);
            if (!prevout_body_.truncate(0))
                ec = error::prune_table;
        }

        if (!ec)
        {
            // Reclaim disk space to logical extent.
            handler(event_t::unload_file, table_t::prevout_body);
            if (!ec) ec = prevout_body_.shrink();
            handler(event_t::load_file, table_t::prevout_body);

            handler(event_t::unload_file, table_t::ecdsa0_body);
            if (!ec) ec = ecdsa0_body_.shrink();
            handler(event_t::load_file, table_t::ecdsa0_body);
            handler(event_t::unload_file, table_t::ecdsa1_body);
            if (!ec) ec = ecdsa1_body_.shrink();
            handler(event_t::load_file, table_t::ecdsa1_body);

            handler(event_t::unload_file, table_t::schnorr0_body);
            if (!ec) ec = schnorr0_body_.shrink();
            handler(event_t::load_file, table_t::schnorr0_body);
            handler(event_t::unload_file, table_t::schnorr1_body);
            if (!ec) ec = schnorr1_body_.shrink();
            handler(event_t::load_file, table_t::schnorr1_body);

            handler(event_t::unload_file, table_t::silent0_body);
            if (!ec) ec = silent0_body_.shrink();
            handler(event_t::load_file, table_t::silent0_body);
            handler(event_t::unload_file, table_t::silent1_body);
            if (!ec) ec = silent1_body_.shrink();
            handler(event_t::load_file, table_t::silent1_body);

            handler(event_t::unload_file, table_t::silent0_prefix_body);
            if (!ec) ec = silent0_prefix_body_.shrink();
            handler(event_t::load_file, table_t::silent0_prefix_body);
            handler(event_t::unload_file, table_t::silent1_prefix_body);
            if (!ec) ec = silent1_prefix_body_.shrink();
            handler(event_t::load_file, table_t::silent1_prefix_body);

            handler(event_t::unload_file, table_t::prevalid0_body);
            if (!ec) ec = prevalid0_body_.shrink();
            handler(event_t::load_file, table_t::prevalid0_body);
            handler(event_t::unload_file, table_t::prevalid1_body);
            if (!ec) ec = prevalid1_body_.shrink();
            handler(event_t::load_file, table_t::prevalid1_body);

            handler(event_t::unload_file, table_t::presilent0_body);
            if (!ec) ec = presilent0_body_.shrink();
            handler(event_t::load_file, table_t::presilent0_body);
            handler(event_t::unload_file, table_t::presilent1_body);
            if (!ec) ec = presilent1_body_.shrink();
            handler(event_t::load_file, table_t::presilent1_body);
        }
    }

    transactor_mutex_.unlock();
    return ec;
}

// protected
// Each kept record moves down to row of its rank, as do its
// spends, over dropped records. Staged body is not rewritable,
// so pool and spends bodies are mapped shared for move. Requires
// nothing reading pool, and failure leaves pool undefined.
TEMPLATE
code CLASS::compact(const event_handler& handler) NOEXCEPT
{
    using namespace system;
    if (!pool.enabled() || is_zero(pool.count().value))
        return error::success;

    retentions keep{};
    if (!retain_pool(keep))
        return error::compact_retain;

    using rows_t = table::pool::link::integer;
    using inputs_t = table::spends::link::integer;
    const auto rows = possible_narrow_cast<rows_t>(keep.size());
    const auto inputs = std::accumulate(keep.cbegin(), keep.cend(),
        inputs_t{}, [](inputs_t total, const retention& kept) NOEXCEPT
        {
            return total + kept.inputs;
        });

    if (rows == pool.count().value && inputs == spends.count().value)
        return error::success;

    code ec{ error::success };
    const auto restage = [&](auto& table, table_t body, bool staged) NOEXCEPT
    {
        if (!ec)
        {
            handler(event_t::unload_file, body);
            ec = table.restage(staged);
            handler(event_t::load_file, body);
        }
    };

    restage(pool, table_t::pool_body, false);
    restage(spends, table_t::spends_body, false);

    if (!ec)
    {
        handler(event_t::prune_table, table_t::pool_head);
        handler(event_t::prune_table, table_t::pool_body);
        handler(event_t::prune_table, table_t::spends_body);
        handler(event_t::prune_table, table_t::wtxid_head);
        handler(event_t::prune_table, table_t::wtxid_body);
        handler(event_t::prune_table, table_t::pool_index_head);
        handler(event_t::prune_table, table_t::spends_index_head);

        query<CLASS> writer{ *this };
        if (!pool.clear())
            ec = error::compact_clear;
        else if (!move_spends(keep))
            ec = error::compact_spends;
        else if (!move_pool(keep))
            ec = error::compact_pool;
        else if (!pool.truncate(rows) || !spends.truncate(inputs))
            ec = error::compact_truncate;
        else if (!index_pool())
            ec = error::compact_wtxid;
        else if (!writer.set_pool_index())
            ec = error::compact_index;
        else if (!pool.backup() || !spends.backup() || !wtxid.backup())
            ec = error::compact_count;
    }

    // Unloading reclaims disk space to logical extent.
    restage(pool, table_t::pool_body, true);
    restage(spends, table_t::spends_body, true);

    if (!ec)
    {
        handler(event_t::unload_file, table_t::wtxid_body);
        ec = wtxid_body_.shrink();
        handler(event_t::load_file, table_t::wtxid_body);
    }

    return ec;
}

// protected
// Retained records in row order (record is first of its key).
TEMPLATE
bool CLASS::retain_pool(retentions& out) NOEXCEPT
{
    const auto end = pool.count().value;
    for (pool_link::integer row{}; row < end; ++row)
    {
        const tx_link tx_fk{ pool.get_key(row) };
        if (pool.first(tx_fk) != row)
            continue;

        bool keep{};
        if (!is_retained(keep, tx_fk, out))
            return false;

        if (!keep)
            continue;

        table::pool_edge edge{};
        if (!pool.edges.get(row, edge))
            return false;

        out.push_back({ row, edge.spends_fk, edge.inputs });
    }

    return true;
}

// protected
// Tx retained if confirmable and every unconfirmed ancestor is pooled.
TEMPLATE
bool CLASS::is_retained(bool& out, const tx_link& link,
    const retentions& kept) NOEXCEPT
{
    using parents = table::spends::get_refs::parents;
    using prevout = table::prevout::slab_get;
    const query<CLASS> reader{ *this };
    std::vector<tx_link::integer> pending{ link };
    std::unordered_set<tx_link::integer> visited{ link };

    out = false;
    if (!reader.is_confirmable_tx(link))
        return true;

    while (!pending.empty())
    {
        const tx_link next{ pending.back() };
        pending.pop_back();

        const auto fk = pool.first(next);
        if (fk.is_terminal())
            return true;

        table::pool_edge edge{};
        if (!pool.edges.get(fk, edge))
            return false;

        parents spent(edge.inputs);
        table::spends::get_refs run{ {}, spent };
        if (!spends.get(edge.spends_fk, run))
            return false;

        for (const auto merged: spent)
        {
            const tx_link parent{ prevout::output_tx_fk(merged) };
            if (reader.is_confirmed_tx(parent))
                continue;

            const auto row = pool.first(parent).value;
            if (std::ranges::binary_search(kept, row, {}, &retention::row))
                continue;

            if (visited.insert(parent).second)
                pending.push_back(parent);
        }
    }

    out = true;
    return true;
}

// protected
// Runs disjoint, so in order of their records
// each moves down over dropped records only.
TEMPLATE
bool CLASS::move_spends(retentions& keep) NOEXCEPT
{
    using parents = table::spends::get_refs::parents;
    std::vector<size_t> order(keep.size());
    std::iota(order.begin(), order.end(), zero);
    std::ranges::sort(order, [&](size_t left, size_t right) NOEXCEPT
    {
        return keep.at(left).spends_fk < keep.at(right).spends_fk;
    });

    table::spends::link::integer next{};
    for (const auto index: order)
    {
        auto& kept = keep.at(index);
        parents spent(kept.inputs);
        table::spends::get_refs run{ {}, spent };
        if (!spends.get(kept.spends_fk, run) ||
            !spends.put(next, table::spends::put_refs{ {}, spent }))
            return false;

        kept.spends_fk = next;
        next += kept.inputs;
    }

    return true;
}

// protected
// Records kept in row order, so each moves down over dropped records only, and
// is committed to cleared head at its new row.
TEMPLATE
bool CLASS::move_pool(const retentions& keep) NOEXCEPT
{
    using word = table::pool_word;
    using scalar = table::pool_scalar;
    pool_link::integer next{};
    for (const auto& kept: keep)
    {
        word id0{}, id1{}, id2{}, id3{};
        scalar flags{}, height{}, median{};
        table::pool_edge edge{};
        table::pool_total total{};
        table::pool::record record{};
        const tx_link tx_fk{ pool.get_key(kept.row) };
        if (!pool.get(kept.row, record) ||
            !pool.id0.get(kept.row, id0) || !pool.id1.get(kept.row, id1) ||
            !pool.id2.get(kept.row, id2) || !pool.id3.get(kept.row, id3) ||
            !pool.totals.get(kept.row, total) ||
            !pool.edges.get(kept.row, edge) ||
            !pool.flags.get(kept.row, flags) ||
            !pool.heights.get(kept.row, height) ||
            !pool.medians.get(kept.row, median))
            return false;

        // Column puts are unguarded, the accessor guards their rows.
        edge.spends_fk = kept.spends_fk;
        auto guard = pool.get_memory();
        if (!guard ||
            !pool.id0.put(next, id0) || !pool.id1.put(next, id1) ||
            !pool.id2.put(next, id2) || !pool.id3.put(next, id3) ||
            !pool.totals.put(next, total) || !pool.edges.put(next, edge) ||
            !pool.flags.put(next, flags) || !pool.heights.put(next, height) ||
            !pool.medians.put(next, median))
            return false;

        guard.reset();

        if (!pool.put(next, tx_fk, record))
            return false;

        ++next;
    }

    return true;
}

// protected
// Wtxid record is that of its pool row,
// so index is rebuilt from witness hash of each row.
TEMPLATE
bool CLASS::index_pool() NOEXCEPT
{
    using namespace system;
    using word = table::pool_word;
    using lane_t = schema::pool::witness_lane;
    using lanes_t = std_array<lane_t, schema::pool::witness_lanes>;
    if (!wtxid.drop())
        return false;

    const auto end = pool.count().value;
    for (pool_link::integer row{}; row < end; ++row)
    {
        word id0{}, id1{}, id2{}, id3{};
        if (!pool.id0.get(row, id0) || !pool.id1.get(row, id1) ||
            !pool.id2.get(row, id2) || !pool.id3.get(row, id3))
            return false;

        const auto words = to_little_endians(lanes_t
        {
            id0.word, id1.word, id2.word, id3.word
        });

        if (!wtxid.put(row, array_cast<uint8_t>(words)))
            return false;
    }

    return true;
}

} // namespace database
} // namespace libbitcoin

#endif
