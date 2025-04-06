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
template <size_t M, size_t K,
    if_not_greater<K, M> = true,
    if_not_greater<M, bits<uint64_t>> = true>
class bloom
{
public:
    /// This produces size_t when disabled.
    using type = unsigned_type<system::to_ceilinged_bytes(M)>;

    /// Bloom is bypassed.
    static constexpr bool disabled = is_zero(M) || is_zero(K);

    /// Did fingerprint collide.
    static constexpr bool is_collision(type previous, type next) NOEXCEPT;

    /// Is potential collision.
    static constexpr bool is_screened(type value, uint64_t entropy) NOEXCEPT;

    /// Add fingerprint to bloom.
    static constexpr type screen(type value, uint64_t entropy) NOEXCEPT;

protected:
    /// Effectively sentinel values.
    static constexpr type saturated = 0;
    static constexpr type empty = system::unmask_right<type>(M);

    /// 2^select must be >= M to address all bits in M (modulo if over).
    static constexpr size_t select = system::ceilinged_log2(M);

    /// For each K, select unique bits of entropy are required.
    static constexpr size_t min_entropy = system::safe_multiply(select, K);
    static_assert(min_entropy <= bits<uint64_t>);

    /// Return the k bit selection for the entropy.
    static constexpr size_t get_bit(size_t k, uint64_t entropy) NOEXCEPT;

    /// Is sentinel value for empty filter.
    static constexpr bool is_empty(type value) NOEXCEPT;

    /// Is sentinel value for saturated filter.
    static constexpr bool is_saturated(type value) NOEXCEPT;
};

} // namespace database
} // namespace libbitcoin

#define TEMPLATE template <size_t M, size_t K, \
    if_not_greater<K, M> If1, \
    if_not_greater<M, bits<uint64_t>> If2>

#define CLASS bloom<M, K, If1, If2>

#include <bitcoin/database/impl/primitives/bloom.ipp>

#undef CLASS
#undef TEMPLATE

#endif
