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

// The sentinel max_uint32 is used to align with fixed-width config settings,
// and size_t is used to align with the database height domain.
const size_t store::without_indexes = max_uint32;

// static
bool store::create(const path& file_path)
{
    bc::ofstream file(file_path.string());

    if (file.bad())
        return false;

    // Write one byte so file is nonzero size.
    file.put('x');
    return true;
}

// Construct.
// ------------------------------------------------------------------------

store::store(const path& prefix, bool with_indexes)
  : use_indexes(with_indexes),
    crash_lock_(prefix / "start_lock"),
    exclusive_lock_(prefix / "exclusive_lock"),

    // Content store.
    block_table(prefix / "block_table"),
    block_index(prefix / "block_index"),
    transaction_table(prefix / "transaction_table"),

    // Optional indexes.
    spend_table(prefix / "spend_table"),
    history_table(prefix / "history_table"),
    history_rows(prefix / "history_rows"),
    stealth_rows(prefix / "stealth_rows")
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
    return crash_lock_.try_lock() && exclusive_lock_.lock();
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
    return (!lock || crash_lock()) && sequential_lock_.begin_write();
}

bool store::end_write(bool unlock)
{
    return sequential_lock_.end_write() && (!unlock || crash_unlock());
}

bool store::crash_lock()
{
    return crash_lock_.lock_shared();
}

bool store::crash_unlock()
{
    return flush() && crash_lock_.unlock_shared();
}

} // namespace data_base
} // namespace libbitcoin
