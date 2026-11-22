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
#ifndef LIBBITCOIN_DATABASE_STORE_OPEN_IPP
#define LIBBITCOIN_DATABASE_STORE_OPEN_IPP

#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

// public
TEMPLATE
code CLASS::open(const event_handler& handler) NOEXCEPT
{
    if (!file::is_directory(configuration_.path))
        return error::missing_directory;

    if (!transactor_mutex_.try_lock())
        return error::transactor_lock;

    if (!process_lock_.try_lock())
    {
        transactor_mutex_.unlock();
        return error::process_lock;
    }

    if (fault_lock_.is_locked())
    {
        /* bool */ process_lock_.try_unlock();
        transactor_mutex_.unlock();
        return error::fault_lock;
    }

    if (!flush_lock_.try_lock())
    {
        /* bool */ process_lock_.try_unlock();
        transactor_mutex_.unlock();
        return error::flush_lock;
    }

    const auto verify = [&handler](code& ec, auto& logical,
        table_t table) NOEXCEPT
    {
        if (!ec)
        {
            handler(event_t::verify_table, table);
            if (!logical.verify())
                ec = error::verify_table;
        }
    };

    auto ec = open_load(handler);

    // The stored envelope governs sizing, so it precedes verification.
    if (!ec)
        ec = load_envelope();

    verify(ec, header, table_t::header_table);
    verify(ec, input, table_t::input_table);
    verify(ec, output, table_t::output_table);
    verify(ec, ins, table_t::ins_table);
    verify(ec, outs, table_t::outs_table);
    verify(ec, tx, table_t::tx_table);
    verify(ec, txs, table_t::txs_table);

    verify(ec, candidate, table_t::candidate_table);
    verify(ec, confirmed, table_t::confirmed_table);
    verify(ec, strong, table_t::strong_table);
    verify(ec, scan, table_t::scan_table);
    verify(ec, scan_prefix, table_t::scan_prefix_table);

    verify(ec, ecdsa0, table_t::ecdsa0_table);
    verify(ec, ecdsa1, table_t::ecdsa1_table);
    verify(ec, schnorr0, table_t::schnorr0_table);
    verify(ec, schnorr1, table_t::schnorr1_table);
    verify(ec, silent0, table_t::silent0_table);
    verify(ec, silent1, table_t::silent1_table);
    verify(ec, silent0_prefix, table_t::silent0_prefix_table);
    verify(ec, silent1_prefix, table_t::silent1_prefix_table);
    verify(ec, envelope, table_t::envelope_table);
    verify(ec, duplicate, table_t::duplicate_table);
    verify(ec, prevalid0, table_t::prevalid0_table);
    verify(ec, prevalid1, table_t::prevalid1_table);
    verify(ec, presilent0, table_t::presilent0_table);
    verify(ec, presilent1, table_t::presilent1_table);
    verify(ec, prevout, table_t::prevout_table);
    verify(ec, state, table_t::state_table);
    verify(ec, pool, table_t::pool_table);
    verify(ec, spends, table_t::spends_table);
    verify(ec, pool_index, table_t::pool_index_table);
    verify(ec, spends_index, table_t::spends_index_table);
    verify(ec, wtxid, table_t::wtxid_table);

    verify(ec, filter_bk, table_t::filter_bk_table);
    verify(ec, filter_tx, table_t::filter_tx_table);

    // Pool records are rewritten, so nothing may be reading the pool.
    const auto verified = !ec;
    if (verified)
        ec = compact(handler);

    if (ec)
    {
        // A failed compaction leaves the pool undefined, as does a fault.
        auto corrupt = verified;

        // open_load leaves nothing open on a schema mismatch.
        if (ec != error::schema_version && unload_close(handler))
            corrupt = true;

        // unlock errors override ec, corruption leaves behind flush_lock.
        if (!corrupt && !get_fault() && !flush_lock_.try_unlock())
            ec = error::flush_unlock;

        if (!process_lock_.try_unlock()) ec = error::process_unlock;
    }

    // process and flush locks remain open until close().
    transactor_mutex_.unlock();
    return ec;
}

