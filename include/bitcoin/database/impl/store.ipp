/**
 * Copyright (c) 2011-2023 libbitcoin developers (see AUTHORS)
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

#include <algorithm>
#include <chrono>
#include <bitcoin/system.hpp>
#include <bitcoin/database/boost.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/file/file.hpp>

// TODO: evaluate performance benefits of concurrency.

namespace libbitcoin {
namespace database {

BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

// head doesn't guard (performance) and parameter is cast to Link object,
// so establish 1 as the minimum value (which also implies disabled).
constexpr auto nonzero = 1_u32;

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

    { table_t::header_table, "header_table" },
    { table_t::header_head, "header_head" },
    { table_t::header_body, "header_body" },
    { table_t::point_table, "point_table" },
    { table_t::point_head, "point_head" },
    { table_t::point_body, "point_body" },
    { table_t::input_table, "input_table" },
    { table_t::input_head, "input_head" },
    { table_t::input_body, "input_body" },
    { table_t::output_table, "output_table" },
    { table_t::output_head, "output_head" },
    { table_t::output_body, "output_body" },
    { table_t::puts_table, "puts_table" },
    { table_t::puts_head, "puts_head" },
    { table_t::puts_body, "puts_body" },
    { table_t::tx_table, "tx_table" },
    { table_t::tx_head, "tx_head" },
    { table_t::txs_table, "txs_table" },
    { table_t::tx_body, "tx_body" },
    { table_t::txs_head, "txs_head" },
    { table_t::txs_body, "txs_body" },

    { table_t::address_table, "address_table" },
    { table_t::address_head, "address_head" },
    { table_t::address_body, "address_body" },
    { table_t::candidate_table, "candidate_table" },
    { table_t::candidate_head, "candidate_head" },
    { table_t::candidate_body, "candidate_body" },
    { table_t::confirmed_table, "confirmed_table" },
    { table_t::confirmed_head, "confirmed_head" },
    { table_t::confirmed_body, "confirmed_body" },
    { table_t::spend_table, "spend_table" },
    { table_t::spend_head, "spend_head" },
    { table_t::spend_body, "spend_body" },
    { table_t::strong_tx_table, "strong_tx_table" },
    { table_t::strong_tx_head, "strong_tx_head" },
    { table_t::strong_tx_body, "strong_tx_body" },

    { table_t::validated_bk_table, "validated_bk_table" },
    { table_t::validated_bk_head, "validated_bk_head" },
    { table_t::validated_bk_body, "validated_bk_body" },
    { table_t::validated_tx_table, "validated_tx_table" },
    { table_t::validated_tx_head, "validated_tx_head" },
    { table_t::validated_tx_body, "validated_tx_body" },
    { table_t::neutrino_table, "neutrino_table" },
    { table_t::neutrino_head, "neutrino_head" },
    { table_t::neutrino_body, "neutrino_body" }
    ////{ table_t::bootstrap_table, "bootstrap_table" },
    ////{ table_t::bootstrap_head, "bootstrap_head" },
    ////{ table_t::bootstrap_body, "bootstrap_body" },
    ////{ table_t::buffer_table, "buffer_table" },
    ////{ table_t::buffer_head, "buffer_head" },
    ////{ table_t::buffer_body, "buffer_body" }
};

TEMPLATE
CLASS::store(const settings& config) NOEXCEPT
  : configuration_(config),

    // Archive.

    header_head_(head(config.path / schema::dir::heads, schema::archive::header)),
    header_body_(body(config.path, schema::archive::header), config.header_size, config.header_rate),
    header(header_head_, header_body_, std::max(config.header_buckets, nonzero)),

    input_head_(head(config.path / schema::dir::heads, schema::archive::input)),
    input_body_(body(config.path, schema::archive::input), config.input_size, config.input_rate),
    input(input_head_, input_body_),

    output_head_(head(config.path / schema::dir::heads, schema::archive::output)),
    output_body_(body(config.path, schema::archive::output), config.output_size, config.output_rate),
    output(output_head_, output_body_),

    point_head_(head(config.path / schema::dir::heads, schema::archive::point)),
    point_body_(body(config.path, schema::archive::point), config.point_size, config.point_rate),
    point(point_head_, point_body_, std::max(config.point_buckets, nonzero)),

    puts_head_(head(config.path / schema::dir::heads, schema::archive::puts)),
    puts_body_(body(config.path, schema::archive::puts), config.puts_size, config.puts_rate),
    puts(puts_head_, puts_body_),

    spend_head_(head(config.path / schema::dir::heads, schema::archive::spend)),
    spend_body_(body(config.path, schema::archive::spend), config.spend_size, config.spend_rate),
    spend(spend_head_, spend_body_, std::max(config.spend_buckets, nonzero)),

    tx_head_(head(config.path / schema::dir::heads, schema::archive::tx)),
    tx_body_(body(config.path, schema::archive::tx), config.tx_size, config.tx_rate),
    tx(tx_head_, tx_body_, std::max(config.tx_buckets, nonzero)),

    txs_head_(head(config.path / schema::dir::heads, schema::archive::txs)),
    txs_body_(body(config.path, schema::archive::txs), config.txs_size, config.txs_rate),
    txs(txs_head_, txs_body_, std::max(config.txs_buckets, nonzero)),

    // Indexes.

    candidate_head_(head(config.path / schema::dir::heads, schema::indexes::candidate)),
    candidate_body_(body(config.path, schema::indexes::candidate), config.candidate_size, config.candidate_rate),
    candidate(candidate_head_, candidate_body_),

    confirmed_head_(head(config.path / schema::dir::heads, schema::indexes::confirmed)),
    confirmed_body_(body(config.path, schema::indexes::confirmed), config.confirmed_size, config.confirmed_rate),
    confirmed(confirmed_head_, confirmed_body_),

    strong_tx_head_(head(config.path / schema::dir::heads, schema::indexes::strong_tx)),
    strong_tx_body_(body(config.path, schema::indexes::strong_tx), config.strong_tx_size, config.strong_tx_rate),
    strong_tx(strong_tx_head_, strong_tx_body_, std::max(config.strong_tx_buckets, nonzero)),

    // Caches.

    validated_bk_head_(head(config.path / schema::dir::heads, schema::caches::validated_bk)),
    validated_bk_body_(body(config.path, schema::caches::validated_bk), config.validated_bk_size, config.validated_bk_rate),
    validated_bk(validated_bk_head_, validated_bk_body_, std::max(config.validated_bk_buckets, nonzero)),

    validated_tx_head_(head(config.path / schema::dir::heads, schema::caches::validated_tx)),
    validated_tx_body_(body(config.path, schema::caches::validated_tx), config.validated_tx_size, config.validated_tx_rate),
    validated_tx(validated_tx_head_, validated_tx_body_, std::max(config.validated_tx_buckets, nonzero)),

    // Optionals.

    address_head_(head(config.path / schema::dir::heads, schema::optionals::address)),
    address_body_(body(config.path, schema::optionals::address), config.address_size, config.address_rate),
    address(address_head_, address_body_, std::max(config.address_buckets, nonzero)),

    neutrino_head_(head(config.path / schema::dir::heads, schema::optionals::neutrino)),
    neutrino_body_(body(config.path, schema::optionals::neutrino), config.neutrino_size, config.neutrino_rate),
    neutrino(neutrino_head_, neutrino_body_, std::max(config.neutrino_buckets, nonzero)),

    ////bootstrap_head_(head(config.path / schema::dir::heads, schema::optionals::bootstrap)),
    ////bootstrap_body_(body(config.path, schema::optionals::bootstrap), config.bootstrap_size, config.bootstrap_rate),
    ////bootstrap(bootstrap_head_, bootstrap_body_),

    ////buffer_head_(head(config.path / schema::dir::heads, schema::optionals::buffer)),
    ////buffer_body_(body(config.path, schema::optionals::buffer), config.buffer_size, config.buffer_rate),
    ////buffer(buffer_head_, buffer_body_, std::max(config.buffer_buckets, nonzero)),

    // Locks.
    flush_lock_(lock(config.path, schema::locks::flush)),
    process_lock_(lock(config.path, schema::locks::process))
{
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
    create(ec, puts_head_, table_t::puts_head);
    create(ec, puts_body_, table_t::puts_body);
    create(ec, spend_head_, table_t::spend_head);
    create(ec, spend_body_, table_t::spend_body);
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

    create(ec, validated_bk_head_, table_t::validated_bk_head);
    create(ec, validated_bk_body_, table_t::validated_bk_body);
    create(ec, validated_tx_head_, table_t::validated_tx_head);
    create(ec, validated_tx_body_, table_t::validated_tx_body);

    create(ec, address_head_, table_t::address_head);
    create(ec, address_body_, table_t::address_body);
    create(ec, neutrino_head_, table_t::neutrino_head);
    create(ec, neutrino_body_, table_t::neutrino_body);
    ////create(ec, bootstrap_head_, table_t::bootstrap_head);
    ////create(ec, bootstrap_body_, table_t::bootstrap_body);
    ////create(ec, buffer_head_, table_t::buffer_head);
    ////create(ec, buffer_body_, table_t::buffer_body);

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
    populate(ec, puts, table_t::puts_table);
    populate(ec, spend, table_t::spend_table);
    populate(ec, tx, table_t::tx_table);
    populate(ec, txs, table_t::txs_table);

    populate(ec, candidate, table_t::candidate_table);
    populate(ec, confirmed, table_t::confirmed_table);
    populate(ec, strong_tx, table_t::strong_tx_table);

    populate(ec, validated_bk, table_t::validated_bk_table);
    populate(ec, validated_tx, table_t::validated_tx_table);

    populate(ec, address, table_t::address_table);
    populate(ec, neutrino, table_t::neutrino_table);
    ////populate(ec, bootstrap, table_t::bootstrap_table);
    ////populate(ec, buffer, table_t::buffer_table);

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
    verify(ec, puts, table_t::puts_table);
    verify(ec, spend, table_t::spend_table);
    verify(ec, tx, table_t::tx_table);
    verify(ec, txs, table_t::txs_table);

    verify(ec, candidate, table_t::candidate_table);
    verify(ec, confirmed, table_t::confirmed_table);
    verify(ec, strong_tx, table_t::strong_tx_table);

    verify(ec, validated_bk, table_t::validated_bk_table);
    verify(ec, validated_tx, table_t::validated_tx_table);

    verify(ec, address, table_t::address_table);
    verify(ec, neutrino, table_t::neutrino_table);
    ////verify(ec, bootstrap, table_t::bootstrap_table);
    ////verify(ec, buffer, table_t::buffer_table);

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
code CLASS::snapshot(const event_handler& handler) NOEXCEPT
{
    while (!transactor_mutex_.try_lock_for(std::chrono::seconds(1)))
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
    flush(ec, puts_body_, table_t::puts_body);
    flush(ec, spend_body_, table_t::spend_body);
    flush(ec, tx_body_, table_t::tx_body);
    flush(ec, txs_body_, table_t::txs_body);

    flush(ec, candidate_body_, table_t::candidate_body);
    flush(ec, confirmed_body_, table_t::confirmed_body);
    flush(ec, strong_tx_body_, table_t::strong_tx_body);

    flush(ec, validated_bk_body_, table_t::validated_bk_body);
    flush(ec, validated_tx_body_, table_t::validated_tx_body);

    flush(ec, address_body_, table_t::address_body);
    flush(ec, neutrino_body_, table_t::neutrino_body);
    ////flush(ec, bootstrap_body_, table_t::bootstrap_body);
    ////flush(ec, buffer_body_, table_t::buffer_body);

    if (!ec) ec = backup(handler);
    transactor_mutex_.unlock();
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
    const auto reload = [&handler](code& ec, auto& storage,
        table_t table) NOEXCEPT
    {
        if (!ec)
        {
            // If any storage has a fault it will return as failure code.
            if (to_bool(storage.get_space()))
            {
                handler(event_t::load_file, table);
                ec = storage.reload();
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
    reload(ec, puts_head_, table_t::puts_head);
    reload(ec, puts_body_, table_t::puts_body);
    reload(ec, spend_head_, table_t::spend_head);
    reload(ec, spend_body_, table_t::spend_body);
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

    reload(ec, validated_bk_head_, table_t::validated_bk_head);
    reload(ec, validated_bk_body_, table_t::validated_bk_body);
    reload(ec, validated_tx_head_, table_t::validated_tx_head);
    reload(ec, validated_tx_body_, table_t::validated_tx_body);

    reload(ec, address_head_, table_t::address_head);
    reload(ec, address_body_, table_t::address_body);
    reload(ec, neutrino_head_, table_t::neutrino_head);
    reload(ec, neutrino_body_, table_t::neutrino_body);
    ////reload(ec, bootstrap_head_, table_t::bootstrap_head);
    ////reload(ec, bootstrap_body_, table_t::bootstrap_body);
    ////reload(ec, buffer_head_, table_t::buffer_head);
    ////reload(ec, buffer_body_, table_t::buffer_body);

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
    close(ec, puts, table_t::puts_table);
    close(ec, spend, table_t::spend_table);
    close(ec, tx, table_t::tx_table);
    close(ec, txs, table_t::txs_table);

    close(ec, candidate, table_t::candidate_table);
    close(ec, confirmed, table_t::confirmed_table);
    close(ec, strong_tx, table_t::strong_tx_table);

    close(ec, validated_bk, table_t::validated_bk_table);
    close(ec, validated_tx, table_t::validated_tx_table);

    close(ec, address, table_t::address_table);
    close(ec, neutrino, table_t::neutrino_table);
    ////close(ec, bootstrap, table_t::bootstrap_table);
    ////close(ec, buffer, table_t::buffer_table);

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
    open(ec, puts_head_, table_t::puts_head);
    open(ec, puts_body_, table_t::puts_body);
    open(ec, spend_head_, table_t::spend_head);
    open(ec, spend_body_, table_t::spend_body);
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

    open(ec, validated_bk_head_, table_t::validated_bk_head);
    open(ec, validated_bk_body_, table_t::validated_bk_body);
    open(ec, validated_tx_head_, table_t::validated_tx_head);
    open(ec, validated_tx_body_, table_t::validated_tx_body);

    open(ec, address_head_, table_t::address_head);
    open(ec, address_body_, table_t::address_body);
    open(ec, neutrino_head_, table_t::neutrino_head);
    open(ec, neutrino_body_, table_t::neutrino_body);
    ////open(ec, bootstrap_head_, table_t::bootstrap_head);
    ////open(ec, bootstrap_body_, table_t::bootstrap_body);
    ////open(ec, buffer_head_, table_t::buffer_head);
    ////open(ec, buffer_body_, table_t::buffer_body);

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
    load(ec, puts_head_, table_t::puts_head);
    load(ec, puts_body_, table_t::puts_body);
    load(ec, spend_head_, table_t::spend_head);
    load(ec, spend_body_, table_t::spend_body);
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

    load(ec, validated_bk_head_, table_t::validated_bk_head);
    load(ec, validated_bk_body_, table_t::validated_bk_body);
    load(ec, validated_tx_head_, table_t::validated_tx_head);
    load(ec, validated_tx_body_, table_t::validated_tx_body);

    load(ec, address_head_, table_t::address_head);
    load(ec, address_body_, table_t::address_body);
    load(ec, neutrino_head_, table_t::neutrino_head);
    load(ec, neutrino_body_, table_t::neutrino_body);
    ////load(ec, bootstrap_head_, table_t::bootstrap_head);
    ////load(ec, bootstrap_body_, table_t::bootstrap_body);
    ////load(ec, buffer_head_, table_t::buffer_head);
    ////load(ec, buffer_body_, table_t::buffer_body);

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
    unload(ec, puts_head_, table_t::puts_head);
    unload(ec, puts_body_, table_t::puts_body);
    unload(ec, spend_head_, table_t::spend_head);
    unload(ec, spend_body_, table_t::spend_body);
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

    unload(ec, validated_bk_head_, table_t::validated_bk_head);
    unload(ec, validated_bk_body_, table_t::validated_bk_body);
    unload(ec, validated_tx_head_, table_t::validated_tx_head);
    unload(ec, validated_tx_body_, table_t::validated_tx_body);

    unload(ec, address_head_, table_t::address_head);
    unload(ec, address_body_, table_t::address_body);
    unload(ec, neutrino_head_, table_t::neutrino_head);
    unload(ec, neutrino_body_, table_t::neutrino_body);
    ////unload(ec, bootstrap_head_, table_t::bootstrap_head);
    ////unload(ec, bootstrap_body_, table_t::bootstrap_body);
    ////unload(ec, buffer_head_, table_t::buffer_head);
    ////unload(ec, buffer_body_, table_t::buffer_body);

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
    close(ec, puts_head_, table_t::puts_head);
    close(ec, puts_body_, table_t::puts_body);
    close(ec, spend_head_, table_t::spend_head);
    close(ec, spend_body_, table_t::spend_body);
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

    close(ec, validated_bk_head_, table_t::validated_bk_head);
    close(ec, validated_bk_body_, table_t::validated_bk_body);
    close(ec, validated_tx_head_, table_t::validated_tx_head);
    close(ec, validated_tx_body_, table_t::validated_tx_body);

    close(ec, address_head_, table_t::address_head);
    close(ec, address_body_, table_t::address_body);
    close(ec, neutrino_head_, table_t::neutrino_head);
    close(ec, neutrino_body_, table_t::neutrino_body);
    ////close(ec, bootstrap_head_, table_t::bootstrap_head);
    ////close(ec, bootstrap_body_, table_t::bootstrap_body);
    ////close(ec, buffer_head_, table_t::buffer_head);
    ////close(ec, buffer_body_, table_t::buffer_body);

    return ec;
}

TEMPLATE
code CLASS::backup(const event_handler& handler) NOEXCEPT
{
    code ec{ error::success };
    const auto backup = [&handler](code& ec, auto& storage,
        table_t table) NOEXCEPT
    {
        if (!ec)
        {
            handler(event_t::backup_table, table);
            if (!storage.backup())
                ec = error::backup_table;
        }
    };

    backup(ec, header, table_t::header_table);
    backup(ec, input, table_t::input_table);
    backup(ec, output, table_t::output_table);
    backup(ec, point, table_t::point_table);
    backup(ec, puts, table_t::puts_table);
    backup(ec, spend, table_t::spend_table);
    backup(ec, tx, table_t::tx_table);
    backup(ec, txs, table_t::txs_table);

    backup(ec, candidate, table_t::candidate_table);
    backup(ec, confirmed, table_t::confirmed_table);
    backup(ec, strong_tx, table_t::strong_tx_table);

    backup(ec, validated_bk, table_t::validated_bk_table);
    backup(ec, validated_tx, table_t::validated_tx_table);

    backup(ec, address, table_t::address_table);
    backup(ec, neutrino, table_t::neutrino_table);
    ////backup(ec, bootstrap, table_t::bootstrap_table);
    ////backup(ec, buffer, table_t::buffer_table);

    if (ec) return ec;

    static const auto primary = configuration_.path / schema::dir::primary;
    static const auto secondary = configuration_.path / schema::dir::secondary;

    handler(event_t::archive_snapshot, table_t::store);

    if (file::is_directory(primary))
    {
        // Delete /secondary, rename /primary to /secondary.
        if ((ec = file::clear_directory_ex(secondary))) return ec;
        if ((ec = file::remove_ex(secondary))) return ec;
        if ((ec = file::rename_ex(primary, secondary))) return ec;
    }

    // Dump /heads memory maps to /primary.
    if ((ec = file::clear_directory_ex(primary))) return ec;
    ec = dump(primary, handler);

    // If failed clear primary and rename secondary to primary.
    if (ec && file::clear_directory(primary) && file::remove(primary))
        /* bool */ file::rename(secondary, primary);

    return ec;
}

