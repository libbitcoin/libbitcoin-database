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

    silent_head_(head(config.path / schema::dir::heads, schema::caches::silent), 1, 0, random),
    silent_body_(body(config.path, schema::caches::silent), config.silent_size, config.silent_rate, sequential),

    duplicate_head_(head(config.path / schema::dir::heads, schema::caches::duplicate), 1, 0, random),
    duplicate_body_(body(config.path, schema::caches::duplicate), config.duplicate_size, config.duplicate_rate, sequential),

    prevalid_head_(head(config.path / schema::dir::heads, schema::caches::prevalid), 1, 0, random),
    prevalid_body_(body(config.path, schema::caches::prevalid), config.prevalid_size, config.prevalid_rate, sequential),

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
    silent(silent_head_, silent_body_),
    duplicate(duplicate_head_, duplicate_body_, config.duplicate_buckets),
    prevalid(prevalid_head_, prevalid_body_),
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
bool CLASS::mark_unconfirmable() const NOEXCEPT
{
    return configuration_.mark_unconfirmable;
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
