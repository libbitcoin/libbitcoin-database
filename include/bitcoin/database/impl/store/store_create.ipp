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
#ifndef LIBBITCOIN_DATABASE_STORE_CREATE_IPP
#define LIBBITCOIN_DATABASE_STORE_CREATE_IPP

#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

// public
TEMPLATE
code CLASS::create(const event_handler& handler) NOEXCEPT
{
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

    const auto create = [&handler](code& ec, const auto& file,
        table_t table) NOEXCEPT
    {
        if (!ec)
        {
            handler(event_t::create_file, table);
            ec = file.create();
        }
    };

    static const auto heads = configuration_.path / schema::dir::heads;
    auto ec = file::clear_directory_ex(heads);

    create(ec, header_head_, table_t::header_head);
    create(ec, header_body_, table_t::header_body);
    create(ec, input_head_, table_t::input_head);
    create(ec, input_body_, table_t::input_body);
    create(ec, output_head_, table_t::output_head);
    create(ec, output_body_, table_t::output_body);
    create(ec, point_head_, table_t::point_head);
    create(ec, point_body_, table_t::point_body);
    create(ec, ins_head_, table_t::ins_head);
    create(ec, ins_body_, table_t::ins_body);
    create(ec, outs_head_, table_t::outs_head);
    create(ec, outs_body_, table_t::outs_body);
    create(ec, tx_head_, table_t::tx_head);
    create(ec, tx_body_, table_t::tx_body);
    create(ec, txs_head_, table_t::txs_head);
    create(ec, txs_body_, table_t::txs_body);

    create(ec, candidate_head_, table_t::candidate_head);
    create(ec, candidate_body_, table_t::candidate_body);
    create(ec, confirmed_head_, table_t::confirmed_head);
    create(ec, confirmed_body_, table_t::confirmed_body);
    create(ec, strong_tx_head_, table_t::strong_tx_head);
    create(ec, strong_tx_body_, table_t::strong_tx_body);

    create(ec, ecdsa_head_, table_t::ecdsa_head);
    create(ec, ecdsa_body_, table_t::ecdsa_body);
    create(ec, schnorr_head_, table_t::schnorr_head);
    create(ec, schnorr_body_, table_t::schnorr_body);
    create(ec, silent_head_, table_t::silent_head);
    create(ec, silent_body_, table_t::silent_body);
    create(ec, duplicate_head_, table_t::duplicate_head);
    create(ec, duplicate_body_, table_t::duplicate_body);
    create(ec, prevalid_head_, table_t::prevalid_head);
    create(ec, prevalid_body_, table_t::prevalid_body);
    create(ec, prevout_head_, table_t::prevout_head);
    create(ec, prevout_body_, table_t::prevout_body);
    create(ec, validated_bk_head_, table_t::validated_bk_head);
    create(ec, validated_bk_body_, table_t::validated_bk_body);
    create(ec, validated_tx_head_, table_t::validated_tx_head);
    create(ec, validated_tx_body_, table_t::validated_tx_body);

    create(ec, address_head_, table_t::address_head);
    create(ec, address_body_, table_t::address_body);
    create(ec, filter_bk_head_, table_t::filter_bk_head);
    create(ec, filter_bk_body_, table_t::filter_bk_body);
    create(ec, filter_tx_head_, table_t::filter_tx_head);
    create(ec, filter_tx_body_, table_t::filter_tx_body);

    const auto populate = [&handler](code& ec, auto& logical,
        table_t table) NOEXCEPT
    {
        if (!ec)
        {
            handler(event_t::create_table, table);
            if (!logical.create())
                ec = error::create_table;
        }
    };

    if (!ec) ec = open_load(handler);

    // Populate /heads files and truncate body sizes to zero.
    populate(ec, header, table_t::header_table);
    populate(ec, input, table_t::input_table);
    populate(ec, output, table_t::output_table);
    populate(ec, point, table_t::point_table);
    populate(ec, ins, table_t::ins_table);
    populate(ec, outs, table_t::outs_table);
    populate(ec, tx, table_t::tx_table);
    populate(ec, txs, table_t::txs_table);

    populate(ec, candidate, table_t::candidate_table);
    populate(ec, confirmed, table_t::confirmed_table);
    populate(ec, strong_tx, table_t::strong_tx_table);

    populate(ec, ecdsa, table_t::ecdsa_table);
    populate(ec, schnorr, table_t::schnorr_table);
    populate(ec, silent, table_t::silent_table);
    populate(ec, duplicate, table_t::duplicate_table);
    populate(ec, prevalid, table_t::prevalid_table);
    populate(ec, prevout, table_t::prevout_table);
    populate(ec, validated_bk, table_t::validated_bk_table);
    populate(ec, validated_tx, table_t::validated_tx_table);

    populate(ec, address, table_t::address_table);
    populate(ec, filter_bk, table_t::filter_bk_table);
    populate(ec, filter_tx, table_t::filter_tx_table);

    if (ec)
    {
        /* code */ unload_close(handler);

        // unlock errors override ec.
        if (!flush_lock_.try_unlock()) ec = error::flush_unlock;
        if (!process_lock_.try_unlock()) ec = error::process_unlock;
        /* bool */ file::clear_directory(configuration_.path);
        /* bool */ file::remove(configuration_.path);
    }

    // process and flush locks remain open until close().
    transactor_mutex_.unlock();
    return ec;
}

} // namespace database
} // namespace libbitcoin

#endif
