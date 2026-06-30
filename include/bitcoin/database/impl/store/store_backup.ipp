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
#ifndef LIBBITCOIN_DATABASE_STORE_BACKUP_IPP
#define LIBBITCOIN_DATABASE_STORE_BACKUP_IPP

#include <atomic>
#include <chrono>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

// public
TEMPLATE
code CLASS::backup(const event_handler& handler, bool prune) NOEXCEPT
{
    code ec{ error::success };
    const auto backup = [&handler](code& ec, auto& logical,
        table_t table, bool prune=false) NOEXCEPT
    {
        if (!ec)
        {
            handler(event_t::backup_table, table);
            if (!logical.backup(prune))
                ec = error::backup_table;
        }
    };

    backup(ec, header, table_t::header_table);
    backup(ec, input, table_t::input_table);
    backup(ec, output, table_t::output_table);
    backup(ec, point, table_t::point_table);
    backup(ec, ins, table_t::ins_table);
    backup(ec, outs, table_t::outs_table);
    backup(ec, tx, table_t::tx_table);
    backup(ec, txs, table_t::txs_table);

    backup(ec, candidate, table_t::candidate_table);
    backup(ec, confirmed, table_t::confirmed_table);
    backup(ec, strong_tx, table_t::strong_tx_table);

    backup(ec, ecdsa, table_t::ecdsa_table);
    backup(ec, schnorr, table_t::schnorr_table);
    backup(ec, silent, table_t::silent_table);
    backup(ec, duplicate, table_t::duplicate_table);
    backup(ec, prevalid, table_t::prevalid_table, prune);
    backup(ec, prevout, table_t::prevout_table, prune);
    backup(ec, validated_bk, table_t::validated_bk_table);
    backup(ec, validated_tx, table_t::validated_tx_table);

    backup(ec, address, table_t::address_table);
    backup(ec, filter_bk, table_t::filter_bk_table);
    backup(ec, filter_tx, table_t::filter_tx_table);

    if (ec) return ec;

    static const auto primary = configuration_.path / schema::dir::primary;
    static const auto secondary = configuration_.path / schema::dir::secondary;
    static const auto temporary = configuration_.path / schema::dir::temporary;

    handler(event_t::archive_snapshot, table_t::store);

    // Ensure existing and empty /temporary.
    if ((ec = file::clear_directory_ex(temporary))) return ec;

    // Ensure no /primary.
    if (file::is_directory(primary))
    {
        // Delete /secondary.
        if ((ec = file::clear_directory_ex(secondary))) return ec;
        if ((ec = file::remove_ex(secondary))) return ec;

        // Rename /primary to /secondary (atomic).
        if ((ec = file::rename_ex(primary, secondary))) return ec;
    }

    // Dump /heads memory maps to /temporary.
    if ((ec = dump(temporary, handler)))
    {
        // Failed dump, clear temporary and rename secondary to primary.
        if (file::clear_directory(temporary) && file::remove(temporary))
            file::rename(secondary, primary);

        // Return original fault.
        return ec;
    }

    // Rename /temporary to /primary (atomic).
    return file::rename_ex(temporary, primary);
}

} // namespace database
} // namespace libbitcoin

#endif
