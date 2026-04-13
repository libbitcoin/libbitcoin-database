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
#ifndef LIBBITCOIN_DATABASE_QUERY_ADDRESS_UNSPENT_IPP
#define LIBBITCOIN_DATABASE_QUERY_ADDRESS_UNSPENT_IPP

#include <atomic>
#include <algorithm>
#include <utility>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

// Address unspent.
// ----------------------------------------------------------------------------
// A list of all unspent output transactions in canonical order.
// Unconfirmed unspent are included at end of list in consistent order.

// ununsed
TEMPLATE
code CLASS::get_unconfirmed_unspent(const stopper& cancel, unspents& out,
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
            if (cancel || fail)
                return unspent{};

            table::output::get_parent_value out{};
            if (!store_.output.get(link, out))
            {
                fail = true;
                return unspent{};
            }

            // chain::outpoint invalid in default construction (filter).
            const auto& tx = out.parent_fk;
            if (is_confirmed_block(find_strong(tx)))
                return unspent{};

            auto hash = get_tx_key(tx);
            const auto index = to_output_index(tx, link);
            if ((index == point::null_index) || (hash == system::null_hash))
            {
                fail = true;
                return unspent{};
            }

            return unspent{ { { std::move(hash), index }, out.value },
                unspent::unconfirmed_height, unspent::unconfirmed_position };
        });
}

// ununsed
TEMPLATE
code CLASS::get_confirmed_unspent(const stopper& cancel, unspents& out,
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
            if (cancel || fail)
                return unspent{};

            table::output::get_parent_value out{};
            if (!store_.output.get(link, out))
            {
                fail = true;
                return unspent{};
            }

            // chain::outpoint invalid in default construction (filter).
            const auto& tx = out.parent_fk;
            const auto block = find_strong(tx);
            if (!is_confirmed_block(block))
                return unspent{};

            size_t height{}, position{};
            auto hash = get_tx_key(tx);
            const auto index = to_output_index(tx, link);
            if ((index == point::null_index) || (hash == system::null_hash) ||
                !get_height(height, block) ||
                !get_tx_position(position, tx, block))
            {
                fail = true;
                return unspent{};
            }

            return unspent{ { { std::move(hash), index }, out.value }, height,
                position };
        });
}

// server/electrum
TEMPLATE
code CLASS::get_unspent(const stopper& cancel, unspents& out,
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
            if (cancel || fail)
                return unspent{};

            table::output::get_parent_value out{};
            if (!store_.output.get(link, out))
            {
                fail = true;
                return unspent{};
            }

            const auto& tx = out.parent_fk;
            auto hash = get_tx_key(tx);
            const auto index = to_output_index(tx, link);
            if ((index == point::null_index) || (hash == system::null_hash))
            {
                fail = true;
                return unspent{};
            }

            auto height = unspent::unconfirmed_height;
            auto position = unspent::unconfirmed_position;
            if (const auto block = find_strong(tx);
                is_confirmed_block(block))
            {
                if (!get_height(height, block) ||
                    !get_tx_position(position, tx, block))
                {
                    fail = true;
                    return unspent{};
                }
            }

            return unspent{ { { std::move(hash), index }, out.value }, height,
                position };
        });
}

// utilities
// ----------------------------------------------------------------------------
// private/static

TEMPLATE
template <typename Functor>
code CLASS::parallel_unspent_transform(const stopper& cancel, bool turbo,
    unspents& out, const output_links& outs, Functor&& functor) NOEXCEPT
{
    const auto policy = poolstl::execution::par_if(turbo);
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
        return error::canceled;

    unspent::filter_sort_and_dedup(out);
    return error::success;
}

} // namespace database
} // namespace libbitcoin

#endif