// The envelope is fixed width, so it is rewritten at its allocated link.
TEMPLATE
void CLASS::store_envelope(code& ec) NOEXCEPT
{
    if (ec)
        return;

    const table::envelope::record record{ envelope_ };
    if (is_zero(envelope.head_size()) &&
        (!envelope.reserve(record.count()) ||
            envelope.allocate(record.count()).is_terminal()))
    {
        ec = error::create_table;
        return;
    }

    if (!envelope.put(zero, record))
        ec = error::create_table;
}

// Head expanded before envelope rewritten, so failed expansion (disk full)
// leaves stored envelope unchanged.
TEMPLATE
code CLASS::put_envelope(const database::envelope& stored) NOEXCEPT
{
    using integer = schema::envelope::link::integer;
    const table::envelope::record record{ stored };
    const auto size = envelope.head_size();
    const auto count = record.count();
    if ((count > size) && envelope.allocate(
        system::possible_narrow_cast<integer>(count - size)).is_terminal())
        return error::disk_full;

    return envelope.put(zero, record) ? error::success : error::verify_table;
}

// Migration
// ----------------------------------------------------------------------------

// public
// Stored envelope read in, migrated by each step from its schema, and written
// out. Failure once store is changed leaves fault and flush locks.
TEMPLATE
code CLASS::migrate(system::config::version& original,
    const database::envelope& configured,
    const event_handler& handler) NOEXCEPT
{
    if (!file::is_directory(configuration_.path))
        return error::missing_directory;

    if (!transactor_mutex_.try_lock())
        return error::transactor_lock;

    if (!process_lock_.try_lock())
    {
        transactor_mutex_.unlock();
        return error::process_lock;
    }

    if (fault_lock_.is_locked())
    {
        /* bool */ process_lock_.try_unlock();
        transactor_mutex_.unlock();
        return error::fault_lock;
    }

    if (!flush_lock_.try_lock())
    {
        /* bool */ process_lock_.try_unlock();
        transactor_mutex_.unlock();
        return error::flush_lock;
    }

    handler(event_t::open_file, table_t::envelope_head);
    auto ec = envelope_head_.open();
    if (!ec)
    {
        handler(event_t::load_file, table_t::envelope_head);
        ec = envelope_head_.load();
    }

    table::envelope::raw raw{};
    if (!ec && (is_zero(envelope.head_size()) || !envelope.get(zero, raw)))
        ec = error::verify_table;

    if (!ec)
    {
        original = database::envelope::get_schema(raw.data);
        if (!database::envelope::is_readable(original))
            ec = error::schema_version;
    }

    auto faulted = false;
    if (!ec && original != database::envelope::compiled)
    {
        database::envelope stored{};
        if (!read_migrated(stored, raw.data, configured))
            ec = error::verify_table;

        if (!ec)
        {
            faulted = fault_lock_.try_lock();
            ec = faulted ? purge_snapshots() : error::fault_lock;
        }

        auto schema = original;
        while (!ec && schema != database::envelope::compiled)
        {
            if (schema == system::config::version{ 4, 0, 30, 0 })
            {
                ec = migrate_4_0_30_to_4_0_31(stored, configured, handler);
                schema = { 4, 0, 31, 0 };
            }
            else if (schema == system::config::version{ 4, 0, 31, 0 })
            {
                ec = migrate_4_0_31_to_4_0_32(stored, configured);
                schema = { 4, 0, 32, 0 };
            }
            else if (schema == system::config::version{ 4, 0, 32, 0 })
            {
                ec = migrate_4_0_32_to_4_0_33(handler);
                schema = { 4, 0, 33, 0 };
            }
            else if (schema == system::config::version{ 4, 0, 33, 0 })
            {
                schema = { 4, 0, 34, 0 };
            }
            else
            {
                ec = error::schema_version;
            }
        }

        if (!ec)
            ec = put_envelope(stored);
    }

    // unload and close errors override ec.
    handler(event_t::unload_file, table_t::envelope_head);
    if (const auto unloaded = envelope_head_.unload()) ec = unloaded;
    handler(event_t::close_file, table_t::envelope_head);
    if (const auto closed = envelope_head_.close()) ec = closed;

    // unlock errors override ec.
    if (!faulted || !ec)
    {
        if (faulted && !fault_lock_.try_unlock()) ec = error::fault_unlock;
        if (!flush_lock_.try_unlock()) ec = error::flush_unlock;
    }

    if (!process_lock_.try_unlock()) ec = error::process_unlock;
    transactor_mutex_.unlock();
    return ec;
}

