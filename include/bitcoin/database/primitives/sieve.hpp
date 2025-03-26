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
template <size_t SieveBits, size_t SelectorBits,
    if_not_greater<SelectorBits, SieveBits> = true,
    if_not_greater<SieveBits, bits<uint64_t>> = true>
class sieve
{
public:
    using sieve_t = unsigned_type<system::to_ceilinged_bytes(SieveBits)>;

    /// Initialize empty sieve.
    constexpr sieve() NOEXCEPT;

    /// Initialize existing sieve.
    constexpr sieve(sieve_t value) NOEXCEPT;

    /// The fingerprint value.
    constexpr sieve_t value() const NOEXCEPT;

    /// True if fingerprint aligns with sieve screen(s), bucket may contain.
    constexpr bool screened(sieve_t fingerprint) const NOEXCEPT;

    /// Add fingerprint to sieve, false if already screened.
    constexpr bool screen(sieve_t fingerprint) NOEXCEPT;

    /// The fingerprint value.
    constexpr operator sieve_t() const NOEXCEPT;

protected:
    static constexpr auto sieve_bits = SieveBits;
    static constexpr auto selector_bits = SelectorBits;
    static constexpr auto screen_bits = sieve_bits - selector_bits;
    static constexpr auto screens = system::power2(selector_bits);
    static constexpr auto limit = sub1(screens);
    static constexpr auto empty = system::unmask_right<sieve_t>(sieve_bits);
    static constexpr auto saturated = system::mask_right(empty, sub1(screen_bits));
    static constexpr auto selector_mask = system::mask_left<sieve_t>(selector_bits);

