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
#ifndef LIBBITCOIN_DATABASE_MEMORY_HPP
#define LIBBITCOIN_DATABASE_MEMORY_HPP

#include <cstddef>
#include <cstdint>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

/// This interface defines remap safe unrestricted access to a memory map.
class BCD_API memory
{
public:
    /// Get the address indicated by the pointer.
    virtual uint8_t* buffer();

    /// Increment the pointer the specified number of bytes within the record.
    void increment(size_t value);
};

} // namespace database
} // namespace libbitcoin

#endif
