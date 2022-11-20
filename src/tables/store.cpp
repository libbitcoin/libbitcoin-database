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

#include <filesystem>
#include <bitcoin/system.hpp>
#include <bitcoin/database/memory/memory.hpp>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {
    
// TODO: evaluate performance benefits of concurrency.
// TODO: error codes and config settings.
// TODO: name, size, bucket, rate should be independently configurations.
store::store(const settings& config) NOEXCEPT
  : configuration_(config),

    header_head_(config.dir / "index/archive_header.idx"),
    header_body_(config.dir / "archive_header.dat", config.size, config.rate),
    header(header_head_, header_body_, config.buckets),

    point_head_(config.dir / "index/archive_point.idx"),
    point_body_(config.dir / "archive_point.dat", config.size, config.rate),
    point(point_head_, point_body_, config.buckets),

    input_head_(config.dir / "index/archive_input.idx"),
    input_body_(config.dir / "archive_input.dat", config.size, config.rate),
    input(input_head_, input_body_, config.buckets),

    output_body_(config.dir / "archive_output.dat", config.size, config.rate),
    output(output_body_),

    puts_body_(config.dir / "archive_puts.dat", config.size, config.rate),
    puts(puts_body_),

    tx_head_(config.dir / "index/archive_tx.idx"),
    tx_body_(config.dir / "archive_tx.dat", config.size, config.rate),
    tx(tx_head_, tx_body_, config.buckets),

    txs_head_(config.dir / "index/archive_txs.idx"),
    txs_body_(config.dir / "archive_txs.dat", config.size, config.rate),
    txs(txs_head_, txs_body_, config.buckets),

    flush_lock_(config.dir / "flush.lck"),
    process_lock_(config.dir / "process.lck")
{
}

// TODO: expose file path from maps and use here.
code store::create() NOEXCEPT
{
    if (!transactor_mutex_.try_lock())
        return system::error::unknown;

    if (!process_lock_.try_lock())
    {
        transactor_mutex_.unlock();
        return system::error::unknown;
    }

    if (!flush_lock_.try_lock())
    {
        // Suppress process unlock error in favor of flush lock error.
        /* bool */ process_lock_.try_unlock();
        transactor_mutex_.unlock();
        return system::error::unknown;
    }

    auto error = system::error::success;

    // Short-circuiting (returns first code).
    if (!file::clear(configuration_.dir))
        error = system::error::unknown;
    else if (!file::create(configuration_.dir / "archive_header.idx"))
        error = system::error::unknown;
    else if (!file::create(configuration_.dir / "archive_header.dat"))
        error = system::error::unknown;
    else if (!file::create(configuration_.dir / "archive_point.idx"))
        error = system::error::unknown;
    else if (!file::create(configuration_.dir / "archive_point.dat"))
        error = system::error::unknown;
    else if (!file::create(configuration_.dir / "archive_input.idx"))
        error = system::error::unknown;
    else if (!file::create(configuration_.dir / "archive_input.dat"))
        error = system::error::unknown;
    else if (!file::create(configuration_.dir / "archive_output.dat"))
        error = system::error::unknown;
    else if (!file::create(configuration_.dir / "archive_puts.dat"))
        error = system::error::unknown;
    else if (!file::create(configuration_.dir / "archive_tx.idx"))
        error = system::error::unknown;
    else if (!file::create(configuration_.dir / "archive_tx.dat"))
        error = system::error::unknown;
    else if (!file::create(configuration_.dir / "archive_txs.idx"))
        error = system::error::unknown;
    else if (!file::create(configuration_.dir / "archive_txs.dat"))
        error = system::error::unknown;

    if (!flush_lock_.try_unlock())
        error = system::error::unknown;
    if (!process_lock_.try_unlock())
        error = system::error::unknown;

    // Suppress clear error in favor of first code.
    if (!error)
        /* bool */ file::clear(configuration_.dir);

    transactor_mutex_.unlock();
    return error;
}

code store::open() NOEXCEPT
{
    if (!transactor_mutex_.try_lock())
        return system::error::unknown;

    if (!process_lock_.try_lock())
    {
        transactor_mutex_.unlock();
        return system::error::unknown;
    }

    if (!flush_lock_.try_lock())
    {
        // Suppress process unlock error in favor of flush lock error.
        /* bool */ process_lock_.try_unlock();
        transactor_mutex_.unlock();
        return system::error::unknown;
    }

    // open and load are not idempotent (fail if open/loaded).
    auto error = system::error::success;

    // Short-circuiting (returns first code).

    if (!header_head_.open())
        error = system::error::unknown;
    else if (!header_body_.open())
        error = system::error::unknown;
    else if (!point_head_.open())
        error = system::error::unknown;
    else if (!point_body_.open())
        error = system::error::unknown;
    else if (!input_head_.open())
        error = system::error::unknown;
    else if (!input_body_.open())
        error = system::error::unknown;
    else if (!output_body_.open())
        error = system::error::unknown;
    else if (!puts_body_.open())
        error = system::error::unknown;
    else if (!tx_head_.open())
        error = system::error::unknown;
    else if (!tx_body_.open())
        error = system::error::unknown;
    else if (!txs_head_.open())
        error = system::error::unknown;
    else if (!txs_body_.open())
        error = system::error::unknown;

    else if (!header_head_.load())
        error = system::error::unknown;
    else if (!header_body_.load())
        error = system::error::unknown;
    else if (!point_head_.load())
        error = system::error::unknown;
    else if (!point_body_.load())
        error = system::error::unknown;
    else if (!input_head_.load())
        error = system::error::unknown;
    else if (!input_body_.load())
        error = system::error::unknown;
    else if (!output_body_.load())
        error = system::error::unknown;
    else if (!puts_body_.load())
        error = system::error::unknown;
    else if (!tx_head_.load())
        error = system::error::unknown;
    else if (!tx_body_.load())
        error = system::error::unknown;
    else if (!txs_head_.load())
        error = system::error::unknown;
    else if (!txs_body_.load())
        error = system::error::unknown;

    transactor_mutex_.unlock();

    // Suppress close error code in favor of first open code.
    if (!error)
        /* code */ close();

    return error;
}

