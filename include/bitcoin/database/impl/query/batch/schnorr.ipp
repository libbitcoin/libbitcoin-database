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
#ifndef LIBBITCOIN_DATABASE_QUERY_BATCH_SCHNORR_IPP
#define LIBBITCOIN_DATABASE_QUERY_BATCH_SCHNORR_IPP

#include <bitcoin/database/define.hpp>
#include <bitcoin/database/types/types.hpp>

namespace libbitcoin {
namespace database {

TEMPLATE
bool CLASS::verify_schnorr_signatures(const stopper& cancel,
    header_links& links) NOEXCEPT
{
    // False return only implies canceled.
    using batch = system::schnorr::batch;
    const auto count = store_.schnorr.count().value;
    const auto ptr = store_.schnorr.get_memory();
    const auto rows = system::pointer_cast<const batch>(ptr->data());
    links = batch::verify(cancel, { rows, count });
    return !cancel;
}

// setters
// ----------------------------------------------------------------------------

TEMPLATE
bool CLASS::purge_schnorr_signatures() NOEXCEPT
{
    // ========================================================================
    const auto scope = store_.get_transactor();
    return store_.schnorr.truncate(0);
    // ========================================================================
}


TEMPLATE
bool CLASS::set_signature(const hash_digest& digest, const ec_xonly& point,
    const ec_signature& signature, uint16_t id, const header_link& link) NOEXCEPT
{
    // ========================================================================
    const auto scope = store_.get_transactor();

    // Clean single allocation failure (e.g. disk full).
    return store_.schnorr.put(table::schnorr::put_single_ref
    {
        {},
        digest,
        point,
        signature,
        id,
        link
    });
    // ========================================================================
}

TEMPLATE
bool CLASS::set_signatures(const threshold& batch, uint16_t id,
    const header_link& link) NOEXCEPT
{
    // ========================================================================
    const auto scope = store_.get_transactor();

    // Clean single allocation failure (e.g. disk full).
    return store_.schnorr.put(table::schnorr::put_multiple_ref
    {
        {},
        batch,
        id,
        link
    });
    // ========================================================================
}

} // namespace database
} // namespace libbitcoin

#endif
