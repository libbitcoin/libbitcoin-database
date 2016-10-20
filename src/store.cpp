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
    write_lock_(prefix / "write_lock", false),
    store_lock_(prefix / "store_lock", true),

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
    return store_lock_.try_lock();
}

bool store::close()
{
    return store_lock_.unlock();
}

} // namespace data_base
} // namespace libbitcoin
