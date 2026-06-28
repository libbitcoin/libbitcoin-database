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
#ifndef LIBBITCOIN_DATABASE_QUERY_BATCH_ECDSA_IPP
#define LIBBITCOIN_DATABASE_QUERY_BATCH_ECDSA_IPP

#include <bitcoin/database/define.hpp>
#include <bitcoin/database/types/types.hpp>

namespace libbitcoin {
namespace database {

TEMPLATE
bool CLASS::verify_ecdsa_signatures(const stopper& cancel,
    header_links& links) NOEXCEPT
{
    const auto correlate_ptr = store_.ecdsa.correlate.get_memory();
    const auto digest_ptr = store_.ecdsa.digest.get_memory();
    const auto compressed_ptr = store_.ecdsa.compressed.get_memory();
    const auto signature_ptr = store_.ecdsa.signature.get_memory();

    using correlate_t = const system::ecdsa::batch::correlate_t;
    using digest_t = const table::ecdsa_digest::span;
    using compressed_t = const table::ecdsa_compressed::span;
    using signature_t = const table::ecdsa_signature::span;

    using namespace system;
    const auto correlate = pointer_cast<correlate_t>(correlate_ptr->data());
    const auto digest = pointer_cast<digest_t>(digest_ptr->data());
    const auto compressed = pointer_cast<compressed_t>(compressed_ptr->data());
    const auto signature = pointer_cast<signature_t>(signature_ptr->data());

    // Shortest column.
    const auto count = store_.ecdsa.count();
    const ecdsa::batch batch
    {
        .correlates = { correlate, count },
        .digests = { digest, count },
        .points = { compressed, count },
        .signatures = { signature, count }
    };

    // False return only implies canceled.
    links = ecdsa::batch::verify(cancel, batch);
    return !cancel;
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
bool CLASS::set_signature(const hash_digest& digest, const ec_compressed& point,
    const ec_signature& signature, uint16_t id,
    const header_link& link) NOEXCEPT
{
    using allocate_t = table::ecdsa_correlate::allocate1;
    using correlate_t = table::ecdsa_correlate::put_ref;
    using digest_t = table::ecdsa_digest::put_ref;
    using compressed_t = table::ecdsa_compressed::put_ref;
    using signature_t = table::ecdsa_signature::put_ref;
    using namespace system;

    // All values in the table are only valid under write exclusion.
    // Table row cannot be assumed equal and tables remap independently.
    // ========================================================================
    const auto scope = store_.get_transactor();

    ecdsa_link fk{};
    const auto row = possible_narrow_cast<ecdsa_link::integer>(one);

    // Allocate one correlate row and write terminal to it, gets fk.
    // Then expand (as necessary) subordinate tables to same size.
    if (!store_.ecdsa.correlate.put_link(fk, allocate_t{}) ||
        !store_.ecdsa.digest.expand(fk + row) ||
        !store_.ecdsa.compressed.expand(fk + row) ||
        !store_.ecdsa.signature.expand(fk + row))
        return false;

    // Write one value to each column in corresponding positions.
    return
        store_.ecdsa.correlate.put(fk, correlate_t{ {}, link, id }) &&
        store_.ecdsa.digest.put(fk, digest_t{ {}, digest }) &&
        store_.ecdsa.compressed.put(fk, compressed_t{ {}, point }) &&
        store_.ecdsa.signature.put(fk, signature_t{ {}, signature });
    // ========================================================================
}

TEMPLATE
bool CLASS::set_signatures(const hash_digest& digest,
    const ec_compresseds& keys, const ec_signatures& sigs, uint16_t id,
    const header_link& link) NOEXCEPT
{
    using allocate_t = table::ecdsa_correlate::allocate;
    using correlate_t = table::ecdsa_correlate::put_refs;
    using digest_t = table::ecdsa_digest::put_refs;
    using compressed_t = table::ecdsa_compressed::put_refs;
    using signature_t = table::ecdsa_signature::put_refs;
    using namespace system;

    const auto csigs = sigs.size();
    const auto ckeys = keys.size();
    const auto count = table::ecdsa_count(csigs, ckeys);

    // All values in the table are only valid under write exclusion.
    // Table row cannot be assumed equal and tables remap independently.
    // ========================================================================
    const auto scope = store_.get_transactor();

    ecdsa_link fk{};
    const auto rows = possible_narrow_cast<ecdsa_link::integer>(count);

    // Allocate contiguous correlate rows and write terminal to each, gets fk.
    // Then expand (as necessary) subordinate tables to same size.
    if (!store_.ecdsa.correlate.put_link(fk, allocate_t{ {}, rows }) ||
        !store_.ecdsa.digest.expand(fk + rows) ||
        !store_.ecdsa.compressed.expand(fk + rows) ||
        !store_.ecdsa.signature.expand(fk + rows))
        return false;

    // Write values to each column in corresponding positions.
    return
        store_.ecdsa.correlate.put(fk, correlate_t{ {}, link, ckeys, csigs, id }) &&
        store_.ecdsa.digest.put(fk, digest_t{ {}, ckeys, csigs, digest }) &&
        store_.ecdsa.compressed.put(fk, compressed_t{ {}, keys, csigs }) &&
        store_.ecdsa.signature.put(fk, signature_t{ {}, ckeys, sigs });
    // ========================================================================
}

} // namespace database
} // namespace libbitcoin

#endif
