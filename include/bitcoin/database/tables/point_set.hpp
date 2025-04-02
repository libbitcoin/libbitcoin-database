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
#ifndef LIBBITCOIN_DATABASE_TABLES_POINT_SET_HPP
#define LIBBITCOIN_DATABASE_TABLES_POINT_SET_HPP

#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/tables/schema.hpp>

namespace libbitcoin {
namespace database {

struct point_set
{
    using tx_link = schema::transaction::link;

    struct point
    {
        // From header->prevouts cache.
        tx_link tx{};
        bool coinbase{};
        uint32_t sequence{};
    };

    // From block->txs->tx get version and points.resize(count).
    uint32_t version{};
    std::vector<point> points{};
};
using point_sets = std::vector<point_set>;

} // namespace database
} // namespace libbitcoin

#endif
