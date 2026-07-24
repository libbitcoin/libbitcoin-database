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
    const hash_digest& key, size_t limit, bool turbo) const NOEXCEPT
{
    tx_links txs{};
    if (const auto ec = get_address_txs(cancel, txs, key, limit))
        return ec;

    // There is no cursor for unconfirmed, since it's height-based.
    out.clear();
    out.resize(txs.size());
    return parallel_history_transform(cancel, turbo, out, txs,
        [this](const auto& link, auto& cancel, auto& fail) NOEXCEPT
        {
            if (cancel || fail) return history{};
            const auto out = this->get_tx_unconfirmed_history(link);
            if (out.fault()) fail = true;
            return out;
        });
}

// ununsed
TEMPLATE
code CLASS::get_confirmed_history(const stopper& cancel, height_link& cursor,
    histories& out, const hash_digest& key, size_t limit,
    bool turbo) const NOEXCEPT
{
    tx_links txs{};
    if (const auto ec = get_address_txs(cancel, txs, key, limit))
        return ec;

    // Cursor is still advanced in the case of (integrity) failure.
    // End is required because of the possiblity of intervening organization.
    const auto start = cursor.is_terminal() ? zero : cursor.value;
    const auto end = get_top_confirmed();
    cursor = system::possible_narrow_cast<height_link::integer>(add1(end));

    out.clear();
    out.resize(txs.size());
    return parallel_history_transform(cancel, turbo, out, txs,
        [this, start, end](const auto& link, auto& cancel, auto& fail) NOEXCEPT
        {
            if (cancel || fail) return history{};
            const auto out = this->get_tx_confirmed_history(link, start, end);
            if (out.fault()) fail = true;
            return out;
        });
}

// server/electrum
TEMPLATE
code CLASS::get_history(const stopper& cancel, height_link& cursor,
    histories& out, const hash_digest& key, size_t limit,
    bool turbo) const NOEXCEPT
{
    tx_links txs{};
    if (const auto ec = get_address_txs(cancel, txs, key, limit))
        return ec;

    // Cursor is still advanced in the case of (integrity) failure.
    // End is required because of the possiblity of intervening organization.
    const auto start = cursor.is_terminal() ? zero : cursor.value;
    const auto end = get_top_confirmed();
    cursor = system::possible_narrow_cast<height_link::integer>(add1(end));

    out.clear();
    out.resize(txs.size());
    return parallel_history_transform(cancel, turbo, out, txs,
        [this, start, end](const auto& link, auto& cancel, auto& fail) NOEXCEPT
        {
            if (cancel || fail) return history{};
            const auto out = this->get_tx_history(link, start, end);
            if (out.fault()) fail = true;
            return out;
        });
}

// get_tx_history
// ----------------------------------------------------------------------------

TEMPLATE
history CLASS::get_tx_history(const tx_link& link, size_t start,
    size_t end) const NOEXCEPT
{
    return get_tx_history(get_tx_key(link), link, start, end);
}

// protected
TEMPLATE
history CLASS::get_tx_history(hash_digest&& key, const tx_link& link,
    size_t start, size_t end) const NOEXCEPT
{
    // history is invalid in default construction.
    if (link.is_terminal())
        return {};

    // Electrum uses fees only on unconfirmed.
    auto fee = history::missing_prevout;
    auto height = history::unrooted_height;
    auto position = history::unconfirmed_position;

    const auto block = find_strong(link);
    const auto at = get_confirmed_height(block);
    if (at.is_terminal())
    {
        if (!get_tx_fee(fee, link))
            fee = history::missing_prevout;

        if (is_confirmed_all_prevouts(link))
            height = history::rooted_height;
    }
    else if (system::is_limited(at.value, start, end))
    {
        // Invalid with unconfirmed_position signals no fault.
        return { .position = history::unconfirmed_position };
    }
    else
    {
        height = at.value;
        if (!get_tx_position(position, link, block))
            return {};
    }

    return { { std::move(key), height }, fee, position };
}

TEMPLATE
history CLASS::get_tx_confirmed_history(const tx_link& link, size_t start,
    size_t end) const NOEXCEPT
{
    return get_tx_confirmed_history(get_tx_key(link), link, start, end);
}

// protected
TEMPLATE
history CLASS::get_tx_confirmed_history(hash_digest&& key, const tx_link& link,
    size_t start, size_t end) const NOEXCEPT
{
    // history is invalid in default construction with position.
    if (link.is_terminal())
        return {};

    const auto block = find_strong(link);
    const auto at = get_confirmed_height(block);

    // Invalid with unconfirmed_position signals no fault.
    if (at.is_terminal() || system::is_limited(at.value, start, end))
        return { .position = history::unconfirmed_position };

    size_t position{};
    if (!get_tx_position(position, link, block))
        return {};

    // Electrum uses fees only on unconfirmed (expensive).
    return { { std::move(key), at.value }, history::missing_prevout, position };
}

TEMPLATE
history CLASS::get_tx_unconfirmed_history(const tx_link& link) const NOEXCEPT
{
    return get_tx_unconfirmed_history(get_tx_key(link), link);
}

// protected
TEMPLATE
history CLASS::get_tx_unconfirmed_history(hash_digest&& key,
    const tx_link& link) const NOEXCEPT
{
    // history is invalid in default construction with position.
    if (link.is_terminal())
        return {};

    // Invalid with unconfirmed_position signals no fault.
    if (!get_confirmed_height(find_strong(link)).is_terminal())
        return { .position = history::unconfirmed_position };

    uint64_t fee{};
    if (!get_tx_fee(fee, link))
        fee = history::missing_prevout;

    const auto height = is_confirmed_all_prevouts(link) ?
        history::rooted_height : history::unrooted_height;

    return { { std::move(key), height }, fee, history::unconfirmed_position };
}

// get_spenders_history
// ----------------------------------------------------------------------------

// server/electrum
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

TEMPLATE
code CLASS::get_address_txs(const stopper& cancel, tx_links& out,
    const hash_digest& key, size_t limit) const NOEXCEPT
{
    output_links links{};
    address_link cursor{};
    if (const auto ec = to_address_outputs(cancel, cursor, links, key, limit))
        return ec;

    return to_touched_txs(cancel, out, links);
}

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
        return error::query_canceled;

    history::filter_sort_and_dedup(out);
    return error::success;
}

} // namespace database
} // namespace libbitcoin

#endif
