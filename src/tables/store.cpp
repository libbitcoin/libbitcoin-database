/**
 * Copyright (c) 2011-2022 libbitcoin developers (see AUTHORS)
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
#include <bitcoin/database/tables/store.hpp>

#include <bitcoin/system.hpp>
#include <bitcoin/database/memory/memory.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/tables/schema.hpp>

// TODO: evaluate performance benefits of concurrency.

namespace libbitcoin {
namespace database {

store::store(const settings& config) NOEXCEPT
  : configuration_(config),

    header_head_(index(config.dir, schema::archive::header)),
    header_body_(body(config.dir, schema::archive::header),
        config.header_size, config.header_rate),
    header(header_head_, header_body_, config.header_buckets),

    point_head_(index(config.dir, schema::archive::point)),
    point_body_(body(config.dir, schema::archive::point),
        config.point_size, config.point_rate),
    point(point_head_, point_body_, config.point_buckets),

    input_head_(index(config.dir, schema::archive::input)),
    input_body_(body(config.dir, schema::archive::input),
        config.input_size, config.input_rate),
    input(input_head_, input_body_, config.input_buckets),

    output_body_(body(config.dir, schema::archive::output),
        config.output_size, config.output_rate),
    output(output_body_),

    puts_body_(body(config.dir, schema::archive::puts),
        config.puts_size, config.puts_rate),
    puts(puts_body_),

    tx_head_(index(config.dir, schema::archive::tx)),
    tx_body_(body(config.dir, schema::archive::tx),
        config.tx_size, config.tx_rate),
    tx(tx_head_, tx_body_, config.tx_buckets),

    txs_head_(index(config.dir, schema::archive::txs)),
    txs_body_(body(config.dir, schema::archive::txs),
        config.txs_size, config.txs_rate),
    txs(txs_head_, txs_body_, config.txs_buckets),

    flush_lock_(lock(config.dir, schema::lock::flush)),
    process_lock_(lock(config.dir, schema::lock::process))
{
}

code store::create() NOEXCEPT
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
    static const auto indexes = configuration_.dir / schema::dir::indexes;

    if (!file::clear(indexes)) ec = error::clear_directory;
    else if (!file::create(header_head_.file())) ec = error::create_file;
    else if (!file::create(header_body_.file())) ec = error::create_file;
    else if (!file::create(point_head_.file())) ec = error::create_file;
    else if (!file::create(point_body_.file())) ec = error::create_file;
    else if (!file::create(input_head_.file())) ec = error::create_file;
    else if (!file::create(input_body_.file())) ec = error::create_file;
    else if (!file::create(output_body_.file())) ec = error::create_file;
    else if (!file::create(puts_body_.file())) ec = error::create_file;
    else if (!file::create(tx_head_.file())) ec = error::create_file;
    else if (!file::create(tx_body_.file())) ec = error::create_file;
    else if (!file::create(txs_head_.file())) ec = error::create_file;
    else if (!file::create(txs_body_.file())) ec = error::create_file;

    if (!flush_lock_.try_unlock()) ec = error::flush_unlock;
    if (!process_lock_.try_unlock()) ec = error::process_unlock;

    if (ec) /* bool */ file::clear(configuration_.dir);
    transactor_mutex_.unlock();
    return ec;
}

code store::open() NOEXCEPT
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

    if (!ec) ec = header_head_.open();
    if (!ec) ec = header_body_.open();
    if (!ec) ec = point_head_.open();
    if (!ec) ec = point_body_.open();
    if (!ec) ec = input_head_.open();
    if (!ec) ec = input_body_.open();
    if (!ec) ec = output_body_.open();
    if (!ec) ec = puts_body_.open();
    if (!ec) ec = tx_head_.open();
    if (!ec) ec = tx_body_.open();
    if (!ec) ec = txs_head_.open();
    if (!ec) ec = txs_body_.open();

    if (!ec) ec = header_head_.load();
    if (!ec) ec = header_body_.load();
    if (!ec) ec = point_head_.load();
    if (!ec) ec = point_body_.load();
    if (!ec) ec = input_head_.load();
    if (!ec) ec = input_body_.load();
    if (!ec) ec = output_body_.load();
    if (!ec) ec = puts_body_.load();
    if (!ec) ec = tx_head_.load();
    if (!ec) ec = tx_body_.load();
    if (!ec) ec = txs_head_.load();
    if (!ec) ec = txs_body_.load();

    // process and flush locks remain open until close().
    transactor_mutex_.unlock();
    if (ec) /* code */ close();
    return ec;
}

