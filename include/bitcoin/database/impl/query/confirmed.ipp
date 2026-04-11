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
#ifndef LIBBITCOIN_DATABASE_QUERY_CONFIRM_IPP
#define LIBBITCOIN_DATABASE_QUERY_CONFIRM_IPP

#include <algorithm>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {
  
// find_confirmed
// ----------------------------------------------------------------------------
// These ensure both strong and candidate/confirmed indexation.

TEMPLATE
header_link CLASS::find_confirmed_block(
    const hash_digest& tx_hash) const NOEXCEPT
{
    const auto block = find_strong(tx_hash);
    if (is_confirmed_block(block))
        return block;

    return {};
}

TEMPLATE
point_link CLASS::find_confirmed_spender(const point& prevout) const NOEXCEPT
{
    for (const auto& in: to_spenders(prevout))
        if (is_confirmed_input(in))
            return in;

    return {};
}

// is_confirmed
// ----------------------------------------------------------------------------

// protected
TEMPLATE
bool CLASS::is_confirmed_unspent(const output_link& link) const NOEXCEPT
{
    return is_confirmed_output(link) && !is_confirmed_spent_output(link);
}

TEMPLATE
bool CLASS::is_candidate_header(const header_link& link) const NOEXCEPT
{
    // The header is candidate (by height).
    const auto height = get_height(link);
    if (height.is_terminal())
        return false;

    table::height::record candidate{};
    return store_.candidate.get(height, candidate) &&
        (candidate.header_fk == link);
}

TEMPLATE
bool CLASS::is_confirmed_block(const header_link& link) const NOEXCEPT
{
    // The block is confirmed (by height).
    const auto height = get_height(link);
    if (height.is_terminal())
        return false;

    table::height::record confirmed{};
    return store_.confirmed.get(height, confirmed) &&
        (confirmed.header_fk == link);
}

TEMPLATE
bool CLASS::is_confirmed_tx(const tx_link& link) const NOEXCEPT
{
    // The tx is strong *and* its block is confirmed (by height).
    const auto fk = find_strong(link);
    return !fk.is_terminal() && is_confirmed_block(fk);
}

TEMPLATE
bool CLASS::is_confirmed_input(const point_link& link) const NOEXCEPT
{
    // The spend.tx is strong *and* its block is confirmed (by height).
    const auto fk = to_input_tx(link);
    return !fk.is_terminal() && is_confirmed_tx(fk);
}

TEMPLATE
bool CLASS::is_confirmed_output(const output_link& link) const NOEXCEPT
{
    // The output.tx is strong *and* its block is confirmed (by height).
    const auto fk = to_output_tx(link);
    return !fk.is_terminal() && is_confirmed_tx(fk);
}

TEMPLATE
bool CLASS::is_confirmed_spent_output(const output_link& link) const NOEXCEPT
{
    // The spender is strong *and* its block is confirmed (by height).
    const auto ins = to_spenders(link);
    return std::any_of(ins.cbegin(), ins.cend(), [&](const auto& in) NOEXCEPT
    {
        return is_confirmed_input(in);
    });
}

TEMPLATE
bool CLASS::is_confirmed_all_prevouts(const tx_link& link) const NOEXCEPT
{
    // All prevouts of the tx's inputs are confirmed.
    const auto ins = to_points(link);
    return std::all_of(ins.cbegin(), ins.cend(), [&](const auto& in) NOEXCEPT
    {
        return is_confirmed_input(in);
    });
}

} // namespace database
} // namespace libbitcoin

#endif