// A serialized readable envelope, read in at the compiled schema.
TEMPLATE
bool CLASS::read_migrated(database::envelope& out, system::data_chunk data,
    const database::envelope& configured) NOEXCEPT
{
    using namespace system;
    if (!database::envelope::migrate(data, configured))
        return false;

    const auto size = possible_narrow_sign_cast<ptrdiff_t>(data.size());
    iostream stream{ data.data(), size };
    reader source{ stream };
    return out.from_data(source);
}

TEMPLATE
code CLASS::purge_snapshots() NOEXCEPT
{
    constexpr std::array folders
    {
        schema::dir::primary, schema::dir::secondary, schema::dir::temporary
    };

    for (const auto name: folders)
    {
        const auto folder = configuration_.path / name;
        if (!file::is_directory(folder))
            continue;

        if (const auto ec = file::clear_directory_ex(folder)) return ec;
        if (const auto ec = file::remove_ex(folder)) return ec;
    }

    return error::success;
}

// The pool tables are discarded and created empty at configured sizing.
TEMPLATE
code CLASS::migrate_4_0_30_to_4_0_31(database::envelope& stored,
    const database::envelope& configured,
    const event_handler& handler) NOEXCEPT
{
    using namespace system;
    const auto pool_buckets = configured.pool_buckets;
    const auto wtxid_buckets = configured.wtxid_buckets;
    const auto pool_k = table::pool::optimal_k(
        configuration_.pool.expected_(), pool_buckets);
    const auto wtxid_k = table::wtxid::optimal_k(
        configuration_.wtxid.expected_(), wtxid_buckets);

    if (!pool.set_buckets(pool_buckets) || !pool.set_filter_k(pool_k) ||
        !wtxid.set_buckets(wtxid_buckets) || !wtxid.set_filter_k(wtxid_k))
        return error::verify_table;

    const auto create = [&handler](code& ec, const auto& file,
        table_t table) NOEXCEPT
    {
        if (!ec)
        {
            handler(event_t::create_file, table);
            ec = file.create();
        }
    };

    const auto open = [&handler](code& ec, auto& file, table_t table) NOEXCEPT
    {
        if (!ec)
        {
            handler(event_t::open_file, table);
            ec = file.open();
        }
    };

    const auto load = [&handler](code& ec, auto& file, table_t table) NOEXCEPT
    {
        if (!ec)
        {
            handler(event_t::load_file, table);
            ec = file.load();
        }
    };

    const auto populate = [&handler](code& ec, auto& logical,
        table_t table) NOEXCEPT
    {
        if (!ec)
        {
            handler(event_t::create_table, table);
            if (!logical.create())
                ec = error::create_table;
        }
    };

    const auto unload = [&handler](code& ec, auto& file, table_t table) NOEXCEPT
    {
        handler(event_t::unload_file, table);
        if (const auto unloaded = file.unload(); !ec) ec = unloaded;
    };

    const auto close = [&handler](code& ec, auto& file, table_t table) NOEXCEPT
    {
        handler(event_t::close_file, table);
        if (const auto closed = file.close(); !ec) ec = closed;
    };

    const auto obsolete = body(configuration_.path,
        std::string{ schema::caches::pool } + "_context");

    auto ec = file::remove_ex(obsolete);
    create(ec, pool_head_, table_t::pool_head);
    create(ec, pool_body_, table_t::pool_body);
    create(ec, spends_head_, table_t::spends_head);
    create(ec, spends_body_, table_t::spends_body);
    create(ec, pool_index_head_, table_t::pool_index_head);
    create(ec, spends_index_head_, table_t::spends_index_head);
    create(ec, wtxid_head_, table_t::wtxid_head);
    create(ec, wtxid_body_, table_t::wtxid_body);

    open(ec, pool_head_, table_t::pool_head);
    open(ec, pool_body_, table_t::pool_body);
    open(ec, spends_head_, table_t::spends_head);
    open(ec, spends_body_, table_t::spends_body);
    open(ec, pool_index_head_, table_t::pool_index_head);
    open(ec, spends_index_head_, table_t::spends_index_head);
    open(ec, wtxid_head_, table_t::wtxid_head);
    open(ec, wtxid_body_, table_t::wtxid_body);

    load(ec, pool_head_, table_t::pool_head);
    load(ec, pool_body_, table_t::pool_body);
    load(ec, spends_head_, table_t::spends_head);
    load(ec, spends_body_, table_t::spends_body);
    load(ec, pool_index_head_, table_t::pool_index_head);
    load(ec, spends_index_head_, table_t::spends_index_head);
    load(ec, wtxid_head_, table_t::wtxid_head);
    load(ec, wtxid_body_, table_t::wtxid_body);

    populate(ec, pool, table_t::pool_table);
    populate(ec, spends, table_t::spends_table);
    populate(ec, pool_index, table_t::pool_index_table);
    populate(ec, spends_index, table_t::spends_index_table);
    populate(ec, wtxid, table_t::wtxid_table);

    unload(ec, pool_head_, table_t::pool_head);
    unload(ec, pool_body_, table_t::pool_body);
    unload(ec, spends_head_, table_t::spends_head);
    unload(ec, spends_body_, table_t::spends_body);
    unload(ec, pool_index_head_, table_t::pool_index_head);
    unload(ec, spends_index_head_, table_t::spends_index_head);
    unload(ec, wtxid_head_, table_t::wtxid_head);
    unload(ec, wtxid_body_, table_t::wtxid_body);

    close(ec, pool_head_, table_t::pool_head);
    close(ec, pool_body_, table_t::pool_body);
    close(ec, spends_head_, table_t::spends_head);
    close(ec, spends_body_, table_t::spends_body);
    close(ec, pool_index_head_, table_t::pool_index_head);
    close(ec, spends_index_head_, table_t::spends_index_head);
    close(ec, wtxid_head_, table_t::wtxid_head);
    close(ec, wtxid_body_, table_t::wtxid_body);

    if (ec)
        return ec;

    stored.pool_buckets = pool_buckets;
    stored.wtxid_buckets = wtxid_buckets;
    stored.pool_k = possible_narrow_cast<uint8_t>(pool_k);
    stored.wtxid_k = possible_narrow_cast<uint8_t>(wtxid_k);
    return error::success;
}

