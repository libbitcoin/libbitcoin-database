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
#include <bitcoin/database/types/unspent.hpp>

#include <algorithm>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

// local
bool less_than(const unspent& a, const unspent& b) NOEXCEPT
{
    const auto a_point = a.tx.point();
    const auto b_point = b.tx.point();
    const bool a_confirmed = !is_zero(a.height);
    const bool b_confirmed = !is_zero(b.height);

    // Confirmed before unconfirmed.
    if (a_confirmed != b_confirmed)
        return a_confirmed;

    if (a_confirmed)
    {
        // Chain.block height ascending (x < y).
        if (a.height != b.height)
            return a.height < b.height;

        // Block.tx position ascending.
        if (a.position != b.position)
            return a.position < b.position;

        // Tx.output index ascending.
        return a_point.index() < b_point.index();
    }

    // Unconfirmed have 0 height/position, arbitrary sort (hash:index).
    return a_point < b_point;
}

bool unspent::operator<(const unspent& other) const NOEXCEPT
{
    return less_than(*this, other);
}

bool unspent::operator==(const unspent& other) const NOEXCEPT
{
    return !(*this < other) && !(other < *this);
}

void unspent::sort_and_dedup(std::vector<unspent>& out) NOEXCEPT
{
    const auto excluded = std::remove_if(out.begin(), out.end(),
        [](const unspent& element) NOEXCEPT
        {
            return !element.tx.is_valid();
        });

    out.erase(excluded, out.end());
    std::sort(out.begin(), out.end());
    out.erase(std::unique(out.begin(), out.end()), out.end());
}

} // namespace database
} // namespace libbitcoin
