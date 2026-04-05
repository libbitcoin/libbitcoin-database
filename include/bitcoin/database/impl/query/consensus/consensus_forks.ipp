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
#ifndef LIBBITCOIN_DATABASE_QUERY_CONSENSUS_FORKS_IPP
#define LIBBITCOIN_DATABASE_QUERY_CONSENSUS_FORKS_IPP

#include <mutex>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

// protected fork readers.
// ----------------------------------------------------------------------------
// Protected against index pop (low contention) to ensure branch consistency.

// node/snapshot (via is_coalesced())
TEMPLATE
size_t CLASS::get_fork() const NOEXCEPT
{
    ///////////////////////////////////////////////////////////////////////////
    std::shared_lock interlock1{ candidate_reorganization_mutex_ };
    std::shared_lock interlock2{ confirmed_reorganization_mutex_ };
    return get_fork_();
    ///////////////////////////////////////////////////////////////////////////
}

// node/organizer
TEMPLATE
header_links CLASS::get_candidate_fork(size_t& fork_point) const NOEXCEPT
{
    // Reservation may limit allocation to most common single block scenario.
    header_links out{};
    out.reserve(one);

    ///////////////////////////////////////////////////////////////////////////
    std::shared_lock interlock{ candidate_reorganization_mutex_ };

    fork_point = get_fork_();
    auto height = add1(fork_point);
    auto link = to_candidate(height);
    while (!link.is_terminal())
    {
        out.push_back(link);
        link = to_candidate(++height);
    }

    return out;
    ///////////////////////////////////////////////////////////////////////////
}

// node/organizer
TEMPLATE
header_links CLASS::get_confirmed_fork(const header_link& fork) const NOEXCEPT
{
    if (fork.is_terminal())
        return {};

    // Reservation may limit allocation to most common single block scenario.
    header_links out{};
    out.reserve(one);

    ///////////////////////////////////////////////////////////////////////////
    std::shared_lock interlock{ confirmed_reorganization_mutex_ };

    // Verify fork block is still confirmed and get its height.
    auto height = get_height(fork);
    auto link = to_confirmed(height);
    if (link != fork)
        return out;

    // First link above fork.
    link = to_confirmed(++height);
    while (!link.is_terminal())
    {
        out.push_back(link);
        link = to_confirmed(++height);
    }

    return out;
    ///////////////////////////////////////////////////////////////////////////
}

// node/confirmer
TEMPLATE
header_states CLASS::get_validated_fork(size_t& fork_point,
    size_t top_checkpoint, bool filter) const NOEXCEPT
{
    // Reservation may limit allocation to most common scenario.
    header_states out{};
    out.reserve(one);
    code ec{};

    // Disable filter constraint if filtering is disabled.
    filter &= filter_enabled();

    ///////////////////////////////////////////////////////////////////////////
    std::shared_lock interlock{ candidate_reorganization_mutex_ };

    fork_point = get_fork_();
    auto height = add1(fork_point);
    auto link = to_candidate(height);
    while (is_block_validated(ec, link, height, top_checkpoint) &&
        (!filter || is_filtered_body(link)))
    {
        out.emplace_back(link, ec);
        link = to_candidate(++height);
    }

    return out;
    ///////////////////////////////////////////////////////////////////////////
}

// utility
// ----------------------------------------------------------------------------
// private

// Without both interlocks, or caller controlling one side with the other
// locked, this is unsafe, since the compare of heights requires atomicity.
TEMPLATE
size_t CLASS::get_fork_() const NOEXCEPT
{
    for (auto height = get_top_confirmed(); is_nonzero(height); --height)
        if (to_confirmed(height) == to_candidate(height))
            return height;

    return zero;
}

} // namespace database
} // namespace libbitcoin

#endif
