/**
 * Copyright (c) 2011-2025 libbitcoin developers (see AUTHORS)
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
#ifndef LIBBITCOIN_DATABASE_STORE_IPP
#define LIBBITCOIN_DATABASE_STORE_IPP

#include <chrono>
#include <unordered_map>
#include <bitcoin/database/boost.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/file/file.hpp>
#include <bitcoin/database/tables/schema.hpp>

// TODO: evaluate performance benefits of concurrency.

namespace libbitcoin {
namespace database {

constexpr auto random = true;
constexpr auto sequential = false;

BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

// public
// ----------------------------------------------------------------------------

TEMPLATE
const std::unordered_map<event_t, std::string> CLASS::events
{
    { event_t::create_file, "create_file" },
    { event_t::open_file,   "open_file" },
    { event_t::load_file, "load_file" },
    { event_t::unload_file, "unload_file" },
    { event_t::close_file, "close_file" },
    { event_t::create_table, "create_table" },
    { event_t::verify_table, "verify_table" },
    { event_t::close_table, "close_table" },

    { event_t::wait_lock, "wait_lock" },
    { event_t::flush_body, "flush_body" },
    { event_t::prune_table, "prune_table" },
    { event_t::backup_table, "backup_table" },
    { event_t::copy_header, "copy_header" },
    { event_t::archive_snapshot, "archive_snapshot" },

    { event_t::restore_table, "restore_table" },
    { event_t::recover_snapshot, "recover_snapshot" }
};

TEMPLATE
const std::unordered_map<table_t, std::string> CLASS::tables
{
    { table_t::store, "store" },

    // Archives.
    { table_t::header_table, "header_table" },
    { table_t::header_head, "header_head" },
    { table_t::header_body, "header_body" },
    { table_t::input_table, "input_table" },
    { table_t::input_head, "input_head" },
    { table_t::input_body, "input_body" },
    { table_t::output_table, "output_table" },
    { table_t::output_head, "output_head" },
    { table_t::output_body, "output_body" },
    { table_t::point_table, "point_table" },
    { table_t::point_head, "point_head" },
    { table_t::point_body, "point_body" },
    { table_t::ins_table, "ins_table" },
    { table_t::ins_head, "ins_head" },
    { table_t::ins_body, "ins_body" },
    { table_t::outs_table, "outs_table" },
    { table_t::outs_head, "outs_head" },
    { table_t::outs_body, "outs_body" },
    { table_t::tx_table, "tx_table" },
    { table_t::tx_head, "tx_head" },
    { table_t::txs_table, "txs_table" },
    { table_t::tx_body, "tx_body" },
    { table_t::txs_head, "txs_head" },
    { table_t::txs_body, "txs_body" },

    // Indexes.
    { table_t::candidate_table, "candidate_table" },
    { table_t::candidate_head, "candidate_head" },
    { table_t::candidate_body, "candidate_body" },
    { table_t::confirmed_table, "confirmed_table" },
    { table_t::confirmed_head, "confirmed_head" },
    { table_t::confirmed_body, "confirmed_body" },
    { table_t::strong_tx_table, "strong_tx_table" },
    { table_t::strong_tx_head, "strong_tx_head" },
    { table_t::strong_tx_body, "strong_tx_body" },

    // Caches.
    { table_t::duplicate_table, "duplicate_table" },
    { table_t::duplicate_head, "duplicate_head" },
    { table_t::duplicate_body, "duplicate_body" },
    { table_t::prevout_table, "prevout_table" },
    { table_t::prevout_head, "prevout_head" },
    { table_t::prevout_body, "prevout_body" },
    { table_t::validated_bk_table, "validated_bk_table" },
    { table_t::validated_bk_head, "validated_bk_head" },
    { table_t::validated_bk_body, "validated_bk_body" },
    { table_t::validated_tx_table, "validated_tx_table" },
    { table_t::validated_tx_head, "validated_tx_head" },
    { table_t::validated_tx_body, "validated_tx_body" },

    // Optionals.
    { table_t::address_table, "address_table" },
    { table_t::address_head, "address_head" },
    { table_t::address_body, "address_body" },
    { table_t::filter_bk_table, "filter_bk_table" },
    { table_t::filter_bk_head, "filter_bk_head" },
    { table_t::filter_bk_body, "filter_bk_body" },
    { table_t::filter_tx_table, "filter_tx_table" },
    { table_t::filter_tx_head, "filter_tx_head" },
    { table_t::filter_tx_body, "filter_tx_body" }
};

TEMPLATE
CLASS::store(const settings& config) NOEXCEPT
  : configuration_(config),

    // Archive.
    // ------------------------------------------------------------------------

    header_head_(head(config.path / schema::dir::heads, schema::archive::header), 1, 0, random),
    header_body_(body(config.path, schema::archive::header), config.header_size, config.header_rate, sequential),
    header(header_head_, header_body_, config.header_buckets),

    input_head_(head(config.path / schema::dir::heads, schema::archive::input), 1, 0, random),
    input_body_(body(config.path, schema::archive::input), config.input_size, config.input_rate, sequential),
    input(input_head_, input_body_),

    output_head_(head(config.path / schema::dir::heads, schema::archive::output), 1, 0, random),
    output_body_(body(config.path, schema::archive::output), config.output_size, config.output_rate, sequential),
    output(output_head_, output_body_),

    point_head_(head(config.path / schema::dir::heads, schema::archive::point), 1, 0, random),
    point_body_(body(config.path, schema::archive::point), config.point_size, config.point_rate, sequential),
    point(point_head_, point_body_, config.point_buckets),

    ins_head_(head(config.path / schema::dir::heads, schema::archive::ins), 1, 0, random),
    ins_body_(body(config.path, schema::archive::ins), config.ins_size, config.ins_rate, sequential),
    ins(ins_head_, ins_body_),

    outs_head_(head(config.path / schema::dir::heads, schema::archive::outs), 1, 0, random),
    outs_body_(body(config.path, schema::archive::outs), config.outs_size, config.outs_rate, sequential),
    outs(outs_head_, outs_body_),

    tx_head_(head(config.path / schema::dir::heads, schema::archive::tx), 1, 0, random),
    tx_body_(body(config.path, schema::archive::tx), config.tx_size, config.tx_rate, sequential),
    tx(tx_head_, tx_body_, config.tx_buckets),

    txs_head_(head(config.path / schema::dir::heads, schema::archive::txs), 1, 0, random),
    txs_body_(body(config.path, schema::archive::txs), config.txs_size, config.txs_rate, sequential),
    txs(txs_head_, txs_body_, config.txs_buckets),

    // Indexes.
    // ------------------------------------------------------------------------

    candidate_head_(head(config.path / schema::dir::heads, schema::indexes::candidate), 1, 0, random),
    candidate_body_(body(config.path, schema::indexes::candidate), config.candidate_size, config.candidate_rate, sequential),
    candidate(candidate_head_, candidate_body_),

    confirmed_head_(head(config.path / schema::dir::heads, schema::indexes::confirmed), 1, 0, random),
    confirmed_body_(body(config.path, schema::indexes::confirmed), config.confirmed_size, config.confirmed_rate, sequential),
    confirmed(confirmed_head_, confirmed_body_),

    strong_tx_head_(head(config.path / schema::dir::heads, schema::indexes::strong_tx), 1, 0, random),
    strong_tx_body_(body(config.path, schema::indexes::strong_tx), config.strong_tx_size, config.strong_tx_rate, sequential),
    strong_tx(strong_tx_head_, strong_tx_body_, config.strong_tx_buckets),

    // Caches.
    // ------------------------------------------------------------------------

    duplicate_head_(head(config.path / schema::dir::heads, schema::caches::duplicate), 1, 0, random),
    duplicate_body_(body(config.path, schema::caches::duplicate), config.duplicate_size, config.duplicate_rate, sequential),
    duplicate(duplicate_head_, duplicate_body_, config.duplicate_buckets),

    prevout_head_(head(config.path / schema::dir::heads, schema::caches::prevout), 1, 0, random),
    prevout_body_(body(config.path, schema::caches::prevout), config.prevout_size, config.prevout_rate, sequential),
    prevout(prevout_head_, prevout_body_, config.prevout_buckets),

    validated_bk_head_(head(config.path / schema::dir::heads, schema::caches::validated_bk), 1, 0, random),
    validated_bk_body_(body(config.path, schema::caches::validated_bk), config.validated_bk_size, config.validated_bk_rate, sequential),
    validated_bk(validated_bk_head_, validated_bk_body_, config.validated_bk_buckets),

    validated_tx_head_(head(config.path / schema::dir::heads, schema::caches::validated_tx), 1, 0, random),
    validated_tx_body_(body(config.path, schema::caches::validated_tx), config.validated_tx_size, config.validated_tx_rate, sequential),
    validated_tx(validated_tx_head_, validated_tx_body_, config.validated_tx_buckets),

    // Optionals.
    // ------------------------------------------------------------------------

    address_head_(head(config.path / schema::dir::heads, schema::optionals::address), 1, 0, random),
    address_body_(body(config.path, schema::optionals::address), config.address_size, config.address_rate, sequential),
    address(address_head_, address_body_, config.address_buckets),

    filter_bk_head_(head(config.path / schema::dir::heads, schema::optionals::filter_bk), 1, 0, random),
    filter_bk_body_(body(config.path, schema::optionals::filter_bk), config.filter_bk_size, config.filter_bk_rate, sequential),
    filter_bk(filter_bk_head_, filter_bk_body_, config.filter_bk_buckets),

    filter_tx_head_(head(config.path / schema::dir::heads, schema::optionals::filter_tx), 1, 0, random),
    filter_tx_body_(body(config.path, schema::optionals::filter_tx), config.filter_tx_size, config.filter_tx_rate, sequential),
    filter_tx(filter_tx_head_, filter_tx_body_, config.filter_tx_buckets),

    // Locks.
    // ------------------------------------------------------------------------

    flush_lock_(lock(config.path, schema::locks::flush)),
    process_lock_(lock(config.path, schema::locks::process))
{
}

TEMPLATE
bool CLASS::turbo() const NOEXCEPT
{
    return configuration_.turbo;
}

TEMPLATE
uint8_t CLASS::interval_depth() const NOEXCEPT
{
    return configuration_.interval_depth;
}

TEMPLATE
code CLASS::create(const event_handler& handler) NOEXCEPT
{
    if (!transactor_mutex_.try_lock())
        return error::transactor_lock;

    if (!process_lock_.try_lock())
    {
        transactor_mutex_.unlock();
        return error::process_lock;
    }

    if (!flush_lock_.try_lock())
    {
        /* bool */ process_lock_.try_unlock();
        transactor_mutex_.unlock();
        return error::flush_lock;
    }

    const auto create = [&handler](code& ec, const auto& storage,
        table_t table) NOEXCEPT
    {
        if (!ec)
        {
            handler(event_t::create_file, table);
            ec = file::create_file_ex(storage.file());
        }
    };

    static const auto heads = configuration_.path / schema::dir::heads;
    auto ec = file::clear_directory_ex(heads);

    create(ec, header_head_, table_t::header_head);
    create(ec, header_body_, table_t::header_body);
    create(ec, input_head_, table_t::input_head);
    create(ec, input_body_, table_t::input_body);
    create(ec, output_head_, table_t::output_head);
    create(ec, output_body_, table_t::output_body);
    create(ec, point_head_, table_t::point_head);
    create(ec, point_body_, table_t::point_body);
    create(ec, ins_head_, table_t::ins_head);
    create(ec, ins_body_, table_t::ins_body);
    create(ec, outs_head_, table_t::outs_head);
    create(ec, outs_body_, table_t::outs_body);
    create(ec, tx_head_, table_t::tx_head);
    create(ec, tx_body_, table_t::tx_body);
    create(ec, txs_head_, table_t::txs_head);
    create(ec, txs_body_, table_t::txs_body);

    create(ec, candidate_head_, table_t::candidate_head);
    create(ec, candidate_body_, table_t::candidate_body);
    create(ec, confirmed_head_, table_t::confirmed_head);
    create(ec, confirmed_body_, table_t::confirmed_body);
    create(ec, strong_tx_head_, table_t::strong_tx_head);
    create(ec, strong_tx_body_, table_t::strong_tx_body);

    create(ec, duplicate_head_, table_t::duplicate_head);
    create(ec, duplicate_body_, table_t::duplicate_body);
    create(ec, prevout_head_, table_t::prevout_head);
    create(ec, prevout_body_, table_t::prevout_body);
    create(ec, validated_bk_head_, table_t::validated_bk_head);
    create(ec, validated_bk_body_, table_t::validated_bk_body);
    create(ec, validated_tx_head_, table_t::validated_tx_head);
    create(ec, validated_tx_body_, table_t::validated_tx_body);

    create(ec, address_head_, table_t::address_head);
    create(ec, address_body_, table_t::address_body);
    create(ec, filter_bk_head_, table_t::filter_bk_head);
    create(ec, filter_bk_body_, table_t::filter_bk_body);
    create(ec, filter_tx_head_, table_t::filter_tx_head);
    create(ec, filter_tx_body_, table_t::filter_tx_body);

    const auto populate = [&handler](code& ec, auto& storage,
        table_t table) NOEXCEPT
    {
        if (!ec)
        {
            handler(event_t::create_table, table);
            if (!storage.create())
                ec = error::create_table;
        }
    };

    if (!ec) ec = open_load(handler);

    // Populate /heads files and truncate body sizes to zero.
    populate(ec, header, table_t::header_table);
    populate(ec, input, table_t::input_table);
    populate(ec, output, table_t::output_table);
    populate(ec, point, table_t::point_table);
    populate(ec, ins, table_t::ins_table);
    populate(ec, outs, table_t::outs_table);
    populate(ec, tx, table_t::tx_table);
    populate(ec, txs, table_t::txs_table);

    populate(ec, candidate, table_t::candidate_table);
    populate(ec, confirmed, table_t::confirmed_table);
    populate(ec, strong_tx, table_t::strong_tx_table);

    populate(ec, duplicate, table_t::duplicate_table);
    populate(ec, prevout, table_t::prevout_table);
    populate(ec, validated_bk, table_t::validated_bk_table);
    populate(ec, validated_tx, table_t::validated_tx_table);

    populate(ec, address, table_t::address_table);
    populate(ec, filter_bk, table_t::filter_bk_table);
    populate(ec, filter_tx, table_t::filter_tx_table);

    if (ec)
    {
        /* code */ unload_close(handler);

        // unlock errors override ec.
        if (!flush_lock_.try_unlock()) ec = error::flush_unlock;
        if (!process_lock_.try_unlock()) ec = error::process_unlock;
        /* bool */ file::clear_directory(configuration_.path);
        /* bool */ file::remove(configuration_.path);
    }

    // process and flush locks remain open until close().
    transactor_mutex_.unlock();
    return ec;
}

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

    if (!flush_lock_.try_lock())
    {
        /* bool */ process_lock_.try_unlock();
        transactor_mutex_.unlock();
        return error::flush_lock;
    }

    const auto verify = [&handler](code& ec, auto& storage,
        table_t table) NOEXCEPT
    {
        if (!ec)
        {
            handler(event_t::verify_table, table);
            if (!storage.verify())
                ec = error::verify_table;
        }
    };

    auto ec = open_load(handler);

    verify(ec, header, table_t::header_table);
    verify(ec, input, table_t::input_table);
    verify(ec, output, table_t::output_table);
    verify(ec, point, table_t::point_table);
    verify(ec, ins, table_t::ins_table);
    verify(ec, outs, table_t::outs_table);
    verify(ec, tx, table_t::tx_table);
    verify(ec, txs, table_t::txs_table);

    verify(ec, candidate, table_t::candidate_table);
    verify(ec, confirmed, table_t::confirmed_table);
    verify(ec, strong_tx, table_t::strong_tx_table);

    verify(ec, duplicate, table_t::duplicate_table);
    verify(ec, prevout, table_t::prevout_table);
    verify(ec, validated_bk, table_t::validated_bk_table);
    verify(ec, validated_tx, table_t::validated_tx_table);

    verify(ec, address, table_t::address_table);
    verify(ec, filter_bk, table_t::filter_bk_table);
    verify(ec, filter_tx, table_t::filter_tx_table);

    if (ec)
    {
        /* code */ unload_close(handler);

        // unlock errors override ec.
        if (!flush_lock_.try_unlock()) ec = error::flush_unlock;
        if (!process_lock_.try_unlock()) ec = error::process_unlock;
    }

    // process and flush locks remain open until close().
    transactor_mutex_.unlock();
    return ec;
}

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
        // nullify table head, set reference body count to zero.
        handler(event_t::prune_table, table_t::prevout_head);
        if (!prevout.clear())
        {
            ec = error::prune_table;
        }
        else
        {
            // Snapshot with nullified head and zero body count.
            // The 'prune' parameter signals to not reset body count.
            ec = snapshot(handler, true);

            // If the pruning fails here the snapshot remains valid.
            if (!ec)
            {
                // zeroize table body, set logical body count to zero.
                handler(event_t::prune_table, table_t::prevout_body);
                if (!prevout_body_.truncate(zero))
                {
                    ec = error::prune_table;
                }
                else
                {
                    // unmap body, setting mapped size to logical size (zero).
                    handler(event_t::unload_file, table_t::prevout_body);
                    ec = prevout_body_.unload();

                    if (!ec)
                    {
                        // map body, making table usable again.
                        handler(event_t::load_file, table_t::prevout_body);
                        ec = prevout_body_.load();
                    }
                }
            }
        }
    }

    transactor_mutex_.unlock();
    return ec;
}

