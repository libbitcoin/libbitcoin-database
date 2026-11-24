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
#ifndef LIBBITCOIN_DATABASE_QUERY_CONSENSUS_FORKS_IPP
#define LIBBITCOIN_DATABASE_QUERY_CONSENSUS_FORKS_IPP

#include <mutex>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

// get_confirmed
// ----------------------------------------------------------------------------

TEMPLATE
height_link CLASS::get_confirmed_height(const header_link& link) const NOEXCEPT
{
    if (const auto height = get_height(link); !height.is_terminal())
    {
        // The block is confirmed (by height).
        if (store_.confirmed.at(height) == link)
            return height;
    }

    return {};
}

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

// server/native
TEMPLATE
header_links CLASS::get_branch_tops() const NOEXCEPT
{
    using namespace system;
    using link_t = header_link::integer;
    const auto top = std::max(get_top_candidate(), get_top_confirmed());
    const auto records = header_records();

    // A height of more than one header is forked, as is any above the tops.
    size_t height{};
    std::vector<bool> seen(add1(top), false);
    std::vector<bool> forked(add1(top), false);
    for (size_t record{}; record < records; ++record)
    {
        if (!get_height(height, possible_narrow_cast<link_t>(record)) ||
            height > top)
            continue;

        if (seen.at(height))
            forked.at(height) = true;
        else
            seen.at(height) = true;
    }

    std::unordered_set<link_t> forks{};
    for (size_t record{}; record < records; ++record)
    {
        const auto link = possible_narrow_cast<link_t>(record);
        if (get_height(height, link) && (height > top || forked.at(height)))
            forks.insert(link);
    }

    // A child follows its parent, so the last link of a branch is its top.
    header_links tops{};
    for (auto record = records; is_nonzero(record) && !forks.empty();)
    {
        const auto link = possible_narrow_cast<link_t>(--record);
        if (!forks.contains(link))
            continue;

        tops.push_back(link);
        for (auto next = link; is_nonzero(forks.erase(next));)
            next = to_parent(next).value;
    }

    ///////////////////////////////////////////////////////////////////////////
    std::shared_lock interlock1{ candidate_reorganization_mutex_ };
    std::shared_lock interlock2{ confirmed_reorganization_mutex_ };
    std::erase_if(tops, [&](const auto& link) NOEXCEPT
    {
        return is_candidate_header(link) || is_confirmed_block(link);
    });
    ///////////////////////////////////////////////////////////////////////////

    return tops;
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
header_states CLASS::get_validated_fork(size_t& fork_point) const NOEXCEPT
{
    // Reservation may limit allocation to most common scenario.
    header_states out{};
    out.reserve(one);

    ///////////////////////////////////////////////////////////////////////////
    std::shared_lock interlock{ candidate_reorganization_mutex_ };

    fork_point = get_fork_();
    auto height = add1(fork_point);
    auto link = to_candidate(height);

    while (true)
    {
        const auto state = get_block_state(link);
        if (!database::is_associated(state) ||
            !database::is_scanned(state) ||
            !database::is_filtered(state))
            break;

        if (!database::is_bypass(state) && !database::is_validated(state))
            break;

        out.emplace_back(link, state);
        link = to_candidate(++height);
    }

    return out;
    ///////////////////////////////////////////////////////////////////////////
}

// utility
// ----------------------------------------------------------------------------
// private

// Without both interlocks, or caller controlling one side with other locked,
// this is unsafe, since compare of heights requires atomicity.
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