code store::snapshot() NOEXCEPT
{
    while (!transactor_mutex_.try_lock_for(boost::chrono::seconds(1)))
    {
        // TODO: log deadlock_hint
    }

    // Flush all bodies (headers unnecessary).
    auto error = system::error::success;
    if (!header_body_.flush())
        error = system::error::unknown;
    if (!point_body_.flush())
        error = system::error::unknown;
    if (!input_body_.flush())
        error = system::error::unknown;
    if (!output_body_.flush())
        error = system::error::unknown;
    if (!puts_body_.flush())
        error = system::error::unknown;
    if (!tx_body_.flush())
        error = system::error::unknown;
    if (!txs_body_.flush())
        error = system::error::unknown;

    if (!backup())
        error = system::error::unknown;

    transactor_mutex_.unlock();
    return error;
}

code store::close() NOEXCEPT
{
    auto error = system::error::success;

    // unload and close are idempotent (success if unloaded/closed).
    if (transactor_mutex_.try_lock())
    {
        // Not short-circuiting (returns last code).

        if (!header_head_.unload())
            error = system::error::unknown;
        if (!header_body_.unload())
            error = system::error::unknown;
        if (!point_head_.unload())
            error = system::error::unknown;
        if (!point_body_.unload())
            error = system::error::unknown;
        if (!input_head_.unload())
            error = system::error::unknown;
        if (!input_body_.unload())
            error = system::error::unknown;
        if (!output_body_.unload())
            error = system::error::unknown;
        if (!puts_body_.unload())
            error = system::error::unknown;
        if (!tx_head_.unload())
            error = system::error::unknown;
        if (!tx_body_.unload())
            error = system::error::unknown;
        if (!txs_head_.unload())
            error = system::error::unknown;
        if (!txs_body_.unload())
            error = system::error::unknown;

        if (!header_head_.close())
            error = system::error::unknown;
        if (!header_body_.close())
            error = system::error::unknown;
        if (!point_head_.close())
            error = system::error::unknown;
        if (!point_body_.close())
            error = system::error::unknown;
        if (!input_head_.close())
            error = system::error::unknown;
        if (!input_body_.close())
            error = system::error::unknown;
        if (!output_body_.close())
            error = system::error::unknown;
        if (!puts_body_.close())
            error = system::error::unknown;
        if (!tx_head_.close())
            error = system::error::unknown;
        if (!tx_body_.close())
            error = system::error::unknown;
        if (!txs_head_.close())
            error = system::error::unknown;
        if (!txs_body_.close())
            error = system::error::unknown;

        if (!process_lock_.try_unlock())
            error = system::error::unknown;
        if (!flush_lock_.try_unlock())
            error = system::error::unknown;

        transactor_mutex_.unlock();
    }

    return error;
}

typename store::transactor store::get_transactor() NOEXCEPT
{
    return transactor{ transactor_mutex_ };
}

code store::backup() NOEXCEPT
{
    const auto index = configuration_.dir / "index";
    const auto second = configuration_.dir / "second";
    const auto first = configuration_.dir / "first";

    auto data1 = header_head_.get();
    auto data2 = point_head_.get();
    auto data3 = input_head_.get();
    auto data4 = tx_head_.get();
    auto data5 = txs_head_.get();

    // Clear second and rename first to second (unless no first).
    auto error = system::error::success;
    if (file::exists(first))
    {
        if (!file::clear(second))
            return system::error::unknown;
        if (!file::rename(first, second))
            return system::error::unknown;
    }

    // Export current (index) to first.
    else if (!file::copy(first / "archive_header.idx", data1->begin(), data1->size()))
        error = system::error::unknown;
    else if (!file::copy(first / "archive_point.idx", data2->begin(), data2->size()))
        error = system::error::unknown;
    else if (!file::copy(first / "archive_input.idx", data3->begin(), data3->size()))
        error = system::error::unknown;
    else if (!file::copy(first / "archive_tx.idx", data4->begin(), data4->size()))
        error = system::error::unknown;
    else if (!file::copy(first / "archive_txs.idx", data5->begin(), data5->size()))
        error = system::error::unknown;

    // Suppress clear error code in favor of first copy code.
    if (error)
        /* bool */ file::clear(first);

    return error;
}

code store::restore() NOEXCEPT
{
    const auto index = configuration_.dir / "index";
    const auto first = configuration_.dir / "first";
    const auto second = configuration_.dir / "second";

    // Clear invalid index and recover first (or second).
    if (file::exists(first))
    {
        if (!file::clear(index))
            return system::error::unknown;
        if (!file::rename(first, index))
            return system::error::unknown;
    }
    else if (file::exists(second))
    {
        if (!file::clear(index))
            return system::error::unknown;
        if (!file::rename(second, index))
            return system::error::unknown;
    }

    return system::error::success;
}

} // namespace database
} // namespace libbitcoin
