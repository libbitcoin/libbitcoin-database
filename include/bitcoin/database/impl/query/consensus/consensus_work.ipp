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
#ifndef LIBBITCOIN_DATABASE_QUERY_CONSENSUS_WORK_IPP
#define LIBBITCOIN_DATABASE_QUERY_CONSENSUS_WORK_IPP

#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

// Fork/work computation.
// 
// ----------------------------------------------------------------------------

TEMPLATE
bool CLASS::get_work(uint256_t& fork_work,
    const header_states& states) const NOEXCEPT
{
    for (const auto& state: states)
    {
        uint32_t bits{};
        if (!get_bits(bits, state.link))
            return false;

        fork_work += system::chain::header::proof(bits);
    }

    return true;
}

TEMPLATE
bool CLASS::get_branch(header_states& branch,
    const hash_digest& hash) const NOEXCEPT
{
    for (auto link = to_header(hash); !is_candidate_header(link);
        link = to_parent(link))
    {
        if (link.is_terminal())
            return false;

        branch.emplace_back(link, code{});
    }

    return true;
}

TEMPLATE
bool CLASS::get_strong_branch(bool& strong, const uint256_t& branch_work,
    size_t branch_point, bool tie) const NOEXCEPT
{
    const auto top = get_top_candidate();
    if (branch_point >= top)
    {
        strong = true;
        return true;
    }

    uint256_t top_work{};
    uint256_t point_work{};
    if (!get_branch_work(top_work, to_candidate(top)) ||
        !get_branch_work(point_work, to_candidate(branch_point)))
        return false;

    const auto work = top_work - point_work;
    strong = tie ? !(work > branch_work) : !(work >= branch_work);
    return true;
}

TEMPLATE
bool CLASS::get_strong_fork(bool& strong, const uint256_t& fork_work,
    size_t fork_point, bool tie) const NOEXCEPT
{
    const auto top = get_top_confirmed();
    if (fork_point >= top)
    {
        strong = true;
        return true;
    }

    uint256_t top_work{};
    uint256_t point_work{};
    if (!get_branch_work(top_work, to_confirmed(top)) ||
        !get_branch_work(point_work, to_confirmed(fork_point)))
        return false;

    const auto work = top_work - point_work;
    strong = tie ? !(work > fork_work) : !(work >= fork_work);
    return true;
}

} // namespace database
} // namespace libbitcoin

#endif