TEMPLATE
code CLASS::snapshot(const event_handler& handler, bool prune) NOEXCEPT
{
    while (!prune && !transactor_mutex_.try_lock_for(std::chrono::seconds(1)))
    {
        handler(event_t::wait_lock, table_t::store);
    }

    code ec{ error::success };
    const auto flush = [&handler](code& ec, auto& storage, table_t table) NOEXCEPT
    {
        if (!ec)
        {
            handler(event_t::flush_body, table);
            ec = storage.flush();
        }
    };

    // Assumes/requires tables open/loaded.
    flush(ec, header_body_, table_t::header_body);
    flush(ec, input_body_, table_t::input_body);
    flush(ec, output_body_, table_t::output_body);
    flush(ec, point_body_, table_t::point_body);
    flush(ec, ins_body_, table_t::ins_body);
    flush(ec, outs_body_, table_t::outs_body);
    flush(ec, tx_body_, table_t::tx_body);
    flush(ec, txs_body_, table_t::txs_body);

    flush(ec, candidate_body_, table_t::candidate_body);
    flush(ec, confirmed_body_, table_t::confirmed_body);
    flush(ec, strong_tx_body_, table_t::strong_tx_body);

    flush(ec, duplicate_body_, table_t::duplicate_body);
    if (!prune) flush(ec, prevout_body_, table_t::prevout_body);
    flush(ec, validated_bk_body_, table_t::validated_bk_body);
    flush(ec, validated_tx_body_, table_t::validated_tx_body);

    flush(ec, address_body_, table_t::address_body);
    flush(ec, filter_bk_body_, table_t::filter_bk_body);
    flush(ec, filter_tx_body_, table_t::filter_tx_body);

    if (!ec) ec = backup(handler, prune);
    if (!prune) transactor_mutex_.unlock();
    return ec;
}

