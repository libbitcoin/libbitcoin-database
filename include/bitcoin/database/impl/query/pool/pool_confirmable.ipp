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
#ifndef LIBBITCOIN_DATABASE_QUERY_POOL_CONFIRMABLE_IPP
#define LIBBITCOIN_DATABASE_QUERY_POOL_CONFIRMABLE_IPP

#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

TEMPLATE
bool CLASS::is_confirmable_tx(const tx_link& link) const NOEXCEPT
{
    confirmables memo{};
    return is_confirmable_tx(memo, link);
}

TEMPLATE
bool CLASS::is_confirmable_spent(const output_link& link) const NOEXCEPT
{
    for (const auto& in: to_spenders(link))
    {
        const auto tx = to_input_tx(in);
        if (is_pooled(tx) && is_confirmable_tx(tx))
            return true;
    }

    return false;
}

TEMPLATE
ins_link CLASS::find_confirmable_spender(const point& prevout) const NOEXCEPT
{
    for (const auto& in: to_spenders(prevout))
    {
        const auto tx = to_input_tx(in);
        if (is_pooled(tx) && is_confirmable_tx(tx))
            return in;
    }

    return {};
}

// A terminal cursor starts at the first record, and the cursor is set to end
// once the records are exhausted.
TEMPLATE
code CLASS::get_confirmable_txs(pool_link& cursor, tx_links& out,
    const pool_link& end, size_t limit) const NOEXCEPT
{
    out.clear();
    auto fk = cursor.is_terminal() ? pool_link::integer{} : cursor.value;
    if (fk > end)
        return error::invalid_cursor;

    if (!store_.pool.enabled())
    {
        cursor = end;
        return error::success;
    }

    confirmables memo{};
    const auto stop = std::min(end.value, store_.pool.count().value);
    for (; fk < stop && out.size() < limit; ++fk)
    {
        const auto link = to_pool_tx(fk);
        if (!link.is_terminal() && is_confirmable_tx(memo, link))
            out.push_back(link);
    }

    cursor = fk == stop ? end : pool_link{ fk };
    return error::success;
}

TEMPLATE
code CLASS::get_confirmable_txs(tx_links& out) const NOEXCEPT
{
    out.clear();
    if (!store_.pool.enabled())
        return error::success;

    pool_link cursor{};
    return get_confirmable_txs(cursor, out, store_.pool.count(), max_size_t);
}

// An unconfirmed parent of a confirmable tx is itself confirmable.
TEMPLATE
tx_links CLASS::to_confirmable_parents(const tx_link& link) const NOEXCEPT
{
    tx_links out{};
    for (const auto& point: to_points(link))
    {
        const auto parent = to_tx(get_point_key(point).hash());
        if (!parent.is_terminal() && !is_confirmed_tx(parent) &&
            is_pooled(parent) && !system::contains(out, parent.value))
            out.push_back(parent.value);
    }

    return out;
}

TEMPLATE
tx_links CLASS::to_confirmable_children(const tx_link& link) const NOEXCEPT
{
    tx_links out{};
    for (const auto& output: to_outputs(link))
    {
        for (const auto& in: to_spenders(output))
        {
            const auto child = to_input_tx(in);
            if (!child.is_terminal() && is_pooled(child) &&
                is_confirmable_tx(child) &&
                !system::contains(out, child.value))
                out.push_back(child.value);
        }
    }

    return out;
}

TEMPLATE
tx_links CLASS::to_confirmable_ancestors(const tx_link& link) const NOEXCEPT
{
    return to_confirmable_relatives(link, true);
}

TEMPLATE
tx_links CLASS::to_confirmable_descendants(const tx_link& link) const NOEXCEPT
{
    return to_confirmable_relatives(link, false);
}

TEMPLATE
bool CLASS::get_ancestor_totals(fee_totals& out,
    const tx_link& link) const NOEXCEPT
{
    return get_confirmable_totals(out, link, true);
}

TEMPLATE
bool CLASS::get_descendant_totals(fee_totals& out,
    const tx_link& link) const NOEXCEPT
{
    return get_confirmable_totals(out, link, false);
}

TEMPLATE
bool CLASS::get_confirmable_fees(fee_rates& out) const NOEXCEPT
{
    out.clear();
    tx_links links{};
    if (get_confirmable_txs(links))
        return false;

    out.resize(links.size());
    auto rate = out.begin();
    for (const auto& link: links)
        if (!get_tx_fees(*rate++, link))
            return false;

    return true;
}

// A walk that finds no confirmed spend proves each tx visited confirmable.
TEMPLATE
bool CLASS::is_confirmable_tx(confirmables& memo,
    const tx_link& link) const NOEXCEPT
{
    if (const auto it = memo.find(link.value); it != memo.cend())
        return it->second;

    const auto unconfirmable = [&]() NOEXCEPT
    {
        memo.emplace(link.value, false);
        return false;
    };

    if (is_confirmed_tx(link))
        return unconfirmable();

    std::vector<tx_link::integer> pending{ link.value };
    std::unordered_set<tx_link::integer> visited{ link.value };
    while (!pending.empty())
    {
        const tx_link next{ pending.back() };
        pending.pop_back();

        if (const auto it = memo.find(next.value); it != memo.cend())
        {
            if (!it->second)
                return unconfirmable();

            continue;
        }

        for (const auto& point: to_points(next))
        {
            const auto prevout = get_point_key(point);
            if (!find_confirmed_spender(prevout).is_terminal())
                return unconfirmable();

            const auto parent = to_tx(prevout.hash());
            if (!parent.is_terminal() && !is_confirmed_tx(parent) &&
                visited.insert(parent.value).second)
                pending.push_back(parent.value);
        }
    }

    for (const auto tx: visited)
        memo.emplace(tx, true);

    return true;
}

TEMPLATE
bool CLASS::get_confirmable_totals(fee_totals& out, const tx_link& link,
    bool ancestors) const NOEXCEPT
{
    using namespace system;
    auto links = to_confirmable_relatives(link, ancestors);
    links.push_back(link.value);

    out = {};
    for (const auto fk: links)
    {
        fee_rate rate{};
        if (!get_tx_fees(rate, fk))
            return false;

        // A confirmed relative is not pooled (and has no sigops total).
        table::pool_total total{};
        const auto row = store_.pool.first(tx_link{ fk });
        if (!row.is_terminal() && !store_.pool.totals.get(row, total))
            return false;

        ++out.count;
        out.bytes = ceilinged_add(out.bytes, rate.bytes);
        out.sigops = ceilinged_add(out.sigops, total.sigops);
        out.fee = ceilinged_add(out.fee, rate.fee);
    }

    return true;
}

TEMPLATE
tx_links CLASS::to_confirmable_relatives(const tx_link& link,
    bool ancestors) const NOEXCEPT
{
    tx_links out{};
    std::vector<tx_link::integer> pending{ link.value };
    std::unordered_set<tx_link::integer> visited{ link.value };
    while (!pending.empty())
    {
        const tx_link next{ pending.back() };
        pending.pop_back();

        const auto links = ancestors ? to_confirmable_parents(next) :
            to_confirmable_children(next);

        for (const auto fk: links)
        {
            if (visited.insert(fk).second)
            {
                out.push_back(fk);
                pending.push_back(fk);
            }
        }
    }

    return out;
}

} // namespace database
} // namespace libbitcoin

#endif
