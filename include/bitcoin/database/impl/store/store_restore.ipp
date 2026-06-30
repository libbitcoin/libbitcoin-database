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
#ifndef LIBBITCOIN_DATABASE_STORE_RESTORE_IPP
#define LIBBITCOIN_DATABASE_STORE_RESTORE_IPP

#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

// public
TEMPLATE
code CLASS::restore(const event_handler& handler) NOEXCEPT
{
    if (!transactor_mutex_.try_lock())
        return error::transactor_lock;

    if (!process_lock_.try_lock())
    {
        transactor_mutex_.unlock();
        return error::process_lock;
    }

    // Requires that the store is already flush locked (corrupted).
    if (!flush_lock_.is_locked())
    {
        /* bool */ process_lock_.try_unlock();
        transactor_mutex_.unlock();
        return error::flush_lock;
    }

    code ec{ error::success };
    static const auto heads = configuration_.path / schema::dir::heads;
    static const auto primary = configuration_.path / schema::dir::primary;
    static const auto secondary = configuration_.path / schema::dir::secondary;
    static const auto temporary = configuration_.path / schema::dir::temporary;

    handler(event_t::recover_snapshot, table_t::store);

    // Clean up any residual /temporary.
    file::clear_directory(temporary);
    file::remove(temporary);

    if (file::is_directory(primary))
    {
        // Clear invalid /heads, recover from /primary, clone to /primary.
        ec = file::clear_directory_ex(heads);
        if (!ec) ec = file::remove_ex(heads);
        if (!ec) ec = file::rename_ex(primary, heads);
        if (!ec) ec = file::copy_directory_ex(heads, primary);
    }
    else if (file::is_directory(secondary))
    {
        // Clear invalid /heads, recover from /secondary, clone to /primary.
        ec = file::clear_directory_ex(heads);
        if (!ec) ec = file::remove_ex(heads);
        if (!ec) ec = file::rename_ex(secondary, heads);
        if (!ec) ec = file::copy_directory_ex(heads, primary);
    }
    else
    {
        ec = error::missing_snapshot;
    }

    // BUGBUG: height indexes are not append only, resulting in failure to
    // recover and allowing insidious integrity failure after recovery,
    // specifically orphaned header links in the index due to write below snap.
    // Regarding height index recovery there are two criteria for the fix.
    // (1) we must accept the existing body size if smaller than head indicates
    // (reset head), and (2) prune the body (with head indicator) to highest
    // contiguous set of links that do not exceed the header table count
    // (orphaned by snapshot restore). This requires a scan upon restore.
    const auto restore = [&handler](code& ec, auto& logical,
        table_t table) NOEXCEPT
    {
        if (!ec)
        {
            handler(event_t::restore_table, table);
            if (!logical.restore())
                ec = error::restore_table;
        }
    };

    const auto dropped = [&handler](code& ec, auto& logical,
        table_t table) NOEXCEPT
    {
        if (!ec)
        {
            handler(event_t::restore_table, table);
            if (!logical.drop())
                ec = error::restore_table;
        }
    };

    if (!ec)
        ec = open_load(handler);

    if (!ec)
    {
        restore(ec, header, table_t::header_table);
        restore(ec, input, table_t::input_table);
        restore(ec, output, table_t::output_table);
        restore(ec, point, table_t::point_table);
        restore(ec, ins, table_t::ins_table);
        restore(ec, outs, table_t::outs_table);
        restore(ec, tx, table_t::tx_table);
        restore(ec, txs, table_t::txs_table);

        restore(ec, candidate, table_t::candidate_table);
        restore(ec, confirmed, table_t::confirmed_table);
        restore(ec, strong_tx, table_t::strong_tx_table);

        restore(ec, ecdsa, table_t::ecdsa_table);
        restore(ec, schnorr, table_t::schnorr_table);
        restore(ec, silent, table_t::silent_table);
        restore(ec, duplicate, table_t::duplicate_table);
        dropped(ec, prevalid, table_t::prevalid_table);
        restore(ec, prevout, table_t::prevout_table);
        restore(ec, validated_bk, table_t::validated_bk_table);
        restore(ec, validated_tx, table_t::validated_tx_table);

        restore(ec, address, table_t::address_table);
        restore(ec, filter_bk, table_t::filter_bk_table);
        restore(ec, filter_tx, table_t::filter_tx_table);

        if (ec)
            /* code */ unload_close(handler);
    }

    if (ec)
    {
        // unlock errors override ec.
        // on failure flush_lock is left in place (store corrupt).
        // on success process and flush locks are held until close().
        ////if (!flush_lock_.try_unlock()) ec = error::flush_unlock;
        if (!process_lock_.try_unlock()) ec = error::process_unlock;
    }

    // store is open after successful restore but not otherwise.
    transactor_mutex_.unlock();
    return ec;
}

} // namespace database
} // namespace libbitcoin

#endif