// Dump memory maps of /heads to new files in /primary.
// Heads are copied from RAM, not flushed to disk and copied as files.
TEMPLATE
code CLASS::dump(const path& folder,
    const event_handler& handler) NOEXCEPT
{
    auto header_buffer = header_head_.get();
    auto input_buffer = input_head_.get();
    auto output_buffer = output_head_.get();
    auto point_buffer = point_head_.get();
    auto puts_buffer = puts_head_.get();
    auto spend_buffer = spend_head_.get();
    auto tx_buffer = tx_head_.get();
    auto txs_buffer = txs_head_.get();

    auto candidate_buffer = candidate_head_.get();
    auto confirmed_buffer = confirmed_head_.get();
    auto strong_tx_buffer = strong_tx_head_.get();

    auto validated_bk_buffer = validated_bk_head_.get();
    auto validated_tx_buffer = validated_tx_head_.get();

    auto address_buffer = address_head_.get();
    auto neutrino_buffer = neutrino_head_.get();
    ////auto bootstrap_buffer = bootstrap_head_.get();
    ////auto buffer_buffer = buffer_head_.get();

    if (!header_buffer) return error::unloaded_file;
    if (!input_buffer) return error::unloaded_file;
    if (!output_buffer) return error::unloaded_file;
    if (!point_buffer) return error::unloaded_file;
    if (!puts_buffer) return error::unloaded_file;
    if (!spend_buffer) return error::unloaded_file;
    if (!tx_buffer) return error::unloaded_file;
    if (!txs_buffer) return error::unloaded_file;

    if (!candidate_buffer) return error::unloaded_file;
    if (!confirmed_buffer) return error::unloaded_file;
    if (!strong_tx_buffer) return error::unloaded_file;

    if (!validated_bk_buffer) return error::unloaded_file;
    if (!validated_tx_buffer) return error::unloaded_file;

    if (!address_buffer) return error::unloaded_file;
    if (!neutrino_buffer) return error::unloaded_file;
    ////if (!bootstrap_buffer) return error::unloaded_file;
    ////if (!buffer_buffer) return error::unloaded_file;

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
    dump(ec, puts_buffer, schema::archive::puts, table_t::puts_head);
    dump(ec, spend_buffer, schema::archive::spend, table_t::spend_head);
    dump(ec, tx_buffer, schema::archive::tx, table_t::tx_head);
    dump(ec, txs_buffer, schema::archive::txs, table_t::txs_head);

    dump(ec, candidate_buffer, schema::indexes::candidate, table_t::candidate_head);
    dump(ec, confirmed_buffer, schema::indexes::confirmed, table_t::confirmed_head);
    dump(ec, strong_tx_buffer, schema::indexes::strong_tx, table_t::strong_tx_head);

    dump(ec, validated_bk_buffer, schema::caches::validated_bk, table_t::validated_bk_head);
    dump(ec, validated_tx_buffer, schema::caches::validated_tx, table_t::validated_tx_head);

    dump(ec, address_buffer, schema::optionals::address, table_t::address_head);
    dump(ec, neutrino_buffer, schema::optionals::neutrino, table_t::neutrino_head);
    ////dump(ec, bootstrap_buffer, schema::optionals::bootstrap, table_t::bootstrap_head);
    ////dump(ec, buffer_buffer, schema::optionals::buffer, table_t::buffer_head);

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

    handler(event_t::recover_snapshot, table_t::store);

    if (file::is_directory(primary))
    {
        // Clear invalid /heads, recover from /primary, clone to /primary.
        ec = file::clear_directory_ex(heads);
        if (!ec) ec = file::remove_ex(heads);
        if (!ec) ec = file::rename_ex(primary, heads);
        if (!ec) ec = file::copy_directory_ex(heads, primary);
        if (ec) /* bool */ file::remove_ex(primary);
    }
    else if (file::is_directory(secondary))
    {
        // Clear invalid /heads, recover from /secondary, clone to /primary.
        ec = file::clear_directory_ex(heads);
        if (!ec) ec = file::remove_ex(heads);
        if (!ec) ec = file::rename_ex(secondary, heads);
        if (!ec) ec = file::copy_directory_ex(heads, primary);
        if (ec) /* bool */ file::remove_ex(secondary);
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
        restore(ec, puts, table_t::puts_table);
        restore(ec, spend, table_t::spend_table);
        restore(ec, tx, table_t::tx_table);
        restore(ec, txs, table_t::txs_table);

        restore(ec, candidate, table_t::candidate_table);
        restore(ec, confirmed, table_t::confirmed_table);
        restore(ec, strong_tx, table_t::strong_tx_table);

        restore(ec, validated_bk, table_t::validated_bk_table);
        restore(ec, validated_tx, table_t::validated_tx_table);

        restore(ec, address, table_t::address_table);
        restore(ec, neutrino, table_t::neutrino_table);
        ////restore(ec, bootstrap, table_t::bootstrap_table);
        ////restore(ec, buffer, table_t::buffer_table);

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
code CLASS::get_fault() const NOEXCEPT
{
    code ec{ error::success };
    if ((ec = header_body_.get_fault())) return ec;
    if ((ec = input_body_.get_fault())) return ec;
    if ((ec = output_body_.get_fault())) return ec;
    if ((ec = point_body_.get_fault())) return ec;
    if ((ec = puts_body_.get_fault())) return ec;
    if ((ec = spend_body_.get_fault())) return ec;
    if ((ec = tx_body_.get_fault())) return ec;
    if ((ec = txs_body_.get_fault())) return ec;
    if ((ec = candidate_body_.get_fault())) return ec;
    if ((ec = confirmed_body_.get_fault())) return ec;
    if ((ec = strong_tx_body_.get_fault())) return ec;
    if ((ec = validated_bk_body_.get_fault())) return ec;
    if ((ec = validated_tx_body_.get_fault())) return ec;
    if ((ec = address_body_.get_fault())) return ec;
    if ((ec = neutrino_body_.get_fault())) return ec;
    ////if ((ec = bootstrap_body_.get_fault())) return ec;
    ////if ((ec = buffer_body_.get_fault())) return ec;
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
    space(puts_body_);
    space(spend_body_);
    space(tx_body_);
    space(txs_body_);
    space(candidate_body_);
    space(confirmed_body_);
    space(strong_tx_body_);
    space(validated_bk_body_);
    space(validated_tx_body_);
    space(address_body_);
    space(neutrino_body_);
    ////space(bootstrap_body_);
    ////space(buffer_body_);

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
    report(puts_body_, table_t::puts_body);
    report(spend_body_, table_t::spend_body);
    report(tx_body_, table_t::tx_body);
    report(txs_body_, table_t::txs_body);
    report(candidate_body_, table_t::candidate_body);
    report(confirmed_body_, table_t::confirmed_body);
    report(strong_tx_body_, table_t::strong_tx_body);
    report(validated_bk_body_, table_t::validated_bk_body);
    report(validated_tx_body_, table_t::validated_tx_body);
    report(address_body_, table_t::address_body);
    report(neutrino_body_, table_t::neutrino_body);
    ////report(bootstrap_body_, table_t::bootstrap_body);
    ////report(buffer_body_, table_t::buffer_body);
}

BC_POP_WARNING()

} // namespace database
} // namespace libbitcoin

#endif
