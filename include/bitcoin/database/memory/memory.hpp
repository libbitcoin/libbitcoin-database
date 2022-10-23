/**
 * Copyright (c) 2011-2019 libbitcoin developers (see AUTHORS)
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
#ifndef LIBBITCOIN_DATABASE_MEMORY_MEMORY_HPP
#define LIBBITCOIN_DATABASE_MEMORY_MEMORY_HPP

#include <memory>
#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

/// Protected memory access.
class BCD_API memory
{
public:
    typedef std::shared_ptr<memory> ptr;

    /// Get the byte pointer.
    virtual uint8_t* data() NOEXCEPT = 0;

    /// Increment the pointer the specified number of bytes within the record.
    virtual void increment(size_t value) NOEXCEPT = 0;
};

typedef memory::ptr memory_ptr;

} // namespace database
} // namespace libbitcoin

#endif
