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
#ifndef LIBBITCOIN_DATABASE_QUERY_SIGNATURES_IPP
#define LIBBITCOIN_DATABASE_QUERY_SIGNATURES_IPP

#include <bitcoin/database/define.hpp>
#include <bitcoin/database/types/types.hpp>

namespace libbitcoin {
namespace database {

TEMPLATE
bool CLASS::set_signature(const hash_digest& digest, const ec_xonly& point,
    const ec_signature& signature, const header_link& link) NOEXCEPT
{
    // ========================================================================
    const auto scope = store_.get_transactor();

    // Clean single allocation failure (e.g. disk full).
    return store_.schnorr.put(table::schnorr::record
    {
        {},
        digest,
        point,
        signature,
        link
    });
    // ========================================================================
}

TEMPLATE
bool CLASS::set_signature(const hash_digest& digest, const ec_compressed& point,
    const ec_signature& signature, const header_link& link) NOEXCEPT
{
    // ========================================================================
    const auto scope = store_.get_transactor();

    // Clean single allocation failure (e.g. disk full).
    return store_.ecdsa.put(table::ecdsa::record
    {
        {},
        digest,
        point,
        signature,
        link
    });
    // ========================================================================
}

TEMPLATE
bool CLASS::set_signatures(const hash_digest&, const ec_compresseds&,
    const ec_signatures&, size_t, const header_link&) NOEXCEPT
{
    // ========================================================================
    const auto scope = store_.get_transactor();

    ////// Clean single allocation failure (e.g. disk full).
    ////return store_.multisig.put(table::multisig::put_ref
    ////{
    ////    {},
    ////    digest,
    ////    keys,
    ////    sigs,
    ////    group,
    ////    link
    ////});
    // ========================================================================

    // false will result in local signature validation (performance).
    return true;
}

TEMPLATE
bool CLASS::set_signatures(const threshold& , size_t ,
    const header_link& ) NOEXCEPT
{
    // ========================================================================
    const auto scope = store_.get_transactor();

    ////// Clean single allocation failure (e.g. disk full).
    ////return store_.multisig.put(table::multisig::put_ref
    ////{
    ////    {},
    ////    digest,
    ////    keys,
    ////    sigs,
    ////    group,
    ////    link
    ////});
    // ========================================================================

    // false will result in script validation failure (consensus).
    return true;
}

} // namespace database
} // namespace libbitcoin

#endif
