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
#ifndef LIBBITCOIN_DATABASE_STORE_IPP
#define LIBBITCOIN_DATABASE_STORE_IPP

#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

TEMPLATE
const CLASS::event_map CLASS::events
{
    { event_t::create_file, "create_file" },
    { event_t::open_file,   "open_file" },
    { event_t::load_file, "load_file" },
    { event_t::unload_file, "unload_file" },
    { event_t::close_file, "close_file" },
    { event_t::create_table, "create_table" },
    { event_t::verify_table, "verify_table" },
    { event_t::close_table, "close_table" },

    { event_t::wait_lock, "wait_lock" },
    { event_t::flush_body, "flush_body" },
    { event_t::prune_table, "prune_table" },
    { event_t::backup_table, "backup_table" },
    { event_t::copy_header, "copy_header" },
    { event_t::archive_snapshot, "archive_snapshot" },

    { event_t::restore_table, "restore_table" },
    { event_t::recover_snapshot, "recover_snapshot" }
};

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
    { table_t::duplicate_table, "duplicate_table" },
    { table_t::duplicate_head, "duplicate_head" },
    { table_t::duplicate_body, "duplicate_body" },
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

TEMPLATE
CLASS::store(const settings& config) NOEXCEPT
  : configuration_(config),

    // Archive.
    // ------------------------------------------------------------------------

    header_head_(head(config.path / schema::dir::heads, schema::archive::header), 1, 0, random),
    header_body_(body(config.path, schema::archive::header), config.header_size, config.header_rate, sequential),

    input_head_(head(config.path / schema::dir::heads, schema::archive::input), 1, 0, random),
    input_body_(body(config.path, schema::archive::input), config.input_size, config.input_rate, sequential),

    output_head_(head(config.path / schema::dir::heads, schema::archive::output), 1, 0, random),
    output_body_(body(config.path, schema::archive::output), config.output_size, config.output_rate, sequential),

    point_head_(head(config.path / schema::dir::heads, schema::archive::point), 1, 0, random),
    point_body_(body(config.path, schema::archive::point), config.point_size, config.point_rate, sequential),

    ins_head_(head(config.path / schema::dir::heads, schema::archive::ins), 1, 0, random),
    ins_body_(body(config.path, schema::archive::ins), config.ins_size, config.ins_rate, sequential),

    outs_head_(head(config.path / schema::dir::heads, schema::archive::outs), 1, 0, random),
    outs_body_(body(config.path, schema::archive::outs), config.outs_size, config.outs_rate, sequential),

    tx_head_(head(config.path / schema::dir::heads, schema::archive::tx), 1, 0, random),
    tx_body_(body(config.path, schema::archive::tx), config.tx_size, config.tx_rate, sequential),

    txs_head_(head(config.path / schema::dir::heads, schema::archive::txs), 1, 0, random),
    txs_body_(body(config.path, schema::archive::txs), config.txs_size, config.txs_rate, sequential),

    // Indexes.
    // ------------------------------------------------------------------------

    candidate_head_(head(config.path / schema::dir::heads, schema::indexes::candidate), 1, 0, random),
    candidate_body_(body(config.path, schema::indexes::candidate), config.candidate_size, config.candidate_rate, sequential),

    confirmed_head_(head(config.path / schema::dir::heads, schema::indexes::confirmed), 1, 0, random),
    confirmed_body_(body(config.path, schema::indexes::confirmed), config.confirmed_size, config.confirmed_rate, sequential),

    strong_tx_head_(head(config.path / schema::dir::heads, schema::indexes::strong_tx), 1, 0, random),
    strong_tx_body_(body(config.path, schema::indexes::strong_tx), config.strong_tx_size, config.strong_tx_rate, sequential),

    // Caches.
    // ------------------------------------------------------------------------

    ecdsa_head_(head(config.path / schema::dir::heads, schema::caches::ecdsa), 1, 0, random),
    ecdsa_body_(body(config.path, schema::caches::ecdsa), config.ecdsa_size, config.ecdsa_rate, sequential),

    schnorr_head_(head(config.path / schema::dir::heads, schema::caches::schnorr), 1, 0, random),
    schnorr_body_(body(config.path, schema::caches::schnorr), config.schnorr_size, config.schnorr_rate, sequential),

    duplicate_head_(head(config.path / schema::dir::heads, schema::caches::duplicate), 1, 0, random),
    duplicate_body_(body(config.path, schema::caches::duplicate), config.duplicate_size, config.duplicate_rate, sequential),

    prevout_head_(head(config.path / schema::dir::heads, schema::caches::prevout), 1, 0, random),
    prevout_body_(body(config.path, schema::caches::prevout), config.prevout_size, config.prevout_rate, sequential),

    validated_bk_head_(head(config.path / schema::dir::heads, schema::caches::validated_bk), 1, 0, random),
    validated_bk_body_(body(config.path, schema::caches::validated_bk), config.validated_bk_size, config.validated_bk_rate, sequential),

    validated_tx_head_(head(config.path / schema::dir::heads, schema::caches::validated_tx), 1, 0, random),
    validated_tx_body_(body(config.path, schema::caches::validated_tx), config.validated_tx_size, config.validated_tx_rate, sequential),

    // Optionals.
    // ------------------------------------------------------------------------

    address_head_(head(config.path / schema::dir::heads, schema::optionals::address), 1, 0, random),
    address_body_(body(config.path, schema::optionals::address), config.address_size, config.address_rate, sequential),

    filter_bk_head_(head(config.path / schema::dir::heads, schema::optionals::filter_bk), 1, 0, random),
    filter_bk_body_(body(config.path, schema::optionals::filter_bk), config.filter_bk_size, config.filter_bk_rate, sequential),

    filter_tx_head_(head(config.path / schema::dir::heads, schema::optionals::filter_tx), 1, 0, random),
    filter_tx_body_(body(config.path, schema::optionals::filter_tx), config.filter_tx_size, config.filter_tx_rate, sequential),

    // Locks.
    // ------------------------------------------------------------------------

    flush_lock_(lock(config.path, schema::locks::flush)),
    process_lock_(lock(config.path, schema::locks::process)),

    // Tables.
    // ------------------------------------------------------------------------

    header(header_head_, header_body_, config.header_buckets),
    input(input_head_, input_body_),
    output(output_head_, output_body_),
    point(point_head_, point_body_, config.point_buckets),
    ins(ins_head_, ins_body_),
    outs(outs_head_, outs_body_),
    tx(tx_head_, tx_body_, config.tx_buckets),
    txs(txs_head_, txs_body_, config.txs_buckets),

    candidate(candidate_head_, candidate_body_),
    confirmed(confirmed_head_, confirmed_body_),
    strong_tx(strong_tx_head_, strong_tx_body_, config.strong_tx_buckets),

    ecdsa(ecdsa_head_, ecdsa_body_),
    schnorr(schnorr_head_, schnorr_body_),
    duplicate(duplicate_head_, duplicate_body_, config.duplicate_buckets),
    prevout(prevout_head_, prevout_body_, config.prevout_buckets),
    validated_bk(validated_bk_head_, validated_bk_body_, config.validated_bk_buckets),
    validated_tx(validated_tx_head_, validated_tx_body_, config.validated_tx_buckets),

    address(address_head_, address_body_, config.address_buckets),
    filter_bk(filter_bk_head_, filter_bk_body_, config.filter_bk_buckets),
    filter_tx(filter_tx_head_, filter_tx_body_, config.filter_tx_buckets)
{
}

TEMPLATE
bool CLASS::turbo() const NOEXCEPT
{
    return configuration_.turbo;
}

TEMPLATE
uint8_t CLASS::interval_depth() const NOEXCEPT
{
    // Configuration uses uint16_t because of boost parser bug for single byte.
    // But 2^255 is sufficient given that interval is limited by chain length.
    return system::limit<uint8_t>(configuration_.interval_depth);
}

TEMPLATE
bool CLASS::is_dirty() const NOEXCEPT
{
    return dirty_.load(std::memory_order_relaxed);
}

TEMPLATE
void CLASS::set_dirty() NOEXCEPT
{
    return dirty_.store(true, std::memory_order_relaxed);
}

TEMPLATE
const typename CLASS::transactor CLASS::get_transactor() NOEXCEPT
{
    return transactor{ transactor_mutex_ };
}

} // namespace database
} // namespace libbitcoin

#endif
