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
        [this](const auto& , auto& cancel, auto& fail) NOEXCEPT -> unspent
        {
            if (cancel || fail)
                return {};

            // TODO: return unconfirmed unspent outputs for address key.
            return {};
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
        [this](const auto& , auto& cancel, auto& fail) NOEXCEPT -> unspent
        {
            if (cancel || fail)
                return {};

            // TODO: return confirmed unspent outputs for address key.
            return {};
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
        [this](const auto& , auto& cancel, auto& fail) NOEXCEPT -> unspent
        {
            if (cancel || fail)
                return {};

            // TODO: return unspent outputs for address key.
            return {};
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

    unspent::sort_and_dedup(out);
    return error::success;
}

} // namespace database
} // namespace libbitcoin

#endif
