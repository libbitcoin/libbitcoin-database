/**
 * Copyright (c) 2011-2026 libbitcoin developers (see AUTHORS)
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
#include <bitcoin/database/types/history.hpp>

#include <algorithm>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

// local
inline bool hash_less_than(const hash_digest& a, const hash_digest& b) NOEXCEPT
{
    using namespace system;
    constexpr auto hi = to_half(byte_bits);
    constexpr auto lo = sub1(power2<uint8_t>(hi));

    // return encode_hash(a) < encode_hash(a)
    for (auto byte{ hash_size }; !is_zero(byte); --byte)
    {
        const auto byte_a = a.at(sub1(byte));
        const auto byte_b = b.at(sub1(byte));

        const auto hi_a = shift_right(byte_a, hi);
        const auto hi_b = shift_right(byte_b, hi);
        if (hi_a != hi_b)
            return hi_a < hi_b;

        const auto lo_a = bit_and(byte_a, lo);
        const auto lo_b = bit_and(byte_b, lo);
        if (lo_a != lo_b)
            return lo_a < lo_b;
    }

    return false;
}

// local
inline bool less_than(const history& a, const history& b) NOEXCEPT
{
    const auto a_height = a.tx.height();
    const auto b_height = b.tx.height();
    const auto a_confirmed = (a.position != history::unconfirmed_position);
    const auto b_confirmed = (b.position != history::unconfirmed_position);

    // Confirmed before unconfirmed.
    if (a_confirmed != b_confirmed)
        return a_confirmed;

    // Chain.block height ascending (0 < max | x < y).
    if (a_height != b_height)
        return a_height < b_height;

    // Block.tx position ascending (positions must differ).
    if (a_confirmed)
        return a.position < b.position;

    // Both unconfirmed (0 or max), base16 lexical txid ascending.
    return hash_less_than(a.tx.hash(), b.tx.hash());
}

bool history::operator<(const history& other) const NOEXCEPT
{
    return less_than(*this, other);
}

bool history::operator==(const history& other) const NOEXCEPT
{
    return !(*this < other) && !(other < *this);
}

void history::filter_sort_and_dedup(std::vector<history>& out) NOEXCEPT
{
    const auto excluded = std::remove_if(out.begin(), out.end(),
        [](const history& element) NOEXCEPT
        {
            return !element.tx.is_valid();
        });

    out.erase(excluded, out.end());
    std::sort(out.begin(), out.end());
    out.erase(std::unique(out.begin(), out.end()), out.end());
}

} // namespace database
} // namespace libbitcoin
