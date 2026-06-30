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
#ifndef LIBBITCOIN_DATABASE_STORE_SNAPSHOT_IPP
#define LIBBITCOIN_DATABASE_STORE_SNAPSHOT_IPP

#include <chrono>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

// public
TEMPLATE
code CLASS::snapshot(const event_handler& handler, bool prune) NOEXCEPT
{
    while (!prune && !transactor_mutex_.try_lock_for(std::chrono::seconds(1)))
    {
        handler(event_t::wait_lock, table_t::store);
    }

    code ec{ error::success };
    const auto flush = [&handler](code& ec, auto& file, table_t table) NOEXCEPT
    {
        if (!ec)
        {
            handler(event_t::flush_body, table);
            ec = file.flush();
        }
    };

    // Assumes/requires tables open/loaded.
    flush(ec, header_body_, table_t::header_body);
    flush(ec, input_body_, table_t::input_body);
    flush(ec, output_body_, table_t::output_body);
    flush(ec, point_body_, table_t::point_body);
    flush(ec, ins_body_, table_t::ins_body);
    flush(ec, outs_body_, table_t::outs_body);
    flush(ec, tx_body_, table_t::tx_body);
    flush(ec, txs_body_, table_t::txs_body);

    flush(ec, candidate_body_, table_t::candidate_body);
    flush(ec, confirmed_body_, table_t::confirmed_body);
    flush(ec, strong_tx_body_, table_t::strong_tx_body);

    flush(ec, ecdsa_body_, table_t::ecdsa_body);
    flush(ec, schnorr_body_, table_t::schnorr_body);
    flush(ec, silent_body_, table_t::silent_body);
    flush(ec, duplicate_body_, table_t::duplicate_body);
    flush(ec, prevalid_body_, table_t::prevalid_body);
    if (!prune) flush(ec, prevout_body_, table_t::prevout_body);
    flush(ec, validated_bk_body_, table_t::validated_bk_body);
    flush(ec, validated_tx_body_, table_t::validated_tx_body);

    flush(ec, address_body_, table_t::address_body);
    flush(ec, filter_bk_body_, table_t::filter_bk_body);
    flush(ec, filter_tx_body_, table_t::filter_tx_body);

    if (!ec) ec = backup(handler, prune);
    if (!prune) transactor_mutex_.unlock();
    return ec;
}

} // namespace database
} // namespace libbitcoin

#endif
