/**
 * Copyright (c) 2011-2025 libbitcoin developers (see AUTHORS)
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
#ifndef LIBBITCOIN_DATABASE_QUERY_CONSENSUS_IPP
#define LIBBITCOIN_DATABASE_QUERY_CONSENSUS_IPP

#include <algorithm>
#include <atomic>
#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/error.hpp>

namespace libbitcoin {
namespace database {

// unspent_duplicates (bip30)
// ----------------------------------------------------------------------------

// private/static
TEMPLATE
inline bool CLASS::contains(const maybe_strongs& pairs,
    const header_link& block) NOEXCEPT
{
    return std::any_of(pairs.begin(), pairs.end(), [&](const auto& pair) NOEXCEPT
    {
        return block == pair.block;
    });
}

// private/static
TEMPLATE
inline tx_links CLASS::strong_only(const maybe_strongs& pairs) NOEXCEPT
{
    tx_links links{};
    for (const auto& pair: pairs)
        if (pair.strong)
            links.push_back(pair.tx);

    // Reduced to the subset of strong duplicate (by hash) tx records.
    return links;
}

// protected
// Required for bip30 processing.
// The top of the strong_tx table will reflect the current state of only one
// block association. This scans the multimap for the first instance of each
// block that is associated by the tx.link and returns that set of block links.
// Return distinct set of txs by link where each is strong by block.
TEMPLATE
tx_links CLASS::get_strong_txs(const tx_link& link) const NOEXCEPT
{
    auto it = store_.strong_tx.it(link);
    if (!it)
        return {};

    // Obtain all first (by block) duplicate (by link) tx records.
    maybe_strongs pairs{};
    do
    {
        table::strong_tx::record strong{};
        if (store_.strong_tx.get(it, strong) &&
            !contains(pairs, strong.header_fk))
        {
            pairs.emplace_back(strong.header_fk, it.self(), strong.positive);
        }

    }
    while (it.advance());
    it.reset();

    return strong_only(pairs);
}

// protected
// Required for bip30 processing.
// Return distinct set of txs by link for hash where each is strong by block.
TEMPLATE
tx_links CLASS::get_strong_txs(const hash_digest& tx_hash) const NOEXCEPT
{
    // TODO: avoid nested iterators. accumulate set of tx_links and iterate set
    // TODO: after releasing the initial iterator.

    // TODO: deadlock.
    auto it = store_.tx.it(tx_hash);
    if (!it)
        return {};

    tx_links links{};
    do
    {
        // TODO: deadlock.
        for (const auto& tx: get_strong_txs(it.self()))
            links.push_back(tx);
    }
    while (it.advance());
    return links;
}

////// protected
////TEMPLATE
////bool CLASS::is_spent_prevout(const point_link& link, index index) const NOEXCEPT
////{
////    table::point::get_stub get{};
////    if (!store_.point.get(link, get))
////        return false;
////
////    // Prevout is spent by any confirmed transaction.
////    return spent(link, index, get.stub, spend_link::terminal) ==
////        error::confirmed_double_spend;
////}

TEMPLATE
bool CLASS::is_spent_coinbase(const tx_link&) const NOEXCEPT
{
    // TODO: identify spends by stub.
    ////// All outputs of the tx are confirmed spent.
    ////const auto point_fk = to_point(get_tx_key(link));
    ////for (index index{}; index < output_count(link); ++index)
    ////    if (!is_spent_prevout(point_fk, index))
    ////        return false;

    return true;
}

// Duplicate tx instances (with the same hash) may result from a write race.
// Duplicate cb tx instances are allowed by consensus. Apart from two bip30
// exceptions, duplicate cb txs are allowed only if previous are fully spent.
TEMPLATE
code CLASS::unspent_duplicates(const header_link& link,
    const context& ctx) const NOEXCEPT
{
    // This is generally going to be disabled.
    if (!ctx.is_enabled(system::chain::flags::bip30_rule))
        return error::success;

    // Get the block's first tx link.
    const auto cb = to_coinbase(link);
    if (cb.is_terminal())
        return error::integrity1;

    // TODO: deadlock risk.
    // Iterate all strong records for each tx link of the same hash.
    // The same link may be the coinbase for more than one block.
    // Distinct links may be the coinbase for independent blocks.
    // Duplicate instances of a tx (including cb) may exist because of a race.
    // Strong records for a link may be reorganized and again organized.
    auto it = store_.tx.it(get_tx_key(cb));
    if (!it)
        return error::integrity2;

    // TODO: avoid nested iterators. accumulate set of tx_links and iterate set
    // TODO: after releasing the initial iterator.
    tx_links coinbases{};
    do
    {
        for (const auto& tx: get_strong_txs(it.self()))
            coinbases.push_back(tx);
    }
    while (it.advance());
    it.reset();
    
    if (prevout_enabled())
    {
        // Current block is not set strong, so zero is expected.
        if (is_zero(coinbases.size()))
            return error::success;
    }
    else
    {
        // Current block is set strong, so self is expected.
        if (is_one(coinbases.size()))
            return error::success;
    
        // Remove self (will be not found if current block is not set_strong).
        const auto self = std::find(coinbases.begin(), coinbases.end(), link);
        if (self == coinbases.end() || coinbases.erase(self) == coinbases.end())
            return error::integrity3;
    }
    
    // bip30: all outputs of all previous duplicate coinbases must be spent.
    // BUGBUG: with !prevout_enabled() this can locate spends in current block.
    // BUGBUG: with multiple previous duplicates there must be same number of
    // BUGBUG: spends of each coinbase output, but this will identify only one.
    for (const auto coinbase: coinbases)
        if (!is_spent_coinbase(coinbase))
            return error::unspent_coinbase_collision;

    return error::success;
}

// spent
// ----------------------------------------------------------------------------
// TODO: reuse to_spenders() and check for confirmed.

// protected
TEMPLATE
error::error_t CLASS::get_conflicts(point_links& points,
    const point_link& self, spend_key&& key) const NOEXCEPT
{
    // Iterate to all matching spend table keys. Since these are stubs of the
    // full tx hash there will be false positives at the rate of the original
    // "foreign point" indexation. This means two sets of conflicts, first the
    // spend table and then the stubs. But this dramatically reduces paging in
    // the double spend search. The spend table load controls for primary but
    // not secondary conflicts. Reducing secondary conflicts requires increase
    // to the stub hash portion of the spend key. Given that the spend key has
    // two parts (stub|index) there is never a secondary conflict produced by
    // the existence of multiple spends of outputs in the same transaction.
    auto it = store_.spend.it(std::move(key));
    if (!it)
        return self.is_terminal() ? error::success : error::integrity4;

    do
    {
        table::spend::record get{};
        if (!store_.spend.get(it, get))
            return error::integrity5;

        if (get.point_fk != self)
            points.push_back(get.point_fk);
    }
    
    while (it.advance());
    return error::success;
}

// protected
TEMPLATE
error::error_t CLASS::get_doubles(tx_links& spenders,
    const point_links& points, const point_link& self) const NOEXCEPT
{
    // The expected self spend and primary conflicts are removed. This serves
    // to remove secondary conflicts, leaving only actual additional spends.
    const auto ptr = store_.point.get_memory();

    table::point::get_key self_tx_point{};
    if (!store_.point.get(ptr, self, self_tx_point))
        return error::integrity6;

    for (auto point: points)
    {
        table::point::get_parent_key parent_tx{};
        if (!store_.point.get(ptr, point, parent_tx))
            return error::integrity7;

        if (parent_tx.hash == self_tx_point.hash)
            spenders.push_back(parent_tx.fk);
    }

    return error::success;
}

// protected
TEMPLATE
error::error_t CLASS::spent(const point_link& self, const point_stub& stub,
    index index) const NOEXCEPT
{
    error::error_t ec{};

    // No heap allocation if no hashmap conflicts.
    point_links points{};
    if ((ec = get_conflicts(points, self, table::spend::compose(stub, index))))
        return ec;

    if (points.empty())
        return error::success;

    // No heap allocation if no double spends.
    tx_links spenders{};
    if ((ec = get_doubles(spenders, points, self)))
        return ec;

    // Check all double spends for confirmation.
    for (auto spender: spenders)
        if (is_strong_tx(spender))
            return error::confirmed_double_spend;

    return error::success;
}

// unspendable
// ----------------------------------------------------------------------------

// protected
TEMPLATE
error::error_t CLASS::unspendable(uint32_t sequence, bool coinbase,
    const tx_link& tx, uint32_t version, const context& ctx) const NOEXCEPT
{
    ///////////////////////////////////////////////////////////////////////////
    // TODO: If to_block(tx) is terminal, may be a duplicate tx, so perform
    // TODO: search for each tx instance of same hash as tx until block
    // TODO: associated (otherwise missing/unconfirmed prevout).
    ///////////////////////////////////////////////////////////////////////////
    const auto strong = to_block(tx);
    if (strong.is_terminal())
        return error::unconfirmed_spend;

    const auto bip68 = ctx.is_enabled(system::chain::flags::bip68_rule) &&
        (version >= system::chain::relative_locktime_min_version);

    if (bip68 || coinbase)
    {
        context out{};
        if (!get_context(out, strong))
            return error::integrity8;

        if (bip68 &&
            input::is_locked(sequence, ctx.height, ctx.mtp, out.height, out.mtp))
            return error::relative_time_locked;

        if (coinbase &&
            !transaction::is_coinbase_mature(out.height, ctx.height))
            return error::coinbase_maturity;
    }

    return error::success;
}

// get_point_set
// ----------------------------------------------------------------------------

// protected
TEMPLATE
bool CLASS::get_point_set(point_set& set, const tx_link& link) const NOEXCEPT
{
    table::transaction::get_version_inputs tx{};
    if (!store_.tx.get(link, tx))
        return false;

    ///////////////////////////////////////////////////////////////////////////
    // TODO: puts table does not need to be traversed for points if sequential.
    ///////////////////////////////////////////////////////////////////////////
    table::puts::get_points puts{};
    puts.point_fks.resize(tx.ins_count);
    if (!store_.puts.get(tx.puts_fk, puts))
        return false;

    set.version = tx.version;
    set.points.reserve(puts.point_fks.size());

    ///////////////////////////////////////////////////////////////////////////
    // TODO: point rows could be allocated sequentially for the tx.
    // TODO: so this could be changed to a single get for all values.
    ///////////////////////////////////////////////////////////////////////////
    for (const auto& self: puts.point_fks)
    {
        table::point::get_spend_key_sequence point{ {}, { self } };
        if (!store_.point.get(self, point))
            return false;

        set.points.push_back(std::move(point.value));
    }

    return true;
}

TEMPLATE
bool CLASS::populate_prevouts(point_sets& sets, size_t points,
    const header_link& link) const NOEXCEPT
{
    table::prevout::record_get prevouts{};
    prevouts.values.resize(points);
    if (!store_.prevout.at(link, prevouts))
        return false;

    // This technique stores internal points as null points in order to
    // maintain relative point positions.
    size_t index{};
    for (auto& set: sets)
        for (auto& point: set.points)
        {
            // TODO: this is dereferencing vector position twice.
            // These are generated during validation and stored into a single
            // arraymap allocation at that time (no search).
            point.tx = prevouts.output_tx_fk(index);
            point.coinbase = prevouts.coinbase(index++);
        }

    return true;
}

TEMPLATE
bool CLASS::populate_prevouts(point_sets& sets) const NOEXCEPT
{
    // This technique does not benefit from skipping internal spends, and
    // therefore also requires set_strong before query, and self removal.
    for (auto& set: sets)
        for (auto& point: set.points)
        {
            // TODO: is_coinbase and to_tx can be combined.
            // These are the queries avoided by the points table.
            point.tx = to_tx(get_point_key(point.self));
            point.coinbase = is_coinbase(point.tx);

            // Guard against because terminal is excluded in common code.
            if (point.tx == table::prevout::tx::terminal)
                return false;
        }

    return true;
}

// block_confirmable
// ----------------------------------------------------------------------------

TEMPLATE
code CLASS::block_confirmable(const header_link& link) const NOEXCEPT
{
    constexpr auto parallel = poolstl::execution::par;

    context ctx{};
    if (!get_context(ctx, link))
        return error::integrity9;

    // bip30 coinbase check.
    ////code ec{};
    ////if ((ec = unspent_duplicates(link, ctx)))
    ////    return ec;

    // Empty block is success.
    const auto txs = to_spending_txs(link);
    if (txs.empty())
        return error::success;

    // One point set per tx.
    point_sets sets(txs.size());
    std::atomic<size_t> points{ zero };
    std::atomic<error::error_t> failure{ error::success };

    const auto to_set = [this, &points, &failure](const auto& tx) NOEXCEPT
    {
        point_set set{};
        if (!get_point_set(set, tx))
            failure.store(error::integrity10);

        points.fetch_add(set.points.size(), std::memory_order_relaxed);
        return set;
    };

    std::transform(parallel, txs.begin(), txs.end(), sets.begin(), to_set);
    if (failure)
        return { failure.load() };

    if (!(prevout_enabled() ? populate_prevouts(sets, points, link) :
        populate_prevouts(sets)))
        return error::integrity11;

    const auto is_unspendable = [this, &ctx, &failure](const auto& set) NOEXCEPT
    {
        error::error_t ec{};
        for (const auto& point: set.points)
            if (!point.tx.is_terminal() && ((ec = unspendable(point.sequence,
                point.coinbase, point.tx, set.version, ctx))))
                failure.store(ec);

        return failure != error::success;
    };

    const auto is_spent = [this, &failure](const auto& set) NOEXCEPT
    {
        error::error_t ec{};
        for (const auto& point: set.points)
            if (!point.tx.is_terminal() && ((ec = spent(point.self, point.stub,
                point.index))))
                failure.store(ec);

        return failure != error::success;
    };

    if (std::any_of(parallel, sets.begin(), sets.end(), is_unspendable))
        return { failure.load() };

    if (std::any_of(parallel, sets.begin(), sets.end(), is_spent))
        return { failure.load() };

    return error::success;
}

// setters
// ----------------------------------------------------------------------------

// protected
// This is also invoked from block.txs archival (when checkpoint/milestone).
TEMPLATE
bool CLASS::set_strong(const header_link& link, const tx_links& txs,
    bool positive) NOEXCEPT
{
    using namespace system;
    using link_t = table::strong_tx::link;
    using element_t = table::strong_tx::record;

    // Preallocate all strong_tx records for the block and reuse memory ptr.
    const auto records = possible_narrow_cast<link_t::integer>(txs.size());
    auto record = store_.strong_tx.allocate(records);
    const auto ptr = store_.strong_tx.get_memory();

    for (const auto tx: txs)
        if (!store_.strong_tx.put(ptr, record++, link_t{ tx }, element_t
        {
            {},
            link,
            positive
        })) return false;

    return true;
}

TEMPLATE
bool CLASS::set_strong(const header_link& link) NOEXCEPT
{
    const auto txs = to_transactions(link);
    if (txs.empty())
        return false;

    // ========================================================================
    const auto scope = store_.get_transactor();

    // Clean allocation failure (e.g. disk full).
    return set_strong(link, txs, true);
    // ========================================================================
}

TEMPLATE
bool CLASS::set_unstrong(const header_link& link) NOEXCEPT
{
    const auto txs = to_transactions(link);
    if (txs.empty())
        return false;

    // ========================================================================
    const auto scope = store_.get_transactor();

    // Clean allocation failure (e.g. disk full).
    return set_strong(link, txs, false);
    // ========================================================================
}

TEMPLATE
bool CLASS::set_prevouts(const header_link& link, const block& block) NOEXCEPT
{
    if (!prevout_enabled())
        return true;

    // Empty or coinbase only implies no spends.
    if (block.transactions() <= one)
        return true;

    // ========================================================================
    const auto scope = store_.get_transactor();

    // Clean single allocation failure (e.g. disk full).
    const table::prevout::record_put_ref prevouts{ {}, block };
    return store_.prevout.put(link, prevouts);
    // ========================================================================
}

} // namespace database
} // namespace libbitcoin

#endif
