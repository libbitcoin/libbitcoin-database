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
#ifndef LIBBITCOIN_DATABASE_QUERY_ADDRESS_UNSPENT_IPP
#define LIBBITCOIN_DATABASE_QUERY_ADDRESS_UNSPENT_IPP

#include <atomic>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

// Address unspent.
// ----------------------------------------------------------------------------
// A list of all unspent output transactions in canonical order.
// Unconfirmed unspent are included at end of list in consistent order.

// ununsed
// Only unconfirmed outputs that are not spent (by an unconfirmed tx).
TEMPLATE
code CLASS::get_unconfirmed_unspent(const stopper& cancel, unspent_outputs& out,
    const hash_digest& key, bool turbo) const NOEXCEPT
{
    output_links outs{};
    if (const auto ec = to_address_outputs(cancel, outs, key))
        return ec;

    out.clear();
    out.resize(outs.size());
    return parallel_unspent_transform(cancel, turbo, out, outs,
        [this](const output_link& link, auto& cancel, auto& fail) NOEXCEPT
        {
            // Exclude if spent by any tx. Given that output must be confirmed
            // this is only unconfirmeds, but checking both via is_spent() is
            // much faster than calling is_unconfirmed_spent().
            if (cancel || fail || is_spent(link))
                return unspent_output{};

            const auto out = get_tx_unconfirmed_unspent(link);
            if (out.fault()) fail = true;
            return out;
        });
}

// ununsed
// Only confirmed outputs that are not spent by a confirmed tx.
TEMPLATE
code CLASS::get_confirmed_unspent(const stopper& cancel, unspent_outputs& out,
    const hash_digest& key, bool turbo) const NOEXCEPT
{
    output_links outs{};
    if (const auto ec = to_address_outputs(cancel, outs, key))
        return ec;

    out.clear();
    out.resize(outs.size());
    return parallel_unspent_transform(cancel, turbo, out, outs,
        [this](const output_link& link, auto& cancel, auto& fail) NOEXCEPT
        {
            // Exclude if spent by confirmed tx, ignore unconfirmed spenders.
            if (cancel || fail || is_confirmed_spent(link))
                return unspent_output{};

            const auto out = get_tx_confirmed_unspent(link);
            if (out.fault()) fail = true;
            return out;
        });
}

// server/electrum
// All outputs that are not spent by any tx (confirmed or unconfirmed).
TEMPLATE
code CLASS::get_unspent(const stopper& cancel, unspent_outputs& out,
    const hash_digest& key, bool turbo) const NOEXCEPT
{
    output_links outs{};
    if (const auto ec = to_address_outputs(cancel, outs, key))
        return ec;

    out.clear();
    out.resize(outs.size());
    return parallel_unspent_transform(cancel, turbo, out, outs,
        [this](const output_link& link, auto& cancel, auto& fail) NOEXCEPT
        {
            // Exclude if spent by any tx, confirmed or unconfirmed.
            if (cancel || fail || is_spent(link))
                return unspent_output{};

            auto out = get_tx_unspent(link);
            if (out.fault()) fail = true;
            return out;
        });
}

// Unspent queries.
// ----------------------------------------------------------------------------

TEMPLATE
unspent_output CLASS::get_tx_unspent(const output_link& link) const NOEXCEPT
{
    table::output::get_parent_value out{};
    if (!store_.output.get(link, out))
        return {};

    const auto tx = out.parent_fk;
    auto hash = get_tx_key(tx);
    const auto index = to_output_index(tx, link);
    if ((index == point::null_index) || (hash == system::null_hash))
        return {};

    auto height = unspent_output::unused_height;
    auto position = unspent_output::unconfirmed_position;

    tx_link strong{};
    const auto block = find_strong(strong, tx);
    const auto at = get_confirmed_height(block);
    if (!at.is_terminal())
    {
        height = at.value;
        if (!get_tx_position(position, strong, block))
            return {};
    }

    return { { { std::move(hash), index }, out.value }, height, position };
}

TEMPLATE
unspent_output CLASS::get_tx_confirmed_unspent(const output_link& link) const NOEXCEPT
{
    // unspent_output is invalid in default construction with position.
    table::output::get_parent_value out{};
    if (!store_.output.get(link, out))
        return {};

    const auto tx = out.parent_fk;
    tx_link strong{};
    const auto block = find_strong(strong, tx);
    const auto at = get_confirmed_height(block);

    // Invalid with unconfirmed_position signals no fault.
    if (at.is_terminal())
        return { .position = unspent_output::unconfirmed_position };

    size_t position{};
    auto hash = get_tx_key(tx);
    const auto index = to_output_index(tx, link);
    if ((index == point::null_index) || (hash == system::null_hash) ||
        !get_tx_position(position, strong, block))
        return {};

    return { { { std::move(hash), index }, out.value }, at.value, position };
}

TEMPLATE
unspent_output CLASS::get_tx_unconfirmed_unspent(const output_link& link) const NOEXCEPT
{
    // unspent_output is invalid in default construction with position.
    table::output::get_parent_value out{};
    if (!store_.output.get(link, out))
        return {};

    const auto tx = out.parent_fk;

    // Invalid with unconfirmed_position signals no fault.
    if (!get_confirmed_height(find_strong(tx)).is_terminal())
        return { .position = unspent_output::unconfirmed_position };

    auto hash = get_tx_key(tx);
    const auto index = to_output_index(tx, link);
    if ((index == point::null_index) || (hash == system::null_hash))
        return {};

    return { { { std::move(hash), index }, out.value }, unspent_output::unused_height,
        unspent_output::unconfirmed_position };
}

// utilities
// ----------------------------------------------------------------------------
// private/static

TEMPLATE
template <typename Functor>
code CLASS::parallel_unspent_transform(const stopper& cancel, bool turbo,
    unspent_outputs& out, const output_links& outs, Functor&& functor) NOEXCEPT
{
    const auto policy = poolstl::execution::par_if(turbo, parallel_floor);
    stopper fail{};

    out.clear();
    out.resize(outs.size());
    std::transform(policy, outs.cbegin(), outs.cend(), out.begin(),
        [&functor, &cancel, &fail](const auto& output) NOEXCEPT
        {
            return functor(output, cancel, fail);
        });

    if (fail)
        return error::integrity;

    if (cancel)
        return error::query_canceled;

    unspent_output::filter_sort_and_dedup(out);
    return error::success;
}

} // namespace database
} // namespace libbitcoin

#endif
