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
#ifndef LIBBITCOIN_DATABASE_PRIMITIVES_SIEVE_HPP
#define LIBBITCOIN_DATABASE_PRIMITIVES_SIEVE_HPP

#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

/// Sieve is limited to integral types.
template <size_t SieveBits, size_t SelectBits,
    if_not_greater<SelectBits, SieveBits> = true,
    if_not_greater<SieveBits, bits<uint64_t>> = true>
class sieve
{
public:
    /// Functionality is bypassed.
    static constexpr bool disabled = is_zero(SieveBits) || is_zero(SelectBits);

    /// This produces type of size_t when disabled.
    using type = unsigned_type<system::to_ceilinged_bytes(SieveBits)>;

    /// Initialize empty sieve.
    constexpr sieve() NOEXCEPT;

    /// Initialize existing sieve.
    constexpr sieve(type value) NOEXCEPT;

    /// The fingerprint value.
    constexpr type value() const NOEXCEPT;

    /// True if fingerprint aligns with sieve screen(s), bucket may contain.
    constexpr bool screened(type fingerprint) const NOEXCEPT;

    /// Add fingerprint to sieve, false if already screened.
    constexpr bool screen(type fingerprint) NOEXCEPT;

    /// The fingerprint value.
    constexpr operator type() const NOEXCEPT;

protected:
    static constexpr auto sieve_bits = SieveBits;
    static constexpr auto select_bits = SelectBits;
    static constexpr auto screen_bits = sieve_bits - select_bits;

    static constexpr auto sentinel = sub1(screen_bits);
    static constexpr auto empty = system::unmask_right<type>(sieve_bits);
    static constexpr auto saturated = system::mask_right(empty, sentinel);
    static constexpr auto first_mask = system::unmask_right<type>(screen_bits);
    static constexpr auto selector_mask = first_mask;

    static constexpr auto screens = system::power2(select_bits);
    static constexpr auto mask_count = to_half(screens * add1(screens));
    static constexpr auto limit = sub1(screens);
    using masks_t = std_array<type, mask_count>;
    using offsets_t = std_array<type, screens>;

    /// Generate compression offsets at compile.
    static CONSTEVAL offsets_t generate_offsets() NOEXCEPT;

    /// Compile-time mask table generator.
    static CONSTEVAL masks_t generate_masks() NOEXCEPT;

    /// Read member compressed mask array as if it was a two-dimesional array.
    constexpr type masks(size_t row, size_t column) const NOEXCEPT;

private:
    // Logically sparse, e.g. 16 x 16 = 256 table of uint32_t (1024 bytes).
    // Compressed to one-dimensional 136 element array of uint32_t (544 bytes).
    static constexpr masks_t masks_ = generate_masks();
    type sieve_;
};

} // namespace database
} // namespace libbitcoin

#define TEMPLATE template <size_t SieveBits, size_t SelectBits, \
    if_not_greater<SelectBits, SieveBits> If1, \
    if_not_greater<SieveBits, bits<uint64_t>> If2>

#define CLASS sieve<SieveBits, SelectBits, If1, If2>

#include <bitcoin/database/impl/primitives/sieve.ipp>

#undef CLASS
#undef TEMPLATE

#endif
