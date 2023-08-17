/**
 * Copyright (c) 2011-2023 libbitcoin developers (see AUTHORS)
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
#ifndef LIBBITCOIN_DATABASE_PRIMITIVES_LINKAGE_HPP
#define LIBBITCOIN_DATABASE_PRIMITIVES_LINKAGE_HPP

#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

/// Link serialization is always little-endian.
template <size_t Size>
struct linkage
{
    using integer = unsigned_type<Size>;
    using bytes = std_array<uint8_t, Size>;
    static constexpr auto size = Size;
    static constexpr auto terminal = system::unmask_right<integer>(to_bits(Size));

    /// Construct a terminal link.
    constexpr linkage() NOEXCEPT;

    /// Integral and array constructors.
    constexpr linkage(integer other) NOEXCEPT;
    inline linkage(const bytes& other) NOEXCEPT;

    /// Integral and array assignment operators.
    constexpr linkage<Size>& operator=(integer other) NOEXCEPT;
    inline linkage<Size>& operator=(const bytes& other) NOEXCEPT;

    /// Integral and array cast operators.
    ////constexpr operator bool() const NOEXCEPT;
    constexpr operator integer() const NOEXCEPT;
    inline operator bytes() const NOEXCEPT;

    /// True when value is terminal.
    constexpr bool is_terminal() const NOEXCEPT;

    integer value;
};

template <size_t Size>
bool operator==(const linkage<Size>& left, const linkage<Size>& right) NOEXCEPT
{
    return left.value == right.value;
}

template <size_t Size>
bool operator!=(const linkage<Size>& left, const linkage<Size>& right) NOEXCEPT
{
    return !(left == right);
}

} // namespace database
} // namespace libbitcoin

#define TEMPLATE template <size_t Size>
#define CLASS linkage<Size>

#include <bitcoin/database/impl/primitives/linkage.ipp>

#undef CLASS
#undef TEMPLATE

#endif