TEMPLATE
code CLASS::migrate_4_0_31_to_4_0_32(database::envelope& stored,
    const database::envelope& configured) NOEXCEPT
{
    stored.require_witness = configured.require_witness;
    return error::success;
}

// Each pruned block, with each of its txs, flagged in its heavy size in place,
// found by sizes of its coinbase. Tx of no block (archived before failed
// association, as on disk full) found once its block archived again, as points
// it spends are then duplicated.
TEMPLATE
code CLASS::migrate_4_0_32_to_4_0_33(const event_handler& handler) NOEXCEPT
{
    const auto open = [&handler](code& ec, auto& file, table_t table) NOEXCEPT
    {
        if (!ec)
        {
            handler(event_t::open_file, table);
            ec = file.open();
        }
    };

    const auto load = [&handler](code& ec, auto& file, table_t table) NOEXCEPT
    {
        if (!ec)
        {
            handler(event_t::load_file, table);
            ec = file.load();
        }
    };

    const auto unload = [&handler](code& ec, auto& file, table_t table) NOEXCEPT
    {
        handler(event_t::unload_file, table);
        if (const auto unloaded = file.unload(); !ec) ec = unloaded;
    };

    const auto close = [&handler](code& ec, auto& file, table_t table) NOEXCEPT
    {
        handler(event_t::close_file, table);
        if (const auto closed = file.close(); !ec) ec = closed;
    };

    code ec{};
    open(ec, input_head_, table_t::input_head);
    open(ec, input_body_, table_t::input_body);
    open(ec, output_head_, table_t::output_head);
    open(ec, output_body_, table_t::output_body);
    open(ec, ins_head_, table_t::ins_head);
    open(ec, ins_body_, table_t::ins_body);
    open(ec, outs_head_, table_t::outs_head);
    open(ec, *outs_body_, table_t::outs_body);
    open(ec, tx_head_, table_t::tx_head);
    open(ec, tx_body_, table_t::tx_body);
    open(ec, txs_head_, table_t::txs_head);
    open(ec, txs_body_, table_t::txs_body);
    open(ec, duplicate_head_, table_t::duplicate_head);
    open(ec, duplicate_body_, table_t::duplicate_body);

    load(ec, input_head_, table_t::input_head);
    load(ec, input_body_, table_t::input_body);
    load(ec, output_head_, table_t::output_head);
    load(ec, output_body_, table_t::output_body);
    load(ec, ins_head_, table_t::ins_head);
    load(ec, ins_body_, table_t::ins_body);
    load(ec, outs_head_, table_t::outs_head);
    load(ec, *outs_body_, table_t::outs_body);
    load(ec, tx_head_, table_t::tx_head);
    load(ec, tx_body_, table_t::tx_body);
    load(ec, txs_head_, table_t::txs_head);
    load(ec, txs_body_, table_t::txs_body);
    load(ec, duplicate_head_, table_t::duplicate_head);
    load(ec, duplicate_body_, table_t::duplicate_body);

    // Rows rewritten in place, so bodies mapped rewritable (staged body is
    // append-only, its settled rows read-only).
    const auto restage = [&handler](code& ec, auto& file,
        table_t table) NOEXCEPT
    {
        if (!ec)
        {
            handler(event_t::unload_file, table);
            ec = file.restage(false);
            handler(event_t::load_file, table);
        }
    };

    restage(ec, tx_body_, table_t::tx_body);
    restage(ec, txs_body_, table_t::txs_body);

    // Each header bucket is associated block or terminal. Block pruned as unit,
    // so its coinbase (first tx) decides all of its txs.
    const auto count = tx.count().value;
    std::vector<bool> associated(count);
    if (!ec)
    {
        handler(event_t::create_table, table_t::txs_table);
        const auto buckets = txs.buckets();
        for (size_t key{}; !ec && key < buckets; ++key)
        {
            const auto link = txs.at(key);
            if (link.is_terminal())
                continue;

            auto pruned = false;
            table::txs::slab slab{};
            table::transaction::record coinbase{};
            if (!txs.get(link, slab) || slab.tx_fks.empty() ||
                !tx.get(slab.tx_fks.front(), coinbase))
            {
                ec = error::integrity;
                continue;
            }

            for (const auto fk: slab.tx_fks)
                if (fk < count)
                    associated[fk] = true;

            if ((ec = get_pruned(pruned, coinbase)) || !pruned)
                continue;

            for (const auto fk: slab.tx_fks)
            {
                table::transaction::record record{};
                if (!tx.get(fk, record))
                {
                    ec = error::integrity;
                    break;
                }

                record.pruned = true;
                if (!tx.set(fk, tx.get_key(fk), record))
                {
                    ec = error::integrity;
                    break;
                }
            }

            slab.pruned = true;
            if (!ec && !txs.set(link, slab))
                ec = error::integrity;
        }
    }

    if (!ec)
    {
        handler(event_t::create_table, table_t::duplicate_table);
        using point_link = table::duplicate::link;
        const auto duplicates = duplicate.count().value;
        for (point_link::integer value{}; !ec && value < duplicates; ++value)
        {
            const auto point = duplicate.get_key(point_link{ value });
            for (auto it = ins.it(point); !ec && it; ++it)
            {
                table::ins_sequence::get_parent spender{};
                if (!ins.sequence.get(*it, spender))
                {
                    ec = error::integrity;
                    break;
                }

                const auto fk = spender.parent_fk;
                if (fk >= count || associated[fk])
                    continue;

                auto pruned = false;
                table::transaction::record record{};
                if (!tx.get(fk, record))
                {
                    ec = error::integrity;
                    break;
                }

                if ((ec = get_pruned(pruned, record)) || !pruned)
                    continue;

                record.pruned = true;
                if (!tx.set(fk, tx.get_key(fk), record))
                    ec = error::integrity;
            }
        }
    }

    unload(ec, input_head_, table_t::input_head);
    unload(ec, input_body_, table_t::input_body);
    unload(ec, output_head_, table_t::output_head);
    unload(ec, output_body_, table_t::output_body);
    unload(ec, ins_head_, table_t::ins_head);
    unload(ec, ins_body_, table_t::ins_body);
    unload(ec, outs_head_, table_t::outs_head);
    unload(ec, *outs_body_, table_t::outs_body);
    unload(ec, tx_head_, table_t::tx_head);
    unload(ec, tx_body_, table_t::tx_body);
    unload(ec, txs_head_, table_t::txs_head);
    unload(ec, txs_body_, table_t::txs_body);
    unload(ec, duplicate_head_, table_t::duplicate_head);
    unload(ec, duplicate_body_, table_t::duplicate_body);

    close(ec, input_head_, table_t::input_head);
    close(ec, input_body_, table_t::input_body);
    close(ec, output_head_, table_t::output_head);
    close(ec, output_body_, table_t::output_body);
    close(ec, ins_head_, table_t::ins_head);
    close(ec, ins_body_, table_t::ins_body);
    close(ec, outs_head_, table_t::outs_head);
    close(ec, *outs_body_, table_t::outs_body);
    close(ec, tx_head_, table_t::tx_head);
    close(ec, tx_body_, table_t::tx_body);
    close(ec, txs_head_, table_t::txs_head);
    close(ec, txs_body_, table_t::txs_body);
    close(ec, duplicate_head_, table_t::duplicate_head);
    close(ec, duplicate_body_, table_t::duplicate_body);
    return ec;
}

