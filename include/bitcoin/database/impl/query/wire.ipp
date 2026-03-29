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
#ifndef LIBBITCOIN_DATABASE_QUERY_WIRE_IPP
#define LIBBITCOIN_DATABASE_QUERY_WIRE_IPP

#include <algorithm>
#include <utility>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

// Wire serialized objects.
// ----------------------------------------------------------------------------

TEMPLATE
bool CLASS::get_wire_header(byteflipper& flipper,
    const header_link& link) const NOEXCEPT
{
    const auto start = flipper.get_write_position();
    table::header::wire_header header{ {}, flipper };
    if (!store_.header.get(link, header))
    {
        flipper.invalidate();
        return false;
    }

    // Genesis header parent is defaulted, others must be looked up.
    if (header.parent_fk != schema::header::link::terminal)
    {
        flipper.set_position(start);
        table::header::wire_key key{ {}, flipper };
        if (!store_.header.get(header.parent_fk, key))
        {
            flipper.invalidate();
            return false;
        }
 
        flipper.set_position(start + system::chain::header::serialized_size());
    }

    return true;
}

TEMPLATE
bool CLASS::get_wire_tx(byteflipper& , const tx_link& ,
    bool ) const NOEXCEPT
{
    return {};
}

TEMPLATE
bool CLASS::get_wire_block(byteflipper& , const header_link& ,
    bool ) const NOEXCEPT
{
    return {};
}

TEMPLATE
data_chunk CLASS::get_wire_header(const header_link& link) const NOEXCEPT
{
    using namespace system;
    data_chunk data(chain::header::serialized_size());
    stream::flip::fast ostream(data);
    flip::bytes::fast out(ostream);
    if (!get_wire_header(out, link) || !out)
        data.clear();

    return data;
}

TEMPLATE
data_chunk CLASS::get_wire_tx(const tx_link& , bool ) const NOEXCEPT
{
    return {};
}

TEMPLATE
data_chunk CLASS::get_wire_block(const header_link& , bool ) const NOEXCEPT
{
    return {};
}

} // namespace database
} // namespace libbitcoin

#endif
