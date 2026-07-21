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
    const auto correlate_ptr = store_.schnorr.correlate.get_memory();
    const auto digest_ptr = store_.schnorr.digest.get_memory();
    const auto xonly_ptr = store_.schnorr.xonly.get_memory();
    const auto signature_ptr = store_.schnorr.signature.get_memory();

    using correlate_t = const system::schnorr::batch::correlate_t;
    using digest_t = const table::schnorr_digest::span;
    using xonly_t = const table::schnorr_xonly::span;
    using signature_t = const table::schnorr_signature::span;

    using namespace system;
    const auto correlate = pointer_cast<correlate_t>(correlate_ptr.data());
    const auto digest = pointer_cast<digest_t>(digest_ptr.data());
    const auto xonly = pointer_cast<xonly_t>(xonly_ptr.data());
    const auto signature = pointer_cast<signature_t>(signature_ptr.data());

    // Shortest column.
    const auto count = store_.schnorr.count();
    const schnorr::batch batch
    {
        .correlates = { correlate, count },
        .digests = { digest, count },
        .points = { xonly, count },
        .signatures = { signature, count }
    };

    // False return only implies canceled.
    links = schnorr::batch::verify(cancel, batch);
    return !cancel;
}

// setters
// ----------------------------------------------------------------------------

TEMPLATE
bool CLASS::purge_schnorr_signatures() NOEXCEPT
{
    // ========================================================================
    const auto scope = get_transactor();
    return store_.schnorr.truncate(0);
    // ========================================================================
}

TEMPLATE
bool CLASS::set_signatures(const system::chain::schnorr_signatures& sigs,
    const header_link& link) NOEXCEPT
{
    using correlate_t = table::schnorr_correlate::put_signatures;
    using digest_t = table::schnorr_digest::put_signatures;
    using xonly_t = table::schnorr_xonly::put_signatures;
    using signature_t = table::schnorr_signature::put_signatures;

    if (sigs.empty())
        return true;

    using namespace system;
    const auto rows = possible_narrow_cast<schnorr_link::integer>(
        sigs.rows().size());

    // Caller must guard reads, this is writing into hot storage.
    // ========================================================================
    const auto scope = get_transactor();

    // Allocate all of the block's rows across all columns.
    const auto fk = store_.schnorr.allocate(rows);
    if (fk.is_terminal())
        return false;

    // Guard against remap (required for nomaps::put(fk)).
    const auto guard = store_.schnorr.guard();

    // Deinterleave the accumulator into the columns.
    return
        store_.schnorr.correlate.put(fk, correlate_t{ {}, link, sigs }) &&
        store_.schnorr.digest.put(fk, digest_t{ {}, sigs }) &&
        store_.schnorr.xonly.put(fk, xonly_t{ {}, sigs }) &&
        store_.schnorr.signature.put(fk, signature_t{ {}, sigs });
    // ========================================================================
}

} // namespace database
} // namespace libbitcoin

#endif
