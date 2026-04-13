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
#ifndef LIBBITCOIN_DATABASE_TYPES_HISTORY_HPP
#define LIBBITCOIN_DATABASE_TYPES_HISTORY_HPP

#include <bitcoin/database/define.hpp>
#include <bitcoin/database/types/type.hpp>

namespace libbitcoin {
namespace database {

struct BCD_API history
{
    static constexpr size_t rooted_height = zero;
    static constexpr size_t unrooted_height = max_size_t;
    static constexpr size_t missing_prevout = max_uint64;
    static constexpr size_t unconfirmed_position = max_size_t;

    static void filter_sort_and_dedup(std::vector<history>& history) NOEXCEPT;

    bool operator<(const history& other) const NOEXCEPT;
    bool operator==(const history& other) const NOEXCEPT;

    checkpoint tx{};
    uint64_t fee{};
    size_t position{};
};

using histories = std::vector<history>;

} // namespace database
} // namespace libbitcoin

#endif
