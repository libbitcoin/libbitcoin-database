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

// TODO: expose file path from maps and use here.
code store::create() NOEXCEPT
{
    if (!transactor_mutex_.try_lock())
        return error::unknown;

    if (!process_lock_.try_lock())
    {
        transactor_mutex_.unlock();
        return error::unknown;
    }

    if (!flush_lock_.try_lock())
    {
        // Suppress process unlock error in favor of flush lock error.
        /* bool */ process_lock_.try_unlock();
        transactor_mutex_.unlock();
        return error::unknown;
    }

    auto error = error::success;

    // Short-circuiting (returns first code).
    if (!file::clear(configuration_.dir))
        error = error::unknown;
    else if (!file::create(configuration_.dir / "archive_header.idx"))
        error = error::unknown;
    else if (!file::create(configuration_.dir / "archive_header.dat"))
        error = error::unknown;
    else if (!file::create(configuration_.dir / "archive_point.idx"))
        error = error::unknown;
    else if (!file::create(configuration_.dir / "archive_point.dat"))
        error = error::unknown;
    else if (!file::create(configuration_.dir / "archive_input.idx"))
        error = error::unknown;
    else if (!file::create(configuration_.dir / "archive_input.dat"))
        error = error::unknown;
    else if (!file::create(configuration_.dir / "archive_output.dat"))
        error = error::unknown;
    else if (!file::create(configuration_.dir / "archive_puts.dat"))
        error = error::unknown;
    else if (!file::create(configuration_.dir / "archive_tx.idx"))
        error = error::unknown;
    else if (!file::create(configuration_.dir / "archive_tx.dat"))
        error = error::unknown;
    else if (!file::create(configuration_.dir / "archive_txs.idx"))
        error = error::unknown;
    else if (!file::create(configuration_.dir / "archive_txs.dat"))
        error = error::unknown;

    if (!flush_lock_.try_unlock())
        error = error::unknown;
    if (!process_lock_.try_unlock())
        error = error::unknown;

    // Suppress clear error in favor of first code.
    if (!error)
        /* bool */ file::clear(configuration_.dir);

    transactor_mutex_.unlock();
    return error;
}

code store::open() NOEXCEPT
{
    if (!transactor_mutex_.try_lock())
        return error::unknown;

    if (!process_lock_.try_lock())
    {
        transactor_mutex_.unlock();
        return error::unknown;
    }

    if (!flush_lock_.try_lock())
    {
        // Suppress process unlock error in favor of flush lock error.
        /* bool */ process_lock_.try_unlock();
        transactor_mutex_.unlock();
        return error::unknown;
    }

    // Short-circuiting (returns first code).
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

    // process and flush locks remain open.
    transactor_mutex_.unlock();

    // Suppress close error code in favor of first open code.
    if (!ec)
        /* code */ close();

    return ec;
}

code store::snapshot() NOEXCEPT
{
    while (!transactor_mutex_.try_lock_for(boost::chrono::seconds(1)))
    {
        // TODO: log deadlock_hint
    }

    code ec{ error::success };

    // Short-circuiting, flush bodys (returns first code).
    if (!ec) ec = header_body_.flush();
    if (!ec) ec = point_body_.flush();
    if (!ec) ec = input_body_.flush();
    if (!ec) ec = output_body_.flush();
    if (!ec) ec = puts_body_.flush();
    if (!ec) ec = tx_body_.flush();
    if (!ec) ec = txs_body_.flush();

    // Short-circuiting, dump headers (returns first code).
    if (!ec) ec = backup();

    transactor_mutex_.unlock();
    return ec;
}

code store::close() NOEXCEPT
{
    // unload and close are idempotent (success if unloaded/closed).
    if (!transactor_mutex_.try_lock())
        return error::unknown;

    // Short-circuiting (returns first code).
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

    if (!flush_lock_.try_unlock())
        ec = error::unknown;
    if (!process_lock_.try_unlock())
        ec = error::unknown;

    transactor_mutex_.unlock();
    return ec;
}

typename store::transactor store::get_transactor() NOEXCEPT
{
    return transactor{ transactor_mutex_ };
}

code store::backup() NOEXCEPT
{
    static const auto indexes = configuration_.dir / schema::dir::indexes;
    static const auto primary = configuration_.dir / schema::dir::primary;
    static const auto secondary = configuration_.dir / schema::dir::secondary;

    auto data1 = header_head_.get();
    auto data2 = point_head_.get();
    auto data3 = input_head_.get();
    auto data4 = tx_head_.get();
    auto data5 = txs_head_.get();

    // Clear secondary and rename primary to secondary (unless no primary).
    auto error = error::success;
    if (file::exists(primary))
    {
        if (!file::clear(secondary))
            return error::unknown;
        if (!file::rename(primary, secondary))
            return error::unknown;
    }

    // Export current (indexes) to primary.
    if (!file::dump(primary / "archive_header.idx", data1->begin(), data1->size()))
        error = error::unknown;
    else if (!file::dump(primary / "archive_point.idx", data2->begin(), data2->size()))
        error = error::unknown;
    else if (!file::dump(primary / "archive_input.idx", data3->begin(), data3->size()))
        error = error::unknown;
    else if (!file::dump(primary / "archive_tx.idx", data4->begin(), data4->size()))
        error = error::unknown;
    else if (!file::dump(primary / "archive_txs.idx", data5->begin(), data5->size()))
        error = error::unknown;

    // Suppress clear error code in favor of primary dump code.
    if (error)
        /* bool */ file::clear(primary);

    return error;
}

code store::restore() NOEXCEPT
{
    static const auto indexes = configuration_.dir / schema::dir::indexes;
    static const auto primary = configuration_.dir / schema::dir::primary;
    static const auto secondary = configuration_.dir / schema::dir::secondary;

    // Clear invalid indexes and recover primary (or secondary).
    if (file::exists(primary))
    {
        if (!file::clear(indexes))
            return error::unknown;
        if (!file::rename(primary, indexes))
            return error::unknown;
    }
    else if (file::exists(secondary))
    {
        if (!file::clear(indexes))
            return error::unknown;
        if (!file::rename(secondary, indexes))
            return error::unknown;
    }
    else
    {
        return error::unknown;
    }

    return error::success;
}

} // namespace database
} // namespace libbitcoin
