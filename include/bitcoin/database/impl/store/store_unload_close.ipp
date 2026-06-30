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
#ifndef LIBBITCOIN_DATABASE_STORE_UNLOAD_CLOSE_IPP
#define LIBBITCOIN_DATABASE_STORE_UNLOAD_CLOSE_IPP

#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

// protected
TEMPLATE
code CLASS::unload_close(const event_handler& handler) NOEXCEPT
{
    code ec{ error::success };
    const auto unload = [&handler](code& ec, auto& file, table_t table) NOEXCEPT
    {
        if (!ec)
        {
            handler(event_t::unload_file, table);
            ec = file.unload();
        }
    };

    unload(ec, header_head_, table_t::header_head);
    unload(ec, header_body_, table_t::header_body);
    unload(ec, input_head_, table_t::input_head);
    unload(ec, input_body_, table_t::input_body);
    unload(ec, output_head_, table_t::output_head);
    unload(ec, output_body_, table_t::output_body);
    unload(ec, point_head_, table_t::point_head);
    unload(ec, point_body_, table_t::point_body);
    unload(ec, ins_head_, table_t::ins_head);
    unload(ec, ins_body_, table_t::ins_body);
    unload(ec, outs_head_, table_t::outs_head);
    unload(ec, outs_body_, table_t::outs_body);
    unload(ec, tx_head_, table_t::tx_head);
    unload(ec, tx_body_, table_t::tx_body);
    unload(ec, txs_head_, table_t::txs_head);
    unload(ec, txs_body_, table_t::txs_body);

    unload(ec, candidate_head_, table_t::candidate_head);
    unload(ec, candidate_body_, table_t::candidate_body);
    unload(ec, confirmed_head_, table_t::confirmed_head);
    unload(ec, confirmed_body_, table_t::confirmed_body);
    unload(ec, strong_tx_head_, table_t::strong_tx_head);
    unload(ec, strong_tx_body_, table_t::strong_tx_body);

    unload(ec, ecdsa_head_, table_t::ecdsa_head);
    unload(ec, ecdsa_body_, table_t::ecdsa_body);
    unload(ec, schnorr_head_, table_t::schnorr_head);
    unload(ec, schnorr_body_, table_t::schnorr_body);
    unload(ec, silent_head_, table_t::silent_head);
    unload(ec, silent_body_, table_t::silent_body);
    unload(ec, duplicate_head_, table_t::duplicate_head);
    unload(ec, duplicate_body_, table_t::duplicate_body);
    unload(ec, prevalid_head_, table_t::prevalid_head);
    unload(ec, prevalid_body_, table_t::prevalid_body);
    unload(ec, prevout_head_, table_t::prevout_head);
    unload(ec, prevout_body_, table_t::prevout_body);
    unload(ec, validated_bk_head_, table_t::validated_bk_head);
    unload(ec, validated_bk_body_, table_t::validated_bk_body);
    unload(ec, validated_tx_head_, table_t::validated_tx_head);
    unload(ec, validated_tx_body_, table_t::validated_tx_body);

    unload(ec, address_head_, table_t::address_head);
    unload(ec, address_body_, table_t::address_body);
    unload(ec, filter_bk_head_, table_t::filter_bk_head);
    unload(ec, filter_bk_body_, table_t::filter_bk_body);
    unload(ec, filter_tx_head_, table_t::filter_tx_head);
    unload(ec, filter_tx_body_, table_t::filter_tx_body);

    const auto close = [&handler](code& ec, auto& file, table_t table) NOEXCEPT
    {
        if (!ec)
        {
            handler(event_t::close_file, table);
            ec = file.close();
        }
    };

    close(ec, header_head_, table_t::header_head);
    close(ec, header_body_, table_t::header_body);
    close(ec, input_head_, table_t::input_head);
    close(ec, input_body_, table_t::input_body);
    close(ec, output_head_, table_t::output_head);
    close(ec, output_body_, table_t::output_body);
    close(ec, point_head_, table_t::point_head);
    close(ec, point_body_, table_t::point_body);
    close(ec, ins_head_, table_t::ins_head);
    close(ec, ins_body_, table_t::ins_body);
    close(ec, outs_head_, table_t::outs_head);
    close(ec, outs_body_, table_t::outs_body);
    close(ec, tx_head_, table_t::tx_head);
    close(ec, tx_body_, table_t::tx_body);
    close(ec, txs_head_, table_t::txs_head);
    close(ec, txs_body_, table_t::txs_body);

    close(ec, candidate_head_, table_t::candidate_head);
    close(ec, candidate_body_, table_t::candidate_body);
    close(ec, confirmed_head_, table_t::confirmed_head);
    close(ec, confirmed_body_, table_t::confirmed_body);
    close(ec, strong_tx_head_, table_t::strong_tx_head);
    close(ec, strong_tx_body_, table_t::strong_tx_body);

    close(ec, ecdsa_head_, table_t::ecdsa_head);
    close(ec, ecdsa_body_, table_t::ecdsa_body);
    close(ec, schnorr_head_, table_t::schnorr_head);
    close(ec, schnorr_body_, table_t::schnorr_body);
    close(ec, silent_head_, table_t::silent_head);
    close(ec, silent_body_, table_t::silent_body);
    close(ec, duplicate_head_, table_t::duplicate_head);
    close(ec, duplicate_body_, table_t::duplicate_body);
    close(ec, prevalid_head_, table_t::prevalid_head);
    close(ec, prevalid_body_, table_t::prevalid_body);
    close(ec, prevout_head_, table_t::prevout_head);
    close(ec, prevout_body_, table_t::prevout_body);
    close(ec, validated_bk_head_, table_t::validated_bk_head);
    close(ec, validated_bk_body_, table_t::validated_bk_body);
    close(ec, validated_tx_head_, table_t::validated_tx_head);
    close(ec, validated_tx_body_, table_t::validated_tx_body);

    close(ec, address_head_, table_t::address_head);
    close(ec, address_body_, table_t::address_body);
    close(ec, filter_bk_head_, table_t::filter_bk_head);
    close(ec, filter_bk_body_, table_t::filter_bk_body);
    close(ec, filter_tx_head_, table_t::filter_tx_head);
    close(ec, filter_tx_body_, table_t::filter_tx_body);

    return ec;
}

} // namespace database
} // namespace libbitcoin

#endif
