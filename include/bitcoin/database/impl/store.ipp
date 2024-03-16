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

#include <chrono>
#include <bitcoin/system.hpp>
#include <bitcoin/database/boost.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/file/file.hpp>

// TODO: evaluate performance benefits of concurrency.

namespace libbitcoin {
namespace database {

BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

TEMPLATE
CLASS::store(const settings& config) NOEXCEPT
  : configuration_(config),

    // Archive.

    header_head_(head(config.path / schema::dir::heads, schema::archive::header)),
    header_body_(body(config.path, schema::archive::header), config.header_size, config.header_rate),
    header(header_head_, header_body_, config.header_buckets),

    point_head_(head(config.path / schema::dir::heads, schema::archive::point)),
    point_body_(body(config.path, schema::archive::point), config.point_size, config.point_rate),
    point(point_head_, point_body_, config.point_buckets),

    input_head_(head(config.path / schema::dir::heads, schema::archive::input)),
    input_body_(body(config.path, schema::archive::input), config.input_size, config.input_rate),
    input(input_head_, input_body_),

    output_head_(head(config.path / schema::dir::heads, schema::archive::output)),
    output_body_(body(config.path, schema::archive::output), config.output_size, config.output_rate),
    output(output_head_, output_body_),

    puts_head_(head(config.path / schema::dir::heads, schema::archive::puts)),
    puts_body_(body(config.path, schema::archive::puts), config.puts_size, config.puts_rate),
    puts(puts_head_, puts_body_),

    tx_head_(head(config.path / schema::dir::heads, schema::archive::tx)),
    tx_body_(body(config.path, schema::archive::tx), config.tx_size, config.tx_rate),
    tx(tx_head_, tx_body_, config.tx_buckets),

    txs_head_(head(config.path / schema::dir::heads, schema::archive::txs)),
    txs_body_(body(config.path, schema::archive::txs), config.txs_size, config.txs_rate),
    txs(txs_head_, txs_body_, config.txs_buckets),

    // Indexes.

    address_head_(head(config.path / schema::dir::heads, schema::indexes::address)),
    address_body_(body(config.path, schema::indexes::address), config.address_size, config.address_rate),
    address(address_head_, address_body_, config.address_buckets),

    candidate_head_(head(config.path / schema::dir::heads, schema::indexes::candidate)),
    candidate_body_(body(config.path, schema::indexes::candidate), config.candidate_size, config.candidate_rate),
    candidate(candidate_head_, candidate_body_),

    confirmed_head_(head(config.path / schema::dir::heads, schema::indexes::confirmed)),
    confirmed_body_(body(config.path, schema::indexes::confirmed), config.confirmed_size, config.confirmed_rate),
    confirmed(confirmed_head_, confirmed_body_),

    spend_head_(head(config.path / schema::dir::heads, schema::indexes::spend)),
    spend_body_(body(config.path, schema::indexes::spend), config.spend_size, config.spend_rate),
    spend(spend_head_, spend_body_, config.spend_buckets),

    strong_tx_head_(head(config.path / schema::dir::heads, schema::indexes::strong_tx)),
    strong_tx_body_(body(config.path, schema::indexes::strong_tx), config.strong_tx_size, config.strong_tx_rate),
    strong_tx(strong_tx_head_, strong_tx_body_, config.strong_tx_buckets),

    // Caches.

    bootstrap_head_(head(config.path / schema::dir::heads, schema::caches::bootstrap)),
    bootstrap_body_(body(config.path, schema::caches::bootstrap), config.bootstrap_size, config.bootstrap_rate),
    bootstrap(bootstrap_head_, bootstrap_body_),

    buffer_head_(head(config.path / schema::dir::heads, schema::caches::buffer)),
    buffer_body_(body(config.path, schema::caches::buffer), config.buffer_size, config.buffer_rate),
    buffer(buffer_head_, buffer_body_, config.buffer_buckets),

    neutrino_head_(head(config.path / schema::dir::heads, schema::caches::neutrino)),
    neutrino_body_(body(config.path, schema::caches::neutrino), config.neutrino_size, config.neutrino_rate),
    neutrino(neutrino_head_, neutrino_body_, config.neutrino_buckets),

