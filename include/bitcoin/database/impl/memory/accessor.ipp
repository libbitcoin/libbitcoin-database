/**
 * Copyright (c) 2011-2022 libbitcoin developers (see AUTHORS)
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
#ifndef LIBBITCOIN_DATABASE_MEMORY_ACCESSOR_IPP
#define LIBBITCOIN_DATABASE_MEMORY_ACCESSOR_IPP

#include <iterator>
#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>

// Zero/negative size is allowed (automatically handled by bc streams).

namespace libbitcoin {
namespace database {

BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

TEMPLATE
inline CLASS::accessor(Mutex& mutex) NOEXCEPT
  : shared_lock_(mutex)
{
}

TEMPLATE
inline void CLASS::assign(uint8_t* begin, uint8_t* end) NOEXCEPT
{
    begin_ = begin;
    end_ = end;
    ////BC_ASSERT(!system::is_negative(size()));
}

TEMPLATE
inline uint8_t* CLASS::offset(size_t bytes) NOEXCEPT
{
    if (system::is_greater(bytes, size()))
        return nullptr;

    return std::next(begin_, bytes);
}

TEMPLATE
inline ptrdiff_t CLASS::size() const NOEXCEPT
{
    return std::distance(begin_, end_);
}

TEMPLATE
inline uint8_t* CLASS::begin() NOEXCEPT
{
    return begin_;
}

TEMPLATE
inline uint8_t* CLASS::end() NOEXCEPT
{
    return end_;
}

BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

} // namespace database
} // namespace libbitcoin

#endif
