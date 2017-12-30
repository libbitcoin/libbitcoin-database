/**
 * Copyright (c) 2011-2017 libbitcoin developers (see AUTHORS)
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
#include <bitcoin/database/store.hpp>

#include <cstddef>
#include <memory>
#include <bitcoin/bitcoin.hpp>

namespace libbitcoin {
namespace database {

using namespace bc::chain;
using namespace bc::database;

// Database file names.
#define FLUSH_LOCK "flush_lock"
#define EXCLUSIVE_LOCK "exclusive_lock"
#define HEADER_INDEX "header_index"
#define BLOCK_INDEX "block_index"
#define BLOCK_TABLE "block_table"
#define TRANSACTION_INDEX "transaction_index"
#define TRANSACTION_TABLE "transaction_table"
#define HISTORY_TABLE "history_table"
#define HISTORY_ROWS "history_rows"
#define STEALTH_ROWS "stealth_rows"
#define SPEND_TABLE "spend_table"

// static
bool store::create(const path& file_path)
{
    bc::ofstream file(file_path.string());

    if (file.bad())
        return false;

    // Write one byte so file is nonzero size (for memory map validation).
    file.put('x');
    return true;
}

// Construct.
// ------------------------------------------------------------------------

store::store(const path& prefix, bool with_indexes, bool flush_each_write)
  : use_indexes(with_indexes),
    flush_each_write_(flush_each_write),
    flush_lock_(prefix / FLUSH_LOCK),
    exclusive_lock_(prefix / EXCLUSIVE_LOCK),

    // Content store.
    header_index(prefix / HEADER_INDEX),
    block_index(prefix / BLOCK_INDEX),
    block_table(prefix / BLOCK_TABLE),
    transaction_index(prefix / TRANSACTION_INDEX),
    transaction_table(prefix / TRANSACTION_TABLE),

    // Optional indexes.
    history_table(prefix / HISTORY_TABLE),
    history_rows(prefix / HISTORY_ROWS),
    stealth_rows(prefix / STEALTH_ROWS),
    spend_table(prefix / SPEND_TABLE)
{
}

// Open and close.
// ------------------------------------------------------------------------

// Create files.
bool store::create()
{
    const auto created =
        create(header_index) &&
        create(block_index) &&
        create(block_table) &&
        create(transaction_index) &&
        create(transaction_table);

    if (!use_indexes)
        return created;

    return
        created &&
        create(history_table) &&
        create(history_rows) &&
        create(stealth_rows) &&
        create(spend_table);
}

bool store::open()
{
    return exclusive_lock_.lock() && flush_lock_.try_lock() &&
        (flush_each_write() || flush_lock_.lock_shared());
}

bool store::close()
{
    return (flush_each_write() || flush_lock_.unlock_shared()) &&
        exclusive_lock_.unlock();
}

bool store::begin_write() const
{
    return flush_lock();
}

bool store::end_write() const
{
    return flush_unlock();
}

bool store::flush_lock() const
{
    return !flush_each_write() || flush_lock_.lock_shared();
}

bool store::flush_unlock() const
{
    return !flush_each_write() || (flush() && flush_lock_.unlock_shared());
}

bool store::flush_each_write() const
{
    return flush_each_write_;
}

} // namespace database
} // namespace libbitcoin