TEMPLATE
code CLASS::reload(const event_handler& handler) NOEXCEPT
{
    while (!transactor_mutex_.try_lock_for(std::chrono::seconds(1)))
    {
        handler(event_t::wait_lock, table_t::store);
    }

    code ec{ error::success };
    const auto reload = [&handler, this](code& ec, auto& storage,
        table_t table) NOEXCEPT
    {
        if (!ec)
        {
            // If any storage has a fault it will return as failure code.
            if (to_bool(storage.get_space()))
            {
                handler(event_t::load_file, table);
                ec = storage.reload();
                this->dirty_ = true;
            }
        }
    };

    reload(ec, header_head_, table_t::header_head);
    reload(ec, header_body_, table_t::header_body);
    reload(ec, input_head_, table_t::input_head);
    reload(ec, input_body_, table_t::input_body);
    reload(ec, output_head_, table_t::output_head);
    reload(ec, output_body_, table_t::output_body);
    reload(ec, point_head_, table_t::point_head);
    reload(ec, point_body_, table_t::point_body);
    reload(ec, ins_head_, table_t::ins_head);
    reload(ec, ins_body_, table_t::ins_body);
    reload(ec, outs_head_, table_t::outs_head);
    reload(ec, outs_body_, table_t::outs_body);
    reload(ec, tx_head_, table_t::tx_head);
    reload(ec, tx_body_, table_t::tx_body);
    reload(ec, txs_head_, table_t::txs_head);
    reload(ec, txs_body_, table_t::txs_body);

    reload(ec, candidate_head_, table_t::candidate_head);
    reload(ec, candidate_body_, table_t::candidate_body);
    reload(ec, confirmed_head_, table_t::confirmed_head);
    reload(ec, confirmed_body_, table_t::confirmed_body);
    reload(ec, strong_tx_head_, table_t::strong_tx_head);
    reload(ec, strong_tx_body_, table_t::strong_tx_body);

    reload(ec, duplicate_head_, table_t::duplicate_head);
    reload(ec, duplicate_body_, table_t::duplicate_body);
    reload(ec, prevout_head_, table_t::prevout_head);
    reload(ec, prevout_body_, table_t::prevout_body);
    reload(ec, validated_bk_head_, table_t::validated_bk_head);
    reload(ec, validated_bk_body_, table_t::validated_bk_body);
    reload(ec, validated_tx_head_, table_t::validated_tx_head);
    reload(ec, validated_tx_body_, table_t::validated_tx_body);

    reload(ec, address_head_, table_t::address_head);
    reload(ec, address_body_, table_t::address_body);
    reload(ec, filter_bk_head_, table_t::filter_bk_head);
    reload(ec, filter_bk_body_, table_t::filter_bk_body);
    reload(ec, filter_tx_head_, table_t::filter_tx_head);
    reload(ec, filter_tx_body_, table_t::filter_tx_body);

    transactor_mutex_.unlock();
    return ec;
}

