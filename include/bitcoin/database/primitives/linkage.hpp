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
#ifndef LIBBITCOIN_DATABASE_PRIMITIVES_LINKAGE_HPP
#define LIBBITCOIN_DATABASE_PRIMITIVES_LINKAGE_HPP

#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

/// Link serialization is always little-endian.
/// This provides portability between array and integral representation.
template <size_t Size>
struct linkage
{
    using integer = unsigned_type<Size>;
    using bytes = system::data_array<Size>;
    static constexpr auto eof = system::bit_all<integer>;
    static constexpr auto size = Size;

    constexpr linkage() NOEXCEPT;
    constexpr linkage(integer other) NOEXCEPT;
    inline linkage(const bytes& other) NOEXCEPT;

    constexpr linkage<Size>& operator=(integer other) NOEXCEPT;
    inline linkage<Size>& operator=(const bytes& other) NOEXCEPT;

    constexpr operator integer() const NOEXCEPT;
    inline operator bytes() const NOEXCEPT;

    constexpr bool is_eof() const NOEXCEPT;

    integer value;
};

} // namespace database
} // namespace libbitcoin

#define TEMPLATE template <size_t Size>
#define CLASS linkage<Size>

#include <bitcoin/database/impl/primitives/linkage.ipp>

#undef CLASS
#undef TEMPLATE

#endif
