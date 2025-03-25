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
#ifndef LIBBITCOIN_DATABASE_PRIMITIVES_SIEVE_IPP
#define LIBBITCOIN_DATABASE_PRIMITIVES_SIEVE_IPP

#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>

// [-------------------screen----------][--------------link--------------]
// [[selector][--------masks----------]][--------------link--------------]

// [[111][1111111111111111111111111111]] terminal sentinal (empty/default)
// [[000][1111111111111111111111111111]] 1 screen
// [[001][2222222222222211111111111111]] 2 screens
// [[010][3333333333222222222111111111]] 3 screens
// [[011][4444444333333322222221111111]] 4 screens
// [[100][5555554444443333332222211111]] 5 screens
// [[101][6666655555444443333322221111]] 6 screens
// [[110][7777666655554444333322221111]] 7 screens
// [[111][-777766665555444433332222111]] 8 screens (partial)
// [[111][1000000000000000000000000000]] overflow sentinal

// To minimize masking and shifting in alignment, the sieve is defined as a two
// dimensional matrix of precomputed. One dimension is determined by the
// screen selector the other is used to iterate through maskes for that screen.
// A single screen requires one mask, a double two, and so on. The max selector
// value indicates that the first bit of the screen is a sentinel. This is set
// for either terminal (all other bits set) or overflow (no other bits set).
// If the sentinel is unset then the max screen is in used on remaining bits.
// The seive should be terminal if and only if the link is terminal, and when
// overflowed implies that the bucket is saturated, rendering it unscreened.

// A 28 bit sieve requires 28 bits from each hashmap key, independent of both
// the hashmap hash function and each other. The mask is 

namespace libbitcoin {
namespace database {

TEMPLATE
constexpr CLASS::sieve() NOEXCEPT
{
}

} // namespace database
} // namespace libbitcoin

#endif
