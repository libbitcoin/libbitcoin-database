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
#ifndef LIBBITCOIN_DATABASE_STORE_OPEN_LOAD_IPP
#define LIBBITCOIN_DATABASE_STORE_OPEN_LOAD_IPP

#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

// protected
TEMPLATE
code CLASS::open_load(const event_handler& handler) NOEXCEPT
{
    code ec{ error::success };
    const auto open = [&handler](code& ec, auto& file, table_t table) NOEXCEPT
    {
        if (!ec)
        {
            handler(event_t::open_file, table);
            ec = file.open();
        }
    };

    open(ec, header_head_, table_t::header_head);
    open(ec, header_body_, table_t::header_body);
    open(ec, input_head_, table_t::input_head);
    open(ec, input_body_, table_t::input_body);
    open(ec, output_head_, table_t::output_head);
    open(ec, output_body_, table_t::output_body);
    open(ec, point_head_, table_t::point_head);
    open(ec, point_body_, table_t::point_body);
    open(ec, ins_head_, table_t::ins_head);
    open(ec, ins_body_, table_t::ins_body);
    open(ec, outs_head_, table_t::outs_head);
    open(ec, outs_body_, table_t::outs_body);
    open(ec, tx_head_, table_t::tx_head);
    open(ec, tx_body_, table_t::tx_body);
    open(ec, txs_head_, table_t::txs_head);
    open(ec, txs_body_, table_t::txs_body);

    open(ec, candidate_head_, table_t::candidate_head);
    open(ec, candidate_body_, table_t::candidate_body);
    open(ec, confirmed_head_, table_t::confirmed_head);
    open(ec, confirmed_body_, table_t::confirmed_body);
    open(ec, strong_tx_head_, table_t::strong_tx_head);
    open(ec, strong_tx_body_, table_t::strong_tx_body);

    open(ec, ecdsa_head_, table_t::ecdsa_head);
    open(ec, ecdsa_body_, table_t::ecdsa_body);
    open(ec, schnorr_head_, table_t::schnorr_head);
    open(ec, schnorr_body_, table_t::schnorr_body);
    open(ec, silent_head_, table_t::silent_head);
    open(ec, silent_body_, table_t::silent_body);
    open(ec, duplicate_head_, table_t::duplicate_head);
    open(ec, duplicate_body_, table_t::duplicate_body);
    open(ec, prevalid_head_, table_t::prevalid_head);
    open(ec, prevalid_body_, table_t::prevalid_body);
    open(ec, prevout_head_, table_t::prevout_head);
    open(ec, prevout_body_, table_t::prevout_body);
    open(ec, validated_bk_head_, table_t::validated_bk_head);
    open(ec, validated_bk_body_, table_t::validated_bk_body);
    open(ec, validated_tx_head_, table_t::validated_tx_head);
    open(ec, validated_tx_body_, table_t::validated_tx_body);

    open(ec, address_head_, table_t::address_head);
    open(ec, address_body_, table_t::address_body);
    open(ec, filter_bk_head_, table_t::filter_bk_head);
    open(ec, filter_bk_body_, table_t::filter_bk_body);
    open(ec, filter_tx_head_, table_t::filter_tx_head);
    open(ec, filter_tx_body_, table_t::filter_tx_body);

    const auto load = [&handler](code& ec, auto& file, table_t table) NOEXCEPT
    {
        if (!ec)
        {
            handler(event_t::load_file, table);
            ec = file.load();
        }
    };

    load(ec, header_head_, table_t::header_head);
    load(ec, header_body_, table_t::header_body);
    load(ec, input_head_, table_t::input_head);
    load(ec, input_body_, table_t::input_body);
    load(ec, output_head_, table_t::output_head);
    load(ec, output_body_, table_t::output_body);
    load(ec, point_head_, table_t::point_head);
    load(ec, point_body_, table_t::point_body);
    load(ec, ins_head_, table_t::ins_head);
    load(ec, ins_body_, table_t::ins_body);
    load(ec, outs_head_, table_t::outs_head);
    load(ec, outs_body_, table_t::outs_body);
    load(ec, tx_head_, table_t::tx_head);
    load(ec, tx_body_, table_t::tx_body);
    load(ec, txs_head_, table_t::txs_head);
    load(ec, txs_body_, table_t::txs_body);

    load(ec, candidate_head_, table_t::candidate_head);
    load(ec, candidate_body_, table_t::candidate_body);
    load(ec, confirmed_head_, table_t::confirmed_head);
    load(ec, confirmed_body_, table_t::confirmed_body);
    load(ec, strong_tx_head_, table_t::strong_tx_head);
    load(ec, strong_tx_body_, table_t::strong_tx_body);

    load(ec, ecdsa_head_, table_t::ecdsa_head);
    load(ec, ecdsa_body_, table_t::ecdsa_body);
    load(ec, schnorr_head_, table_t::schnorr_head);
    load(ec, schnorr_body_, table_t::schnorr_body);
    load(ec, silent_head_, table_t::silent_head);
    load(ec, silent_body_, table_t::silent_body);
    load(ec, duplicate_head_, table_t::duplicate_head);
    load(ec, duplicate_body_, table_t::duplicate_body);
    load(ec, prevalid_head_, table_t::prevalid_head);
    load(ec, prevalid_body_, table_t::prevalid_body);
    load(ec, prevout_head_, table_t::prevout_head);
    load(ec, prevout_body_, table_t::prevout_body);
    load(ec, validated_bk_head_, table_t::validated_bk_head);
    load(ec, validated_bk_body_, table_t::validated_bk_body);
    load(ec, validated_tx_head_, table_t::validated_tx_head);
    load(ec, validated_tx_body_, table_t::validated_tx_body);

    load(ec, address_head_, table_t::address_head);
    load(ec, address_body_, table_t::address_body);
    load(ec, filter_bk_head_, table_t::filter_bk_head);
    load(ec, filter_bk_body_, table_t::filter_bk_body);
    load(ec, filter_tx_head_, table_t::filter_tx_head);
    load(ec, filter_tx_body_, table_t::filter_tx_body);

    // create, open, and restore each invoke open_load.
    const auto dirty = header_body_.size() > schema::header::minrow;
    dirty_.store(dirty, std::memory_order_relaxed);
    return ec;
}

} // namespace database
} // namespace libbitcoin

#endif