    validated_bk_head_(head(config.path / schema::dir::heads, schema::caches::validated_bk)),
    validated_bk_body_(body(config.path, schema::caches::validated_bk), config.validated_bk_size, config.validated_bk_rate),
    validated_bk(validated_bk_head_, validated_bk_body_, config.validated_bk_buckets),

    validated_tx_head_(head(config.path / schema::dir::heads, schema::caches::validated_tx)),
    validated_tx_body_(body(config.path, schema::caches::validated_tx), config.validated_tx_size, config.validated_tx_rate),
    validated_tx(validated_tx_head_, validated_tx_body_, config.validated_tx_buckets),

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

    code ec{ error::success };
    static const auto heads = configuration_.path / schema::dir::heads;

    const auto create = [&handler](const auto& storage, table_t table) NOEXCEPT
    {
        handler(event_t::create_file, table);
        return file::create_file(storage.file());
    };

    if (!file::clear_directory(heads)) ec = error::clear_directory;

    else if (!create(header_head_, table_t::header_head)) ec = error::create_file;
    else if (!create(header_body_, table_t::header_body)) ec = error::create_file;
    else if (!create(point_head_, table_t::point_head)) ec = error::create_file;
    else if (!create(point_body_, table_t::point_body)) ec = error::create_file;
    else if (!create(input_head_, table_t::input_head)) ec = error::create_file;
    else if (!create(input_body_, table_t::input_body)) ec = error::create_file;
    else if (!create(output_head_, table_t::output_head)) ec = error::create_file;
    else if (!create(output_body_, table_t::output_body)) ec = error::create_file;
    else if (!create(puts_head_, table_t::puts_head)) ec = error::create_file;
    else if (!create(puts_body_, table_t::puts_body)) ec = error::create_file;
    else if (!create(tx_head_, table_t::tx_head)) ec = error::create_file;
    else if (!create(tx_body_, table_t::tx_body)) ec = error::create_file;
    else if (!create(txs_head_, table_t::txs_head)) ec = error::create_file;
    else if (!create(txs_body_, table_t::txs_body)) ec = error::create_file;

    else if (!create(address_head_, table_t::address_head)) ec = error::create_file;
    else if (!create(address_body_, table_t::address_body)) ec = error::create_file;
    else if (!create(candidate_head_, table_t::candidate_head)) ec = error::create_file;
    else if (!create(candidate_body_, table_t::candidate_body)) ec = error::create_file;
    else if (!create(confirmed_head_, table_t::confirmed_head)) ec = error::create_file;
    else if (!create(confirmed_body_, table_t::confirmed_body)) ec = error::create_file;
    else if (!create(spend_head_, table_t::spend_head)) ec = error::create_file;
    else if (!create(spend_body_, table_t::spend_body)) ec = error::create_file;
    else if (!create(strong_tx_head_, table_t::strong_tx_head)) ec = error::create_file;
    else if (!create(strong_tx_body_, table_t::strong_tx_body)) ec = error::create_file;

    else if (!create(bootstrap_head_, table_t::bootstrap_head)) ec = error::create_file;
    else if (!create(bootstrap_body_, table_t::bootstrap_body)) ec = error::create_file;
    else if (!create(buffer_head_, table_t::buffer_head)) ec = error::create_file;
    else if (!create(buffer_body_, table_t::buffer_body)) ec = error::create_file;
    else if (!create(neutrino_head_, table_t::neutrino_head)) ec = error::create_file;
    else if (!create(neutrino_body_, table_t::neutrino_body)) ec = error::create_file;
    else if (!create(validated_bk_head_, table_t::validated_bk_head)) ec = error::create_file;
    else if (!create(validated_bk_body_, table_t::validated_bk_body)) ec = error::create_file;
    else if (!create(validated_tx_head_, table_t::validated_tx_head)) ec = error::create_file;
    else if (!create(validated_tx_body_, table_t::validated_tx_body)) ec = error::create_file;

    if (!ec) ec = open_load(handler);

