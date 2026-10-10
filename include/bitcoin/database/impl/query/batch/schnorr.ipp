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
#ifndef LIBBITCOIN_DATABASE_QUERY_BATCH_SCHNORR_IPP
#define LIBBITCOIN_DATABASE_QUERY_BATCH_SCHNORR_IPP

#include <bitcoin/database/define.hpp>
#include <bitcoin/database/types/types.hpp>

namespace libbitcoin {
namespace database {

TEMPLATE
size_t CLASS::schnorr_records(bool bank) const NOEXCEPT
{
    return store_.schnorr_bank(bank).count();
}

TEMPLATE
bool CLASS::verify_schnorr_signatures(const stopper& cancel,
    header_links& links, bool& device, bool bank) NOEXCEPT
{
    auto& bank_table = store_.schnorr_bank(bank);
    const auto correlate_ptr = bank_table.correlate.get_memory();
    const auto row_ptr = bank_table.row.get_memory();

    using correlate_t = const system::schnorr::batch::correlate_t;
    using row_t = const system::schnorr::batch::row_t;

    using namespace system;
    const auto correlate = pointer_cast<correlate_t>(correlate_ptr.data());
    const auto row = pointer_cast<row_t>(row_ptr.data());

    const auto count = bank_table.count();
    const schnorr::batch batch
    {
        .correlates = { correlate, count },
        .rows = { row, count }
    };

    // False return only implies canceled.
    links = schnorr::batch::verify(device, cancel, batch);
    return !cancel;
}

// setters
// ----------------------------------------------------------------------------

TEMPLATE
bool CLASS::purge_schnorr_signatures(bool bank) NOEXCEPT
{
    // ========================================================================
    const auto scope = get_transactor();
    return store_.schnorr_bank(bank).truncate(0);
    // ========================================================================
}

TEMPLATE
bool CLASS::set_signatures(const system::chain::schnorr_signatures& sigs,
    const header_link& link, bool bank) NOEXCEPT
{
    using correlate_t = table::schnorr_correlate::put_signatures;
    using row_t = table::schnorr_row::put_signatures;

    if (sigs.empty())
        return true;

    using namespace system;
    const auto rows = possible_narrow_cast<schnorr_link::integer>(
        sigs.rows().size());

    // Caller must guard reads, this is writing into hot storage.
    // ========================================================================
    const auto scope = get_transactor();
    auto& bank_table = store_.schnorr_bank(bank);

    // Allocate all of the block's rows across all columns.
    const auto fk = bank_table.allocate(rows);
    if (fk.is_terminal())
        return false;

    // Guard against remap (required for nomaps::put(fk)).
    const auto guard = bank_table.guard();

    // Write the accumulator to the columns.
    return
        bank_table.correlate.put(fk, correlate_t{ {}, link, sigs }) &&
        bank_table.row.put(fk, row_t{ {}, sigs });
    // ========================================================================
}

} // namespace database
} // namespace libbitcoin

#endif
