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
    auto header_buffer = header_head_.get();
    auto input_buffer = input_head_.get();
    auto output_buffer = output_head_.get();
    auto point_buffer = point_head_.get();
    auto ins_buffer = ins_head_.get();
    auto outs_buffer = outs_head_.get();
    auto tx_buffer = tx_head_.get();
    auto txs_buffer = txs_head_.get();

    auto candidate_buffer = candidate_head_.get();
    auto confirmed_buffer = confirmed_head_.get();
    auto strong_tx_buffer = strong_tx_head_.get();

    auto ecdsa_buffer = ecdsa_head_.get();
    auto schnorr_buffer = schnorr_head_.get();
    auto silent_buffer = silent_head_.get();
    auto duplicate_buffer = duplicate_head_.get();
    auto prevout_buffer = prevout_head_.get();
    auto validated_bk_buffer = validated_bk_head_.get();
    auto validated_tx_buffer = validated_tx_head_.get();

    auto address_buffer = address_head_.get();
    auto filter_bk_buffer = filter_bk_head_.get();
    auto filter_tx_buffer = filter_tx_head_.get();

    if (!header_buffer) return error::unloaded_file;
    if (!input_buffer) return error::unloaded_file;
    if (!output_buffer) return error::unloaded_file;
    if (!point_buffer) return error::unloaded_file;
    if (!ins_buffer) return error::unloaded_file;
    if (!outs_buffer) return error::unloaded_file;
    if (!tx_buffer) return error::unloaded_file;
    if (!txs_buffer) return error::unloaded_file;

    if (!candidate_buffer) return error::unloaded_file;
    if (!confirmed_buffer) return error::unloaded_file;
    if (!strong_tx_buffer) return error::unloaded_file;

    if (!ecdsa_buffer) return error::unloaded_file;
    if (!schnorr_buffer) return error::unloaded_file;
    if (!silent_buffer) return error::unloaded_file;
    if (!duplicate_buffer) return error::unloaded_file;
    if (!prevout_buffer) return error::unloaded_file;
    if (!validated_bk_buffer) return error::unloaded_file;
    if (!validated_tx_buffer) return error::unloaded_file;

    if (!address_buffer) return error::unloaded_file;
    if (!filter_bk_buffer) return error::unloaded_file;
    if (!filter_tx_buffer) return error::unloaded_file;

    code ec{ error::success };
    const auto dump = [&handler, &folder](code& ec, const auto& storage,
        const auto& name, table_t table) NOEXCEPT
    {
        if (!ec)
        {
            handler(event_t::copy_header, table);
            ec = file::create_file_ex(head(folder, name), storage->begin(),
                storage->size());
        }
    };

    dump(ec, header_buffer, schema::archive::header, table_t::header_head);
    dump(ec, input_buffer, schema::archive::input, table_t::input_head);
    dump(ec, output_buffer, schema::archive::output, table_t::output_head);
    dump(ec, point_buffer, schema::archive::point, table_t::point_head);
    dump(ec, ins_buffer, schema::archive::ins, table_t::ins_head);
    dump(ec, outs_buffer, schema::archive::outs, table_t::outs_head);
    dump(ec, tx_buffer, schema::archive::tx, table_t::tx_head);
    dump(ec, txs_buffer, schema::archive::txs, table_t::txs_head);

    dump(ec, candidate_buffer, schema::indexes::candidate, table_t::candidate_head);
    dump(ec, confirmed_buffer, schema::indexes::confirmed, table_t::confirmed_head);
    dump(ec, strong_tx_buffer, schema::indexes::strong_tx, table_t::strong_tx_head);

    dump(ec, ecdsa_buffer, schema::caches::ecdsa, table_t::ecdsa_head);
    dump(ec, schnorr_buffer, schema::caches::schnorr, table_t::schnorr_head);
    dump(ec, silent_buffer, schema::caches::silent, table_t::silent_head);
    dump(ec, duplicate_buffer, schema::caches::duplicate, table_t::duplicate_head);
    dump(ec, prevout_buffer, schema::caches::prevout, table_t::prevout_head);
    dump(ec, validated_bk_buffer, schema::caches::validated_bk, table_t::validated_bk_head);
    dump(ec, validated_tx_buffer, schema::caches::validated_tx, table_t::validated_tx_head);

    dump(ec, address_buffer, schema::optionals::address, table_t::address_head);
    dump(ec, filter_bk_buffer, schema::optionals::filter_bk, table_t::filter_bk_head);
    dump(ec, filter_tx_buffer, schema::optionals::filter_tx, table_t::filter_tx_head);

    return ec;
}

} // namespace database
} // namespace libbitcoin

#endif