    if (!ec)
    {
        const auto populate = [&handler](auto& storage, table_t table) NOEXCEPT
        {
            handler(event_t::create_table, table);
            return storage.create();
        };

        // Populate /heads files and truncate body sizes to zero.
        if      (!populate(header, table_t::header_table)) ec = error::create_table;
        else if (!populate(point, table_t::point_table)) ec = error::create_table;
        else if (!populate(input, table_t::input_table)) ec = error::create_table;
        else if (!populate(output, table_t::output_table)) ec = error::create_table;
        else if (!populate(puts, table_t::puts_table)) ec = error::create_table;
        else if (!populate(tx, table_t::tx_table)) ec = error::create_table;
        else if (!populate(txs, table_t::txs_table)) ec = error::create_table;

        else if (!populate(address, table_t::address_table)) ec = error::create_table;
        else if (!populate(candidate, table_t::candidate_table)) ec = error::create_table;
        else if (!populate(confirmed, table_t::confirmed_table)) ec = error::create_table;
        else if (!populate(spend, table_t::spend_table)) ec = error::create_table;
        else if (!populate(strong_tx, table_t::strong_tx_table)) ec = error::create_table;

        else if (!populate(bootstrap, table_t::bootstrap_table)) ec = error::create_table;
        else if (!populate(buffer, table_t::buffer_table)) ec = error::create_table;
        else if (!populate(neutrino, table_t::neutrino_table)) ec = error::create_table;
        else if (!populate(validated_bk, table_t::validated_bk_table)) ec = error::create_table;
        else if (!populate(validated_tx, table_t::validated_tx_table)) ec = error::create_table;
    }

    if (!ec) ec = unload_close(handler);

    // unlock errors override ec.
    if (!flush_lock_.try_unlock()) ec = error::flush_unlock;
    if (!process_lock_.try_unlock()) ec = error::process_unlock;
    if (ec) /* bool */ file::clear_directory(configuration_.path);
    transactor_mutex_.unlock();
    return ec;
}

TEMPLATE
code CLASS::open(const event_handler& handler) NOEXCEPT
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

    auto ec = open_load(handler);

    if (!ec)
    {
        const auto verify = [&handler](auto& storage, table_t table) NOEXCEPT
        {
            handler(event_t::verify_table, table);
            return storage.verify();
        };

        if      (!verify(header, table_t::header_table)) ec = error::verify_table;
        else if (!verify(point, table_t::point_table)) ec = error::verify_table;
        else if (!verify(input, table_t::input_table)) ec = error::verify_table;
        else if (!verify(output, table_t::output_table)) ec = error::verify_table;
        else if (!verify(puts, table_t::puts_table)) ec = error::verify_table;
        else if (!verify(tx, table_t::tx_table)) ec = error::verify_table;
        else if (!verify(txs, table_t::txs_table)) ec = error::verify_table;

        else if (!verify(address, table_t::address_table)) ec = error::verify_table;
        else if (!verify(candidate, table_t::candidate_table)) ec = error::verify_table;
        else if (!verify(confirmed, table_t::confirmed_table)) ec = error::verify_table;
        else if (!verify(spend, table_t::spend_table)) ec = error::verify_table;
        else if (!verify(strong_tx, table_t::strong_tx_table)) ec = error::verify_table;

        else if (!verify(bootstrap, table_t::bootstrap_table)) ec = error::verify_table;
        else if (!verify(buffer, table_t::buffer_table)) ec = error::verify_table;
        else if (!verify(neutrino, table_t::neutrino_table)) ec = error::verify_table;
        else if (!verify(validated_bk, table_t::validated_bk_table)) ec = error::verify_table;
        else if (!verify(validated_tx, table_t::validated_tx_table)) ec = error::verify_table;
    }

