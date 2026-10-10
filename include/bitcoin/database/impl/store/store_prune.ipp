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
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

// public
TEMPLATE
code CLASS::prune(const event_handler& handler) NOEXCEPT
{
    // Transactor lock generally only covers writes, but in this case prevout
    // reads must also be guarded since the body shrinks and head is cleared.
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
        handler(event_t::prune_table, table_t::pool_head);

        // nullify table heads, set reference body counts to zero.
        // If snapshot from this state fails previous snapshot remains valid.
        // Batch tables drop at start and under lock after verify (not here).
        if (!prevout.clear() || !pool.clear())
        {
            ec = error::prune_table;
        }
        else
        {
            // Snapshot with nullified head and zero body count.
            // The 'prune' parameter signals to not reset body count.
            // Success deletes the pre-prune snapshot (trim strands nothing).
            ec = snapshot(handler, true);

            // If the pruning fails here the snapshot remains valid.
            if (!ec)
            {
                // Reclaim logical extent.
                handler(event_t::prune_table, table_t::prevout_body);
                if (!prevout_body_.truncate(0))
                    ec = error::prune_table;
                else
                    ec = compact(handler);

                if (!ec)
                {
                    // Reclaim disk space to logical extent.
                    handler(event_t::unload_file, table_t::prevout_body);
                    if (!ec) ec = prevout_body_.shrink();
                    handler(event_t::load_file, table_t::prevout_body);

                    handler(event_t::unload_file, table_t::pool_body);
                    if (!ec) ec = pool_body_.shrink();
                    handler(event_t::load_file, table_t::pool_body);

                    handler(event_t::unload_file, table_t::spends_body);
                    if (!ec) ec = spends_body_.shrink();
                    handler(event_t::load_file, table_t::spends_body);

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
                }
            }
        }
    }

    transactor_mutex_.unlock();
    return ec;
}

// protected
// Every write follows the prune snapshot, which records pool and spends empty,
// so a fault before the next snapshot restores to the empty pool. Bodies are
// staged (settled rows are read-only), so kept rows are copied to temporary
// tables and back, never rewritten in place.
TEMPLATE
code CLASS::compact(const event_handler& handler) NOEXCEPT
{
    const auto empty = [&]() NOEXCEPT
    {
        handler(event_t::prune_table, table_t::pool_body);
        handler(event_t::prune_table, table_t::spends_body);
        return pool_body_.truncate(0) && spends_body_.truncate(0);
    };

    if (!pool.enabled() || is_zero(pool.count().value))
        return empty() ? error::success : error::prune_table;

    const auto folder = configuration_.path / schema::dir::temporary;
    if (file::clear_directory_ex(folder))
        return empty() ? error::success : error::prune_table;

    Storage<one> pool_head{ head(folder, schema::caches::pool),
        head_settings(configuration_.pool), random };
    table::pool_storage<Storage> pool_body{ body(folder, schema::caches::pool),
        configuration_.pool, sequential, staged };
    Storage<one> spends_head{ head(folder, schema::caches::spends),
        head_settings(configuration_.spends), sequential };
    Storage<one> spends_body{ body(folder, schema::caches::spends),
        configuration_.spends, sequential, staged };

    using namespace system;
    using bucket = table::pool::link::integer;
    table::pool temp_pool{ pool_head, pool_body,
        possible_narrow_cast<bucket>(pool.buckets()),
        configuration_.pool.expected };
    table::spends temp_spends{ spends_head, spends_body };

    code ec{ error::success };
    const auto create = [&](auto& storage, table_t table) NOEXCEPT
    {
        if (!ec)
        {
            handler(event_t::create_file, table);
            ec = storage.create();
        }
    };

    const auto open = [&](auto& storage, table_t table) NOEXCEPT
    {
        if (!ec)
        {
            handler(event_t::open_file, table);
            ec = storage.open();
        }
    };

    const auto load = [&](auto& storage, table_t table) NOEXCEPT
    {
        if (!ec)
        {
            handler(event_t::load_file, table);
            ec = storage.load();
        }
    };

    create(pool_head, table_t::pool_head);
    create(pool_body, table_t::pool_body);
    create(spends_head, table_t::spends_head);
    create(spends_body, table_t::spends_body);
    open(pool_head, table_t::pool_head);
    open(pool_body, table_t::pool_body);
    open(spends_head, table_t::spends_head);
    open(spends_body, table_t::spends_body);
    load(pool_head, table_t::pool_head);
    load(pool_body, table_t::pool_body);
    load(spends_head, table_t::spends_head);
    load(spends_body, table_t::spends_body);

    if (!ec)
    {
        handler(event_t::create_table, table_t::pool_table);
        handler(event_t::create_table, table_t::spends_table);
        if (!temp_pool.set_filter_k(pool.filter_k()) ||
            !temp_pool.create() || !temp_spends.create())
            ec = error::create_table;
    }

    // A tx is kept if no point it spends is spent by a confirmed tx (which
    // drops a confirmed tx, as it spends its own points) and each parent is
    // confirmed or kept (parents precede children in row order).
    using parents = table::spends::get_refs::parents;
    using prevout = table::prevout::slab_get;
    const query<CLASS> reader{ *this };
    const auto keep = [&](const table::transaction::only& tx,
        const parents& spent) NOEXCEPT
    {
        const auto end = tx.point_fk + tx.ins_count;
        for (auto fk = tx.point_fk; fk < end; ++fk)
            for (const auto spender: reader.to_spenders(reader.get_point_key(fk)))
                if (reader.is_confirmed_input(spender))
                    return false;

        return std::ranges::all_of(spent, [&](auto merged) NOEXCEPT
        {
            const tx_link parent{ prevout::output_tx_fk(merged) };
            return reader.is_confirmed_tx(parent) ||
                !temp_pool.first(parent).is_terminal();
        });
    };

    const auto all = [](const table::transaction::only&,
        const parents&) NOEXCEPT
    {
        return true;
    };

    // A failure in the temporary tables leaves the live pool empty.
    const auto copied = !ec &&
        copy_pool(temp_pool, temp_spends, pool, spends, keep);

    ec = error::success;
    if (!empty())
        ec = error::prune_table;
    else if (copied && !copy_pool(pool, spends, temp_pool, temp_spends, all))
        ec = error::prune_table;

    const auto unload = [&](auto& storage, table_t table) NOEXCEPT
    {
        handler(event_t::unload_file, table);
        /* code */ storage.unload();
        handler(event_t::close_file, table);
        /* code */ storage.close();
    };

    unload(pool_head, table_t::pool_head);
    unload(pool_body, table_t::pool_body);
    unload(spends_head, table_t::spends_head);
    unload(spends_body, table_t::spends_body);
    /* bool */ file::clear_directory(folder);
    /* bool */ file::remove(folder);
    return ec;
}

