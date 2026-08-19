/**
 * Copyright (c) 2011-2026 libbitcoin developers
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

    header_head_(head(config.path / schema::dir::heads, schema::archive::header), head_settings(config.header), random),
    header_body_(body(config.path, schema::archive::header), config.header, sequential, staged),

    input_head_(head(config.path / schema::dir::heads, schema::archive::input), head_settings(config.input), sequential),
    input_body_(body(config.path, schema::archive::input), config.input, sequential, staged),

    output_head_(head(config.path / schema::dir::heads, schema::archive::output), head_settings(config.output), sequential),
    output_body_(body(config.path, schema::archive::output), config.output, sequential, staged),

    ins_head_(head(config.path / schema::dir::heads, schema::archive::ins), head_settings(config.ins), random),
    ins_body_(body(config.path, schema::archive::ins), config.ins, sequential, staged),

    outs_head_(head(config.path / schema::dir::heads, schema::archive::outs), head_settings(config.outs), random),
    outs_body_(body(config.path, schema::archive::outs), config.outs, sequential, staged),

    tx_head_(head(config.path / schema::dir::heads, schema::archive::tx), head_settings(config.tx), random),
    tx_body_(body(config.path, schema::archive::tx), config.tx, sequential, staged),

    txs_head_(head(config.path / schema::dir::heads, schema::archive::txs), head_settings(config.txs), random),
    txs_body_(body(config.path, schema::archive::txs), config.txs, sequential, staged),

    // Indexes.
    // ------------------------------------------------------------------------

    candidate_head_(head(config.path / schema::dir::heads, schema::indexes::candidate), head_settings<schema::height::cell>(config.candidate), sequential),
    confirmed_head_(head(config.path / schema::dir::heads, schema::indexes::confirmed), head_settings<schema::height::cell>(config.confirmed), sequential),

    strong_tx_head_(head(config.path / schema::dir::heads, schema::indexes::strong_tx), head_settings(config.strong_tx), random),
    strong_tx_body_(body(config.path, schema::indexes::strong_tx), config.strong_tx, sequential, staged),

    // Caches.
    // ------------------------------------------------------------------------

    // TODO: body not random, but keep in memory.
    ecdsa_head_(head(config.path / schema::dir::heads, schema::caches::ecdsa), head_settings(config.ecdsa), sequential),
    ecdsa_body_(body(config.path, schema::caches::ecdsa), config.ecdsa, sequential, staged),

    // TODO: body not random, but keep in memory.
    schnorr_head_(head(config.path / schema::dir::heads, schema::caches::schnorr), head_settings(config.schnorr), sequential),
    schnorr_body_(body(config.path, schema::caches::schnorr), config.schnorr, sequential, staged),

    silent_head_(head(config.path / schema::dir::heads, schema::caches::silent), head_settings(config.silent), sequential),
    silent_body_(body(config.path, schema::caches::silent), config.silent, sequential, staged),

    duplicate_head_(head(config.path / schema::dir::heads, schema::caches::duplicate), head_settings(config.duplicate), random),
    duplicate_body_(body(config.path, schema::caches::duplicate), config.duplicate, sequential, staged),

    prevalid_head_(head(config.path / schema::dir::heads, schema::caches::prevalid), head_settings(config.prevalid), sequential),
    prevalid_body_(body(config.path, schema::caches::prevalid), config.prevalid, sequential, staged),

    prevout_head_(head(config.path / schema::dir::heads, schema::caches::prevout), head_settings(config.prevout), random),
    prevout_body_(body(config.path, schema::caches::prevout), config.prevout, sequential, staged),

    validated_bk_head_(head(config.path / schema::dir::heads, schema::caches::validated_bk), head_settings(config.validated_bk), random),
    validated_bk_body_(body(config.path, schema::caches::validated_bk), config.validated_bk, sequential, staged),

    validated_tx_head_(head(config.path / schema::dir::heads, schema::caches::validated_tx), head_settings(config.validated_tx), random),
    validated_tx_body_(body(config.path, schema::caches::validated_tx), config.validated_tx, sequential, staged),

    // Optionals.
    // ------------------------------------------------------------------------

    filter_bk_head_(head(config.path / schema::dir::heads, schema::optionals::filter_bk), head_settings(config.filter_bk), random),
    filter_bk_body_(body(config.path, schema::optionals::filter_bk), config.filter_bk, sequential, staged),

    filter_tx_head_(head(config.path / schema::dir::heads, schema::optionals::filter_tx), head_settings(config.filter_tx), random),
    filter_tx_body_(body(config.path, schema::optionals::filter_tx), config.filter_tx, sequential, staged),

    // Locks.
    // ------------------------------------------------------------------------

    flush_lock_(lock(config.path, schema::locks::flush)),
    process_lock_(lock(config.path, schema::locks::process)),

    // Tables.
    // ------------------------------------------------------------------------

    header(header_head_, header_body_, config.header.buckets),
    input(input_head_, input_body_),
    output(output_head_, output_body_),
    ins(ins_head_, ins_body_, config.ins.buckets),
    outs(outs_head_, outs_body_, config.outs.buckets),
    tx(tx_head_, tx_body_, config.tx.buckets),
    txs(txs_head_, txs_body_, config.txs.buckets),

    candidate(candidate_head_),
    confirmed(confirmed_head_),
    strong_tx(strong_tx_head_, strong_tx_body_, config.strong_tx.buckets),

    ecdsa(ecdsa_head_, ecdsa_body_),
    schnorr(schnorr_head_, schnorr_body_),
    silent(silent_head_, silent_body_),
    duplicate(duplicate_head_, duplicate_body_, config.duplicate.buckets),
    prevalid(prevalid_head_, prevalid_body_),
    prevout(prevout_head_, prevout_body_, config.prevout.buckets),
    validated_bk(validated_bk_head_, validated_bk_body_, config.validated_bk.buckets),
    validated_tx(validated_tx_head_, validated_tx_body_, config.validated_tx.buckets),

    filter_bk(filter_bk_head_, filter_bk_body_, config.filter_bk.buckets),
    filter_tx(filter_tx_head_, filter_tx_body_, config.filter_tx.buckets)
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
uint32_t CLASS::fork_flags() const NOEXCEPT
{
    return configuration_.fork_flags;
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
typename CLASS::transactor CLASS::get_transactor() NOEXCEPT
{
    return transactor{ transactor_mutex_ };
}

} // namespace database
} // namespace libbitcoin

#endif
