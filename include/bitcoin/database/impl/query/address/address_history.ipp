/**
 * Copyright (c) 2011-2026 libbitcoin developers (see AUTHORS)
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
#ifndef LIBBITCOIN_DATABASE_QUERY_ADDRESS_HISTORY_IPP
#define LIBBITCOIN_DATABASE_QUERY_ADDRESS_HISTORY_IPP

#include <atomic>
#include <algorithm>
#include <ranges>
#include <utility>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

// Address history
// ----------------------------------------------------------------------------
// Canonically-sorted/deduped address history.
// root txs (height:zero) sorted before transitive (height:max) txs.
// tied-height transactions sorted by base16 txid (not converted).
// All confirmed txs are root, unconfirmed may or may not be root.

// server/electrum
TEMPLATE
code CLASS::get_unconfirmed_history(const stopper& cancel, histories& out,
    const hash_digest& key, bool turbo) const NOEXCEPT
{
    output_links outs{};
    if (const auto ec = to_address_outputs(cancel, outs, key))
        return ec;

    tx_links txs{};
    if (const auto ec = to_touched_txs(cancel, txs, outs))
        return ec;

    out.clear();
    out.resize(txs.size());
    return parallel_history_transform(cancel, turbo, out, txs,
        [this](const tx_link& link, auto& cancel, auto& fail) NOEXCEPT
        {
            if (cancel || fail)
                return history{};

            // chain::checkpoint invalid in default construction (filter).
            if (is_confirmed_block(find_strong(link)))
                return history{};

            auto hash = get_tx_key(link);
            if (hash == system::null_hash)
            {
                fail = true;
                return history{};
            }

            uint64_t fee{};
            auto height = history::unrooted_height;
            if (!get_tx_fee(fee, link))
                fee = history::missing_prevout;
            else if (is_confirmed_all_prevouts(link))
                height = history::rooted_height;

            return history{ { std::move(hash), height }, fee,
                history::unconfirmed_position };
        });
}

// ununsed
TEMPLATE
code CLASS::get_confirmed_history(const stopper& cancel, histories& out,
    const hash_digest& key, bool turbo) const NOEXCEPT
{
    output_links outs{};
    if (const auto ec = to_address_outputs(cancel, outs, key))
        return ec;

    tx_links txs{};
    if (const auto ec = to_touched_txs(cancel, txs, outs))
        return ec;

    out.clear();
    out.resize(txs.size());
    return parallel_history_transform(cancel, turbo, out, txs,
        [this](const tx_link& link, auto& cancel, auto& fail) NOEXCEPT
        {
            if (cancel || fail)
                return history{};

            // chain::checkpoint invalid in default construction (filter).
            const auto block = find_strong(link);
            if (!is_confirmed_block(block))
                return history{};

            size_t height{}, position{};
            auto hash = get_tx_key(link);
            if (hash == system::null_hash ||
                !get_height(height, block) ||
                !get_tx_position(position, link, block))
            {
                fail = true;
                return history{};
            }

            uint64_t fee{};
            if (!get_tx_fee(fee, link))
                fee = history::missing_prevout;

            return history{ { std::move(hash), height }, fee, position };
        });
}

// server/electrum
TEMPLATE
code CLASS::get_history(const stopper& cancel, histories& out,
    const hash_digest& key, bool turbo) const NOEXCEPT
{
    output_links outs{};
    if (const auto ec = to_address_outputs(cancel, outs, key))
        return ec;

    tx_links links{};
    if (const auto ec = to_touched_txs(cancel, links, outs))
        return ec;

    out.clear();
    out.resize(links.size());
    return parallel_history_transform(cancel, turbo, out, links,
        [this](const tx_link& link, auto& cancel, auto& fail) NOEXCEPT
        {
            if (cancel || fail)
                return history{};

            const auto out = get_tx_history(link);
            if (!out.valid())
                fail = true;

            return out;
        });
}

// History queries.
// ----------------------------------------------------------------------------

TEMPLATE
history CLASS::get_tx_history(const tx_link& link) const NOEXCEPT
{
    return get_tx_history(get_tx_key(link), link);
}

TEMPLATE
history CLASS::get_tx_history(const hash_digest& key) const NOEXCEPT
{
    const auto link = to_tx(key);
    return get_tx_history(hash_digest{ key }, link);
}

// private
TEMPLATE
history CLASS::get_tx_history(hash_digest&& key,
    const tx_link& link) const NOEXCEPT
{
    if (link.is_terminal())
        return {};

    uint64_t fee{};
    if (!get_tx_fee(fee, link))
        fee = history::missing_prevout;

    auto height = history::unrooted_height;
    auto position = history::unconfirmed_position;
    if (const auto block = find_confirmed_block(link); !block.is_terminal())
    {
        if (!get_height(height, block) ||
            !get_tx_position(position, link, block))
            return {};
    }
    else
    {
        if (is_confirmed_all_prevouts(link))
            height = history::rooted_height;
    }

    return { { std::move(key), height }, fee, position };
}

TEMPLATE
histories CLASS::get_spenders_history(
    const system::chain::point& prevout) const NOEXCEPT
{
    const auto ins = to_spenders(prevout);
    histories out(ins.size());
    for (const auto& in: std::views::reverse(ins))
        out.push_back(get_tx_history(to_input_tx(in)));

    history::filter_sort_and_dedup(out);
    return out;
}

TEMPLATE
histories CLASS::get_spenders_history(const hash_digest& key,
    uint32_t index) const NOEXCEPT
{
    return get_spenders_history({ key , index });
}

// utilities
// ----------------------------------------------------------------------------
// private/static

TEMPLATE
template <typename Functor>
code CLASS::parallel_history_transform(const stopper& cancel, bool turbo,
    histories& out, const tx_links& txs, Functor&& functor) NOEXCEPT
{
    const auto policy = poolstl::execution::par_if(turbo);
    stopper fail{};

    out.clear();
    out.resize(txs.size());
    std::transform(policy, txs.cbegin(), txs.cend(), out.begin(),
        [&functor, &cancel, &fail](const auto& tx) NOEXCEPT
        {
            return functor(tx, cancel, fail);
        });

    if (fail)
        return error::integrity;

    if (cancel)
        return error::canceled;

    history::filter_sort_and_dedup(out);
    return error::success;
}

} // namespace database
} // namespace libbitcoin

#endif
