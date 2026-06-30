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
#ifndef LIBBITCOIN_DATABASE_QUERY_BATCH_PREVALID_IPP
#define LIBBITCOIN_DATABASE_QUERY_BATCH_PREVALID_IPP

#include <bitcoin/database/define.hpp>
#include <bitcoin/database/tables/tables.hpp>
#include <bitcoin/database/types/types.hpp>

namespace libbitcoin {
namespace database {

TEMPLATE
header_links CLASS::get_prevalids() const NOEXCEPT
{
    // Get all records, starting at the first position.
    const auto all = prevalid_records();
    const table::prevalid::link first{ 0 };

    header_links links(all);
    table::prevalid::get_refs out{ {}, links };
    if (store_.prevalid.get(first, out))
        return links;

    // Empty return in case of fault is an efficiency loss, table is dropped.
    return {};
}

// setters
// ----------------------------------------------------------------------------

TEMPLATE
bool CLASS::purge_prevalids() NOEXCEPT
{
    // ========================================================================
    const auto scope = store_.get_transactor();
    return store_.prevalid.truncate(0);
    // ========================================================================
}

TEMPLATE
bool CLASS::set_prevalids(const header_links& links) NOEXCEPT
{
    // ========================================================================
    const auto scope = store_.get_transactor();

    // Clean single allocation failure (e.g. disk full).
    const table::prevalid::put_refs prevalids{ {}, links };
    return store_.prevalid.put(prevalids);
    // ========================================================================
}

} // namespace database
} // namespace libbitcoin

#endif