TEMPLATE
code CLASS::close(const event_handler& handler) NOEXCEPT
{
    // Transactor may be held outside of the node, such as for backup. 
    while (!transactor_mutex_.try_lock_for(std::chrono::seconds(1)))
    {
        handler(event_t::wait_lock, table_t::store);
    }

    code ec{ error::success };
    const auto close = [&handler](code& ec, auto& storage, table_t table) NOEXCEPT
    {
        if (!ec)
        {
            handler(event_t::close_table, table);
            if (!storage.close())
                ec = error::close_table;
        }
    };

    close(ec, header, table_t::header_table);
    close(ec, input, table_t::input_table);
    close(ec, output, table_t::output_table);
    close(ec, point, table_t::point_table);
    close(ec, ins, table_t::ins_table);
    close(ec, outs, table_t::outs_table);
    close(ec, tx, table_t::tx_table);
    close(ec, txs, table_t::txs_table);

    close(ec, candidate, table_t::candidate_table);
    close(ec, confirmed, table_t::confirmed_table);
    close(ec, strong_tx, table_t::strong_tx_table);

    close(ec, duplicate, table_t::duplicate_table);
    close(ec, prevout, table_t::prevout_table);
    close(ec, validated_bk, table_t::validated_bk_table);
    close(ec, validated_tx, table_t::validated_tx_table);

    close(ec, address, table_t::address_table);
    close(ec, filter_bk, table_t::filter_bk_table);
    close(ec, filter_tx, table_t::filter_tx_table);

    if (!ec) ec = unload_close(handler);

    // unlock errors override ec.
    if (!process_lock_.try_unlock())
        ec = error::process_unlock;

    // fault overrides unlock errors and leaves behind flush_lock.
    if (get_fault())
        ec = error::integrity;
    else if (!flush_lock_.try_unlock())
        ec = error::flush_unlock;

    transactor_mutex_.unlock();
    return ec;
}

