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
    
    // Current block is set strong, so self is expected.
    if (is_one(coinbases.size()))
        return error::success;
    
    // Remove self (will be not found if current block is not set_strong).
    const auto self = std::find(coinbases.begin(), coinbases.end(), link);
    if (self == coinbases.end() || coinbases.erase(self) == coinbases.end())
        return error::integrity3;
    
    // bip30: all outputs of all previous duplicate coinbases must be spent.
    // BUGBUG: with multiple previous duplicates there must be same number of
    // BUGBUG: spends of each coinbase output, but this will identify only one.
    for (const auto coinbase: coinbases)
        if (!is_spent_coinbase(coinbase))
            return error::unspent_coinbase_collision;

    return error::success;
}

// get_double_spenders
// ----------------------------------------------------------------------------
// called from validation to set prevout state

// protected
TEMPLATE
code CLASS::push_spenders(tx_links& out, const point& point,
    const point_link& self) const NOEXCEPT
{
    auto it = store_.point.it(table::point::compose(point));
    if (!it)
    {
        // This verified that there was a race condition in the intial hashmap
        // memory fence implementation, manifesting above as not found, where
        // immediately after (below) the same object was found. Now fixed.
        ////const auto key1 = it.key();
        ////const auto link1 = it.self();
        ////const auto get1 = it.get();
        ////it.reset();
        ////
        ////if (key1 != table::point::compose(point))
        ////    return error::integrity12;
        ////if (!link1.is_terminal())
        ////    return error::integrity13;
        ////if (is_null(get1))
        ////    return error::integrity14;
        ////
        ////table::point::record get{};
        ////if (!store_.point.get(self, get))
        ////    return error::integrity15;
        ////if (get.hash != point.hash())
        ////    return error::integrity16;
        ////if (get.index != point.index())
        ////    return error::integrity17;

        return error::integrity4;
    }

    point_links points{};
    do
    {
        if (it.self() != self)
            points.push_back(it.self());
    }
    while (it.advance());
    it.reset();

    for (auto link: points)
    {
        table::ins::get_parent get{};
        if (!store_.ins.get(link, get))
            return error::integrity5;

        out.push_back(get.parent_fk);
    }

    return error::success;
}

TEMPLATE
code CLASS::get_double_spenders(tx_links& out,
    const block& block) const NOEXCEPT
{
    // Empty or coinbase only implies no spends.
    const auto& txs = *block.transactions_ptr();
    if (txs.size() <= one)
        return error::success;

    code ec{};
    for (auto tx = std::next(txs.begin()); tx != txs.end(); ++tx)
        for (const auto& in: *(*tx)->inputs_ptr())
            if ((ec = push_spenders(out, in->point(), in->metadata.link)))
                return ec;

    return error::success;
}

// unspendable
// ----------------------------------------------------------------------------

// protected
TEMPLATE
error::error_t CLASS::unspendable(uint32_t sequence, bool coinbase,
    const tx_link& tx, uint32_t version, const context& ctx) const NOEXCEPT
{
    // Ensure prevout tx is in a strong block, first try self link.
    auto strong = to_block(tx);

    // Extremely rare, normally implies a duplicate tx.
    if (strong.is_terminal())
    {
        // Try all txs with same hash as self (any instance will suffice).
        strong = to_strong(get_tx_key(tx));
        if (strong.is_terminal())
            return error::unconfirmed_spend;
    }

    const auto relative = ctx.is_enabled(system::chain::flags::bip68_rule) &&
        transaction::is_relative_locktime_applied(coinbase, version, sequence);

    if (relative || coinbase)
    {
        context out{};
        if (!get_context(out, strong))
            return error::integrity7;

        if (relative &&
            input::is_locked(sequence, ctx.height, ctx.mtp, out.height, out.mtp))
            return error::relative_time_locked;

        if (coinbase &&
            !transaction::is_coinbase_mature(out.height, ctx.height))
            return error::coinbase_maturity;
    }

    return error::success;
}

// populate_prevouts
// ----------------------------------------------------------------------------

TEMPLATE
code CLASS::populate_prevouts(point_sets& sets, size_t points,
    const header_link& link) const NOEXCEPT
{
    // Don't hit prevout table for empty block.
    if (sets.empty())
        return error::success;

    table::prevout::slab_get cache{};
    cache.spends.resize(points);
    if (!store_.prevout.at(link, cache))
        return error::integrity8;

    for (const auto& spender: cache.conflicts)
        if (is_strong_tx(spender))
            return error::confirmed_double_spend;

    auto it = cache.spends.begin();
    for (auto& set: sets)
        for (auto& point: set.points)
        {
            const auto& pair = *it++;
            point.tx = table::prevout::slab_get::output_tx_fk(pair.first);
            point.coinbase = table::prevout::slab_get::coinbase(pair.first);
            point.sequence = pair.second;
        }

    return error::success;
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
    code ec{};
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
        table::transaction::get_set_ref get{ {}, set };
        if (!store_.tx.get(tx, get))
            failure.store(error::integrity10);

        points.fetch_add(set.points.size(), std::memory_order_relaxed);
        return set;
    };

    std::transform(parallel, txs.begin(), txs.end(), sets.begin(), to_set);
    if (failure)
        return { failure.load() };

    // Checks for double spends and populates prevout parent tx/cb links.
    if ((ec = populate_prevouts(sets, points, link)))
        return ec;

    const auto is_unspendable = [this, &ctx, &failure](const auto& set) NOEXCEPT
    {
        error::error_t ec{};
        for (const auto& point: set.points)
            if (!point.tx.is_terminal() && ((ec = unspendable(point.sequence,
                point.coinbase, point.tx, set.version, ctx))))
                failure.store(ec);

        return failure != error::success;
    };

    if (std::any_of(parallel, sets.begin(), sets.end(), is_unspendable))
        return { failure.load() };

    return error::success;
}

// setters
// ----------------------------------------------------------------------------

// protected
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

// protected
// This is invoked from block.txs archival (when checkpoint/milestone).
TEMPLATE
bool CLASS::set_strong(const header_link& link, size_t count,
    const tx_link& first_fk, bool positive) NOEXCEPT
{
    using namespace system;
    using link_t = table::strong_tx::link;
    using element_t = table::strong_tx::record;

    // Preallocate all strong_tx records for the block and reuse memory ptr.
    const auto records = possible_narrow_cast<link_t::integer>(count);
    auto record = store_.strong_tx.allocate(records);
    const auto ptr = store_.strong_tx.get_memory();
    const auto end = first_fk + count;

    // Contiguous tx links.
    for (auto fk = first_fk; fk < end; ++fk)
        if (!store_.strong_tx.put(ptr, record++, fk, element_t
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
code CLASS::set_prevouts(const header_link& link, const block& block) NOEXCEPT
{
    code ec{};
    tx_links spenders{};
    if ((ec = get_double_spenders(spenders, block)))
        return ec;

    // ========================================================================
    const auto scope = store_.get_transactor();

    // Clean single allocation failure (e.g. disk full).
    const table::prevout::slab_put_ref prevouts{ {}, spenders, block };
    return store_.prevout.put(link, prevouts) ? error::success :
        error::integrity11;
    // ========================================================================
}

} // namespace database
} // namespace libbitcoin

#endif
