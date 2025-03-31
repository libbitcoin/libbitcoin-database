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
#include <utility>
#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/error.hpp>

namespace libbitcoin {
namespace database {

// unspent_duplicates (bip30)
// ----------------------------------------------------------------------------

// protected
TEMPLATE
bool CLASS::is_spent_prevout(const point_link&, index) const NOEXCEPT
{
    // TODO: with multiple previous duplicates there must be same number of
    // TODO: spends of each coinbase output, but this will identify only one.
    return false;
}

TEMPLATE
bool CLASS::is_spent_coinbase(const tx_link& link) const NOEXCEPT
{
    // All outputs of the tx are confirmed spent.
    const auto point_fk = to_point(get_tx_key(link));
    for (index index{}; index < output_count(link); ++index)
        if (!is_spent_prevout(point_fk, index))
            return false;

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

    // Get coinbase (block's first tx link).
    const auto cb = to_coinbase(link);
    if (cb.is_terminal())
        return error::integrity1;

    // Get all coinbases of the same hash (must be at least one, because self).
    // ........................................................................
    auto it = store_.tx.it(get_tx_key(cb));
    if (!it)
        return error::integrity2;

    tx_links coinbases{};
    coinbases.reserve(one);
    do
    {
        // Optimal for very short or empty sets.
        if (!system::contains(coinbases, it.self()))
            coinbases.push_back(it.self());
    }
    while (it.advance());
    it.reset();

    // remove non-strong cbs (usually empty, self not strong w/prevout table).
    // strong_tx records are always populated by the associating block header.
    // Coinbase txs are always set strong in assocition with the associating
    // header, both for block associations and compact. The compact block cb
    // may be archived only when it is also associated via txs to the header.
    // ........................................................................
    std::erase_if(coinbases, [this](const auto& tx) NOEXCEPT
    {
        table::strong_tx::record strong{};
        return store_.strong_tx.find(tx, strong) && strong.positive;
    });

    ////// Strong must be other blocks, dis/re-orged blocks are not strong.
    ////// Remove self (will be not found if current block is not set_strong).
    ////const auto self = std::find(coinbases.begin(), coinbases.end(), link);
    ////if (self == coinbases.end() || coinbases.erase(self) == coinbases.end())
    ////    return error::integrity3;
    
    // bip30: all outputs of all previous duplicate coinbases must be spent.
    for (const auto coinbase: coinbases)
        if (!is_spent_coinbase(coinbase))
            return error::unspent_coinbase_collision;

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
            return error::integrity4;

        if (relative &&
            input::is_relative_locked(sequence, ctx.height, ctx.mtp,
                out.height, out.mtp))
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
        return error::integrity5;

    // Is any duplicated point in the block is confirmed (generally empty).
    for (const auto& spender: cache.conflicts)
        if (is_strong_tx(spender))
            return error::confirmed_double_spend;

    // Augments spend.points with metadata.
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
        return error::integrity6;

    // bip30 coinbase check.
    code ec{};
    if ((ec = unspent_duplicates(link, ctx)))
        return ec;

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
            failure.store(error::integrity7);

        points.fetch_add(set.points.size(), std::memory_order_relaxed);
        return set;
    };

    // Get points for each tx, and sum the total number.
    std::transform(parallel, txs.begin(), txs.end(), sets.begin(), to_set);
    if (failure)
        return { failure.load() };

    // Check double spends strength, populates prevout parent tx/cb/sq links.
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

    // Check all spends for spendability (strong, unlocked and mature).
    if (std::any_of(parallel, sets.begin(), sets.end(), is_unspendable))
        return { failure.load() };

    return error::success;
}

// compact blocks methods (full query, no doubles table)
// ----------------------------------------------------------------------------
// TODO: currently unused (apply in compacy blocks).

// protected
TEMPLATE
bool CLASS::get_double_spenders(tx_links& out, const point& point,
    const point_link& self) const NOEXCEPT
{
    // This is most of the expense of confirmation, and is not mitigated by the
    // point table sieve, since self always exists.
    auto it = store_.point.it(point);
    if (!it)
        return false;

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
            return false;

        out.push_back(get.parent_fk);
    }

    return true;
}

// protected
TEMPLATE
bool CLASS::get_double_spenders(tx_links& out,
    const block& block) const NOEXCEPT
{
    // Empty or coinbase only implies no spends.
    const auto& txs = *block.transactions_ptr();
    if (txs.size() <= one)
        return true;

    for (auto tx = std::next(txs.begin()); tx != txs.end(); ++tx)
        for (const auto& in: *(*tx)->inputs_ptr())
            if (!get_double_spenders(out, in->point(), in->metadata.link))
                return false;

    return true;
}

// set_strong
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

// set_prevouts
// ----------------------------------------------------------------------------
// called from validation to set prevout state

TEMPLATE
bool CLASS::get_doubles(tx_links&, const point&) const NOEXCEPT
{
    // TODO: Get all parent tx links for any point of block in doubles table.
    return true;
}

TEMPLATE
bool CLASS::get_doubles(tx_links& out, const block& block) const NOEXCEPT
{
    // Empty or coinbase only implies no spends.
    const auto& txs = *block.transactions_ptr();
    if (txs.size() <= one)
        return true;

    for (auto tx = std::next(txs.begin()); tx != txs.end(); ++tx)
        for (const auto& in: *(*tx)->inputs_ptr())
            if (!get_doubles(out, in->point()))
                return false;

    return true;
}

TEMPLATE
bool CLASS::set_prevouts(const header_link& link, const block& block) NOEXCEPT
{
    tx_links doubles{};
    if (!get_doubles(doubles, block))
        return false;

    // ========================================================================
    const auto scope = store_.get_transactor();

    // Clean single allocation failure (e.g. disk full).
    const table::prevout::slab_put_ref prevouts{ {}, doubles, block };
    return store_.prevout.put(link, prevouts);
    // ========================================================================
}

} // namespace database
} // namespace libbitcoin

#endif
