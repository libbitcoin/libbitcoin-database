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
#ifndef LIBBITCOIN_DATABASE_PRIMITIVES_BLOOM_HPP
#define LIBBITCOIN_DATABASE_PRIMITIVES_BLOOM_HPP

#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

/// Bloom is limited to integral types.
template <size_t m, size_t k,
    if_not_greater<k, m> = true,
    if_not_greater<m, bits<uint64_t>> = true>
class bloom
{
public:
    /// This produces size_t when disabled.
    using type = unsigned_type<system::to_ceilinged_bytes(m)>;

    /// Bloom is bypassed.
    static constexpr bool disabled = is_zero(m) || is_zero(k);

    /// Did fingerprint collide.
    static constexpr bool is_collision(type previous, type next) NOEXCEPT;

    /// Is potential collision.
    static constexpr bool is_screened(type value, type fingerprint) NOEXCEPT;

    /// Add fingerprint to bloom.
    static constexpr type screen(type value, type fingerprint) NOEXCEPT;
};

} // namespace database
} // namespace libbitcoin

#define TEMPLATE template <size_t m, size_t k, \
    if_not_greater<k, m> If1, \
    if_not_greater<m, bits<uint64_t>> If2>

#define CLASS bloom<m, k, If1, If2>

#include <bitcoin/database/impl/primitives/bloom.ipp>

#undef CLASS
#undef TEMPLATE

#endif
