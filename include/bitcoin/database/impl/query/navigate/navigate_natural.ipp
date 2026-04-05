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
#ifndef LIBBITCOIN_DATABASE_QUERY_NAVIGATE_NATURAL_IPP
#define LIBBITCOIN_DATABASE_QUERY_NAVIGATE_NATURAL_IPP

#include <algorithm>
#include <utility>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

// Use natural key (entry).
// ----------------------------------------------------------------------------

TEMPLATE
inline header_link CLASS::to_candidate(size_t height) const NOEXCEPT
{
    using namespace system;
    using link = table::height::header::integer;
    if (height >= store_.candidate.count())
        return {};

    table::height::record index{};
    if (!store_.candidate.get(possible_narrow_cast<link>(height), index))
        return {};

    return index.header_fk;
}

TEMPLATE
inline header_link CLASS::to_confirmed(size_t height) const NOEXCEPT
{
    using namespace system;
    using link = table::height::header::integer;
    if (height >= store_.confirmed.count())
        return {};

    table::height::record index{};
    if (!store_.confirmed.get(possible_narrow_cast<link>(height), index))
        return {};

    return index.header_fk;
}

TEMPLATE
inline header_link CLASS::to_header(const hash_digest& key) const NOEXCEPT
{
    return store_.header.first(key);
}

TEMPLATE
inline tx_link CLASS::to_tx(const hash_digest& key) const NOEXCEPT
{
    return store_.tx.first(key);
}

TEMPLATE
inline filter_link CLASS::to_filter(const header_link& key) const NOEXCEPT
{
    return store_.filter_tx.first(key);
}

TEMPLATE
output_link CLASS::to_output(const point& prevout) const NOEXCEPT
{
    return to_output(prevout.hash(), prevout.index());
}

TEMPLATE
output_link CLASS::to_output(const hash_digest& key,
    uint32_t input_index) const NOEXCEPT
{
    return to_output(to_tx(key), input_index);
}

// Obtain natural key (exit).
// ----------------------------------------------------------------------------

TEMPLATE
inline hash_digest CLASS::get_header_key(const header_link& link) const NOEXCEPT
{
    return store_.header.get_key(link);
}

TEMPLATE
inline hash_digest CLASS::get_tx_key(const tx_link& link) const NOEXCEPT
{
    return store_.tx.get_key(link);
}

TEMPLATE
inline point_key CLASS::get_point_key(const point_link& link) const NOEXCEPT
{
    table::point::get_composed point{};
    if (!store_.point.get(link, point))
        return {};

    return point.key;
}

TEMPLATE
inline hash_digest CLASS::get_point_hash(const point_link& link) const NOEXCEPT
{
    table::point::record point{};
    if (!store_.point.get(link, point))
        return {};

    return point.hash;
}

} // namespace database
} // namespace libbitcoin

#endif