// protected
// ----------------------------------------------------------------------------

TEMPLATE
code CLASS::open_load(const event_handler& handler) NOEXCEPT
{
    code ec{ error::success };
    const auto open = [&handler](code& ec, auto& storage, table_t table) NOEXCEPT
    {
        if (!ec)
        {
            handler(event_t::open_file, table);
            ec = storage.open();
        }
    };

    open(ec, header_head_, table_t::header_head);
    open(ec, header_body_, table_t::header_body);
    open(ec, input_head_, table_t::input_head);
    open(ec, input_body_, table_t::input_body);
    open(ec, output_head_, table_t::output_head);
    open(ec, output_body_, table_t::output_body);
    open(ec, point_head_, table_t::point_head);
    open(ec, point_body_, table_t::point_body);
    open(ec, ins_head_, table_t::ins_head);
    open(ec, ins_body_, table_t::ins_body);
    open(ec, outs_head_, table_t::outs_head);
    open(ec, outs_body_, table_t::outs_body);
    open(ec, tx_head_, table_t::tx_head);
    open(ec, tx_body_, table_t::tx_body);
    open(ec, txs_head_, table_t::txs_head);
    open(ec, txs_body_, table_t::txs_body);

    open(ec, candidate_head_, table_t::candidate_head);
    open(ec, candidate_body_, table_t::candidate_body);
    open(ec, confirmed_head_, table_t::confirmed_head);
    open(ec, confirmed_body_, table_t::confirmed_body);
    open(ec, strong_tx_head_, table_t::strong_tx_head);
    open(ec, strong_tx_body_, table_t::strong_tx_body);

    open(ec, duplicate_head_, table_t::duplicate_head);
    open(ec, duplicate_body_, table_t::duplicate_body);
    open(ec, prevout_head_, table_t::prevout_head);
    open(ec, prevout_body_, table_t::prevout_body);
    open(ec, validated_bk_head_, table_t::validated_bk_head);
    open(ec, validated_bk_body_, table_t::validated_bk_body);
    open(ec, validated_tx_head_, table_t::validated_tx_head);
    open(ec, validated_tx_body_, table_t::validated_tx_body);

    open(ec, address_head_, table_t::address_head);
    open(ec, address_body_, table_t::address_body);
    open(ec, filter_bk_head_, table_t::filter_bk_head);
    open(ec, filter_bk_body_, table_t::filter_bk_body);
    open(ec, filter_tx_head_, table_t::filter_tx_head);
    open(ec, filter_tx_body_, table_t::filter_tx_body);

    const auto load = [&handler](code& ec, auto& storage, table_t table) NOEXCEPT
    {
        if (!ec)
        {
            handler(event_t::load_file, table);
            ec = storage.load();
        }
    };

    load(ec, header_head_, table_t::header_head);
    load(ec, header_body_, table_t::header_body);
    load(ec, input_head_, table_t::input_head);
    load(ec, input_body_, table_t::input_body);
    load(ec, output_head_, table_t::output_head);
    load(ec, output_body_, table_t::output_body);
    load(ec, point_head_, table_t::point_head);
    load(ec, point_body_, table_t::point_body);
    load(ec, ins_head_, table_t::ins_head);
    load(ec, ins_body_, table_t::ins_body);
    load(ec, outs_head_, table_t::outs_head);
    load(ec, outs_body_, table_t::outs_body);
    load(ec, tx_head_, table_t::tx_head);
    load(ec, tx_body_, table_t::tx_body);
    load(ec, txs_head_, table_t::txs_head);
    load(ec, txs_body_, table_t::txs_body);

    load(ec, candidate_head_, table_t::candidate_head);
    load(ec, candidate_body_, table_t::candidate_body);
    load(ec, confirmed_head_, table_t::confirmed_head);
    load(ec, confirmed_body_, table_t::confirmed_body);
    load(ec, strong_tx_head_, table_t::strong_tx_head);
    load(ec, strong_tx_body_, table_t::strong_tx_body);

    load(ec, duplicate_head_, table_t::duplicate_head);
    load(ec, duplicate_body_, table_t::duplicate_body);
    load(ec, prevout_head_, table_t::prevout_head);
    load(ec, prevout_body_, table_t::prevout_body);
    load(ec, validated_bk_head_, table_t::validated_bk_head);
    load(ec, validated_bk_body_, table_t::validated_bk_body);
    load(ec, validated_tx_head_, table_t::validated_tx_head);
    load(ec, validated_tx_body_, table_t::validated_tx_body);

    load(ec, address_head_, table_t::address_head);
    load(ec, address_body_, table_t::address_body);
    load(ec, filter_bk_head_, table_t::filter_bk_head);
    load(ec, filter_bk_body_, table_t::filter_bk_body);
    load(ec, filter_tx_head_, table_t::filter_tx_head);
    load(ec, filter_tx_body_, table_t::filter_tx_body);

    // create, open, and restore each invoke open_load.
    dirty_ = header_body_.size() > schema::header::minrow;
    return ec;
}

