/**
 * Copyright (c) 2011-2025 libbitcoin developers (see AUTHORS)
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
#ifndef LIBBITCOIN_DATABASE_MEMORY_INTERFACES_MEMORY_HPP
#define LIBBITCOIN_DATABASE_MEMORY_INTERFACES_MEMORY_HPP

#include <memory>
#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

/// Protected memory access interface.
class memory
{
public:
    typedef uint8_t value_type;
    typedef value_type* iterator;
    typedef const value_type* const_iterator;
    typedef std::shared_ptr<memory> ptr;

    /// Return an offset from begin, nullptr if end or past end.
    virtual uint8_t* offset(size_t value) const NOEXCEPT = 0;

    /// The logical buffer size (from begin to end).
    virtual ptrdiff_t size() const NOEXCEPT = 0;

    /// Get logical buffer.
    virtual uint8_t* begin() NOEXCEPT = 0;
    virtual uint8_t* end() NOEXCEPT = 0;

    /// Alias begin.
    virtual uint8_t* data() NOEXCEPT = 0;
};

typedef memory::ptr memory_ptr;

} // namespace database
} // namespace libbitcoin

#endif
