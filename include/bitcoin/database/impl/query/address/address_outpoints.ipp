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
#ifndef LIBBITCOIN_DATABASE_QUERY_ADDRESS_OUTPOINTS_IPP
#define LIBBITCOIN_DATABASE_QUERY_ADDRESS_OUTPOINTS_IPP

#include <atomic>
#include <algorithm>
#include <utility>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

// Address outpoints.
// ----------------------------------------------------------------------------
// Address table is populated during transaction archival.

// server/native
TEMPLATE
code CLASS::get_confirmed_unspent_outputs(const stopper& cancel,
    outpoints& out, const hash_digest& key, bool turbo) const NOEXCEPT
{
    out.clear();
    output_links links{};
    if (const code ec = to_address_outputs(cancel, links, key))
        return ec;

    return parallel_outpoint_transform(cancel, turbo, out, links,
        [this](const auto& link, auto& cancel, auto& fail) NOEXCEPT -> outpoint
        {
            // !is_confirmed_unspent must be filtered out.
            if (cancel || fail || !is_confirmed_unspent(link))
                return {};

            const auto outpoint = get_outpoint(link);
            if (outpoint.point().is_null())
                fail = true;

            return outpoint;
        });
}

// unused
TEMPLATE
code CLASS::get_minimum_unspent_outputs(const stopper& cancel,
    outpoints& out, const hash_digest& key, uint64_t minimum,
    bool turbo) const NOEXCEPT
{
    out.clear();
    output_links links{};
    if (const code ec = to_address_outputs(cancel, links, key))
        return ec;

    return parallel_outpoint_transform(cancel, turbo, out, links,
        [this, minimum](const auto& link, auto& cancel, auto& fail) NOEXCEPT
            -> outpoint
        {
            // !is_confirmed_unspent must be filtered out.
            if (cancel || fail || !is_confirmed_unspent(link))
                return {};

            uint64_t value{};
            if (!get_value(value, link))
            {
                fail = true;
                return {};
            }

            // Must be filtered out.
            if (value < minimum)
                return {};

            const auto outpoint = get_outpoint(link);
            if (outpoint.point().is_null())
                fail = true;

            return outpoint;
        });
}

// server/native
TEMPLATE
code CLASS::get_address_outputs(const stopper& cancel, outpoints& out,
    const hash_digest& key, bool turbo) const NOEXCEPT
{
    out.clear();
    output_links links{};
    if (const code ec = to_address_outputs(cancel, links, key))
        return ec;

    return parallel_outpoint_transform(cancel, turbo, out, links,
        [this](const auto& link, auto& cancel, auto& fail) NOEXCEPT -> outpoint
        {
            if (cancel || fail)
                return {};

            const auto outpoint = get_outpoint(link);
            if (outpoint.point().is_null())
                fail = true;

            return outpoint;
        });
}

// utilities
// ----------------------------------------------------------------------------
// private/static

TEMPLATE
template <typename Functor>
code CLASS::parallel_outpoint_transform(const stopper& cancel, bool turbo,
    outpoints& out, const output_links& links, Functor&& functor) NOEXCEPT
{
    const auto policy = poolstl::execution::par_if(turbo);
    stopper fail{};

    out.clear();
    out.resize(links.size());

    std::transform(policy, links.cbegin(), links.cend(), out.begin(),
        [&functor, &cancel, &fail](const auto& link) NOEXCEPT
        {
            return functor(link, cancel, fail);
        });

    if (fail)
        return error::integrity;

    if (cancel)
        return error::canceled;

    // Remove default/null points.
    out.erase(std::remove_if(out.begin(), out.end(),
        [](const auto& outpoint) NOEXCEPT
        {
            return outpoint.point().is_null();
        }), out.end());

    // Sort (arbitrary - by index and then binary hash) and remove duplicates.
    std::sort(out.begin(), out.end());
    out.erase(std::unique(out.begin(), out.end()), out.end());
    return error::success;
}

} // namespace database
} // namespace libbitcoin

#endif
