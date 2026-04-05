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
    size_t branch_point) const NOEXCEPT
{
    uint256_t work{};
    for (auto height = get_top_candidate(); height > branch_point; --height)
    {
        uint32_t bits{};
        if (!get_bits(bits, to_candidate(height)))
            return false;

        // Not strong when candidate_work equals or exceeds branch_work.
        work += system::chain::header::proof(bits);
        if (work >= branch_work)
        {
            strong = false;
            return true;
        }
    }

    strong = true;
    return true;
}

TEMPLATE
bool CLASS::get_strong_fork(bool& strong, const uint256_t& fork_work,
    size_t fork_point) const NOEXCEPT
{
    uint256_t work{};
    for (auto height = get_top_confirmed(); height > fork_point; --height)
    {
        uint32_t bits{};
        if (!get_bits(bits, to_confirmed(height)))
            return false;

        // Not strong is confirmed work ever equals or exceeds fork_work.
        work += system::chain::header::proof(bits);
        if (work >= fork_work)
        {
            strong = false;
            return true;
        }
    }

    strong = true;
    return true;
}

} // namespace database
} // namespace libbitcoin

#endif
