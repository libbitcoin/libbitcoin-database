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
bool CLASS::verify_ecdsa_signatures(header_links& links) NOEXCEPT
{
    using batch = system::ecdsa::batch;
    const auto count = store_.ecdsa.count().value;
    const auto ptr = store_.ecdsa.get_memory();
    const auto rows = system::pointer_cast<const batch>(ptr->data());
    links = batch::verify({ rows, count }, store_.turbo());
    return true;
}

TEMPLATE
bool CLASS::verify_schnorr_signatures(header_links& links) NOEXCEPT
{
    using batch = system::schnorr::batch;
    const auto count = store_.schnorr.count().value;
    const auto ptr = store_.schnorr.get_memory();
    const auto rows = system::pointer_cast<const batch>(ptr->data());
    links = batch::verify({ rows, count }, store_.turbo());
    return true;
}

// setters
// ----------------------------------------------------------------------------

TEMPLATE
bool CLASS::purge_ecdsa_signatures() NOEXCEPT
{
    // ========================================================================
    const auto scope = store_.get_transactor();
    return store_.ecdsa.truncate(0);
    // ========================================================================
}

TEMPLATE
bool CLASS::purge_schnorr_signatures() NOEXCEPT
{
    // ========================================================================
    const auto scope = store_.get_transactor();
    return store_.schnorr.truncate(0);
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

    return true;
}

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

    return true;
}

TEMPLATE
bool CLASS::set_signatures(const hash_digest&, const ec_compresseds&,
    const ec_signatures&, size_t, const header_link&) NOEXCEPT
{
    // ========================================================================
    const auto scope = store_.get_transactor();

    // TODO: flatten via store_.ecdsa.put();

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

    return true;
}

TEMPLATE
bool CLASS::set_signatures(const threshold& , size_t ,
    const header_link& ) NOEXCEPT
{
    // ========================================================================
    const auto scope = store_.get_transactor();

    // TODO: flatten via store_.schnorr.put();

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

    return true;
}

} // namespace database
} // namespace libbitcoin

#endif
