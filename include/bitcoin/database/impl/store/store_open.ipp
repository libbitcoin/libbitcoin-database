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
#ifndef LIBBITCOIN_DATABASE_STORE_OPEN_IPP
#define LIBBITCOIN_DATABASE_STORE_OPEN_IPP

#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

// public
TEMPLATE
code CLASS::open(const event_handler& handler) NOEXCEPT
{
    if (!file::is_directory(configuration_.path))
        return error::missing_directory;

    if (!transactor_mutex_.try_lock())
        return error::transactor_lock;

    if (!process_lock_.try_lock())
    {
        transactor_mutex_.unlock();
        return error::process_lock;
    }

    if (!flush_lock_.try_lock())
    {
        /* bool */ process_lock_.try_unlock();
        transactor_mutex_.unlock();
        return error::flush_lock;
    }

    const auto verify = [&handler](code& ec, auto& logical,
        table_t table) NOEXCEPT
    {
        if (!ec)
        {
            handler(event_t::verify_table, table);
            if (!logical.verify())
                ec = error::verify_table;
        }
    };

    auto ec = open_load(handler);

    verify(ec, header, table_t::header_table);
    verify(ec, input, table_t::input_table);
    verify(ec, output, table_t::output_table);
    verify(ec, point, table_t::point_table);
    verify(ec, ins, table_t::ins_table);
    verify(ec, outs, table_t::outs_table);
    verify(ec, tx, table_t::tx_table);
    verify(ec, txs, table_t::txs_table);

    verify(ec, candidate, table_t::candidate_table);
    verify(ec, confirmed, table_t::confirmed_table);
    verify(ec, strong_tx, table_t::strong_tx_table);

    verify(ec, ecdsa, table_t::ecdsa_table);
    verify(ec, schnorr, table_t::schnorr_table);
    verify(ec, silent, table_t::silent_table);
    verify(ec, duplicate, table_t::duplicate_table);
    verify(ec, prevalid, table_t::prevalid_table);
    verify(ec, prevout, table_t::prevout_table);
    verify(ec, validated_bk, table_t::validated_bk_table);
    verify(ec, validated_tx, table_t::validated_tx_table);

    verify(ec, address, table_t::address_table);
    verify(ec, filter_bk, table_t::filter_bk_table);
    verify(ec, filter_tx, table_t::filter_tx_table);

    if (ec)
    {
        /* code */ unload_close(handler);

        // unlock errors override ec.
        if (!flush_lock_.try_unlock()) ec = error::flush_unlock;
        if (!process_lock_.try_unlock()) ec = error::process_unlock;
    }

    // process and flush locks remain open until close().
    transactor_mutex_.unlock();
    return ec;
}

} // namespace database
} // namespace libbitcoin

#endif