code store::snapshot() NOEXCEPT
{
    while (!transactor_mutex_.try_lock_for(boost::chrono::seconds(1)))
    {
        // TODO: log deadlock_hint
    }

    code ec{ error::success };

    if (!ec) ec = header_body_.flush();
    if (!ec) ec = point_body_.flush();
    if (!ec) ec = input_body_.flush();
    if (!ec) ec = output_body_.flush();
    if (!ec) ec = puts_body_.flush();
    if (!ec) ec = tx_body_.flush();
    if (!ec) ec = txs_body_.flush();

    if (!ec) ec = backup();
    transactor_mutex_.unlock();
    return ec;
}

code store::close() NOEXCEPT
{
    if (!transactor_mutex_.try_lock()) return error::transactor_lock;

    code ec{ error::success };

    if (!ec) ec = header_head_.unload();
    if (!ec) ec = header_body_.unload();
    if (!ec) ec = point_head_.unload();
    if (!ec) ec = point_body_.unload();
    if (!ec) ec = input_head_.unload();
    if (!ec) ec = input_body_.unload();
    if (!ec) ec = output_body_.unload();
    if (!ec) ec = puts_body_.unload();
    if (!ec) ec = tx_head_.unload();
    if (!ec) ec = tx_body_.unload();
    if (!ec) ec = txs_head_.unload();
    if (!ec) ec = txs_body_.unload();

    if (!ec) ec = header_head_.close();
    if (!ec) ec = header_body_.close();
    if (!ec) ec = point_head_.close();
    if (!ec) ec = point_body_.close();
    if (!ec) ec = input_head_.close();
    if (!ec) ec = input_body_.close();
    if (!ec) ec = output_body_.close();
    if (!ec) ec = puts_body_.close();
    if (!ec) ec = tx_head_.close();
    if (!ec) ec = tx_body_.close();
    if (!ec) ec = txs_head_.close();
    if (!ec) ec = txs_body_.close();

    if (!flush_lock_.try_unlock()) ec = error::flush_unlock;
    if (!process_lock_.try_unlock()) ec = error::process_unlock;

    transactor_mutex_.unlock();
    return ec;
}

const typename store::transactor store::get_transactor() NOEXCEPT
{
    return transactor{ transactor_mutex_ };
}

code store::backup() NOEXCEPT
{
    static const auto primary = configuration_.dir / schema::dir::primary;
    static const auto secondary = configuration_.dir / schema::dir::secondary;

    // Clear /secondary and rename /primary to /secondary (unless missing).
    if (file::exists(primary))
    {
        if (!file::clear(secondary)) return error::clear_directory;
        if (!file::rename(primary, secondary)) return error::rename_directory;
    }

    const auto ec = dump();
    if (ec) /* bool */ file::clear(primary);
    return ec;
}

// Dump memory maps of /indexes to new files in /primary.
code store::dump() NOEXCEPT
{
    auto header_buffer = header_head_.get();
    auto point_buffer = point_head_.get();
    auto input_buffer = input_head_.get();
    auto tx_buffer = tx_head_.get();
    auto txs_buffer = txs_head_.get();

    if (!header_buffer ||
        !point_buffer ||
        !input_buffer ||
        !tx_buffer ||
        !txs_buffer)
        return error::unloaded_file;

    if (!file::dump(back(configuration_.dir, schema::archive::header),
        header_buffer->begin(), header_buffer->size()))
       return error::dump_file;

    if (!file::dump(back(configuration_.dir, schema::archive::point),
        point_buffer->begin(), point_buffer->size()))
        return error::dump_file;

    if (!file::dump(back(configuration_.dir, schema::archive::input),
        input_buffer->begin(), input_buffer->size()))
        return error::dump_file;

    if (!file::dump(back(configuration_.dir, schema::archive::tx),
        tx_buffer->begin(), tx_buffer->size()))
        return error::dump_file;

    if (!file::dump(back(configuration_.dir, schema::archive::txs),
        txs_buffer->begin(), txs_buffer->size()))
        return error::dump_file;

    return error::success;
}

code store::restore() NOEXCEPT
{
    static const auto indexes = configuration_.dir / schema::dir::indexes;
    static const auto primary = configuration_.dir / schema::dir::primary;
    static const auto secondary = configuration_.dir / schema::dir::secondary;

    // Clear invalid /indexes and recover from /primary (or /secondary).
    if (file::exists(primary))
    {
        if (!file::clear(indexes)) return error::clear_directory;
        if (!file::rename(primary, indexes)) return error::rename_directory;
    }
    else if (file::exists(secondary))
    {
        if (!file::clear(indexes)) return error::clear_directory;
        if (!file::rename(secondary, indexes)) return error::rename_directory;
    }
    else
    {
        return error::missing_backup;
    }

    return error::success;
}

} // namespace database
} // namespace libbitcoin
