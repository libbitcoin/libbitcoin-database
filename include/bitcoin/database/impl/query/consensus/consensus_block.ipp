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
#ifndef LIBBITCOIN_DATABASE_QUERY_CONSENSUS_BLOCK_IPP
#define LIBBITCOIN_DATABASE_QUERY_CONSENSUS_BLOCK_IPP

#include <atomic>
#include <ranges>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/types/types.hpp>

namespace libbitcoin {
namespace database {

// Called by organizer under strict order to determine if block is confirmable.
// ----------------------------------------------------------------------------

TEMPLATE
code CLASS::block_confirmable(const header_link& link) const NOEXCEPT
{
    constexpr auto parallel = poolstl::execution::par;
    constexpr auto relaxed = std::memory_order_relaxed;

    context ctx{};
    if (!get_context(ctx, link))
        return error::integrity_block_confirmable1;

    // bip30 coinbase check (see notes).
    ////code ec{};
    ////if ((ec = spent_duplicates(link, ctx)))
    ////    return ec;

    // Coinbase txs are not populated.
    const auto txs = to_spending_txs(link);
    if (txs.empty())
        return error::success;

    // One point set per tx.
    point_sets sets(txs.size());
    std::atomic<size_t> count{};
    stopper fault{};

    // Get points for each tx and the total count.
    std::transform(parallel, txs.cbegin(), txs.cend(), sets.begin(), 
        [this, &count, &fault](const tx_link& tx) NOEXCEPT
        {
            point_set set{};
            table::transaction::get_set_ref get{ {}, set.version, set.points };
            if (!store_.tx.get(tx, get))
                fault.store(true, relaxed);

            count.fetch_add(set.points.size(), relaxed);
            return set;
        });

    if (fault.load(relaxed))
        return error::integrity_block_confirmable2;

    // Returns database (integrity) or system (consensus) code.
    // Checks double spends strength, populates prevout parent tx/cb/sq links.
    if (const auto ec = get_prevouts(sets, count.load(relaxed), link))
        return ec;

    // Code non-integral (no atomic), so codes must be system::error.
    std::atomic<system::error::transaction_error_t> consensus{};

    // Checks all spends for spendability (strong, unlocked and mature).
    if (std::all_of(parallel, sets.cbegin(), sets.cend(),
        [this, &ctx, &consensus](const point_set& set) NOEXCEPT
        {
            for (const auto& point: set.points)
            {
                if (const auto ec = spendable(point, set.version, ctx))
                {
                    consensus.store(ec, relaxed);
                    return false;
                }
            }

            return true;
        })) return error::success;

    // Recast previous_output_null as database integrity error.
    const auto result = consensus.load(relaxed);
    if (result == system::error::previous_output_null)
        return error::integrity_spendable;

    return result;
}

// Called by validator for a current block, prior to block production.
// ----------------------------------------------------------------------------
// Each non-coinbase tx is validated under its pooled context, which is
// sufficient for the block (absolute locktime is monotonic under bip113). What
// remains is the coinbase, the block checks over all txs, and the prevouts
// row, from which block_confirmable performs the chain checks.

TEMPLATE
code CLASS::validate_pooled(const header_link& link, const chain_context& ctx,
    uint64_t subsidy_interval, uint64_t initial_subsidy) NOEXCEPT
{
    using namespace system;
    using prevout = table::prevout;
    if (!store_.pool.enabled())
        return error::unvalidated;

    const auto txs = to_transactions(link);
    if (txs.empty())
        return error::integrity;

    const auto count = txs.size();
    std::vector<table::transaction::record> records(count);
    for (size_t index{}; index < count; ++index)
        if (!store_.tx.get(txs.at(index), records.at(index)))
            return error::integrity;

    // Sufficiency (early, as insufficiency implies full validation).
    uint64_t fees{};
    size_t sigops{};
    const auto pool = context::from(ctx);
    std::vector<pooled_tx> pooled(count);
    for (size_t index = one; index < count; ++index)
    {
        auto& tx = pooled.at(index);
        tx.prevouts.resize(records.at(index).ins_count);
        if (const auto ec = get_pooled(tx, txs.at(index), pool))
            return ec;

        fees = ceilinged_add(fees, tx.fee);
        sigops = ceilinged_add(sigops, tx.sigops);
    }

    // Block check.
    // ------------------------------------------------------------------------

    size_t light{}, heavy{};
    if (!get_block_sizes(light, heavy, link))
        return error::integrity;

    if (light > chain::max_block_size)
        return system::error::block_size_limit;

    if (!records.front().coinbase)
        return system::error::first_not_coinbase;

    for (size_t index = one; index < count; ++index)
        if (records.at(index).coinbase)
            return system::error::extra_coinbases;

    std::unordered_map<hash_digest, size_t> positions{};
    positions.reserve(count);
    for (size_t index{}; index < count; ++index)
        positions.emplace(get_tx_key(txs.at(index)), index);

    // Points, spends (parent|terminal, sequence), and conflicts (doubles).
    using spend = prevout::slab_put_spends::spend;
    std::unordered_set<chain::point> points{};
    std::vector<spend> spends{};
    tx_links conflicts{};
    const auto doubles = !is_zero(store_.duplicate.body_size());
    for (size_t index = one; index < count; ++index)
    {
        const auto& record = records.at(index);
        auto parent = pooled.at(index).prevouts.cbegin();
        const auto end = record.point_fk + record.ins_count;
        for (auto fk = record.point_fk; fk < end; ++fk, ++parent)
        {
            const auto point = get_point_key(fk);
            if (!points.insert(point).second)
                return system::error::block_internal_double_spend;

            table::ins_sequence::get_input input{};
            if (!store_.ins.sequence.get(fk, input))
                return error::integrity;

            // An internal spend (by hash) is terminal, as confirmation of the
            // pooled parent would otherwise be required.
            auto merged = prevout::tx::terminal;
            if (const auto it = positions.find(point.hash());
                it == positions.end())
                merged = prevout::merge(parent->coinbase, parent->parent);
            else if (it->second >= index)
                return system::error::forward_reference;
            else if (is_zero(it->second))
                return system::error::coinbase_maturity;

            spends.emplace_back(merged, input.sequence);
            if (doubles && !get_doubles(conflicts, point))
                return error::integrity;
        }
    }

    // Block check (context).
    // ------------------------------------------------------------------------

    const auto bip16 = ctx.is_enabled(chain::flags::bip16_rule);
    const auto bip34 = ctx.is_enabled(chain::flags::bip34_rule);
    const auto bip42 = ctx.is_enabled(chain::flags::bip42_rule);
    const auto bip50 = ctx.is_enabled(chain::flags::bip50_rule);
    const auto bip141 = ctx.is_enabled(chain::flags::bip141_rule);

    const auto weight = ceilinged_add(
        ceilinged_multiply(chain::base_size_contribution, light),
        ceilinged_multiply(chain::total_size_contribution, heavy));
    if (bip141 && weight > chain::max_block_weight)
        return system::error::block_weight_limit;

    const auto coinbase = get_transaction(txs.front(), true);
    if (!coinbase)
        return error::integrity;

    if (const auto ec = coinbase->check())
        return ec;

    if (const auto ec = coinbase->check(ctx))
        return ec;

    const auto& ins = *coinbase->inputs_ptr();
    if (bip34 && !ins.empty() &&
        !chain::script::is_coinbase_pattern(ins.front()->script().ops(),
            ctx.height))
        return system::error::coinbase_height_mismatch;

    if (bip50)
    {
        std::unordered_set<hash_digest> hashes{};
        for (const auto& position: positions)
            hashes.insert(position.first);

        for (const auto& point: points)
            hashes.insert(point.hash());

        if (hashes.size() > chain::hash_limit)
            return system::error::temporary_hash_limit;
    }

    // Block accept.
    // ------------------------------------------------------------------------

    const auto subsidy = chain::block::subsidy(ctx.height, subsidy_interval,
        initial_subsidy, bip42);
    if (coinbase->spend() > ceilinged_add(fees, subsidy))
        return system::error::coinbase_value_limit;

    sigops = ceilinged_add(sigops, coinbase->signature_operations(bip16,
        bip141));
    if (sigops > (bip141 ? chain::max_fast_sigops : chain::max_block_sigops))
        return system::error::block_sigop_limit;

    // ========================================================================
    const auto scope = get_transactor();

    // Clean single allocation failure (e.g. disk full).
    return store_.prevout.put(to_prevout(link), prevout::slab_put_spends
    {
        {},
        conflicts,
        spends
    }) ? error::success : error::prevouts_put;
    // ========================================================================
}

// utility
// ----------------------------------------------------------------------------
// All return codes must be system::error::transaction_error_t (see atomic).

TEMPLATE
system::error::transaction_error_t CLASS::spendable(
    const point_set::point& point, uint32_t version,
    const context& ctx) const NOEXCEPT
{
    const auto bip68 = ctx.is_enabled(system::chain::flags::bip68_rule);

    // A spend internal to the block has zero age and a non-coinbase parent
    // (the merged coinbase bit of a terminal point is not meaningful).
    if (point.tx.is_terminal())
    {
        if (bip68 &&
            transaction::is_relative_locktime_applied(false, version,
                point.sequence) &&
            input::is_relative_locked(point.sequence, ctx.height, ctx.mtp,
                ctx.height, ctx.mtp))
            return system::error::relative_time_locked;

        return system::error::transaction_success;
    }

    const auto link = find_strong(point.tx);
    if (link.is_terminal())
        return system::error::unconfirmed_spend;

    // Avoids get_context call when relative locktime is not applicable.
    const auto relative = bip68 && transaction::is_relative_locktime_applied(
        point.coinbase, version, point.sequence);

    if (relative || point.coinbase)
    {
        context prevout{};
        if (!get_context(prevout, link))
            return system::error::previous_output_null;

        if (relative &&
            input::is_relative_locked(point.sequence, ctx.height, ctx.mtp,
                prevout.height, prevout.mtp))
            return system::error::relative_time_locked;

        if (point.coinbase &&
            transaction::is_coinbase_immature(prevout.height, ctx.height))
            return system::error::coinbase_maturity;
    }

    return system::error::transaction_success;
}

// ****************************************************************************
// CONSENSUS: To reproduce the behavior of a UTXO accumulator when reorganizing
// a BIP30 exception block, the first instance of the reorganized coinbase tx
// must be set unstrong, despite its block being strong. This creates the odd
// situation where there is a confirmed block with unconfirmed txs. Otherwise
// the txs are spendable, but in the satoshi client their outputs no longer
// exist (de-accumulated). There is discussion about fixing this issue in the
// satoshi client, which would likely result in our behavior without this
// special handling. This is moot given the existence of checkpoints, so
// presently not consensus.
// ****************************************************************************

} // namespace database
} // namespace libbitcoin

#endif