TEMPLATE
code CLASS::unload_close(const event_handler& handler) NOEXCEPT
{
    code ec{ error::success };
    const auto unload = [&handler](code& ec, auto& storage, table_t table) NOEXCEPT
    {
        if (!ec)
        {
            handler(event_t::unload_file, table);
            ec = storage.unload();
        }
    };

    unload(ec, header_head_, table_t::header_head);
    unload(ec, header_body_, table_t::header_body);
    unload(ec, input_head_, table_t::input_head);
    unload(ec, input_body_, table_t::input_body);
    unload(ec, output_head_, table_t::output_head);
    unload(ec, output_body_, table_t::output_body);
    unload(ec, point_head_, table_t::point_head);
    unload(ec, point_body_, table_t::point_body);
    unload(ec, ins_head_, table_t::ins_head);
    unload(ec, ins_body_, table_t::ins_body);
    unload(ec, outs_head_, table_t::outs_head);
    unload(ec, outs_body_, table_t::outs_body);
    unload(ec, tx_head_, table_t::tx_head);
    unload(ec, tx_body_, table_t::tx_body);
    unload(ec, txs_head_, table_t::txs_head);
    unload(ec, txs_body_, table_t::txs_body);

    unload(ec, candidate_head_, table_t::candidate_head);
    unload(ec, candidate_body_, table_t::candidate_body);
    unload(ec, confirmed_head_, table_t::confirmed_head);
    unload(ec, confirmed_body_, table_t::confirmed_body);
    unload(ec, strong_tx_head_, table_t::strong_tx_head);
    unload(ec, strong_tx_body_, table_t::strong_tx_body);

    unload(ec, duplicate_head_, table_t::duplicate_head);
    unload(ec, duplicate_body_, table_t::duplicate_body);
    unload(ec, prevout_head_, table_t::prevout_head);
    unload(ec, prevout_body_, table_t::prevout_body);
    unload(ec, validated_bk_head_, table_t::validated_bk_head);
    unload(ec, validated_bk_body_, table_t::validated_bk_body);
    unload(ec, validated_tx_head_, table_t::validated_tx_head);
    unload(ec, validated_tx_body_, table_t::validated_tx_body);

    unload(ec, address_head_, table_t::address_head);
    unload(ec, address_body_, table_t::address_body);
    unload(ec, filter_bk_head_, table_t::filter_bk_head);
    unload(ec, filter_bk_body_, table_t::filter_bk_body);
    unload(ec, filter_tx_head_, table_t::filter_tx_head);
    unload(ec, filter_tx_body_, table_t::filter_tx_body);

    const auto close = [&handler](code& ec, auto& storage, table_t table) NOEXCEPT
    {
        if (!ec)
        {
            handler(event_t::close_file, table);
            ec = storage.close();
        }
    };

    close(ec, header_head_, table_t::header_head);
    close(ec, header_body_, table_t::header_body);
    close(ec, input_head_, table_t::input_head);
    close(ec, input_body_, table_t::input_body);
    close(ec, output_head_, table_t::output_head);
    close(ec, output_body_, table_t::output_body);
    close(ec, point_head_, table_t::point_head);
    close(ec, point_body_, table_t::point_body);
    close(ec, ins_head_, table_t::ins_head);
    close(ec, ins_body_, table_t::ins_body);
    close(ec, outs_head_, table_t::outs_head);
    close(ec, outs_body_, table_t::outs_body);
    close(ec, tx_head_, table_t::tx_head);
    close(ec, tx_body_, table_t::tx_body);
    close(ec, txs_head_, table_t::txs_head);
    close(ec, txs_body_, table_t::txs_body);

    close(ec, candidate_head_, table_t::candidate_head);
    close(ec, candidate_body_, table_t::candidate_body);
    close(ec, confirmed_head_, table_t::confirmed_head);
    close(ec, confirmed_body_, table_t::confirmed_body);
    close(ec, strong_tx_head_, table_t::strong_tx_head);
    close(ec, strong_tx_body_, table_t::strong_tx_body);

    close(ec, duplicate_head_, table_t::duplicate_head);
    close(ec, duplicate_body_, table_t::duplicate_body);
    close(ec, prevout_head_, table_t::prevout_head);
    close(ec, prevout_body_, table_t::prevout_body);
    close(ec, validated_bk_head_, table_t::validated_bk_head);
    close(ec, validated_bk_body_, table_t::validated_bk_body);
    close(ec, validated_tx_head_, table_t::validated_tx_head);
    close(ec, validated_tx_body_, table_t::validated_tx_body);

    close(ec, address_head_, table_t::address_head);
    close(ec, address_body_, table_t::address_body);
    close(ec, filter_bk_head_, table_t::filter_bk_head);
    close(ec, filter_bk_body_, table_t::filter_bk_body);
    close(ec, filter_tx_head_, table_t::filter_tx_head);
    close(ec, filter_tx_body_, table_t::filter_tx_body);

    return ec;
}

TEMPLATE
code CLASS::backup(const event_handler& handler, bool prune) NOEXCEPT
{
    code ec{ error::success };
    const auto backup = [&handler](code& ec, auto& storage,
        table_t table, bool prune=false) NOEXCEPT
    {
        if (!ec)
        {
            handler(event_t::backup_table, table);
            if (!storage.backup(prune))
                ec = error::backup_table;
        }
    };

    backup(ec, header, table_t::header_table);
    backup(ec, input, table_t::input_table);
    backup(ec, output, table_t::output_table);
    backup(ec, point, table_t::point_table);
    backup(ec, ins, table_t::ins_table);
    backup(ec, outs, table_t::outs_table);
    backup(ec, tx, table_t::tx_table);
    backup(ec, txs, table_t::txs_table);

    backup(ec, candidate, table_t::candidate_table);
    backup(ec, confirmed, table_t::confirmed_table);
    backup(ec, strong_tx, table_t::strong_tx_table);

    backup(ec, duplicate, table_t::duplicate_table);
    backup(ec, prevout, table_t::prevout_table, prune);
    backup(ec, validated_bk, table_t::validated_bk_table);
    backup(ec, validated_tx, table_t::validated_tx_table);

    backup(ec, address, table_t::address_table);
    backup(ec, filter_bk, table_t::filter_bk_table);
    backup(ec, filter_tx, table_t::filter_tx_table);

    if (ec) return ec;

    static const auto primary = configuration_.path / schema::dir::primary;
    static const auto secondary = configuration_.path / schema::dir::secondary;
    static const auto temporary = configuration_.path / schema::dir::temporary;

    handler(event_t::archive_snapshot, table_t::store);

    // Ensure existing and empty /temporary.
    if ((ec = file::clear_directory_ex(temporary))) return ec;

    // Ensure no /primary.
    if (file::is_directory(primary))
    {
        // Delete /secondary.
        if ((ec = file::clear_directory_ex(secondary))) return ec;
        if ((ec = file::remove_ex(secondary))) return ec;

        // Rename /primary to /secondary (atomic).
        if ((ec = file::rename_ex(primary, secondary))) return ec;
    }

    // Dump /heads memory maps to /temporary.
    if ((ec = dump(temporary, handler)))
    {
        // Failed dump, clear temporary and rename secondary to primary.
        if (file::clear_directory(temporary) && file::remove(temporary))
            file::rename(secondary, primary);

        // Return original fault.
        return ec;
    }

    // Rename /temporary to /primary (atomic).
    return file::rename_ex(temporary, primary);
}

