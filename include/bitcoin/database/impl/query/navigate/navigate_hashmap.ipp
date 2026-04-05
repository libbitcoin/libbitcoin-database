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
#ifndef LIBBITCOIN_DATABASE_QUERY_NAVIGATE_HASHMAP_IPP
#define LIBBITCOIN_DATABASE_QUERY_NAVIGATE_HASHMAP_IPP

#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

// hashmap enumeration
// ----------------------------------------------------------------------------

TEMPLATE
header_link CLASS::top_header(size_t bucket) const NOEXCEPT
{
    using namespace system;
    return store_.header.top(possible_narrow_cast<header_link::integer>(bucket));
}

TEMPLATE
point_link CLASS::top_point(size_t bucket) const NOEXCEPT
{
    using namespace system;
    return store_.point.top(possible_narrow_cast<point_link::integer>(bucket));
}

TEMPLATE
tx_link CLASS::top_tx(size_t bucket) const NOEXCEPT
{
    using namespace system;
    return store_.tx.top(possible_narrow_cast<tx_link::integer>(bucket));
}

} // namespace database
} // namespace libbitcoin

#endif
