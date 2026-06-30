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
#ifndef LIBBITCOIN_DATABASE_STORE_RELOAD_IPP
#define LIBBITCOIN_DATABASE_STORE_RELOAD_IPP

#include <chrono>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

// public
TEMPLATE
code CLASS::reload(const event_handler& handler) NOEXCEPT
{
    while (!transactor_mutex_.try_lock_for(std::chrono::seconds(1)))
    {
        handler(event_t::wait_lock, table_t::store);
    }

    code ec{ error::success };
    const auto reload = [&handler, this](code& ec, auto& file,
        table_t table) NOEXCEPT
    {
        if (!ec)
        {
            // If any storage has a fault it will return as failure code.
            if (to_bool(file.get_space()))
            {
                handler(event_t::load_file, table);
                ec = file.reload();
                this->dirty_.store(true, std::memory_order_relaxed);
            }
        }
    };

    reload(ec, header_head_, table_t::header_head);
    reload(ec, header_body_, table_t::header_body);
    reload(ec, input_head_, table_t::input_head);
    reload(ec, input_body_, table_t::input_body);
    reload(ec, output_head_, table_t::output_head);
    reload(ec, output_body_, table_t::output_body);
    reload(ec, point_head_, table_t::point_head);
    reload(ec, point_body_, table_t::point_body);
    reload(ec, ins_head_, table_t::ins_head);
    reload(ec, ins_body_, table_t::ins_body);
    reload(ec, outs_head_, table_t::outs_head);
    reload(ec, outs_body_, table_t::outs_body);
    reload(ec, tx_head_, table_t::tx_head);
    reload(ec, tx_body_, table_t::tx_body);
    reload(ec, txs_head_, table_t::txs_head);
    reload(ec, txs_body_, table_t::txs_body);

    reload(ec, candidate_head_, table_t::candidate_head);
    reload(ec, candidate_body_, table_t::candidate_body);
    reload(ec, confirmed_head_, table_t::confirmed_head);
    reload(ec, confirmed_body_, table_t::confirmed_body);
    reload(ec, strong_tx_head_, table_t::strong_tx_head);
    reload(ec, strong_tx_body_, table_t::strong_tx_body);

    reload(ec, ecdsa_head_, table_t::ecdsa_head);
    reload(ec, ecdsa_body_, table_t::ecdsa_body);
    reload(ec, schnorr_head_, table_t::schnorr_head);
    reload(ec, schnorr_body_, table_t::schnorr_body);
    reload(ec, silent_head_, table_t::silent_head);
    reload(ec, silent_body_, table_t::silent_body);
    reload(ec, duplicate_head_, table_t::duplicate_head);
    reload(ec, duplicate_body_, table_t::duplicate_body);
    reload(ec, prevalid_head_, table_t::prevalid_head);
    reload(ec, prevalid_body_, table_t::prevalid_body);
    reload(ec, prevout_head_, table_t::prevout_head);
    reload(ec, prevout_body_, table_t::prevout_body);
    reload(ec, validated_bk_head_, table_t::validated_bk_head);
    reload(ec, validated_bk_body_, table_t::validated_bk_body);
    reload(ec, validated_tx_head_, table_t::validated_tx_head);
    reload(ec, validated_tx_body_, table_t::validated_tx_body);

    reload(ec, address_head_, table_t::address_head);
    reload(ec, address_body_, table_t::address_body);
    reload(ec, filter_bk_head_, table_t::filter_bk_head);
    reload(ec, filter_bk_body_, table_t::filter_bk_body);
    reload(ec, filter_tx_head_, table_t::filter_tx_head);
    reload(ec, filter_tx_body_, table_t::filter_tx_body);

    transactor_mutex_.unlock();
    return ec;
}

} // namespace database
} // namespace libbitcoin

#endif
