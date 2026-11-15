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

// The address spine is unbacked when its hash head is disabled.
TEMPLATE
std::unique_ptr<storage> CLASS::to_outs(const settings& config) NOEXCEPT
{
    if (is_zero(config.outs.buckets))
        return std::make_unique<table::outs_storage_unbacked<Storage>>(
            body(config.path, schema::archive::outs),
            body_settings(config, config.outs), sequential, staged);

    return std::make_unique<table::outs_storage<Storage>>(
        body(config.path, schema::archive::outs),
        body_settings(config, config.outs), sequential, staged);
}

TEMPLATE
CLASS::store(const settings& config) NOEXCEPT
  : configuration_(config),
    envelope_(config.envelope),

    // Archive.
    // ------------------------------------------------------------------------

    header_head_(head(config.path / schema::dir::heads, schema::archive::header), head_settings(config.header), random),
    header_body_(body(config.path, schema::archive::header), body_settings(config, config.header), sequential, staged),

    input_head_(head(config.path / schema::dir::heads, schema::archive::input), head_settings(config.input), sequential),
    input_body_(body(config.path, schema::archive::input), body_settings(config, config.input), sequential, staged),

    output_head_(head(config.path / schema::dir::heads, schema::archive::output), head_settings(config.output), sequential),
    output_body_(body(config.path, schema::archive::output), body_settings(config, config.output), sequential, staged),

    ins_head_(head(config.path / schema::dir::heads, schema::archive::ins), head_settings(config.ins), random),
    ins_body_(body(config.path, schema::archive::ins), body_settings(config, config.ins), sequential, staged),

    outs_head_(head(config.path / schema::dir::heads, schema::archive::outs), head_settings(config.outs), random),
    outs_body_(to_outs(config)),

    tx_head_(head(config.path / schema::dir::heads, schema::archive::tx), head_settings(config.tx), random),
    tx_body_(body(config.path, schema::archive::tx), body_settings(config, config.tx), sequential, staged),

    txs_head_(head(config.path / schema::dir::heads, schema::archive::txs), head_settings(config.txs), random),
    txs_body_(body(config.path, schema::archive::txs), body_settings(config, config.txs), sequential, staged),

    // Indexes.
    // ------------------------------------------------------------------------

    candidate_head_(head(config.path / schema::dir::heads, schema::indexes::candidate), head_settings<schema::height::cell>(config.candidate), sequential),
    confirmed_head_(head(config.path / schema::dir::heads, schema::indexes::confirmed), head_settings<schema::height::cell>(config.confirmed), sequential),

    strong_head_(head(config.path / schema::dir::heads, schema::indexes::strong), head_settings<schema::strong::cell>(config.strong, advice::sequential, zero), sequential, !staged, shared),
    scan_head_(head(config.path / schema::dir::heads, schema::indexes::scan), head_settings(config.scan), random),
    scan_body_(body(config.path, schema::indexes::scan), body_settings(config, config.scan), sequential, staged),
    scan_prefix_head_(head(config.path / schema::dir::heads, schema::indexes::scan_prefix), head_settings(config.scan), sequential),
    scan_prefix_body_(body(config.path, schema::indexes::scan_prefix), body_settings(config, config.scan), sequential, staged),

    // Caches.
    // ------------------------------------------------------------------------

    // TODO: body not random, but keep in memory.
    ecdsa0_head_(head(config.path / schema::dir::heads, schema::caches::ecdsa0), head_settings(config.ecdsa), sequential),
    ecdsa0_body_(body(config.path, schema::caches::ecdsa0), body_settings(config, config.ecdsa), sequential, staged),
    ecdsa1_head_(head(config.path / schema::dir::heads, schema::caches::ecdsa1), head_settings(config.ecdsa), sequential),
    ecdsa1_body_(body(config.path, schema::caches::ecdsa1), body_settings(config, config.ecdsa), sequential, staged),

    // TODO: body not random, but keep in memory.
    schnorr0_head_(head(config.path / schema::dir::heads, schema::caches::schnorr0), head_settings(config.schnorr), sequential),
    schnorr0_body_(body(config.path, schema::caches::schnorr0), body_settings(config, config.schnorr), sequential, staged),
    schnorr1_head_(head(config.path / schema::dir::heads, schema::caches::schnorr1), head_settings(config.schnorr), sequential),
    schnorr1_body_(body(config.path, schema::caches::schnorr1), body_settings(config, config.schnorr), sequential, staged),

    silent0_head_(head(config.path / schema::dir::heads, schema::caches::silent0), head_settings(config.silent), sequential),
    silent0_body_(body(config.path, schema::caches::silent0), body_settings(config, config.silent), sequential, staged),
    silent1_head_(head(config.path / schema::dir::heads, schema::caches::silent1), head_settings(config.silent), sequential),
    silent1_body_(body(config.path, schema::caches::silent1), body_settings(config, config.silent), sequential, staged),
    silent0_prefix_head_(head(config.path / schema::dir::heads, schema::caches::silent0_prefix), head_settings(config.silent), sequential),
    silent0_prefix_body_(body(config.path, schema::caches::silent0_prefix), body_settings(config, config.silent), sequential, staged),
    silent1_prefix_head_(head(config.path / schema::dir::heads, schema::caches::silent1_prefix), head_settings(config.silent), sequential),
    silent1_prefix_body_(body(config.path, schema::caches::silent1_prefix), body_settings(config, config.silent), sequential, staged),

    envelope_head_(head(config.path / schema::dir::heads, schema::caches::envelope), head_settings(config.duplicate), sequential, !staged, shared),
    duplicate_head_(head(config.path / schema::dir::heads, schema::indexes::duplicate), head_settings(config.duplicate), random),
    duplicate_body_(body(config.path, schema::indexes::duplicate), body_settings(config, config.duplicate), sequential, staged),

    prevalid0_head_(head(config.path / schema::dir::heads, schema::caches::prevalid0), head_settings(config.prevalid), sequential),
    prevalid0_body_(body(config.path, schema::caches::prevalid0), body_settings(config, config.prevalid), sequential, staged),
    prevalid1_head_(head(config.path / schema::dir::heads, schema::caches::prevalid1), head_settings(config.prevalid), sequential),
    prevalid1_body_(body(config.path, schema::caches::prevalid1), body_settings(config, config.prevalid), sequential, staged),
    presilent0_head_(head(config.path / schema::dir::heads, schema::caches::presilent0), head_settings(config.presilent), sequential),
    presilent0_body_(body(config.path, schema::caches::presilent0), body_settings(config, config.presilent), sequential, staged),
    presilent1_head_(head(config.path / schema::dir::heads, schema::caches::presilent1), head_settings(config.presilent), sequential),
    presilent1_body_(body(config.path, schema::caches::presilent1), body_settings(config, config.presilent), sequential, staged),

    prevout_head_(head(config.path / schema::dir::heads, schema::caches::prevout), head_settings(config.prevout), random),
    prevout_body_(body(config.path, schema::caches::prevout), body_settings(config, config.prevout), sequential, staged),

    state_head_(head(config.path / schema::dir::heads, schema::indexes::state), head_settings<schema::state::cell>(config.state, advice::random, zero), random),

    pool_head_(head(config.path / schema::dir::heads, schema::caches::pool), head_settings(config.pool), random),
    pool_body_(body(config.path, schema::caches::pool), body_settings(config, config.pool), sequential, staged),
    spends_head_(head(config.path / schema::dir::heads, schema::caches::spends), head_settings(config.spends), sequential),
    spends_body_(body(config.path, schema::caches::spends), body_settings(config, config.spends), sequential, staged),
    pool_cells_head_(head(config.path / schema::dir::heads, schema::caches::pool_cells), head_settings(config.pool, one, advice::sequential), sequential, !staged, shared),
    pool_children_head_(head(config.path / schema::dir::heads, schema::caches::pool_children), head_settings(config.pool, one, advice::sequential), sequential, !staged, shared),
    spends_rows_head_(head(config.path / schema::dir::heads, schema::caches::spends_rows), head_settings(config.spends, one, advice::sequential), sequential, !staged, shared),
    spends_siblings_head_(head(config.path / schema::dir::heads, schema::caches::spends_siblings), head_settings(config.spends, one, advice::sequential), sequential, !staged, shared),
    spends_rivals_head_(head(config.path / schema::dir::heads, schema::caches::spends_rivals), head_settings(config.spends, one, advice::sequential), sequential, !staged, shared),
    wtxid_head_(head(config.path / schema::dir::heads, schema::indexes::wtxid), head_settings(config.wtxid), random),
    wtxid_body_(body(config.path, schema::indexes::wtxid), body_settings(config, config.wtxid), sequential, staged),

    // Optionals.
    // ------------------------------------------------------------------------

    filter_bk_head_(head(config.path / schema::dir::heads, schema::optionals::filter_bk), head_settings(config.filter_bk), random),
    filter_bk_body_(body(config.path, schema::optionals::filter_bk), body_settings(config, config.filter_bk), sequential, staged),

    filter_tx_head_(head(config.path / schema::dir::heads, schema::optionals::filter_tx), head_settings(config.filter_tx), random),
    filter_tx_body_(body(config.path, schema::optionals::filter_tx), body_settings(config, config.filter_tx), sequential, staged),

    // Locks.
    // ------------------------------------------------------------------------

    flush_lock_(lock(config.path, schema::locks::flush)),
    process_lock_(lock(config.path, schema::locks::process)),

    // Tables.
    // ------------------------------------------------------------------------

    header(header_head_, header_body_, config.header.buckets, config.header.expected_()),
    input(input_head_, input_body_),
    output(output_head_, output_body_),
    ins(ins_head_, ins_body_, config.ins.buckets, config.ins.expected_()),
    outs(outs_head_, *outs_body_, config.outs.buckets, config.outs.expected_()),
    tx(tx_head_, tx_body_, config.tx.buckets, config.tx.expected_()),
    txs(txs_head_, txs_body_, config.txs.buckets),

    candidate(candidate_head_),
    confirmed(confirmed_head_),
    strong(strong_head_, config.strong.buckets),
    scan(scan_head_, scan_body_, config.scan.buckets),
    scan_prefix(scan_prefix_head_, scan_prefix_body_),

    ecdsa0(ecdsa0_head_, ecdsa0_body_),
    ecdsa1(ecdsa1_head_, ecdsa1_body_),
    schnorr0(schnorr0_head_, schnorr0_body_),
    schnorr1(schnorr1_head_, schnorr1_body_),
    silent0(silent0_head_, silent0_body_),
    silent1(silent1_head_, silent1_body_),
    silent0_prefix(silent0_prefix_head_, silent0_prefix_body_),
    silent1_prefix(silent1_prefix_head_, silent1_prefix_body_),
    envelope(envelope_head_),
    duplicate(duplicate_head_, duplicate_body_, config.duplicate.buckets, config.duplicate.expected_()),
    prevalid0(prevalid0_head_, prevalid0_body_),
    prevalid1(prevalid1_head_, prevalid1_body_),
    presilent0(presilent0_head_, presilent0_body_),
    presilent1(presilent1_head_, presilent1_body_),
    prevout(prevout_head_, prevout_body_, config.prevout.buckets),
    state(state_head_, config.state.buckets),
    pool(pool_head_, pool_body_, config.pool.buckets, config.pool.expected_()),
    spends(spends_head_, spends_body_),
    pool_cells(pool_cells_head_),
    pool_children(pool_children_head_),
    spends_rows(spends_rows_head_),
    spends_siblings(spends_siblings_head_),
    spends_rivals(spends_rivals_head_),
    wtxid(wtxid_head_, wtxid_body_, config.wtxid.buckets, config.wtxid.expected_()),

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
    envelope_.duplicate_k = possible_narrow_cast<uint8_t>(duplicate.filter_k());
    envelope_.pool_k = possible_narrow_cast<uint8_t>(pool.filter_k());
    envelope_.wtxid_k = possible_narrow_cast<uint8_t>(wtxid.filter_k());
}