// Pruned tx stores every input script and witness empty (only first input link
// valid), so its stored nominal size is short of its archived.
TEMPLATE
code CLASS::get_pruned(bool& pruned,
    const table::transaction::record& record) const NOEXCEPT
{
    using namespace system;
    constexpr auto empty_input = chain::point::serialized_size() + one +
        sizeof(uint32_t);

    pruned = false;
    table::input::get_empty in{};
    table::ins_sequence::get_input point{};
    if (!ins.sequence.get(record.point_fk, point) ||
        !input.get(point.input_fk, in))
        return error::integrity;

    if (!in.empty)
        return error::success;

    table::outs::record puts{};
    puts.out_fks.resize(record.outs_count);
    if (!outs.puts.get(record.outs_fk, puts))
        return error::integrity;

    size_t light = sizeof(uint32_t) + sizeof(uint32_t) +
        variable_size(record.ins_count) + variable_size(record.outs_count) +
        record.ins_count * empty_input;

    table::output::slab out{};
    for (const auto& fk: puts.out_fks)
    {
        if (!output.get(fk, out))
            return error::integrity;

        light += sizeof(uint64_t) + out.script.serialized_size(true);
    }

    pruned = light != record.light;
    return error::success;
}

// public
// Schema deserialized before validated, so record holds
// it when envelope deserialization fails. Stored argument
// provides configured values for migrated envelope.
TEMPLATE
code CLASS::read_envelope(database::envelope& stored) NOEXCEPT
{
    if (!transactor_mutex_.try_lock())
        return error::transactor_lock;

    auto ec = envelope_head_.open();
    if (!ec) ec = envelope_head_.load();
    if (!ec)
    {
        table::envelope::record record{};
        auto valid = !is_zero(envelope.head_size()) &&
            envelope.get(zero, record);

        // A migratable envelope is migrated in memory, the store is unchanged.
        const auto schema = record.envelope.schema;
        table::envelope::raw raw{};
        if (!valid && database::envelope::is_readable(schema) &&
            envelope.get(zero, raw))
            valid = read_migrated(record.envelope, raw.data, stored);

        // The stored schema is retained, as the store is not yet migrated.
        stored = record.envelope;
        stored.schema = schema;
        if (!database::envelope::is_readable(schema))
            ec = error::schema_version;
        else if (!valid)
            ec = error::verify_table;
    }

    // unload and close errors override ec.
    if (const auto unloaded = envelope_head_.unload()) ec = unloaded;
    if (const auto closed = envelope_head_.close()) ec = closed;

    transactor_mutex_.unlock();
    return ec;
}

