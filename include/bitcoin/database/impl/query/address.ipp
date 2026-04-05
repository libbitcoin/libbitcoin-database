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
#ifndef LIBBITCOIN_DATABASE_QUERY_ADDRESS_IPP
#define LIBBITCOIN_DATABASE_QUERY_ADDRESS_IPP

#include <atomic>
#include <algorithm>
#include <utility>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

// Address (natural-keyed).
// ----------------------------------------------------------------------------
// Address table is populated during transaction archival.

TEMPLATE
code CLASS::to_address_outputs(std::atomic_bool& cancel, output_links& out,
    const hash_digest& key) const NOEXCEPT
{
    // Pushing into the vector is more efficient than precomputation of size.
    out.clear();
    for (auto it = store_.address.it(key); it; ++it)
    {
        if (cancel)
            return error::canceled;

        table::address::record address{};
        if (!store_.address.get(it, address))
            return error::integrity;

        out.push_back(address.output_fk);
    }

    return error::success;
}

// server/native
TEMPLATE
code CLASS::get_address_outputs(std::atomic_bool& cancel, outpoints& out,
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

        out.insert(get_spent(address.output_fk));
    }

    return error::success;
}

// protected
TEMPLATE
code CLASS::get_confirmed_unspent_outputs_turbo(std::atomic_bool& cancel,
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

            auto outpoint = get_spent(link);
            fail = (outpoint.point().index() == point::null_index);
            return outpoint;
        });
}

// server/native
TEMPLATE
code CLASS::get_confirmed_unspent_outputs(std::atomic_bool& cancel,
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
            out.insert(get_spent(address.output_fk));
    }

    return error::success;
}

// protected
TEMPLATE
code CLASS::get_minimum_unspent_outputs_turbo(std::atomic_bool& cancel,
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

            auto outpoint = get_spent(link);
            fail = (outpoint.point().index() == point::null_index);
            return outpoint;
        });
}

// unused
TEMPLATE
code CLASS::get_minimum_unspent_outputs(std::atomic_bool& cancel,
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
                out.insert(get_spent(address.output_fk));
        }
    }

    return error::success;
}

// server/native
TEMPLATE
code CLASS::get_confirmed_balance(std::atomic_bool& cancel, uint64_t& balance,
    const hash_digest& key, bool turbo) const NOEXCEPT
{
    outpoints outs{};
    if (const auto ec = get_confirmed_unspent_outputs(cancel, outs, key, turbo))
    {
        balance = zero;
        return ec;
    }

    // Use of to_confirmed_unspent_outputs() provides necessary deduplication.
    balance = std::accumulate(outs.begin(), outs.end(), zero,
        [](size_t total, const outpoint& out) NOEXCEPT
        {
            return system::ceilinged_add(total, out.value());
        });

    return error::success;
}

// utilities
// ----------------------------------------------------------------------------

// private/static
TEMPLATE
template <typename Functor>
inline code CLASS::parallel_address_transform(std::atomic_bool& cancel,
    outpoints& out, const output_links& links, Functor&& functor) NOEXCEPT
{
    constexpr auto parallel = poolstl::execution::par;

    std::atomic_bool fail{};
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
        if (outpoint.point().index() != point::null_index)
            out.insert(std::move(outpoint));
    }

    return error::success;
}

// protected
TEMPLATE
code CLASS::get_address_outputs_turbo(std::atomic_bool& cancel, outpoints& out,
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
            auto outpoint = get_spent(link);
            fail = (outpoint.point().index() == point::null_index);
            return outpoint;
        });
}

} // namespace database
} // namespace libbitcoin

#endif
