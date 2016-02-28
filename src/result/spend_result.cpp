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
#include <bitcoin/database/result/spend_result.hpp>

#include <algorithm>
#include <cstdint>
#include <bitcoin/bitcoin.hpp>

namespace libbitcoin {
namespace database {

spend_result::spend_result(const uint8_t* record)
  : record_(record)
{
}

spend_result::operator bool() const
{
    return record_ != nullptr;
}

hash_digest spend_result::hash() const
{
    BITCOIN_ASSERT(record_ != nullptr);
    hash_digest result;
    std::copy(record_, record_ + hash_size, result.begin());
    return result;
}

uint32_t spend_result::index() const
{
    BITCOIN_ASSERT(record_ != nullptr);
    return from_little_endian_unsafe<uint32_t>(record_ + hash_size);
}

} // namespace database
} // namespace libbitcoin