// A created store has no envelope until it is populated.
TEMPLATE
code CLASS::load_schema() NOEXCEPT
{
    if (is_zero(envelope.head_size()))
        return error::success;

    system::config::version schema{};
    if (!get_schema(schema))
        return error::verify_table;

    return schema == database::envelope::compiled ? error::success :
        error::schema_version;
}

// Schema deserialized before validated,
// so record holds it when envelope deserialization fails.
TEMPLATE
bool CLASS::get_schema(system::config::version& schema) const NOEXCEPT
{
    if (is_zero(envelope.head_size()))
        return false;

    table::envelope::record record{};
    const auto valid = envelope.get(zero, record);
    schema = record.envelope.schema;
    return valid || (schema != database::envelope::compiled);
}

TEMPLATE
code CLASS::load_envelope() NOEXCEPT
{
    // The stored creation envelope governs.
    table::envelope::record record{};
    if (!is_zero(envelope.head_size()))
    {
        if (!envelope.get(zero, record))
            return error::verify_table;

        envelope_ = record.envelope;
    }

    // The stored latch governs, it is one-way and survives the session.
    scan_frontier_.store(envelope_.scan_frontier, std::memory_order_relaxed);
    latch_tx_.store(envelope_.latch_tx, std::memory_order_relaxed);
    pool_top_.store(envelope_.pool_top, std::memory_order_relaxed);
    for (size_t index{}; index < activation_count; ++index)
    {
        const auto& activation = envelope_.activations.at(index);
        auto& link = activation_links_.at(index);
        auto& flags = activation_flags_.at(index);
        link.store(activation.link, std::memory_order_relaxed);
        flags.store(activation.flags, std::memory_order_release);
    }

    if (envelope_.pooling)
        pooling_.store(true, std::memory_order_relaxed);

    // The stored bucket counts govern the heads.
    if (!header.set_buckets(envelope_.header_buckets) ||
        !ins.set_buckets(envelope_.ins_buckets) ||
        !outs.set_buckets(envelope_.outs_buckets) ||
        !tx.set_buckets(envelope_.tx_buckets) ||
        !duplicate.set_buckets(envelope_.duplicate_buckets) ||
        !pool.set_buckets(envelope_.pool_buckets) ||
        !wtxid.set_buckets(envelope_.wtxid_buckets))
    {
        return error::verify_table;
    }

    // The stored filter k values govern filter bit interpretation.
    if (!header.set_filter_k(envelope_.header_k) ||
        !ins.set_filter_k(envelope_.ins_k) ||
        !outs.set_filter_k(envelope_.outs_k) ||
        !tx.set_filter_k(envelope_.tx_k) ||
        !duplicate.set_filter_k(envelope_.duplicate_k) ||
        !pool.set_filter_k(envelope_.pool_k) ||
        !wtxid.set_filter_k(envelope_.wtxid_k))
    {
        return error::verify_table;
    }

    return error::success;
}

} // namespace database
} // namespace libbitcoin

#endif
