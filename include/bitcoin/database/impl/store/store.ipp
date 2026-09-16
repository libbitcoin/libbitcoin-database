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
    envelope_(config.envelope),

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

    envelope_head_(head(config.path / schema::dir::heads, schema::caches::envelope), head_settings(config.duplicate), sequential),
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

    header(header_head_, header_body_, config.header.buckets, config.header.expected),
    input(input_head_, input_body_),
    output(output_head_, output_body_),
    ins(ins_head_, ins_body_, config.ins.buckets, config.ins.expected),
    outs(outs_head_, outs_body_, config.outs.buckets, config.outs.expected),
    tx(tx_head_, tx_body_, config.tx.buckets, config.tx.expected),
    txs(txs_head_, txs_body_, config.txs.buckets),

    candidate(candidate_head_),
    confirmed(confirmed_head_),
    strong_tx(strong_tx_head_, strong_tx_body_, config.strong_tx.buckets, config.strong_tx.expected),

    ecdsa(ecdsa_head_, ecdsa_body_),
    schnorr(schnorr_head_, schnorr_body_),
    silent(silent_head_, silent_body_),
    envelope(envelope_head_),
    duplicate(duplicate_head_, duplicate_body_, config.duplicate.buckets, config.duplicate.expected),
    prevalid(prevalid_head_, prevalid_body_),
    prevout(prevout_head_, prevout_body_, config.prevout.buckets),
    validated_bk(validated_bk_head_, validated_bk_body_, config.validated_bk.buckets),
    validated_tx(validated_tx_head_, validated_tx_body_, config.validated_tx.buckets, config.validated_tx.expected),

    filter_bk(filter_bk_head_, filter_bk_body_, config.filter_bk.buckets),
    filter_tx(filter_tx_head_, filter_tx_body_, config.filter_tx.buckets)
{
    envelope_.set(config);

    // Filter k derives from configured expected/buckets, envelope records it
    // at create and overrides it at open (stored values govern the store).
    using namespace system;
    envelope_.header_k = possible_narrow_cast<uint8_t>(header.filter_k());
    envelope_.ins_k = possible_narrow_cast<uint8_t>(ins.filter_k());
    envelope_.outs_k = possible_narrow_cast<uint8_t>(outs.filter_k());
    envelope_.tx_k = possible_narrow_cast<uint8_t>(tx.filter_k());
    envelope_.strong_tx_k = possible_narrow_cast<uint8_t>(strong_tx.filter_k());
    envelope_.duplicate_k = possible_narrow_cast<uint8_t>(duplicate.filter_k());
    envelope_.validated_tx_k = possible_narrow_cast<uint8_t>(validated_tx.filter_k());
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
    return system::limit<uint8_t>(envelope_.interval_depth);
}

TEMPLATE
const database::envelope& CLASS::get_envelope() const NOEXCEPT
{
    return envelope_;
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
bool CLASS::is_pooling() const NOEXCEPT
{
    return pooling_.load(std::memory_order_relaxed);
}

TEMPLATE
void CLASS::set_pooling() NOEXCEPT
{
    if (is_pooling())
        return;

    pooling_.store(true, std::memory_order_relaxed);
    envelope_.pooling = true;

    code ec{};
    store_envelope(ec);
    set_dirty();
}

TEMPLATE
typename CLASS::transactor CLASS::get_transactor() NOEXCEPT
{
    return transactor{ transactor_mutex_ };
}

// Current is coalesced (confirmed at candidate top) with a recent top header.
TEMPLATE
bool CLASS::is_current() const NOEXCEPT
{
    const auto count = candidate.count();
    if (is_zero(count) || (count != confirmed.count()))
        return false;

    table::header::get_timestamp top{};
    if (!header.get(candidate.at(sub1(count)), top))
        return false;

    const auto now = system::possible_narrow_sign_cast<uint32_t>(
        std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());

    return (top.timestamp >= now) || ((now - top.timestamp) <= currency_seconds);
}

TEMPLATE
void CLASS::set_current(bool current) NOEXCEPT
{
    if (current_.exchange(current) == current)
        return;

    header_head_.current(current);
    input_head_.current(current);
    output_head_.current(current);
    ins_head_.current(current);
    outs_head_.current(current);
    tx_head_.current(current);
    txs_head_.current(current);
    candidate_head_.current(current);
    confirmed_head_.current(current);
    strong_tx_head_.current(current);
    ecdsa_head_.current(current);
    schnorr_head_.current(current);
    silent_head_.current(current);
    duplicate_head_.current(current);
    prevalid_head_.current(current);
    prevout_head_.current(current);
    validated_bk_head_.current(current);
    validated_tx_head_.current(current);
    filter_bk_head_.current(current);
    filter_tx_head_.current(current);
}

TEMPLATE
void CLASS::evaluate_currency() NOEXCEPT
{
    set_current(is_current());
}

} // namespace database
} // namespace libbitcoin

#endif
