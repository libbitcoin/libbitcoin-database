/**
 * Copyright (c) 2011-2026 libbitcoin developers
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

#include <span>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/types/types.hpp>

namespace libbitcoin {
namespace database {

TEMPLATE
size_t CLASS::ecdsa_records(bool bank) const NOEXCEPT
{
    return store_.ecdsa_bank(bank).count();
}

TEMPLATE
bool CLASS::verify_ecdsa_signatures(const stopper& cancel,
    header_links& links, bool bank) NOEXCEPT
{
    auto& bank_table = store_.ecdsa_bank(bank);
    const auto correlate_ptr = bank_table.correlate.get_memory();
    const auto digest_ptr = bank_table.digest.get_memory();
    const auto compressed_ptr = bank_table.compressed.get_memory();
    const auto signature_ptr = bank_table.signature.get_memory();

    using correlate_t = const system::ecdsa::batch::correlate_t;
    using digest_t = const table::ecdsa_digest::span;
    using compressed_t = const table::ecdsa_compressed::span;
    using signature_t = const table::ecdsa_signature::span;

    using namespace system;
    const auto correlate = pointer_cast<correlate_t>(correlate_ptr.data());
    const auto digest = pointer_cast<digest_t>(digest_ptr.data());
    const auto compressed = pointer_cast<compressed_t>(compressed_ptr.data());
    const auto signature = pointer_cast<signature_t>(signature_ptr.data());

    // Shortest column.
    const auto count = bank_table.count();
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
bool CLASS::purge_ecdsa_signatures(bool bank) NOEXCEPT
{
    // ========================================================================
    const auto scope = get_transactor();
    return store_.ecdsa_bank(bank).truncate(0);
    // ========================================================================
}

TEMPLATE
bool CLASS::set_signatures(const system::chain::ecdsa_signatures& sigs,
    const header_link& link, bool bank) NOEXCEPT
{
    using correlate_t = table::ecdsa_correlate::put_signatures;
    using digest_t = table::ecdsa_digest::put_signatures;
    using compressed_t = table::ecdsa_compressed::put_signatures;
    using signature_t = table::ecdsa_signature::put_signatures;

    if (sigs.empty())
        return true;

    using namespace system;
    const auto rows = possible_narrow_cast<ecdsa_link::integer>(sigs.rows());

    // Caller must guard reads, this is writing into hot storage.
    // ========================================================================
    const auto scope = get_transactor();
    auto& bank_table = store_.ecdsa_bank(bank);

    // Allocate all of the block's rows across all columns.
    const auto fk = bank_table.allocate(rows);
    if (fk.is_terminal())
        return false;

    // Guard against remap (required for nomaps::put(fk)).
    const auto guard = bank_table.guard();

    // Deinterleave the accumulator into the columns, expanding group bands.
    return
        bank_table.correlate.put(fk, correlate_t{ {}, link, sigs }) &&
        bank_table.digest.put(fk, digest_t{ {}, sigs }) &&
        bank_table.compressed.put(fk, compressed_t{ {}, sigs }) &&
        bank_table.signature.put(fk, signature_t{ {}, sigs });
    // ========================================================================
}

} // namespace database
} // namespace libbitcoin

#endif