// Dump memory maps of /heads to new files in /temporary.
// Heads are copied from RAM, not flushed to disk and copied as files.
TEMPLATE
code CLASS::dump(const path& folder,
    const event_handler& handler) NOEXCEPT
{
    auto header_buffer = header_head_.get();
    auto input_buffer = input_head_.get();
    auto output_buffer = output_head_.get();
    auto point_buffer = point_head_.get();
    auto ins_buffer = ins_head_.get();
    auto outs_buffer = outs_head_.get();
    auto tx_buffer = tx_head_.get();
    auto txs_buffer = txs_head_.get();

    auto candidate_buffer = candidate_head_.get();
    auto confirmed_buffer = confirmed_head_.get();
    auto strong_tx_buffer = strong_tx_head_.get();

    auto duplicate_buffer = duplicate_head_.get();
    auto prevout_buffer = prevout_head_.get();
    auto validated_bk_buffer = validated_bk_head_.get();
    auto validated_tx_buffer = validated_tx_head_.get();

    auto address_buffer = address_head_.get();
    auto filter_bk_buffer = filter_bk_head_.get();
    auto filter_tx_buffer = filter_tx_head_.get();

    if (!header_buffer) return error::unloaded_file;
    if (!input_buffer) return error::unloaded_file;
    if (!output_buffer) return error::unloaded_file;
    if (!point_buffer) return error::unloaded_file;
    if (!ins_buffer) return error::unloaded_file;
    if (!outs_buffer) return error::unloaded_file;
    if (!tx_buffer) return error::unloaded_file;
    if (!txs_buffer) return error::unloaded_file;

    if (!candidate_buffer) return error::unloaded_file;
    if (!confirmed_buffer) return error::unloaded_file;
    if (!strong_tx_buffer) return error::unloaded_file;

    if (!duplicate_buffer) return error::unloaded_file;
    if (!prevout_buffer) return error::unloaded_file;
    if (!validated_bk_buffer) return error::unloaded_file;
    if (!validated_tx_buffer) return error::unloaded_file;

    if (!address_buffer) return error::unloaded_file;
    if (!filter_bk_buffer) return error::unloaded_file;
    if (!filter_tx_buffer) return error::unloaded_file;

    code ec{ error::success };
    const auto dump = [&handler, &folder](code& ec, const auto& storage,
        const auto& name, table_t table) NOEXCEPT
    {
        if (!ec)
        {
            handler(event_t::copy_header, table);
            ec = file::create_file_ex(head(folder, name), storage->begin(),
                storage->size());
        }
    };

    dump(ec, header_buffer, schema::archive::header, table_t::header_head);
    dump(ec, input_buffer, schema::archive::input, table_t::input_head);
    dump(ec, output_buffer, schema::archive::output, table_t::output_head);
    dump(ec, point_buffer, schema::archive::point, table_t::point_head);
    dump(ec, ins_buffer, schema::archive::ins, table_t::ins_head);
    dump(ec, outs_buffer, schema::archive::outs, table_t::outs_head);
    dump(ec, tx_buffer, schema::archive::tx, table_t::tx_head);
    dump(ec, txs_buffer, schema::archive::txs, table_t::txs_head);

    dump(ec, candidate_buffer, schema::indexes::candidate, table_t::candidate_head);
    dump(ec, confirmed_buffer, schema::indexes::confirmed, table_t::confirmed_head);
    dump(ec, strong_tx_buffer, schema::indexes::strong_tx, table_t::strong_tx_head);

    dump(ec, duplicate_buffer, schema::caches::duplicate, table_t::duplicate_head);
    dump(ec, prevout_buffer, schema::caches::prevout, table_t::prevout_head);
    dump(ec, validated_bk_buffer, schema::caches::validated_bk, table_t::validated_bk_head);
    dump(ec, validated_tx_buffer, schema::caches::validated_tx, table_t::validated_tx_head);

    dump(ec, address_buffer, schema::optionals::address, table_t::address_head);
    dump(ec, filter_bk_buffer, schema::optionals::filter_bk, table_t::filter_bk_head);
    dump(ec, filter_tx_buffer, schema::optionals::filter_tx, table_t::filter_tx_head);

    return ec;
}