TEMPLATE
bool CLASS::turbo() const NOEXCEPT
{
    return configuration_.turbo;
}

TEMPLATE
size_t CLASS::cold_reads() const NOEXCEPT
{
    return configuration_.cold_reads;
}

TEMPLATE
size_t CLASS::gpu_scan_minimum() const NOEXCEPT
{
    return configuration_.gpu_scan_minimum;
}

TEMPLATE
bool CLASS::mark_unconfirmable() const NOEXCEPT
{
    return configuration_.mark_unconfirmable;
}

TEMPLATE
uint8_t CLASS::interval_depth() const NOEXCEPT
{
    return envelope_.interval_depth;
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
size_t CLASS::get_scan_frontier() const NOEXCEPT
{
    return scan_frontier_.load(std::memory_order_relaxed);
}

TEMPLATE
size_t CLASS::get_latch_tx() const NOEXCEPT
{
    return latch_tx_.load(std::memory_order_relaxed);
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

    envelope_.pooling = true;
    envelope_.scan_frontier = scan.count().value;
    envelope_.latch_tx = tx.count().value;
    scan_frontier_.store(envelope_.scan_frontier, std::memory_order_relaxed);
    latch_tx_.store(envelope_.latch_tx, std::memory_order_relaxed);
    pooling_.store(true, std::memory_order_relaxed);

    using integer = table::envelope::link::integer;
    const auto position = envelope_.pooling_offset();
    const auto offset = system::possible_narrow_cast<integer>(position);
    const table::envelope::put_pooling record
    {
        envelope_.pooling,
        envelope_.scan_frontier,
        envelope_.latch_tx
    };

    /* bool */ envelope.put(offset, record);
    set_dirty();
}

TEMPLATE
database::envelope::activation CLASS::get_activation(
    size_t index) const NOEXCEPT
{
    const auto& flags = activation_flags_.at(index);
    const auto& link = activation_links_.at(index);
    const auto value = flags.load(std::memory_order_acquire);
    return { value, link.load(std::memory_order_relaxed) };
}

TEMPLATE
bool CLASS::set_activation(size_t index, uint32_t flags,
    const header_link& link) NOEXCEPT
{
    envelope_.activations.at(index) = { flags, link.value };
    activation_links_.at(index).store(link.value, std::memory_order_relaxed);
    activation_flags_.at(index).store(flags, std::memory_order_release);

    using integer = table::envelope::link::integer;
    const auto position = envelope_.activation_offset(index);
    const auto offset = system::possible_narrow_cast<integer>(position);
    const table::envelope::put_activation record{ flags, link.value };
    return envelope.put(offset, record);
}

TEMPLATE
typename CLASS::transactor CLASS::get_transactor() NOEXCEPT
{
    return transactor{ transactor_mutex_ };
}

TEMPLATE
table::ecdsa<Storage>& CLASS::ecdsa_bank(bool bank) NOEXCEPT
{
    return bank ? ecdsa1 : ecdsa0;
}

TEMPLATE
table::silent<Storage>& CLASS::silent_bank(bool bank) NOEXCEPT
{
    return bank ? silent1 : silent0;
}

TEMPLATE
table::silent_prefix& CLASS::silent_prefix_bank(bool bank) NOEXCEPT
{
    return bank ? silent1_prefix : silent0_prefix;
}

TEMPLATE
table::schnorr<Storage>& CLASS::schnorr_bank(bool bank) NOEXCEPT
{
    return bank ? schnorr1 : schnorr0;
}

TEMPLATE
table::prevalid& CLASS::prevalid_bank(bool bank) NOEXCEPT
{
    return bank ? prevalid1 : prevalid0;
}

TEMPLATE
table::presilent& CLASS::presilent_bank(bool bank) NOEXCEPT
{
    return bank ? presilent1 : presilent0;
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
    header_body_.current(current);
    input_head_.current(current);
    input_body_.current(current);
    output_head_.current(current);
    output_body_.current(current);
    ins_head_.current(current);
    ins_body_.current(current);
    outs_head_.current(current);
    outs_body_->current(current);
    tx_head_.current(current);
    tx_body_.current(current);
    txs_head_.current(current);
    txs_body_.current(current);
    candidate_head_.current(current);
    confirmed_head_.current(current);
    strong_head_.current(current);
    scan_head_.current(current);
    scan_body_.current(current);
    scan_prefix_head_.current(current);
    scan_prefix_body_.current(current);
    ecdsa0_head_.current(current);
    ecdsa0_body_.current(current);
    ecdsa1_head_.current(current);
    ecdsa1_body_.current(current);
    schnorr0_head_.current(current);
    schnorr0_body_.current(current);
    schnorr1_head_.current(current);
    schnorr1_body_.current(current);
    silent0_head_.current(current);
    silent0_body_.current(current);
    silent1_head_.current(current);
    silent1_body_.current(current);
    silent0_prefix_head_.current(current);
    silent0_prefix_body_.current(current);
    silent1_prefix_head_.current(current);
    silent1_prefix_body_.current(current);
    envelope_head_.current(current);
    duplicate_head_.current(current);
    duplicate_body_.current(current);
    prevalid0_head_.current(current);
    prevalid0_body_.current(current);
    prevalid1_head_.current(current);
    prevalid1_body_.current(current);
    presilent0_head_.current(current);
    presilent0_body_.current(current);
    presilent1_head_.current(current);
    presilent1_body_.current(current);
    prevout_head_.current(current);
    prevout_body_.current(current);
    state_head_.current(current);
    pool_head_.current(current);
    pool_body_.current(current);
    spends_head_.current(current);
    spends_body_.current(current);
    pool_cells_head_.current(current);
    pool_children_head_.current(current);
    spends_rows_head_.current(current);
    spends_siblings_head_.current(current);
    spends_rivals_head_.current(current);
    wtxid_head_.current(current);
    wtxid_body_.current(current);
    filter_bk_head_.current(current);
    filter_bk_body_.current(current);
    filter_tx_head_.current(current);
    filter_tx_body_.current(current);
}

TEMPLATE
void CLASS::evaluate_currency() NOEXCEPT
{
    set_current(is_current());
}

} // namespace database
} // namespace libbitcoin

#endif