    // This prevents close from having to follow open fail.
    if (ec)
    {
        // No need to call close() as tables were just opened.
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

    const auto flush = [&handler](auto& ec, auto& storage, table_t table) NOEXCEPT
    {
        if (!ec)
        {
            handler(event_t::flush_table, table);
            ec = storage.flush();
        }
    };

    code ec{ error::success };

    // Assumes/requires tables open/loaded.
    flush(ec, header_body_, table_t::header_body);
    flush(ec, point_body_, table_t::point_body);
    flush(ec, input_body_, table_t::input_body);
    flush(ec, output_body_, table_t::output_body);
    flush(ec, puts_body_, table_t::puts_body);
    flush(ec, tx_body_, table_t::tx_body);
    flush(ec, txs_body_, table_t::txs_body);

    flush(ec, address_body_, table_t::address_body);
    flush(ec, candidate_body_, table_t::candidate_body);
    flush(ec, confirmed_body_, table_t::confirmed_body);
    flush(ec, spend_body_, table_t::spend_body);
    flush(ec, strong_tx_body_, table_t::strong_tx_body);

    flush(ec, bootstrap_body_, table_t::bootstrap_body);
    flush(ec, buffer_body_, table_t::buffer_body);
    flush(ec, neutrino_body_, table_t::neutrino_body);
    flush(ec, validated_bk_body_, table_t::validated_bk_body);
    flush(ec, validated_tx_body_, table_t::validated_tx_body);

    if (!ec) ec = backup(handler);
    transactor_mutex_.unlock();
    return ec;
}

TEMPLATE
code CLASS::close(const event_handler& handler) NOEXCEPT
{
    if (!transactor_mutex_.try_lock())
        return error::transactor_lock;

    code ec{ error::success };

    if (!ec)
    {
        const auto close = [&handler](auto& storage, table_t table) NOEXCEPT
        {
            handler(event_t::close_table, table);
            return storage.close();
        };

        if      (!close(header, table_t::header_table)) ec = error::close_table;
        else if (!close(point, table_t::point_table)) ec = error::close_table;
        else if (!close(input, table_t::input_table)) ec = error::close_table;
        else if (!close(output, table_t::output_table)) ec = error::close_table;
        else if (!close(puts, table_t::puts_table)) ec = error::close_table;
        else if (!close(tx, table_t::tx_table)) ec = error::close_table;
        else if (!close(txs, table_t::txs_table)) ec = error::close_table;

        else if (!close(address, table_t::address_table)) ec = error::close_table;
        else if (!close(candidate, table_t::candidate_table)) ec = error::close_table;
        else if (!close(confirmed, table_t::confirmed_table)) ec = error::close_table;
        else if (!close(spend, table_t::spend_table)) ec = error::close_table;
        else if (!close(strong_tx, table_t::strong_tx_table)) ec = error::close_table;

        else if (!close(bootstrap, table_t::bootstrap_table)) ec = error::close_table;
        else if (!close(buffer, table_t::buffer_table)) ec = error::close_table;
        else if (!close(neutrino, table_t::neutrino_table)) ec = error::close_table;
        else if (!close(validated_bk, table_t::validated_bk_table)) ec = error::close_table;
        else if (!close(validated_tx, table_t::validated_tx_table)) ec = error::close_table;
    }

    if (!ec) ec = unload_close(handler);

    // unlock errors override ec.
    if (!flush_lock_.try_unlock()) ec = error::flush_unlock;
    if (!process_lock_.try_unlock()) ec = error::process_unlock;
    transactor_mutex_.unlock();
    return ec;
}

TEMPLATE
const typename CLASS::transactor CLASS::get_transactor() NOEXCEPT
{
    return transactor{ transactor_mutex_ };
}

TEMPLATE
code CLASS::open_load(const event_handler& handler) NOEXCEPT
{
    code ec{ error::success };
    const auto open = [&handler](auto& ec, auto& storage, table_t table) NOEXCEPT
    {
        if (!ec)
        {
            handler(event_t::open_file, table);
            ec = storage.open();
        }
    };

    open(ec, header_head_, table_t::header_head);
    open(ec, header_body_, table_t::header_body);
    open(ec, point_head_, table_t::point_head);
    open(ec, point_body_, table_t::point_body);
    open(ec, input_head_, table_t::input_head);
    open(ec, input_body_, table_t::input_body);
    open(ec, output_head_, table_t::output_head);
    open(ec, output_body_, table_t::output_body);
    open(ec, puts_head_, table_t::puts_head);
    open(ec, puts_body_, table_t::puts_body);
    open(ec, tx_head_, table_t::tx_head);
    open(ec, tx_body_, table_t::tx_body);
    open(ec, txs_head_, table_t::txs_head);
    open(ec, txs_body_, table_t::txs_body);

    open(ec, address_head_, table_t::address_head);
    open(ec, address_body_, table_t::address_body);
    open(ec, candidate_head_, table_t::candidate_head);
    open(ec, candidate_body_, table_t::candidate_body);
    open(ec, confirmed_head_, table_t::confirmed_head);
    open(ec, confirmed_body_, table_t::confirmed_body);
    open(ec, spend_head_, table_t::spend_head);
    open(ec, spend_body_, table_t::spend_body);
    open(ec, strong_tx_head_, table_t::strong_tx_head);
    open(ec, strong_tx_body_, table_t::strong_tx_body);

    open(ec, bootstrap_head_, table_t::bootstrap_head);
    open(ec, bootstrap_body_, table_t::bootstrap_body);
    open(ec, buffer_head_, table_t::buffer_head);
    open(ec, buffer_body_, table_t::buffer_body);
    open(ec, neutrino_head_, table_t::neutrino_head);
    open(ec, neutrino_body_, table_t::neutrino_body);
    open(ec, validated_bk_head_, table_t::validated_bk_head);
    open(ec, validated_bk_body_, table_t::validated_bk_body);
    open(ec, validated_tx_head_, table_t::validated_tx_head);
    open(ec, validated_tx_body_, table_t::validated_tx_body);

    const auto load = [&handler](auto& ec, auto& storage, table_t table) NOEXCEPT
    {
        if (!ec)
        {
            handler(event_t::load_file, table);
            ec = storage.load();
        }
    };

    load(ec, header_head_, table_t::header_head);
    load(ec, header_body_, table_t::header_body);
    load(ec, point_head_, table_t::point_head);
    load(ec, point_body_, table_t::point_body);
    load(ec, input_head_, table_t::input_head);
    load(ec, input_body_, table_t::input_body);
    load(ec, output_head_, table_t::output_head);
    load(ec, output_body_, table_t::output_body);
    load(ec, puts_head_, table_t::puts_head);
    load(ec, puts_body_, table_t::puts_body);
    load(ec, tx_head_, table_t::tx_head);
    load(ec, tx_body_, table_t::tx_body);
    load(ec, txs_head_, table_t::txs_head);
    load(ec, txs_body_, table_t::txs_body);

    load(ec, address_head_, table_t::address_head);
    load(ec, address_body_, table_t::address_body);
    load(ec, candidate_head_, table_t::candidate_head);
    load(ec, candidate_body_, table_t::candidate_body);
    load(ec, confirmed_head_, table_t::confirmed_head);
    load(ec, confirmed_body_, table_t::confirmed_body);
    load(ec, spend_head_, table_t::spend_head);
    load(ec, spend_body_, table_t::spend_body);
    load(ec, strong_tx_head_, table_t::strong_tx_head);
    load(ec, strong_tx_body_, table_t::strong_tx_body);

    load(ec, bootstrap_head_, table_t::bootstrap_head);
    load(ec, bootstrap_body_, table_t::bootstrap_body);
    load(ec, buffer_head_, table_t::buffer_head);
    load(ec, buffer_body_, table_t::buffer_body);
    load(ec, neutrino_head_, table_t::neutrino_head);
    load(ec, neutrino_body_, table_t::neutrino_body);
    load(ec, validated_bk_head_, table_t::validated_bk_head);
    load(ec, validated_bk_body_, table_t::validated_bk_body);
    load(ec, validated_tx_head_, table_t::validated_tx_head);
    load(ec, validated_tx_body_, table_t::validated_tx_body);

    return ec;
}

TEMPLATE
code CLASS::unload_close(const event_handler& handler) NOEXCEPT
{
    code ec{ error::success };
    
    const auto unload = [&handler](auto& ec, auto& storage, table_t table) NOEXCEPT
    {
        if (!ec)
        {
            handler(event_t::unload_file, table);
            ec = storage.unload();
        }
    };

    unload(ec, header_head_, table_t::header_head);
    unload(ec, header_body_, table_t::header_body);
    unload(ec, point_head_, table_t::point_head);
    unload(ec, point_body_, table_t::point_body);
    unload(ec, input_head_, table_t::input_head);
    unload(ec, input_body_, table_t::input_body);
    unload(ec, output_head_, table_t::output_head);
    unload(ec, output_body_, table_t::output_body);
    unload(ec, puts_head_, table_t::puts_head);
    unload(ec, puts_body_, table_t::puts_body);
    unload(ec, tx_head_, table_t::tx_head);
    unload(ec, tx_body_, table_t::tx_body);
    unload(ec, txs_head_, table_t::txs_head);
    unload(ec, txs_body_, table_t::txs_body);

    unload(ec, address_head_, table_t::address_head);
    unload(ec, address_body_, table_t::address_body);
    unload(ec, candidate_head_, table_t::candidate_head);
    unload(ec, candidate_body_, table_t::candidate_body);
    unload(ec, confirmed_head_, table_t::confirmed_head);
    unload(ec, confirmed_body_, table_t::confirmed_body);
    unload(ec, spend_head_, table_t::spend_head);
    unload(ec, spend_body_, table_t::spend_body);
    unload(ec, strong_tx_head_, table_t::strong_tx_head);
    unload(ec, strong_tx_body_, table_t::strong_tx_body);

    unload(ec, bootstrap_head_, table_t::bootstrap_head);
    unload(ec, bootstrap_body_, table_t::bootstrap_body);
    unload(ec, buffer_head_, table_t::buffer_head);
    unload(ec, buffer_body_, table_t::buffer_body);
    unload(ec, neutrino_head_, table_t::neutrino_head);
    unload(ec, neutrino_body_, table_t::neutrino_body);
    unload(ec, validated_bk_head_, table_t::validated_bk_head);
    unload(ec, validated_bk_body_, table_t::validated_bk_body);
    unload(ec, validated_tx_head_, table_t::validated_tx_head);
    unload(ec, validated_tx_body_, table_t::validated_tx_body);

    const auto close = [&handler](auto& ec, auto& storage, table_t table) NOEXCEPT
    {
        if (!ec)
        {
            handler(event_t::close_file, table);
            ec = storage.close();
        }
    };

    close(ec, header_head_, table_t::header_head);
    close(ec, header_body_, table_t::header_body);
    close(ec, point_head_, table_t::point_head);
    close(ec, point_body_, table_t::point_body);
    close(ec, input_head_, table_t::input_head);
    close(ec, input_body_, table_t::input_body);
    close(ec, output_head_, table_t::output_head);
    close(ec, output_body_, table_t::output_body);
    close(ec, puts_head_, table_t::puts_head);
    close(ec, puts_body_, table_t::puts_body);
    close(ec, tx_head_, table_t::tx_head);
    close(ec, tx_body_, table_t::tx_body);
    close(ec, txs_head_, table_t::txs_head);
    close(ec, txs_body_, table_t::txs_body);

    close(ec, address_head_, table_t::address_head);
    close(ec, address_body_, table_t::address_body);
    close(ec, candidate_head_, table_t::candidate_head);
    close(ec, candidate_body_, table_t::candidate_body);
    close(ec, confirmed_head_, table_t::confirmed_head);
    close(ec, confirmed_body_, table_t::confirmed_body);
    close(ec, spend_head_, table_t::spend_head);
    close(ec, spend_body_, table_t::spend_body);
    close(ec, strong_tx_head_, table_t::strong_tx_head);
    close(ec, strong_tx_body_, table_t::strong_tx_body);

    close(ec, bootstrap_head_, table_t::bootstrap_head);
    close(ec, bootstrap_body_, table_t::bootstrap_body);
    close(ec, buffer_head_, table_t::buffer_head);
    close(ec, buffer_body_, table_t::buffer_body);
    close(ec, neutrino_head_, table_t::neutrino_head);
    close(ec, neutrino_body_, table_t::neutrino_body);
    close(ec, validated_bk_head_, table_t::validated_bk_head);
    close(ec, validated_bk_body_, table_t::validated_bk_body);
    close(ec, validated_tx_head_, table_t::validated_tx_head);
    close(ec, validated_tx_body_, table_t::validated_tx_body);

    return ec;
}

TEMPLATE
code CLASS::backup(const event_handler& handler) NOEXCEPT
{
    const auto backup = [&handler](auto& storage, table_t table) NOEXCEPT
    {
        handler(event_t::backup_table, table);
        return storage.backup();
    };

    if (!backup(header, table_t::header_table)) return error::backup_table;
    if (!backup(point, table_t::point_table)) return error::backup_table;
    if (!backup(input, table_t::input_table)) return error::backup_table;
    if (!backup(output, table_t::output_table)) return error::backup_table;
    if (!backup(puts, table_t::puts_table)) return error::backup_table;
    if (!backup(tx, table_t::tx_table)) return error::backup_table;
    if (!backup(txs, table_t::txs_table)) return error::backup_table;

    if (!backup(address, table_t::address_table)) return error::backup_table;
    if (!backup(candidate, table_t::candidate_table)) return error::backup_table;
    if (!backup(confirmed, table_t::confirmed_table)) return error::backup_table;
    if (!backup(spend, table_t::spend_table)) return error::backup_table;
    if (!backup(strong_tx, table_t::strong_tx_table)) return error::backup_table;

    if (!backup(bootstrap, table_t::bootstrap_table)) return error::backup_table;
    if (!backup(buffer, table_t::buffer_table)) return error::backup_table;
    if (!backup(neutrino, table_t::neutrino_table)) return error::backup_table;
    if (!backup(validated_bk, table_t::validated_bk_table)) return error::backup_table;
    if (!backup(validated_tx, table_t::validated_tx_table)) return error::backup_table;

    static const auto primary = configuration_.path / schema::dir::primary;
    static const auto secondary = configuration_.path / schema::dir::secondary;

    if (file::is_directory(primary))
    {
        // Delete /secondary, rename /primary to /secondary.
        if (!file::clear_directory(secondary)) return error::clear_directory;
        if (!file::remove(secondary)) return error::remove_directory;
        if (!file::rename(primary, secondary)) return error::rename_directory;
    }

    // Dump /heads memory maps to /primary.
    if (!file::clear_directory(primary)) return error::create_directory;
    const auto ec = dump(primary, handler);
    if (ec) /* bool */ file::clear_directory(primary);
    return ec;
}

// Dump memory maps of /heads to new files in /primary.
TEMPLATE
code CLASS::dump(const path& folder,
    const event_handler& handler) NOEXCEPT
{
    auto header_buffer = header_head_.get();
    auto point_buffer = point_head_.get();
    auto input_buffer = input_head_.get();
    auto output_buffer = output_head_.get();
    auto puts_buffer = puts_head_.get();
    auto tx_buffer = tx_head_.get();
    auto txs_buffer = txs_head_.get();

    auto address_buffer = address_head_.get();
    auto candidate_buffer = candidate_head_.get();
    auto confirmed_buffer = confirmed_head_.get();
    auto spend_buffer = spend_head_.get();
    auto strong_tx_buffer = strong_tx_head_.get();

    auto bootstrap_buffer = bootstrap_head_.get();
    auto buffer_buffer = buffer_head_.get();
    auto neutrino_buffer = neutrino_head_.get();
    auto validated_bk_buffer = validated_bk_head_.get();
    auto validated_tx_buffer = validated_tx_head_.get();

    if (!header_buffer) return error::unloaded_file;
    if (!point_buffer) return error::unloaded_file;
    if (!input_buffer) return error::unloaded_file;
    if (!output_buffer) return error::unloaded_file;
    if (!puts_buffer) return error::unloaded_file;
    if (!tx_buffer) return error::unloaded_file;
    if (!txs_buffer) return error::unloaded_file;

    if (!address_buffer) return error::unloaded_file;
    if (!candidate_buffer) return error::unloaded_file;
    if (!confirmed_buffer) return error::unloaded_file;
    if (!spend_buffer) return error::unloaded_file;
    if (!strong_tx_buffer) return error::unloaded_file;

    if (!bootstrap_buffer) return error::unloaded_file;
    if (!buffer_buffer) return error::unloaded_file;
    if (!neutrino_buffer) return error::unloaded_file;
    if (!validated_bk_buffer) return error::unloaded_file;
    if (!validated_tx_buffer) return error::unloaded_file;

    const auto dump = [&handler, &folder](const auto& storage, const auto& name,
        table_t table) NOEXCEPT
    {
        handler(event_t::dump_table, table);
        return file::create_file(head(folder, name), storage->begin(),
            storage->size());
    };

    if (!dump(header_buffer, schema::archive::header, table_t::header_head))
        return error::dump_file;
    if (!dump(point_buffer, schema::archive::point, table_t::point_head))
        return error::dump_file;
    if (!dump(input_buffer, schema::archive::input, table_t::input_head))
        return error::dump_file;
    if (!dump(output_buffer, schema::archive::output, table_t::output_head))
        return error::dump_file;
    if (!dump(puts_buffer, schema::archive::puts, table_t::puts_head))
        return error::dump_file;
    if (!dump(tx_buffer, schema::archive::tx, table_t::tx_head))
        return error::dump_file;
    if (!dump(txs_buffer, schema::archive::txs, table_t::txs_head))
        return error::dump_file;

    if (!dump(address_buffer, schema::indexes::address, table_t::address_head))
        return error::dump_file;
    if (!dump(candidate_buffer, schema::indexes::candidate, table_t::candidate_head))
        return error::dump_file;
    if (!dump(confirmed_buffer, schema::indexes::confirmed, table_t::confirmed_head))
        return error::dump_file;
    if (!dump(spend_buffer, schema::indexes::spend, table_t::spend_head))
        return error::dump_file;
    if (!dump(strong_tx_buffer, schema::indexes::strong_tx, table_t::strong_tx_head))
        return error::dump_file;

    if (!dump(bootstrap_buffer, schema::caches::bootstrap, table_t::bootstrap_head))
        return error::dump_file;
    if (!dump(buffer_buffer, schema::caches::buffer, table_t::buffer_head))
        return error::dump_file;
    if (!dump(neutrino_buffer, schema::caches::neutrino, table_t::neutrino_head))
        return error::dump_file;
    if (!dump(validated_bk_buffer, schema::caches::validated_bk, table_t::validated_bk_head))
        return error::dump_file;
    if (!dump(validated_tx_buffer, schema::caches::validated_tx, table_t::validated_tx_head))
        return error::dump_file;

    return error::success;
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

    if (!flush_lock_.try_lock())
    {
        /* bool */ process_lock_.try_unlock();
        transactor_mutex_.unlock();
        return error::flush_lock;
    }

    code ec{ error::success };
    static const auto heads = configuration_.path / schema::dir::heads;
    static const auto primary = configuration_.path / schema::dir::primary;
    static const auto secondary = configuration_.path / schema::dir::secondary;

    if (file::is_directory(primary))
    {
        // Clear invalid /heads and recover from /primary.
        if (!file::clear_directory(heads)) ec = error::clear_directory;
        else if (!file::remove(heads)) ec = error::remove_directory;
        else if (!file::rename(primary, heads)) ec = error::rename_directory;
    }
    else if (file::is_directory(secondary))
    {
        // Clear invalid /heads and recover from /secondary.
        if (!file::clear_directory(heads)) ec = error::clear_directory;
        else if (!file::remove(heads)) ec = error::remove_directory;
        else if (!file::rename(secondary, heads)) ec = error::rename_directory;
    }
    else
    {
        ec = error::missing_backup;
    }

    const auto restore = [&handler](auto& storage, table_t table) NOEXCEPT
    {
        handler(event_t::restore_table, table);
        return storage.restore();
    };

    if (!ec)
    {
        ec = open_load(handler);

        if (!restore(header, table_t::header_table)) ec = error::restore_table;
        if (!restore(point, table_t::point_table)) ec = error::restore_table;
        if (!restore(input, table_t::input_table)) ec = error::restore_table;
        if (!restore(output, table_t::output_table)) ec = error::restore_table;
        if (!restore(puts, table_t::puts_table)) ec = error::restore_table;
        if (!restore(tx, table_t::tx_table)) ec = error::restore_table;
        if (!restore(txs, table_t::txs_table)) ec = error::restore_table;

        if (!restore(address, table_t::address_table)) ec = error::restore_table;
        if (!restore(candidate, table_t::candidate_table)) ec = error::restore_table;
        if (!restore(confirmed, table_t::confirmed_table)) ec = error::restore_table;
        if (!restore(spend, table_t::spend_table)) ec = error::restore_table;
        if (!restore(strong_tx, table_t::strong_tx_table)) ec = error::restore_table;

        if (!restore(bootstrap, table_t::bootstrap_table)) ec = error::restore_table;
        if (!restore(buffer, table_t::buffer_table)) ec = error::restore_table;
        if (!restore(neutrino, table_t::neutrino_table)) ec = error::restore_table;
        if (!restore(validated_bk, table_t::validated_bk_table)) ec = error::restore_table;
        if (!restore(validated_tx, table_t::validated_tx_table)) ec = error::restore_table;

        if (!ec) ec = unload_close(handler);
    }

    // unlock errors override ec.
    if (!flush_lock_.try_unlock()) ec = error::flush_unlock;
    if (!process_lock_.try_unlock()) ec = error::process_unlock;
    transactor_mutex_.unlock();
    return ec;
}

BC_POP_WARNING()

} // namespace database
} // namespace libbitcoin

#endif
