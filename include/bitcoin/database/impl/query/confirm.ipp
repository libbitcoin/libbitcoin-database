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
#ifndef LIBBITCOIN_DATABASE_QUERY_CONFIRM_IPP
#define LIBBITCOIN_DATABASE_QUERY_CONFIRM_IPP

#include <algorithm>
#include <atomic>
#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/error.hpp>

namespace libbitcoin {
namespace database {

// Block status (mostly surrogate-keyed).
// ----------------------------------------------------------------------------
// Not for use in validatation (2 additional gets) or confirmation (height).

TEMPLATE
height_link CLASS::get_height(const hash_digest& key) const NOEXCEPT
{
    table::header::get_height header{};
    if (!store_.header.find(key, header))
        return {};

    return header.height;
}

TEMPLATE
height_link CLASS::get_height(const header_link& link) const NOEXCEPT
{
    table::header::get_height header{};
    if (!store_.header.get(link, header))
        return {};

    return header.height;
}

// protected/get_confirmed_balance(address)
TEMPLATE
bool CLASS::is_confirmed_unspent(const output_link& link) const NOEXCEPT
{
    return is_confirmed_output(link) && !is_spent_output(link);
}

TEMPLATE
bool CLASS::is_candidate_header(const header_link& link) const NOEXCEPT
{
    // The header is candidate (by height).
    const auto height = get_height(link);
    if (height.is_terminal())
        return false;

    table::height::record candidate{};
    return store_.candidate.get(height, candidate) &&
        (candidate.header_fk == link);
}

TEMPLATE
bool CLASS::is_confirmed_block(const header_link& link) const NOEXCEPT
{
    // The block is confirmed (by height).
    const auto height = get_height(link);
    if (height.is_terminal())
        return false;

    table::height::record confirmed{};
    return store_.confirmed.get(height, confirmed) &&
        (confirmed.header_fk == link);
}

TEMPLATE
bool CLASS::is_confirmed_tx(const tx_link& link) const NOEXCEPT
{
    // The tx is strong *and* its block is confirmed (by height).
    const auto fk = to_block(link);
    return !fk.is_terminal() && is_confirmed_block(fk);
}

TEMPLATE
bool CLASS::is_confirmed_input(const spend_link& link) const NOEXCEPT
{
    // The spend.tx is strong *and* its block is confirmed (by height).
    const auto fk = to_spend_tx(link);
    return !fk.is_terminal() && is_confirmed_tx(fk);
}

TEMPLATE
bool CLASS::is_confirmed_output(const output_link& link) const NOEXCEPT
{
    // The output.tx is strong *and* its block is confirmed (by height).
    const auto fk = to_output_tx(link);
    return !fk.is_terminal() && is_confirmed_tx(fk);
}

TEMPLATE
bool CLASS::is_spent_output(const output_link& link) const NOEXCEPT
{
    // The spender is strong *and* its block is confirmed (by height).
    const auto ins = to_spenders(link);
    return std::any_of(ins.begin(), ins.end(), [&](const auto& in) NOEXCEPT
    {
        return is_confirmed_input(in);
    });
}

// Confirmation.
// ----------------------------------------------------------------------------
// Block confirmed by height is not used for confirmation (just strong tx).

// unused
TEMPLATE
bool CLASS::is_spent(const spend_link& link) const NOEXCEPT
{
    table::spend::get_prevout_parent spend{};
    if (!store_.spend.get(link, spend))
        return false;

    if (spend.is_null())
        return false;

    return is_spent_prevout(spend.point_fk, spend.point_index,
        spend.parent_fk);
}

// unused
TEMPLATE
bool CLASS::is_strong_spend(const spend_link& link) const NOEXCEPT
{
    return is_strong_tx(to_spend_tx(link));
}

// unused
TEMPLATE
bool CLASS::is_mature(const spend_link& link, size_t height) const NOEXCEPT
{
    table::spend::get_point spend{};
    if (!store_.spend.get(link, spend))
        return false;

    if (spend.is_null())
        return true;

    return !mature_prevout(spend.point_fk, height);
}

// protected (only for is_mature/unused)
TEMPLATE
code CLASS::mature_prevout(const point_link& link,
    size_t height) const NOEXCEPT
{
    // Get hash from point, search for prevout tx and get its link.
    const auto tx_fk = to_tx(get_point_key(link));
    if (tx_fk.is_terminal())
        return error::integrity;

    // to_block assures confirmation by strong_tx traversal so this must remain
    // prior to is_coinbase in execution order, despite the lack of dependency.
    const auto header_fk = to_block(tx_fk);
    if (header_fk.is_terminal())
        return error::unconfirmed_spend;

    if (!is_coinbase(tx_fk))
        return error::success;

    const auto prevout_height = get_height(header_fk);
    if (prevout_height.is_terminal())
        return error::integrity;

    if (!transaction::is_coinbase_mature(prevout_height, height))
        return error::coinbase_maturity;

    return error::success;
}

// unused
TEMPLATE
bool CLASS::is_locked(const spend_link& link, uint32_t sequence,
    const context& ctx) const NOEXCEPT
{
    table::spend::get_point spend{};
    if (!store_.spend.get(link, spend))
        return false;

    if (spend.is_null())
        return true;

    return !locked_prevout(spend.point_fk, sequence, ctx);
}

// protected (only for is_locked/unused)
TEMPLATE
code CLASS::locked_prevout(const point_link& link, uint32_t sequence,
    const context& ctx) const NOEXCEPT
{
    if (!ctx.is_enabled(system::chain::flags::bip68_rule))
        return error::success;

    // Get hash from point, search for prevout tx and get its link.
    const auto tx_fk = to_tx(get_point_key(link));
    if (tx_fk.is_terminal())
        return error::missing_previous_output;

    // to_block assures confirmation by strong_tx traversal.
    const auto header_fk = to_block(tx_fk);
    if (header_fk.is_terminal())
        return error::unconfirmed_spend;

    context prevout_ctx{};
    if (!get_context(prevout_ctx, header_fk))
        return error::integrity;

    if (input::is_locked(sequence, ctx.height, ctx.mtp, prevout_ctx.height,
        prevout_ctx.mtp))
        return error::relative_time_locked;

    return error::success;
}

// protected
TEMPLATE
bool CLASS::is_spent_prevout(const point_link& link, index index,
    const tx_link& self) const NOEXCEPT
{
    return spent_prevout(link, index, self) != error::success;
}

// protected
TEMPLATE
error::error_t CLASS::spent_prevout(const point_link& link, index index,
    const tx_link& self) const NOEXCEPT
{
    // TODO: get_point_key(link) is redundant with unspendable_prevout().
    // searches [point.iterate {new} x (spend.iterate + strong_tx.find)].

    // The search for spenders must be exhaustive.
    // This is walking the full conflict list for the hash, but there is only
    // one match possible (self) unless there are duplicates/conflicts.
    // Conflicts here are both likely tx pool conflicts and rare duplicate txs,
    // since the points table is written for each spend (unless compressed and
    // that is still not a guarantee. So all must be checked. This holds one
    // instance of a tx for ***all spends of all outputs*** of that tx.

    // TODO: evaluate.
    // If point hash was in spend table key there would be just as many but the
    // key would be the hash and index combined, resulting in no unnecessary
    // point hash searches over irrelevant point indexes. Would save some space
    // in table compression, and simplify some code, but would eliminate store
    // compression and might increase paging cost due to spend table increase.
    // There would be only one search unless duplicates, and this would be self
    // so would not result in calling the is_strong_tx search. Spenders of outs
    // of the same prevout.tx would not result in search hits. Table no-hash
    // algorithm would require definition. This would eliminate spend.point_fk
    // and a point.pk link per record, or 8 bytes per spend. This is the amount
    // to be added by the new array cache table, maybe just repurpose point.
    // Because cache can be removed this is a 19GB reduction, for the loss of
    // ability to reduce 50GB, which we don't generally do. So also a win on
    // nominal store size. All we need from the cache is the spend.pk/index.
    // spend.pk size does not change because it's an array (count unchanged).
    // So this is a reduction from plan, 4+3 bytes per cache row vs. 5+3.
    // But if we hold the spend.pk/prevout.tx we can just read the
    // spend.hash/index, so we don't need to store the index, and we need to
    // read the spend.hash anyway, so index is free (no paging). So that's now
    // just spend[4] + tx[4], back to 8 bytes (19GB). But getting spends is
    // relatively cheap, just search txs[hash] and navigate puts. The downside
    // is the extra search and puts is > 2x prevouts and can't be pruned.
    // The upside is half the prevout size (read/write/page) and store increase.

    // Iterate points by point hash (of output tx) because may be conflicts.
    auto point = store_.point.it(get_point_key(link));
    if (!point)
        return error::integrity1;

    do
    {
        // Iterate all spends of the point to find double spends.
        auto it = store_.spend.it(table::spend::compose(point.self(), index));
        if (!it)
            return error::success;

        table::spend::get_parent spend{};
        do
        {
            if (!store_.spend.get(it, spend))
                return error::integrity2;

            // is_strong_tx (search) only called in the case of duplicate.
            // Other parent tx of spend is strong (confirmed spent prevout).
            if ((spend.parent_fk != self) && is_strong_tx(spend.parent_fk))
                return error::confirmed_double_spend;
        }
        while (it.advance());
    }
    while (point.advance());
    return error::success;
}

// Low cost.
// header_link
// header_link.ctx.mtp
// header_link.ctx.flags
// header_link.ctx.height
// header_link:txs.tx.pk
// header_link:txs.tx.version

// unspendable_prevout
// Given that these use the same txs association, there is no way for the 
// header.txs.tx to change, and it is only ever this pk that is set strong.
// If unconfirmed_spend is encountered, perform a search (free). It's not
// possible for a confirmed spend to be the wrong tx instance.
//
// There is no strong (prevout->tx->block) association at this point in validation.
// strong_tx is interrogated for each spend except self (0) and each prevout (2.6B).
// to_tx(get_point_key(header_link:txs.tx.puts.spend.point_fk)):block.ctx.height|mtp
// This is done in populate, except for to_strong, ***so save prevout tx [4]***
//
// is_coinbase_mature(is_coinbase(header_link:txs.tx), ...block.ctx.height), is_locked
// is_locked(header_link:txs.tx.puts.spend.sequence, ...block.ctx.height|mtp)

// spent_prevout (see notes in fn).
// header_link:txs.tx.puts.spend.point_index

// protected
TEMPLATE
error::error_t CLASS::unspendable_prevout(uint32_t sequence, bool coinbase,
    const tx_link& prevout_tx, uint32_t version,
    const context& ctx) const NOEXCEPT
{
    // TODO: don't need to return tx link here, just the block (for strong/context).
    // MOOT: get_point_key(link) is redundant with spent_prevout().
    // to_strong has the only searches [tx.iterate, strong.find].
    ////const auto strong_prevout = to_strong(get_point_key(link));
    ////
    ////// prevout is strong if present.
    ////if (strong_prevout.block.is_terminal())
    ////    return strong_prevout.tx.is_terminal() ?
    ////        error::missing_previous_output : error::unconfirmed_spend;

    // TODO: If unconfirmed_spend is encountered, perform a search (free).
    // It's not possible for a confirmed spend to be the wrong tx instance.
    // This eliminates the hash lookup and to_strong(hash) iteration.
    const auto block = to_block(prevout_tx);

    context out{};
    if (!get_context(out, block))
        return error::integrity3;

    // All txs with same hash must be coinbase or not.
    if (coinbase && !transaction::is_coinbase_mature(out.height, ctx.height))
        return error::coinbase_maturity;

    if (ctx.is_enabled(system::chain::flags::bip68_rule) &&
        (version >= system::chain::relative_locktime_min_version) &&
        input::is_locked(sequence, ctx.height, ctx.mtp, out.height, out.mtp))
        return error::relative_time_locked;

    return error::success;
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

    // [txs.find, {tx.iterate}, strong_tx.it]
    auto coinbases = to_strong_txs(get_tx_key(to_coinbase(link)));

    // Current block is not set strong.
    if (is_zero(coinbases.size()))
        return error::success;

    // [point.first, is_spent_prevout()]
    const auto spent = [this](const auto& tx) NOEXCEPT
    {
        return is_spent_coinbase(tx);
    };

    // bip30: all outputs of all previous duplicate coinbases must be spent.
    return std::all_of(coinbases.begin(), coinbases.end(), spent) ?
        error::success : error::unspent_coinbase_collision;
}

#if defined(UNDEFINED)

// protected
TEMPLATE
spend_sets CLASS::to_spend_sets(const header_link& link) const NOEXCEPT
{
    const auto txs = to_transactions(link);
    if (txs.empty())
        return {};

    // Coinbase optimization.
    spend_sets out{ txs.size() };
    out.front().tx = txs.front();
    if (is_one(out.size()))
        return out;

    spend_sets sets{};
    sets.reserve(sub1(txs.size()));
    for (auto tx = std::next(txs.begin()); tx != txs.end(); ++tx)
        sets.push_back(to_spend_set(*tx));

    return sets;
}

// Used by node for ASIO concurrency by tx.
TEMPLATE
code CLASS::tx_confirmable(const tx_link& link,
    const context& ctx) const NOEXCEPT
{
    // utxo needs spend set for sequence and spend.prevout(), which is the non-
    // iterating key to utxo table. This is the identifier of the point:index
    // which is used to obtain the output for validation by
    // tx.find(get_point_key(link)). However we don't need the output for
    // confirmation, just a unique identifier for it. Yet the point:index (fp)
    // is not unique, because here are many fps for any given output due to tx
    // and therefore point table duplication. So the spend sets identify all of
    // the spends of a given tx, but one tx may have a different point for the
    // same output. So all outputs of the point must be searched for existence,
    // which requires point and tx table traversal just as before. :<
    const auto set = to_spend_set(link);

    code ec{};
    for (const auto& spend: set.spends)
    {
        // If utxo exists then it is strong (push own block first).
        if ((ec = unspendable_prevout(spend.point_fk, spend.sequence,
            set.version, ctx)))
            return ec;

        // This query goes away.
        // If utxo exists then it is not spent (push own block first).
        if (is_spent_prevout(spend.point_fk, spend.point_index, link))
            return error::confirmed_double_spend;
    }

    return error::success;
}

// Used by node for sequential by block (unused).
// split(0) 403 secs for 400k-410k
TEMPLATE
code CLASS::block_confirmable(const header_link& link) const NOEXCEPT
{
    context ctx{};
    if (!get_context(ctx, link))
        return error::integrity;

    const auto txs = to_transactions(link);
    if (txs.empty())
        return error::txs_empty;

    code ec{};
    if ((ec = unspent_duplicates(txs.front(), ctx)))
        return ec;

    for (auto tx = std::next(txs.begin()); tx != txs.end(); ++tx)
        if ((ec = tx_confirmable(*tx, ctx)))
            return ec;

    return ec;
}

#endif

// protected
TEMPLATE
spend_sets CLASS::to_spend_sets(const header_link& link) const NOEXCEPT
{
    // This is the only search [txs.find].
    // Coinbase tx does not spend so is not retrieved.
    const auto txs = to_spending_transactions(link);

    if (txs.empty())
        return {};

    spend_sets sets{ txs.size() };
    const auto to_set = [this](const auto& tx) NOEXCEPT
    {
        return to_spend_set(tx);
    };

    // C++17 incomplete on GCC/CLang, so presently parallel only on MSVC++.
    std_transform(bc::par_unseq, txs.begin(), txs.end(), sets.begin(), to_set);

    const auto count = [](size_t total, const auto& set) NOEXCEPT
    {
        return system::ceilinged_add(total, set.spends.size());
    };
    const auto spends = std::accumulate(sets.begin(), sets.end(), zero, count);

    table::prevout::record_get prevouts{};
    prevouts.values.resize(spends);
    store_.prevout.at(link, prevouts);

    size_t index{};
    for (auto& set: sets)
        for (auto& spend: set.spends)
        {
            spend.coinbase = prevouts.coinbase(index);
            spend.prevout_tx_fk = prevouts.output_tx_fk(index++);
        }

    return sets;
}

// split(3) 219 secs for 400k-410k; split(2) 255 and split(0) 456 (not shown).
TEMPLATE
code CLASS::block_confirmable(const header_link& link) const NOEXCEPT
{
    context ctx{};
    if (!get_context(ctx, link))
        return error::integrity4;

    // This is never invoked (bip30).
    code ec{};
    if ((ec = unspent_duplicates(link, ctx)))
        return ec;

    const auto sets = to_spend_sets(link);
    if (sets.empty())
        return ec;

    std::atomic<error::error_t> fault{ error::success };

    const auto is_unspendable = [this, &ctx, &fault](const auto& set) NOEXCEPT
    {
        // TODO: prevout table optimized, evaluate.
        error::error_t ec{};
        for (const auto& spend: set.spends)
        {
            if (spend.prevout_tx_fk == table::prevout::tx::terminal)
                continue;

            if ((ec = unspendable_prevout(spend.sequence, spend.coinbase,
                    spend.prevout_tx_fk, set.version, ctx)))
            {
                fault.store(ec);
                return true;
            }
        }

        return false;
    };

    const auto is_spent = [this, &fault](const auto& set) NOEXCEPT
    {
        // TODO: point table optimize via consolidation with spend table.
        error::error_t ec{};
        for (const spend_set::spend& spend: set.spends)
        {
            if (spend.prevout_tx_fk == table::prevout::tx::terminal)
                continue;

            if ((ec = spent_prevout(spend.point_fk, spend.point_index, set.tx)))
            {
                fault.store(ec);
                return true;
            }
        }

        return false;
    };

    // C++17 incomplete on GCC/CLang, so presently parallel only on MSVC++.
    if (std_any_of(bc::par_unseq, sets.begin(), sets.end(), is_unspendable))
        return { fault.load() };

    // C++17 incomplete on GCC/CLang, so presently parallel only on MSVC++.
    if (std_any_of(bc::par_unseq, sets.begin(), sets.end(), is_spent))
        return { fault.load() };

    return ec;
}

#if defined(UNDEFINED)

// split(1) 446 secs for 400k-410k
TEMPLATE
code CLASS::block_confirmable(const header_link& link) const NOEXCEPT
{
    context ctx{};
    if (!get_context(ctx, link))
        return error::integrity;

    code ec{};
    if ((ec = unspent_duplicates(to_coinbase(link), ctx)))
        return ec;

    const auto sets = to_spend_sets(link);
    for (const auto& set: sets)
    {
        for (const auto& spend: set.spends)
        {
            if ((ec = unspendable_prevout(spend.point_fk, spend.sequence,
                set.version, ctx)))
                return ec;

            if (is_spent_prevout(spend.point_fk, spend.point_index, set.tx))
                return error::confirmed_double_spend;
        }
    }

    return ec;
}

// split(3) 416 secs for 400k-410k
TEMPLATE
code CLASS::block_confirmable(const header_link& link) const NOEXCEPT
{
    context ctx{};
    if (!get_context(ctx, link))
        return error::integrity;

    code ec{};
    if ((ec = unspent_duplicates(to_coinbase(link), ctx)))
        return ec;

    const auto sets = to_spend_sets(link);
    for (const auto& set: sets)
        for (const auto& spend: set.spends)
            if ((ec = unspendable_prevout(spend.point_fk, spend.sequence,
                set.version, ctx)))
                return ec;
    
    if (ec) return ec;

    for (const auto& set: sets)
        for (const auto& spend: set.spends)
            if (is_spent_prevout(spend.point_fk, spend.point_index, set.tx))
                return error::confirmed_double_spend;

    return ec;
}

#endif // DISABLED

TEMPLATE
bool CLASS::is_spent_coinbase(const tx_link& link) const NOEXCEPT
{
    const auto point_fk = to_point(get_tx_key(link));
    for (index index{}; index < output_count(link); ++index)
        if (!is_spent_prevout(point_fk, index))
            return false;

    return true;
}

TEMPLATE
bool CLASS::is_strong_tx(const tx_link& link) const NOEXCEPT
{
    table::strong_tx::record strong{};
    return store_.strong_tx.find(link, strong) && strong.positive;
}

TEMPLATE
bool CLASS::is_strong_block(const header_link& link) const NOEXCEPT
{
    return is_strong_tx(to_coinbase(link));
}

// protected
TEMPLATE
bool CLASS::set_strong(const header_link& link, const tx_links& txs,
    bool positive) NOEXCEPT
{
    const auto set = [this, &link, positive](const tx_link& tx) NOEXCEPT
    {
        // TODO: eliminate shared memory pointer reallocation.
        return store_.strong_tx.put(tx, table::strong_tx::record
        {
            {},
            link,
            positive
        });
    };

    return std::all_of(txs.begin(), txs.end(), set);
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

TEMPLATE
bool CLASS::initialize(const block& genesis) NOEXCEPT
{
    BC_ASSERT(!is_initialized());
    BC_ASSERT(is_one(genesis.transactions_ptr()->size()));

    // ========================================================================
    const auto scope = store_.get_transactor();

    if (!set(genesis, context{}, false, false))
        return false;

    const auto link = to_header(genesis.hash());

    // Unsafe for allocation failure, but only used in store creation.
    return set_strong(link)
        && push_candidate(link)
        && push_confirmed(link);
    // ========================================================================
}

TEMPLATE
bool CLASS::push_candidate(const header_link& link) NOEXCEPT
{
    if (link.is_terminal())
        return false;

    // ========================================================================
    const auto scope = store_.get_transactor();

    // Clean single allocation failure (e.g. disk full).
    const table::height::record candidate{ {}, link };
    return store_.candidate.put(candidate);
    // ========================================================================
}

TEMPLATE
bool CLASS::push_confirmed(const header_link& link) NOEXCEPT
{
    if (link.is_terminal())
        return false;

    // ========================================================================
    const auto scope = store_.get_transactor();

    // Clean single allocation failure (e.g. disk full).
    const table::height::record confirmed{ {}, link };
    return store_.confirmed.put(confirmed);
    // ========================================================================
}

TEMPLATE
bool CLASS::pop_candidate() NOEXCEPT
{
    using ix = table::transaction::ix::integer;
    const auto top = system::possible_narrow_cast<ix>(get_top_candidate());
    if (is_zero(top))
        return false;

    // ========================================================================
    const auto scope = store_.get_transactor();

    // Clean single allocation failure (e.g. disk full).
    return store_.candidate.truncate(top);
    // ========================================================================
}

TEMPLATE
bool CLASS::pop_confirmed() NOEXCEPT
{
    using ix = table::transaction::ix::integer;
    const auto top = system::possible_narrow_cast<ix>(get_top_confirmed());
    if (is_zero(top))
        return false;

    // ========================================================================
    const auto scope = store_.get_transactor();

    // Clean single allocation failure (e.g. disk full).
    return store_.confirmed.truncate(top);
    // ========================================================================
}

////// TEMP: testing cached values for confirmation.
////struct cached_point
////{
////    // input (under validation)
////    foreign_point key; // double-spendness
////    uint32_t parent;   // input.parent
////    uint32_t sequence; // bip68
////
////    // input->prevout
////    uint32_t tx;       // confirmedness
////    uint32_t height;   // bip68/maturity
////    uint32_t mtp;      // bip68
////    bool coinbase;     // maturity
////};
////// coinbase aligns at 4 bytes on msvc x64.
////////static_assert(sizeof(cached_point) == 32u);
////using cached_points = std_vector<cached_point>;
////bool create_cached_points(cached_points& out,
////    const header_link& link) const NOEXCEPT;
////TEMPLATE
////code CLASS::point_confirmable(const cached_point& point) const NOEXCEPT
////{
////    code ec{ error::success };
////
////    // height is always needed for coinbase (maturity).
////    // sequence value tells which is needed, height or mtp (bip68).
////    // but both may apply, so need to allow for two (no flags at prevout).
////    const context ctx{ 0, point.height, point.mtp };
////    if ((ec = unspendable_prevout(point.tx, point.coinbase, point.sequence, ctx)))
////        return ec;
////
////    // may only be strong-spent by self (and must be but is not checked).
////    if (spent_prevout(point.key, point.parent))
////        return error::confirmed_double_spend;
////
////    return ec;
////}
////
////TEMPLATE
////bool CLASS::create_cached_points(cached_points& out,
////    const header_link& link) const NOEXCEPT
////{
////    context ctx{};
////    if (!get_context(ctx, link))
////        return false;
////
////    table::input::get_prevout_sequence input{};
////    for (const auto& in: to_non_coinbase_inputs(link))
////    {
////        if (!store_.input.get(in, input))
////            return false;
////    
////        out.emplace_back
////        (
////            // input (under validation)
////            input.prevout(),
////            input.parent_fk,
////            input.sequence,
////
////            // input->prevout
////            to_tx(get_point_key(input.point_fk)),
////            system::possible_narrow_cast<uint32_t>(ctx.height),
////            ctx.mtp,
////            is_coinbase(spent_fk)
////        );
////    }
////
////    return true;
////}

} // namespace database
} // namespace libbitcoin

#endif
