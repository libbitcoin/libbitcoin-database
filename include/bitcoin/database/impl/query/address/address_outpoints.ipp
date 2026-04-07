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
code CLASS::get_address_outputs(stopper& cancel, outpoints& out,
    const hash_digest& key, bool turbo) const NOEXCEPT
{
    if (turbo && store_.turbo())
        return get_address_outputs_turbo(cancel, out, key);

    out.clear();
    for (auto it = store_.address.it(key); it; ++it)
    {
        if (cancel)
            return error::canceled;

        table::address::record address{};
        if (!store_.address.get(it, address))
            return error::integrity;

        out.insert(get_outpoint(address.output_fk));
    }

    return error::success;
}

// server/native
TEMPLATE
code CLASS::get_confirmed_unspent_outputs(stopper& cancel,
    outpoints& out, const hash_digest& key, bool turbo) const NOEXCEPT
{
    if (turbo && store_.turbo())
        return get_confirmed_unspent_outputs_turbo(cancel, out, key);

    out.clear();
    for (auto it = store_.address.it(key); it; ++it)
    {
        if (cancel)
            return error::canceled;

        table::address::record address{};
        if (!store_.address.get(it, address))
            return error::integrity;

        if (is_confirmed_unspent(address.output_fk))
            out.insert(get_outpoint(address.output_fk));
    }

    return error::success;
}

// unused
TEMPLATE
code CLASS::get_minimum_unspent_outputs(stopper& cancel,
    outpoints& out, const hash_digest& key, uint64_t minimum,
    bool turbo) const NOEXCEPT
{
    if (turbo && store_.turbo())
        return get_minimum_unspent_outputs_turbo(cancel, out, key, minimum);

    out.clear();
    for (auto it = store_.address.it(key); it; ++it)
    {
        if (cancel)
            return error::canceled;

        table::address::record address{};
        if (!store_.address.get(it, address))
            return error::integrity;

        if (is_confirmed_unspent(address.output_fk))
        {
            uint64_t value{};
            if (!get_value(value, address.output_fk))
                return error::integrity;

            if (value >= minimum)
                out.insert(get_outpoint(address.output_fk));
        }
    }

    return error::success;
}

// turbos
// ----------------------------------------------------------------------------
// protected

TEMPLATE
code CLASS::get_address_outputs_turbo(stopper& cancel, outpoints& out,
    const hash_digest& key) const NOEXCEPT
{
    out.clear();
    output_links links{};
    if (const code ec = to_address_outputs(cancel, links, key))
        return ec;

    return parallel_address_transform(cancel, out, links,
        [this](const auto& link, auto& cancel, auto& fail) NOEXCEPT
        {
            if (cancel || fail) return outpoint{};
            auto outpoint = get_outpoint(link);
            fail = outpoint.point().is_null();
            return outpoint;
        });
}

TEMPLATE
code CLASS::get_confirmed_unspent_outputs_turbo(stopper& cancel,
    outpoints& out, const hash_digest& key) const NOEXCEPT
{
    out.clear();
    output_links links{};
    if (const code ec = to_address_outputs(cancel, links, key))
        return ec;

    return parallel_address_transform(cancel, out, links,
        [this](const auto& link, auto& cancel, auto& fail) NOEXCEPT
        {
            if (cancel || fail || !is_confirmed_unspent(link))
                return outpoint{};

            auto outpoint = get_outpoint(link);
            fail = outpoint.point().is_null();
            return outpoint;
        });
}

TEMPLATE
code CLASS::get_minimum_unspent_outputs_turbo(stopper& cancel,
    outpoints& out, const hash_digest& key, uint64_t minimum) const NOEXCEPT
{
    out.clear();
    output_links links{};
    if (const code ec = to_address_outputs(cancel, links, key))
        return ec;

    return parallel_address_transform(cancel, out, links,
        [this, minimum](const auto& link, auto& cancel, auto& fail) NOEXCEPT
        {
            if (cancel || fail || !is_confirmed_unspent(link))
                return outpoint{};

            uint64_t value{};
            if (!get_value(value, link))
            {
                fail = true;
                return outpoint{};
            }

            if (value < minimum)
                return outpoint{};

            auto outpoint = get_outpoint(link);
            fail = outpoint.point().is_null();
            return outpoint;
        });
}


// utilities
// ----------------------------------------------------------------------------
// private/static

TEMPLATE
template <typename Functor>
inline code CLASS::parallel_address_transform(stopper& cancel,
    outpoints& out, const output_links& links, Functor&& functor) NOEXCEPT
{
    constexpr auto parallel = poolstl::execution::par;

    stopper fail{};
    std::vector<outpoint> outpoints(links.size());
    std::transform(parallel, links.begin(), links.end(), outpoints.begin(),
        [&functor, &cancel, &fail](const auto& link) NOEXCEPT
        {
            return functor(link, cancel, fail);
        });

    out.clear();
    if (fail) return error::integrity;
    if (cancel) return error::canceled;
    for (auto& outpoint: outpoints)
    {
        if (cancel) return error::canceled;
        if (!outpoint.point().is_null())
            out.insert(std::move(outpoint));
    }

    return error::success;
}

} // namespace database
} // namespace libbitcoin

#endif
