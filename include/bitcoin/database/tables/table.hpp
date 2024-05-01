/**
 * Copyright (c) 2011-2023 libbitcoin developers (see AUTHORS)
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
#ifndef LIBBITCOIN_DATABASE_TABLES_TABLE_HPP
#define LIBBITCOIN_DATABASE_TABLES_TABLE_HPP

namespace libbitcoin {
namespace database {

enum class table_t
{
    store,

    header_table,
    header_head,
    header_body,
    point_table,
    point_head,
    point_body,
    input_table,
    input_head,
    input_body,
    output_table,
    output_head,
    output_body,
    puts_table,
    puts_head,
    puts_body,
    tx_table,
    tx_head,
    txs_table,
    tx_body,
    txs_head,
    txs_body,

    address_table,
    address_head,
    address_body,
    candidate_table,
    candidate_head,
    candidate_body,
    confirmed_table,
    confirmed_head,
    confirmed_body,
    spend_table,
    spend_head,
    spend_body,
    strong_tx_table,
    strong_tx_head,
    strong_tx_body,

    ////bootstrap_table,
    ////bootstrap_head,
    ////bootstrap_body,
    ////buffer_table,
    ////buffer_head,
    ////buffer_body,
    neutrino_table,
    neutrino_head,
    neutrino_body,
    validated_bk_table,
    validated_bk_head,
    validated_bk_body,
    validated_tx_table,
    validated_tx_head,
    validated_tx_body
};

} // namespace database
} // namespace libbitcoin

#endif
