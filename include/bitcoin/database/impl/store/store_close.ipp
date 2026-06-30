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
#ifndef LIBBITCOIN_DATABASE_STORE_CLOSE_IPP
#define LIBBITCOIN_DATABASE_STORE_CLOSE_IPP

#include <chrono>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

// public
TEMPLATE
code CLASS::close(const event_handler& handler) NOEXCEPT
{
    // Transactor may be held outside of the node, such as for backup. 
    while (!transactor_mutex_.try_lock_for(std::chrono::seconds(1)))
    {
        handler(event_t::wait_lock, table_t::store);
    }

    code ec{ error::success };
    const auto close = [&handler](code& ec, auto& logical, table_t table) NOEXCEPT
    {
        if (!ec)
        {
            handler(event_t::close_table, table);
            if (!logical.close())
                ec = error::close_table;
        }
    };

    close(ec, header, table_t::header_table);
    close(ec, input, table_t::input_table);
    close(ec, output, table_t::output_table);
    close(ec, point, table_t::point_table);
    close(ec, ins, table_t::ins_table);
    close(ec, outs, table_t::outs_table);
    close(ec, tx, table_t::tx_table);
    close(ec, txs, table_t::txs_table);

    close(ec, candidate, table_t::candidate_table);
    close(ec, confirmed, table_t::confirmed_table);
    close(ec, strong_tx, table_t::strong_tx_table);

    close(ec, ecdsa, table_t::ecdsa_table);
    close(ec, schnorr, table_t::schnorr_table);
    close(ec, silent, table_t::silent_table);
    close(ec, duplicate, table_t::duplicate_table);
    close(ec, prevalid, table_t::prevalid_table);
    close(ec, prevout, table_t::prevout_table);
    close(ec, validated_bk, table_t::validated_bk_table);
    close(ec, validated_tx, table_t::validated_tx_table);

    close(ec, address, table_t::address_table);
    close(ec, filter_bk, table_t::filter_bk_table);
    close(ec, filter_tx, table_t::filter_tx_table);

    if (!ec) ec = unload_close(handler);

    // unlock errors override ec.
    if (!process_lock_.try_unlock())
        ec = error::process_unlock;

    // fault overrides unlock errors and leaves behind flush_lock.
    if (get_fault())
        ec = error::integrity;
    else if (!flush_lock_.try_unlock())
        ec = error::flush_unlock;

    transactor_mutex_.unlock();
    return ec;
}

} // namespace database
} // namespace libbitcoin

#endif
