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
#include <bitcoin/database/result/address_result.hpp>

#include <cstdint>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/databases/transaction_database.hpp>
#include <bitcoin/database/memory/memory.hpp>
#include <bitcoin/database/result/address_iterator.hpp>

namespace libbitcoin {
namespace database {

address_result::address_result(const const_value_type& element,
    const short_hash& hash)
  : hash_(hash), element_(element)
{
}

address_result::operator bool() const
{
    return element_;
}

const short_hash& address_result::hash() const
{
    return hash_;
}

address_iterator address_result::begin() const
{
    return { element_ };
}

address_iterator address_result::end() const
{
    return { element_.terminator() };
}

} // namespace database
} // namespace libbitcoin
