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
#include <boost/filesystem.hpp>
#include <bitcoin/bitcoin.hpp>

namespace libbitcoin {
namespace database {

using namespace bc::chain;
using namespace bc::database;
using namespace boost::filesystem;
using namespace boost::system;

// Database file names.
const std::string store::FLUSH_LOCK = "flush_lock";
const std::string store::EXCLUSIVE_LOCK = "exclusive_lock";
const std::string store::BLOCK_TABLE = "block_table";
const std::string store::CANDIDATE_INDEX = "candidate_index";
const std::string store::CONFIRMED_INDEX = "confirmed_index";
const std::string store::TRANSACTION_INDEX = "transaction_index";
const std::string store::TRANSACTION_TABLE = "transaction_table";
const std::string store::ADDRESS_TABLE = "address_table";
const std::string store::ADDRESS_ROWS = "address_rows";

// Create a single file with one byte of arbitrary data.
static bool create_file(const path& file_path)
{
    // Disallow create with existing file.
    if (bc::ifstream(file_path.string()).good())
        return false;

    bc::ofstream file(file_path.string());

    if (!file.good())
        return false;

    // Write one byte so file is nonzero size (for memory map validation).
    file.put('x');
    return true;
}

// Construct.
// ------------------------------------------------------------------------

store::store(const path& prefix, bool with_indexes, bool flush_each_write)
  : prefix_(prefix),
    with_indexes_(with_indexes),
    flush_each_write_(flush_each_write),
    flush_lock_(prefix / FLUSH_LOCK),
    exclusive_lock_(prefix / EXCLUSIVE_LOCK),

    // Content store.
    block_table(prefix / BLOCK_TABLE),
    candidate_index(prefix / CANDIDATE_INDEX),
    confirmed_index(prefix / CONFIRMED_INDEX),
    transaction_index(prefix / TRANSACTION_INDEX),
    transaction_table(prefix / TRANSACTION_TABLE),

    // Optional indexes.
    address_table(prefix / ADDRESS_TABLE),
    address_rows(prefix / ADDRESS_ROWS)
{
}

// Open and close.
// ------------------------------------------------------------------------

// Create files.
bool store::create()
{
    error_code ec;
    create_directories(prefix_, ec);

    const auto created = !ec &&
        create_file(block_table) &&
        create_file(candidate_index) &&
        create_file(confirmed_index) &&
        create_file(transaction_index) &&
        create_file(transaction_table);

    if (!with_indexes_)
        return created;

    return
        created &&
        create_file(address_table) &&
        create_file(address_rows);
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
    return !flush_each_write() || flush_lock_.lock_shared();
}

bool store::end_write() const
{
    return !flush_each_write() || (flush() && flush_lock_.unlock_shared());
}

bool store::flush_each_write() const
{
    return flush_each_write_;
}

} // namespace database
} // namespace libbitcoin
