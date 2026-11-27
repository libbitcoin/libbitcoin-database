/**
 * Copyright (c) 2011-2026 libbitcoin developers
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
#ifndef LIBBITCOIN_DATABASE_TYPES_TOUCH_HPP
#define LIBBITCOIN_DATABASE_TYPES_TOUCH_HPP

#include <algorithm>
#include <iterator>
#include <vector>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/types/type.hpp>

namespace libbitcoin {
namespace database {

/// Confirmed tx that funds and/or spends a watched output.
struct touch
{
    /// Sort by block order and merge elements of the same tx.
    static inline void sort_and_merge(std::vector<touch>& out) NOEXCEPT
    {
        if (out.empty())
            return;

        std::sort(out.begin(), out.end());
        auto last = out.begin();
        for (auto it = std::next(last); it != out.end(); ++it)
        {
            if (*it == *last)
            {
                last->funds = last->funds || it->funds;
                last->spends = last->spends || it->spends;
            }
            else
            {
                ++last;
                *last = *it;
            }
        }

        out.erase(std::next(last), out.end());
    }

    /// Block order (height then position).
    inline bool operator<(const touch& other) const NOEXCEPT
    {
        return (height == other.height) ? (position < other.position) :
            (height < other.height);
    }

    /// Same confirmed tx (flags are not considered).
    inline bool operator==(const touch& other) const NOEXCEPT
    {
        return height == other.height && position == other.position;
    }

    /// Confirmed block height.
    size_t height{};

    /// Tx position in the confirmed block.
    size_t position{};

    /// Strong tx.
    tx_link link{};

    /// Tx has a watched output.
    bool funds{};

    /// Tx spends a watched output.
    bool spends{};
};

using touches = std::vector<touch>;

} // namespace database
} // namespace libbitcoin

#endif
