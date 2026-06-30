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
#ifndef LIBBITCOIN_DATABASE_STORE_DUMP_IPP
#define LIBBITCOIN_DATABASE_STORE_DUMP_IPP

#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

// public
// Dump memory maps of /heads to new files in /temporary.
// Heads are copied from RAM, not flushed to disk and copied as files.
TEMPLATE
code CLASS::dump(const path& folder,
    const event_handler& handler) NOEXCEPT
{
    code ec{ error::success };
    const auto dump = [&handler, &folder](code& ec, const auto& file,
        const auto& name, table_t table) NOEXCEPT
    {
        if (!ec)
        {
            handler(event_t::copy_header, table);
            ec = file.dump(head(folder, name));
        }
    };

    dump(ec, header_head_, schema::archive::header, table_t::header_head);
    dump(ec, input_head_, schema::archive::input, table_t::input_head);
    dump(ec, output_head_, schema::archive::output, table_t::output_head);
    dump(ec, point_head_, schema::archive::point, table_t::point_head);
    dump(ec, ins_head_, schema::archive::ins, table_t::ins_head);
    dump(ec, outs_head_, schema::archive::outs, table_t::outs_head);
    dump(ec, tx_head_, schema::archive::tx, table_t::tx_head);
    dump(ec, txs_head_, schema::archive::txs, table_t::txs_head);

    dump(ec, candidate_head_, schema::indexes::candidate, table_t::candidate_head);
    dump(ec, confirmed_head_, schema::indexes::confirmed, table_t::confirmed_head);
    dump(ec, strong_tx_head_, schema::indexes::strong_tx, table_t::strong_tx_head);

    dump(ec, ecdsa_head_, schema::caches::ecdsa, table_t::ecdsa_head);
    dump(ec, schnorr_head_, schema::caches::schnorr, table_t::schnorr_head);
    dump(ec, silent_head_, schema::caches::silent, table_t::silent_head);
    dump(ec, duplicate_head_, schema::caches::duplicate, table_t::duplicate_head);
    dump(ec, prevalid_head_, schema::caches::prevalid, table_t::prevalid_head);
    dump(ec, prevout_head_, schema::caches::prevout, table_t::prevout_head);
    dump(ec, validated_bk_head_, schema::caches::validated_bk, table_t::validated_bk_head);
    dump(ec, validated_tx_head_, schema::caches::validated_tx, table_t::validated_tx_head);

    dump(ec, address_head_, schema::optionals::address, table_t::address_head);
    dump(ec, filter_bk_head_, schema::optionals::filter_bk, table_t::filter_bk_head);
    dump(ec, filter_tx_head_, schema::optionals::filter_tx, table_t::filter_tx_head);

    return ec;
}

} // namespace database
} // namespace libbitcoin

#endif