    static_assert(is_same_type<sieve_t, uint32_t>);
    using screen_t = std_array<sieve_t, screens>;
    using matrix_t = std_array<screen_t, screens>;
    static constexpr matrix_t matrix_
    {
        screen_t
        {
            // 1
            0b0000'1111111111111111111111111111
        },
        {
            // 2
            0b0000'1111111111111100000000000000,
            0b0000'0000000000000011111111111111
        },
        {
            // 3
            0b0000'1111111111000000000000000000,
            0b0000'0000000000000011111111100000,
            0b0000'0000000000111100000000011111
        },
        {
            // 4
            0b0000'1111111000000000000000000000,
            0b0000'0000000000000011111110000000,
            0b0000'0000000000111100000000011100,
            0b0000'0000000111000000000001100011
        },
        {
            // 5
            0b0000'1111110000000000000000000000,
            0b0000'0000000000000011111100000000,
            0b0000'0000000000111100000000011000,
            0b0000'0000000111000000000001100000,
            0b0000'0000001000000000000010000111
        },
        {
            // 6
            0b0000'1111100000000000000000000000,
            0b0000'0000000000000011111000000000,
            0b0000'0000000000111100000000010000,
            0b0000'0000000111000000000001100000,
            0b0000'0000001000000000000010000110,
            0b0000'0000010000000000000100001001
        },
        {
            // 7
            0b0000'1111000000000000000000000000,
            0b0000'0000000000000011110000000000,
            0b0000'0000000000111100000000000000,
            0b0000'0000000111000000000001000000,
            0b0000'0000001000000000000010000110,
            0b0000'0000010000000000000100001001,
            0b0000'0000100000000000001000110000
        },
        {
            // 8
            0b0000'1111000000000000000000000000,
            0b0000'0000000000000011110000000000,
            0b0000'0000000000111100000000000000,
            0b0000'0000000111000000000001000000,
            0b0000'0000001000000000000010000100,
            0b0000'0000010000000000000100001000,
            0b0000'0000100000000000001000100000,
            0b0000'0000000000000000000000010011
        },
        {
            // 9
            0b0000'1111000000000000000000000000,
            0b0000'0000000000000011100000000000,
            0b0000'0000000000111000000000000000,
            0b0000'0000000111000000000000000000,
            0b0000'0000001000000000000010000100,
            0b0000'0000010000000000000100001000,
            0b0000'0000100000000000001000100000,
            0b0000'0000000000000000000000010011,
            0b0000'0000000000000100010001000000
        },
        {
            // 10
            0b0000'1110000000000000000000000000,
            0b0000'0000000000000011100000000000,
            0b0000'0000000000111000000000000000,
            0b0000'0000000111000000000000000000,
            0b0000'0000001000000000000010000100,
            0b0000'0000010000000000000100001000,
            0b0000'0000100000000000001000100000,
            0b0000'0000000000000000000000010011,
            0b0000'0000000000000100010000000000,
            0b0000'0001000000000000000001000000
        },
        {
            // 11
            0b0000'1110000000000000000000000000,
            0b0000'0000000000000011100000000000,
            0b0000'0000000000111000000000000000,
            0b0000'0000000111000000000000000000,
            0b0000'0000001000000000000010000100,
            0b0000'0000010000000000000100001000,
            0b0000'0000100000000000001000000000,
            0b0000'0000000000000000000000010010,
            0b0000'0000000000000100010000000000,
            0b0000'0001000000000000000001000000,
            0b0000'0000000000000000000000100001
        },
        {
            // 12
            0b0000'1110000000000000000000000000,
            0b0000'0000000000000011100000000000,
            0b0000'0000000000111000000000000000,
            0b0000'0000000111000000000000000000,
            0b0000'0000001000000000000010000000,
            0b0000'0000010000000000000100000000,
            0b0000'0000100000000000001000000000,
            0b0000'0000000000000000000000010010,
            0b0000'0000000000000100010000000000,
            0b0000'0001000000000000000001000000,
            0b0000'0000000000000000000000100001,
            0b0000'0000000000000000000000001100
        },
        {
            // 13
            0b0000'1110000000000000000000000000,
            0b0000'0000000000000011100000000000,
            0b0000'0000000000110000000000000000,
            0b0000'0000000110000000000000000000,
            0b0000'0000001000000000000010000000,
            0b0000'0000010000000000000100000000,
            0b0000'0000100000000000001000000000,
            0b0000'0000000000000000000000010010,
            0b0000'0000000000000100010000000000,
            0b0000'0001000000000000000001000000,
            0b0000'0000000000000000000000100001,
            0b0000'0000000000000000000000001100,
            0b0000'0000000001001000000000000000
        },
        {
            // 14
            0b0000'1100000000000000000000000000,
            0b0000'0000000000000011000000000000,
            0b0000'0000000000110000000000000000,
            0b0000'0000000110000000000000000000,
            0b0000'0000001000000000000010000000,
            0b0000'0000010000000000000100000000,
            0b0000'0000100000000000001000000000,
            0b0000'0000000000000000000000010010,
            0b0000'0000000000000100010000000000,
            0b0000'0001000000000000000001000000,
            0b0000'0000000000000000000000100001,
            0b0000'0000000000000000000000001100,
            0b0000'0000000001001000000000000000,
            0b0000'0010000000000000100000000000
        },
        {
            // 15
            0b0000'1100000000000000000000000000,
            0b0000'0000000000000011000000000000,
            0b0000'0000000000110000000000000000,
            0b0000'0000000110000000000000000000,
            0b0000'0000001000000000000010000000,
            0b0000'0000010000000000000100000000,
            0b0000'0000100000000000001000000000,
            0b0000'0000000000000000000000010010,
            0b0000'0000000000000100010000000000,
            0b0000'0001000000000000000001000000,
            0b0000'0000000000000000000000100001,
            0b0000'0000000000000000000000001100,
            0b0000'0000000001001000000000000000,
            0b0000'0010000000000000000000000000,
            0b0000'0000000000000000100000000000
        },
        {
            // 16
            0b0000'0'100000000000000000000000000, // first bit sacrificed for sentinel
            0b0000'0000000000000011000000000000,
            0b0000'0000000000110000000000000000,
            0b0000'0000000110000000000000000000,
            0b0000'0000001000000000000010000000,
            0b0000'0000010000000000000100000000,
            0b0000'0000100000000000001000000000,
            0b0000'0000000000000000000000010010,
            0b0000'0000000000000100010000000000,
            0b0000'0001000000000000000001000000,
            0b0000'0000000000000000000000100001,
            0b0000'0000000000000000000000001100,
            0b0000'0000000001000000000000000000,
            0b0000'0010000000000000000000000000,
            0b0000'0000000000000000100000000000,
            0b0000'0000000000001000000000000000
        }
    };

private:
    sieve_t sieve_;
};

} // namespace database
} // namespace libbitcoin

#define TEMPLATE template <size_t SieveBits, size_t SelectorBits, \
    if_not_greater<SelectorBits, SieveBits> If1, \
    if_not_greater<SieveBits, bits<uint64_t>> If2>

#define CLASS sieve<SieveBits, SelectorBits, If1, If2>

#include <bitcoin/database/impl/primitives/sieve.ipp>

#undef CLASS
#undef TEMPLATE

#endif
