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
#ifndef LIBBITCOIN_DATABASE_QUERY_BATCH_SILENT_IPP
#define LIBBITCOIN_DATABASE_QUERY_BATCH_SILENT_IPP

#include <bitcoin/database/define.hpp>
#include <bitcoin/database/types/types.hpp>

namespace libbitcoin {
namespace database {


TEMPLATE
bool CLASS::scan_silent(const stopper& /*cancel*/, const ec_secret& /*scan_key*/,
    const silent_handler& /*callback*/) NOEXCEPT
{
    // TODO: implement.
    return {};
}

// setter
// ----------------------------------------------------------------------------

TEMPLATE
bool CLASS::set_silent(const std::vector<uint64_t>& prefixes,
    const ec_compressed& compressed, const tx_link& link) NOEXCEPT
{
    // ========================================================================
    const auto scope = store_.get_transactor();

    // Clean single allocation failure (e.g. disk full).
    return store_.silent.put(table::silent::put_ref
    {
        {},
        prefixes,
        compressed,
        link
    });
    // ========================================================================
}

} // namespace database
} // namespace libbitcoin

#endif
