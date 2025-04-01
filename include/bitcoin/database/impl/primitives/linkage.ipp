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
#ifndef LIBBITCOIN_DATABASE_PRIMITIVES_LINKAGE_IPP
#define LIBBITCOIN_DATABASE_PRIMITIVES_LINKAGE_IPP

#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

TEMPLATE
constexpr CLASS::linkage() NOEXCEPT
  : value(terminal)
{
}

TEMPLATE
constexpr CLASS::linkage(integer other) NOEXCEPT
  : value(other)
{
}

TEMPLATE
inline CLASS::linkage(const bytes& other) NOEXCEPT
{
    *this = other;
}

TEMPLATE
constexpr CLASS& CLASS::operator=(integer other) NOEXCEPT
{
    value = other;
    return *this;
}

TEMPLATE
inline CLASS& CLASS::operator=(const bytes& other) NOEXCEPT
{
    value = 0;
    system::unsafe_array_cast<uint8_t, Size>(&value) = other;
    ////value = system::native_from_little_end(value);
    return *this;
}

TEMPLATE
inline CLASS& CLASS::operator++() NOEXCEPT
{
    ++value;
    return *this;
}

TEMPLATE
inline CLASS CLASS::operator++(int) NOEXCEPT
{
    auto self = *this;
    ++(*this);
    return self;
}

TEMPLATE
constexpr CLASS::operator CLASS::integer() const NOEXCEPT
{
    return value;
}

TEMPLATE
inline CLASS::operator CLASS::bytes() const NOEXCEPT
{
    ////const auto little = system::native_to_little_end(value);
    return system::unsafe_array_cast<uint8_t, Size>(&value);
}

TEMPLATE
constexpr bool CLASS::is_terminal() const NOEXCEPT
{
    return value == terminal;
}

} // namespace database
} // namespace libbitcoin

#endif
