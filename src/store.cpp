/**
 * Copyright (c) 2011-2015 libbitcoin developers (see AUTHORS)
 *
 * This file is part of libbitcoin.
 *
 * libbitcoin is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License with
 * additional permissions to the one published by the Free Software
 * Foundation, either version 3 of the License, or (at your option)
 * any later version. For more information see LICENSE.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
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
#define BLOCK_TABLE "block_table"
#define BLOCK_INDEX "block_index"
#define TRANSACTION_TABLE "transaction_table"
#define SPEND_TABLE "spend_table"
#define HISTORY_TABLE "history_table"
#define HISTORY_ROWS "history_rows"
#define STEALTH_ROWS "stealth_rows"

// The threashold max_uint32 is used to align with fixed-width config settings,
// and size_t is used to align with the database height domain.
const size_t store::without_indexes = max_uint32;

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

store::store(const path& prefix, bool with_indexes)
  : use_indexes(with_indexes),
    flush_lock_(prefix / FLUSH_LOCK),
    exclusive_lock_(prefix / EXCLUSIVE_LOCK),

    // Content store.
    block_table(prefix / BLOCK_TABLE),
    block_index(prefix / BLOCK_INDEX),
    transaction_table(prefix / TRANSACTION_TABLE),

    // Optional indexes.
    spend_table(prefix / SPEND_TABLE),
    history_table(prefix / HISTORY_TABLE),
    history_rows(prefix / HISTORY_ROWS),
    stealth_rows(prefix / STEALTH_ROWS)
{
}

// Open and close.
// ------------------------------------------------------------------------

// Create files.
bool store::create() const
{
    const auto created =
        create(block_table) &&
        create(block_index) &&
        create(transaction_table);

    if (!use_indexes)
        return created;

    return
        created &&
        create(spend_table) &&
        create(history_table) &&
        create(history_rows) &&
        create(stealth_rows);
}

bool store::open()
{
    return flush_lock_.try_lock() && exclusive_lock_.lock();
}

bool store::close()
{
    return exclusive_lock_.unlock();
}

store::handle store::begin_read() const
{
    return sequential_lock_.begin_read();
}

bool store::is_read_valid(handle value) const
{
    return sequential_lock_.is_read_valid(value);
}

bool store::is_write_locked(handle value) const
{
    return sequential_lock_.is_write_locked(value);
}

bool store::begin_write(bool lock)
{
    return (!lock || flush_lock()) && sequential_lock_.begin_write();
}

bool store::end_write(bool unlock)
{
    return sequential_lock_.end_write() && (!unlock || flush_unlock());
}

bool store::flush_lock()
{
    return flush_lock_.lock_shared();
}

bool store::flush_unlock()
{
    return flush() && flush_lock_.unlock_shared();
}

} // namespace data_base
} // namespace libbitcoin
