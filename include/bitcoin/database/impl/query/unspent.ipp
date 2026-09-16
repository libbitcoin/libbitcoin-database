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
#ifndef LIBBITCOIN_DATABASE_QUERY_UNSPENT_IPP
#define LIBBITCOIN_DATABASE_QUERY_UNSPENT_IPP

#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

TEMPLATE
bool CLASS::is_bip30_exception(bool& out,
    const header_link& link) const NOEXCEPT
{
    out = false;
    size_t height{};
    if (!get_height(height, link))
        return false;

    const auto first = (height == bip30::first_exception);
    const auto second = (height == bip30::second_exception);
    if (!first && !second)
        return true;

    if (!store_.get_envelope().forks.bip30)
        return true;

    const auto& hash = first ? bip30::first.hash() : bip30::second.hash();
    out = (get_header_key(link) == hash);
    return true;
}

TEMPLATE
code CLASS::get_block_amounts(block_amounts& out,
    const header_link& link) const NOEXCEPT
{
    out = {};
    auto bip30_exception = false;
    if (!is_bip30_exception(bip30_exception, link))
        return error::integrity;

    auto coinbase = true;
    table::output::get_spendable value{};
    for (const auto& tx: to_transactions(link))
    {
        for (const auto& put: to_outputs(tx))
        {
            if (!store_.output.get(put, value))
                return error::integrity;

            if (value.unspendable)
                out.unspendable += value.value;

            if (coinbase)
                out.coinbase += value.value;
            else
                out.outputs += value.value;
        }

        coinbase = false;
    }

    // The duplicated coinbase overwrites (destroys) that of the original.
    if (bip30_exception)
        out.bip30 = out.coinbase;

    for (const auto& put: to_block_prevouts(link))
    {
        if (!store_.output.get(put, value))
            return error::integrity;

        out.prevouts += value.value;
    }

    return error::success;
}

TEMPLATE
code CLASS::get_unspent_totals(const stopper& cancel, unspent_totals& out,
    const header_links& branch, bool turbo) const NOEXCEPT
{
    out = {};
    difference_set set{ outs_records() };
    const unspent_scanner<Store> scanner{ *this, store_, cancel, turbo };
    if (const auto ec = scanner.scan(set, out, branch))
        return ec;

    const unspent_counter<Store> counter{ store_, cancel, turbo };
    return counter.count(out, set);
}

TEMPLATE
code CLASS::get_unspent_muhash(const stopper& cancel, unspent_totals& out,
    hash_digest& digest, const header_links& branch, bool turbo) const NOEXCEPT
{
    out = {};
    difference_set set{ outs_records() };
    const unspent_scanner<Store> scanner{ *this, store_, cancel, turbo };
    if (const auto ec = scanner.scan(set, out, branch))
        return ec;

    const unspent_muhash<Store> muhash{ store_, cancel, turbo };
    return muhash.hash(out, digest, set);
}

TEMPLATE
code CLASS::get_unspent_serialized(const stopper& cancel, unspent_totals& out,
    hash_digest& digest, const header_links& branch, bool turbo) const NOEXCEPT
{
    out = {};
    difference_set set{ outs_records() };
    const unspent_scanner<Store> scanner{ *this, store_, cancel, turbo };
    if (const auto ec = scanner.scan(set, out, branch))
        return ec;

    const unspent_serial<Store> serial{ store_, cancel, turbo };
    return serial.hash(out, digest, set);
}

TEMPLATE
code CLASS::get_unspent_matches(const stopper& cancel, unspent_coins& out,
    size_t& txouts, const std::unordered_set<hash_digest>& keys,
    const header_links& branch, bool turbo) const NOEXCEPT
{
    unspent_totals unused{};
    difference_set set{ outs_records() };
    const unspent_scanner<Store> scanner{ *this, store_, cancel, turbo };
    if (const auto ec = scanner.scan(set, unused, branch))
        return ec;

    const unspent_matcher<Store> matcher{ store_, cancel, turbo };
    return matcher.match(out, txouts, keys, set);
}

} // namespace database
} // namespace libbitcoin

#endif
