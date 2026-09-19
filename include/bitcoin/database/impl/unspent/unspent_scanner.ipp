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
#ifndef LIBBITCOIN_DATABASE_UNSPENT_UNSPENT_SCANNER_IPP
#define LIBBITCOIN_DATABASE_UNSPENT_UNSPENT_SCANNER_IPP

#include <atomic>
#include <thread>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/memory.hpp>

namespace libbitcoin {
namespace database {

TEMPLATE
CLASS::unspent_scanner(const query<Store>& query, const Store& store,
    const stopper& cancel, bool turbo) NOEXCEPT
  : query_(query), store_(store), cancel_(cancel), turbo_(turbo)
{
}

TEMPLATE
code CLASS::scan(difference_set& set, unspent_totals& out,
    const header_links& branch) const NOEXCEPT
{
    if (branch.empty())
        return error::success;

    std_vector<size_t> bounds{};
    if (const auto ec = this->bounds(bounds, branch))
        return ec;

    const auto cuts = sub1(bounds.size());
    std_vector<uint64_t> unspendables(cuts, zero);
    std_vector<uint64_t> overwrittens(cuts, zero);
    std::atomic_bool fail{};

    // Static chunking is contiguous, so spans are interleaved across threads.
    const auto width = cores();
    std_vector<size_t> index{};
    index.reserve(cuts);
    for (size_t lane{}; lane < width; ++lane)
        for (auto at = lane; at < cuts; at += width)
            index.push_back(at);

    const auto parallel = poolstl::execution::par_if(turbo_);
    std::for_each(parallel, index.cbegin(), index.cend(),
        [&](size_t at) NOEXCEPT
        {
            if (fail)
                return;

            if (span(set, unspendables.at(at), overwrittens.at(at), branch,
                bounds.at(at), bounds.at(add1(at))))
                fail = true;
        });

    if (fail)
        return cancel_ ? error::query_canceled : error::integrity;

    for (size_t at{}; at < cuts; ++at)
    {
        out.unspendable += unspendables.at(at);
        out.bip30 += overwrittens.at(at);
    }

    return error::success;
}

// Spans are cut to equal tx counts, as blocks vary by orders of magnitude.
TEMPLATE
code CLASS::bounds(std_vector<size_t>& out,
    const header_links& branch) const NOEXCEPT
{
    BC_ASSERT(out.empty());

    std_vector<size_t> counts(branch.size());
    std_vector<size_t> blocks(branch.size());
    std::iota(blocks.begin(), blocks.end(), zero);
    const auto parallel = poolstl::execution::par_if(turbo_);

    std::for_each(parallel, blocks.cbegin(), blocks.cend(),
        [&](size_t block) NOEXCEPT
        {
            counts.at(block) = query_.get_tx_count(branch.at(block));
        });

    size_t txs{};
    for (const auto count: counts)
        txs += count;

    const auto width = cores();
    const auto spans = std::min(branch.size(), width * per_thread);
    const auto per_span = std::max(one, system::ceilinged_divide(txs, spans));

    out.reserve(add1(spans));
    out.push_back(zero);
    size_t carry{};

    for (size_t block{}; block < branch.size(); ++block)
    {
        carry += counts.at(block);
        if (carry >= per_span && out.size() <= spans)
        {
            out.push_back(add1(block));
            carry = zero;
        }
    }

    if (out.back() != branch.size())
        out.push_back(branch.size());

    return error::success;
}

TEMPLATE
code CLASS::span(difference_set& set, uint64_t& unspendable,
    uint64_t& overwritten, const header_links& branch, size_t begin,
    size_t end) const NOEXCEPT
{
    buffers buffer{};
    for (auto at = begin; at < end; ++at)
    {
        if (cancel_)
            return error::query_canceled;

        if (const auto ec = block(set, unspendable, overwritten, buffer,
            branch.at(at)))
            return ec;
    }

    return error::success;
}

TEMPLATE
code CLASS::block(difference_set& set, uint64_t& unspendable,
    uint64_t& overwritten, buffers& buffer,
    const header_link& link) const NOEXCEPT
{
    auto bip30_exception = false;
    if (!query_.is_bip30_exception(bip30_exception, link))
        return error::integrity;

    if (bip30_exception)
        return this->overwritten(overwritten, link);

    if (const auto ec = read_transactions(buffer.txs, link))
        return ec;

    if (const auto ec = toggle_creates(set, buffer.exclusions, buffer.txs))
        return ec;

    if (const auto ec = read_exclusions(unspendable, buffer.exclusions))
        return ec;

    if (const auto ec = read_points(buffer.spends, buffer.txs))
        return ec;

    return toggle_spends(set, buffer.spends);
}

// The bip30 exception blocks are coinbase only, their duplicated outputs
// overwrite (destroy) those of the original blocks.
TEMPLATE
code CLASS::overwritten(uint64_t& out, const header_link& link) const NOEXCEPT
{
    table::output::get_spendable value{};
    for (const auto& tx: query_.to_transactions(link))
    {
        for (const auto& put: query_.to_outputs(tx))
        {
            if (!store_.output.get(put, value))
                return error::integrity;

            out += value.value;
        }
    }

    return error::success;
}

TEMPLATE
code CLASS::read_transactions(gets& out,
    const header_link& link) const NOEXCEPT
{
    const auto links = query_.to_transactions(link);
    out.resize(links.size());

    const auto ptr = store_.tx.get_memory();
    for (size_t at{}; at < links.size(); ++at)
        if (!store_.tx.get(ptr, links.at(at), out.at(at)))
            return error::integrity;

    return error::success;
}

TEMPLATE
code CLASS::toggle_creates(difference_set& set, output_links& exclusions,
    const gets& txs) const NOEXCEPT
{
    exclusions.clear();

    const auto ptr = store_.outs.puts.get_memory();
    for (const auto& tx: txs)
    {
        table::outs::get_excluded outs{ .number = tx.outs_count };
        if (!store_.outs.puts.get(ptr, tx.outs_fk, outs))
            return error::integrity;

        auto excluded = outs.excluded.cbegin();
        for (uint32_t index{}; index < outs.number; ++index)
        {
            if (excluded != outs.excluded.cend() && excluded->first == index)
                exclusions.push_back((excluded++)->second);
            else
                set.toggle(tx.outs_fk + index);
        }
    }

    return error::success;
}

TEMPLATE
code CLASS::read_exclusions(uint64_t& out,
    const output_links& exclusions) const NOEXCEPT
{
    const auto ptr = store_.output.get_memory();
    for (const auto& put: exclusions)
    {
        table::output::get_spendable value{};
        if (!store_.output.get(ptr, put, value))
            return error::integrity;

        out += value.value;
    }

    return error::success;
}

TEMPLATE
code CLASS::read_points(system::chain::points& out,
    const gets& txs) const NOEXCEPT
{
    out.clear();

    const auto ptr = store_.ins.get_memory();
    for (const auto& tx: txs)
    {
        if (tx.coinbase)
            continue;

        for (ins_link::integer in{}; in < tx.ins_count; ++in)
        {
            table::ins_point::get_composed point{};
            if (!store_.ins.get(ptr, tx.points_fk + in, point))
                return error::integrity;

            out.push_back(point.key);
        }
    }

    return error::success;
}

// As validation, the first tx of the hash resolves the spend.
TEMPLATE
code CLASS::toggle_spends(difference_set& set,
    const system::chain::points& spends) const NOEXCEPT
{
    const auto ptr = store_.tx.get_memory();
    for (const auto& spend: spends)
    {
        const auto spent = store_.tx.first(ptr, spend.hash());
        if (spent.is_terminal())
            return error::integrity;

        table::transaction::get_output prevout{ {}, spend.index() };
        if (!store_.tx.get(ptr, spent, prevout) ||
            prevout.outs_fk == outs_link::terminal)
            return error::integrity;

        set.toggle(prevout.outs_fk);
    }

    return error::success;
}

} // namespace database
} // namespace libbitcoin

#endif