TEMPLATE
code CLASS::restore(const event_handler& handler) NOEXCEPT
{
    if (!transactor_mutex_.try_lock())
        return error::transactor_lock;

    if (!process_lock_.try_lock())
    {
        transactor_mutex_.unlock();
        return error::process_lock;
    }

    // Requires that the store is already flush locked (corrupted).
    if (!flush_lock_.is_locked())
    {
        /* bool */ process_lock_.try_unlock();
        transactor_mutex_.unlock();
        return error::flush_lock;
    }

    code ec{ error::success };
    static const auto heads = configuration_.path / schema::dir::heads;
    static const auto primary = configuration_.path / schema::dir::primary;
    static const auto secondary = configuration_.path / schema::dir::secondary;
    static const auto temporary = configuration_.path / schema::dir::temporary;

    handler(event_t::recover_snapshot, table_t::store);

    // Clean up any residual /temporary.
    file::clear_directory(temporary);
    file::remove(temporary);

    if (file::is_directory(primary))
    {
        // Clear invalid /heads, recover from /primary, clone to /primary.
        ec = file::clear_directory_ex(heads);
        if (!ec) ec = file::remove_ex(heads);
        if (!ec) ec = file::rename_ex(primary, heads);
        if (!ec) ec = file::copy_directory_ex(heads, primary);
    }
    else if (file::is_directory(secondary))
    {
        // Clear invalid /heads, recover from /secondary, clone to /primary.
        ec = file::clear_directory_ex(heads);
        if (!ec) ec = file::remove_ex(heads);
        if (!ec) ec = file::rename_ex(secondary, heads);
        if (!ec) ec = file::copy_directory_ex(heads, primary);
    }
    else
    {
        ec = error::missing_snapshot;
    }

    const auto restore = [&handler](code& ec, auto& storage, table_t table) NOEXCEPT
    {
        if (!ec)
        {
            handler(event_t::restore_table, table);
            if (!storage.restore())
                ec = error::restore_table;
        }
    };

    if (!ec)
        ec = open_load(handler);

    if (!ec)
    {
        restore(ec, header, table_t::header_table);
        restore(ec, input, table_t::input_table);
        restore(ec, output, table_t::output_table);
        restore(ec, point, table_t::point_table);
        restore(ec, ins, table_t::ins_table);
        restore(ec, outs, table_t::outs_table);
        restore(ec, tx, table_t::tx_table);
        restore(ec, txs, table_t::txs_table);

        restore(ec, candidate, table_t::candidate_table);
        restore(ec, confirmed, table_t::confirmed_table);
        restore(ec, strong_tx, table_t::strong_tx_table);

        restore(ec, duplicate, table_t::duplicate_table);
        restore(ec, prevout, table_t::prevout_table);
        restore(ec, validated_bk, table_t::validated_bk_table);
        restore(ec, validated_tx, table_t::validated_tx_table);

        restore(ec, address, table_t::address_table);
        restore(ec, filter_bk, table_t::filter_bk_table);
        restore(ec, filter_tx, table_t::filter_tx_table);

        if (ec)
            /* code */ unload_close(handler);
    }

    if (ec)
    {
        // unlock errors override ec.
        // on failure flush_lock is left in place (store corrupt).
        // on success process and flush locks are held until close().
        ////if (!flush_lock_.try_unlock()) ec = error::flush_unlock;
        if (!process_lock_.try_unlock()) ec = error::process_unlock;
    }

    // store is open after successful restore but not otherwise.
    transactor_mutex_.unlock();
    return ec;
}

// context
// ----------------------------------------------------------------------------

TEMPLATE
const typename CLASS::transactor CLASS::get_transactor() NOEXCEPT
{
    return transactor{ transactor_mutex_ };
}

TEMPLATE
bool CLASS::is_dirty() const NOEXCEPT
{
    return dirty_;
}

TEMPLATE
code CLASS::get_fault() const NOEXCEPT
{
    code ec{ error::success };
    if ((ec = header_body_.get_fault())) return ec;
    if ((ec = input_body_.get_fault())) return ec;
    if ((ec = output_body_.get_fault())) return ec;
    if ((ec = point_body_.get_fault())) return ec;
    if ((ec = ins_body_.get_fault())) return ec;
    if ((ec = outs_body_.get_fault())) return ec;
    if ((ec = tx_body_.get_fault())) return ec;
    if ((ec = txs_body_.get_fault())) return ec;
    if ((ec = candidate_body_.get_fault())) return ec;
    if ((ec = confirmed_body_.get_fault())) return ec;
    if ((ec = strong_tx_body_.get_fault())) return ec;
    if ((ec = duplicate_body_.get_fault())) return ec;
    if ((ec = prevout_body_.get_fault())) return ec;
    if ((ec = validated_bk_body_.get_fault())) return ec;
    if ((ec = validated_tx_body_.get_fault())) return ec;
    if ((ec = address_body_.get_fault())) return ec;
    if ((ec = filter_bk_body_.get_fault())) return ec;
    if ((ec = filter_tx_body_.get_fault())) return ec;
    return ec;
}

TEMPLATE
size_t CLASS::get_space() const NOEXCEPT
{
    size_t total{};
    const auto space = [&total](auto& storage) NOEXCEPT
    {
        total = system::ceilinged_add(total, storage.get_space());
    };

    space(header_body_);
    space(input_body_);
    space(output_body_);
    space(point_body_);
    space(ins_body_);
    space(outs_body_);
    space(tx_body_);
    space(txs_body_);
    space(candidate_body_);
    space(confirmed_body_);
    space(strong_tx_body_);
    space(duplicate_body_);
    space(prevout_body_);
    space(validated_bk_body_);
    space(validated_tx_body_);
    space(address_body_);
    space(filter_bk_body_);
    space(filter_tx_body_);

    return total;
}

TEMPLATE
void CLASS::report(const error_handler& handler) const NOEXCEPT
{
    const auto report = [&handler](const auto& storage, table_t table) NOEXCEPT
    {
        auto ec = storage.get_fault();
        if (!ec && to_bool(storage.get_space()))
            ec = error::disk_full;

        handler(ec, table);
    };

    report(header_body_, table_t::header_body);
    report(input_body_, table_t::input_body);
    report(output_body_, table_t::output_body);
    report(point_body_, table_t::point_body);
    report(ins_body_, table_t::ins_body);
    report(outs_body_, table_t::outs_body);
    report(tx_body_, table_t::tx_body);
    report(txs_body_, table_t::txs_body);
    report(candidate_body_, table_t::candidate_body);
    report(confirmed_body_, table_t::confirmed_body);
    report(strong_tx_body_, table_t::strong_tx_body);
    report(duplicate_body_, table_t::duplicate_body);
    report(prevout_body_, table_t::prevout_body);
    report(validated_bk_body_, table_t::validated_bk_body);
    report(validated_tx_body_, table_t::validated_tx_body);
    report(address_body_, table_t::address_body);
    report(filter_bk_body_, table_t::filter_bk_body);
    report(filter_tx_body_, table_t::filter_tx_body);
}

BC_POP_WARNING()

} // namespace database
} // namespace libbitcoin

#endif
