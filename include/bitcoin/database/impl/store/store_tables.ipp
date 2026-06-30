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
#ifndef LIBBITCOIN_DATABASE_STORE_TABLES_IPP
#define LIBBITCOIN_DATABASE_STORE_TABLES_IPP

#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

TEMPLATE
const CLASS::table_map CLASS::tables
{
    { table_t::store, "store" },

    // Archives.
    { table_t::header_table, "header_table" },
    { table_t::header_head, "header_head" },
    { table_t::header_body, "header_body" },
    { table_t::input_table, "input_table" },
    { table_t::input_head, "input_head" },
    { table_t::input_body, "input_body" },
    { table_t::output_table, "output_table" },
    { table_t::output_head, "output_head" },
    { table_t::output_body, "output_body" },
    { table_t::point_table, "point_table" },
    { table_t::point_head, "point_head" },
    { table_t::point_body, "point_body" },
    { table_t::ins_table, "ins_table" },
    { table_t::ins_head, "ins_head" },
    { table_t::ins_body, "ins_body" },
    { table_t::outs_table, "outs_table" },
    { table_t::outs_head, "outs_head" },
    { table_t::outs_body, "outs_body" },
    { table_t::tx_table, "tx_table" },
    { table_t::tx_head, "tx_head" },
    { table_t::txs_table, "txs_table" },
    { table_t::tx_body, "tx_body" },
    { table_t::txs_head, "txs_head" },
    { table_t::txs_body, "txs_body" },

    // Indexes.
    { table_t::candidate_table, "candidate_table" },
    { table_t::candidate_head, "candidate_head" },
    { table_t::candidate_body, "candidate_body" },
    { table_t::confirmed_table, "confirmed_table" },
    { table_t::confirmed_head, "confirmed_head" },
    { table_t::confirmed_body, "confirmed_body" },
    { table_t::strong_tx_table, "strong_tx_table" },
    { table_t::strong_tx_head, "strong_tx_head" },
    { table_t::strong_tx_body, "strong_tx_body" },

    // Caches.
    { table_t::ecdsa_table, "ecdsa_table" },
    { table_t::ecdsa_head, "ecdsa_head" },
    { table_t::ecdsa_body, "ecdsa_body" },
    { table_t::schnorr_table, "schnorr_table" },
    { table_t::schnorr_head, "schnorr_head" },
    { table_t::schnorr_body, "schnorr_body" },
    { table_t::silent_table, "silent_table" },
    { table_t::silent_head, "silent_head" },
    { table_t::silent_body, "silent_body" },
    { table_t::duplicate_table, "duplicate_table" },
    { table_t::duplicate_head, "duplicate_head" },
    { table_t::duplicate_body, "duplicate_body" },
    { table_t::prevalid_table, "prevalid_table" },
    { table_t::prevalid_head, "prevalid_head" },
    { table_t::prevalid_body, "prevalid_body" },
    { table_t::prevout_table, "prevout_table" },
    { table_t::prevout_head, "prevout_head" },
    { table_t::prevout_body, "prevout_body" },
    { table_t::validated_bk_table, "validated_bk_table" },
    { table_t::validated_bk_head, "validated_bk_head" },
    { table_t::validated_bk_body, "validated_bk_body" },
    { table_t::validated_tx_table, "validated_tx_table" },
    { table_t::validated_tx_head, "validated_tx_head" },
    { table_t::validated_tx_body, "validated_tx_body" },

    // Optionals.
    { table_t::address_table, "address_table" },
    { table_t::address_head, "address_head" },
    { table_t::address_body, "address_body" },
    { table_t::filter_bk_table, "filter_bk_table" },
    { table_t::filter_bk_head, "filter_bk_head" },
    { table_t::filter_bk_body, "filter_bk_body" },
    { table_t::filter_tx_table, "filter_tx_table" },
    { table_t::filter_tx_head, "filter_tx_head" },
    { table_t::filter_tx_body, "filter_tx_body" }
};

} // namespace database
} // namespace libbitcoin

#endif
