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
#ifndef LIBBITCOIN_DATABASE_TYPES_UNSPENT_HPP
#define LIBBITCOIN_DATABASE_TYPES_UNSPENT_HPP

#include <bitcoin/database/define.hpp>
#include <bitcoin/database/types/type.hpp>

namespace libbitcoin {
namespace database {

struct BCD_API unspent
{
    static constexpr size_t excluded_position = max_size_t;

    struct less_than
    {
        bool operator()(const unspent& a, const unspent& b) const NOEXCEPT;
    };

    struct equal_to
    {
        bool operator()(const unspent& a, const unspent& b) const NOEXCEPT;
    };

    struct exclude
    {
        bool operator()(const unspent& element) const NOEXCEPT;
    };

    static void sort_and_dedup(std::vector<unspent>& unspent) NOEXCEPT;

    outpoint tx{};
    size_t height{};
    size_t position{};
};

using unspents = std::vector<unspent>;

} // namespace database
} // namespace libbitcoin

#endif