// protected
TEMPLATE
template <typename Keep>
bool CLASS::copy_pool(table::pool& to, table::spends& to_spends,
    table::pool& from, table::spends& from_spends, const Keep& keep) NOEXCEPT
{
    using word = table::pool_word;
    using parents = table::spends::get_refs::parents;
    const auto rows = from.count();
    for (table::pool::link::integer row{}; row < rows; ++row)
    {
        const table::pool::link link{ row };
        const tx_link tx_fk{ from.get_key(link) };

        word id0{}, id1{}, id2{}, id3{};
        table::pool::record record{};
        if (!from.get(link, record) ||
            !from.id0.get(link, id0) || !from.id1.get(link, id1) ||
            !from.id2.get(link, id2) || !from.id3.get(link, id3))
            return false;

        // A row of an unreadable tx is not retained.
        table::transaction::only tx_record{};
        if (!tx.get(tx_fk, tx_record))
            continue;

        parents spent(tx_record.ins_count);
        table::spends::get_refs run{ {}, spent };
        if (!from_spends.get(record.spends_fk, run))
            return false;

        if (!keep(tx_record, spent))
            continue;

        table::spends::link first{};
        if (!to_spends.put_link(first, table::spends::put_refs{ {}, spent }))
            return false;

        const auto target = to.allocate(1);
        if (target.is_terminal())
            return false;

        // Column puts are unguarded, the accessor guards their rows.
        auto guard = to.get_memory();
        if (!guard ||
            !to.id0.put(target, id0) || !to.id1.put(target, id1) ||
            !to.id2.put(target, id2) || !to.id3.put(target, id3))
            return false;

        guard.reset();

        // Commit is deferred until the columns are set.
        record.spends_fk = first;
        if (!to.put(target, tx_fk, record))
            return false;
    }

    return true;
}

} // namespace database
} // namespace libbitcoin

#endif
