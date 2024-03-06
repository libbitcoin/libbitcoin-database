/**
 * Copyright (c) 2011-2023 libbitcoin developers (see AUTHORS)
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
#ifndef LIBBITCOIN_DATABASE_QUERY_INITIALIZE_IPP
#define LIBBITCOIN_DATABASE_QUERY_INITIALIZE_IPP

#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

// Initialization (natural-keyed).
// ----------------------------------------------------------------------------

TEMPLATE
inline bool CLASS::is_initialized() const NOEXCEPT
{
    return !is_zero(store_.confirmed.count()) &&
        !is_zero(store_.candidate.count());
}
TEMPLATE
inline size_t CLASS::get_top_candidate() const NOEXCEPT
{
    BC_ASSERT_MSG(!is_zero(store_.candidate.count()), "empty");
    return sub1(store_.candidate.count());
}

TEMPLATE
inline size_t CLASS::get_top_confirmed() const NOEXCEPT
{
    BC_ASSERT_MSG(!is_zero(store_.confirmed.count()), "empty");
    return sub1(store_.confirmed.count());
}


TEMPLATE
size_t CLASS::get_fork() const NOEXCEPT
{
    for (auto height = get_top_confirmed(); !is_zero(height); --height)
        if (to_confirmed(height) == to_candidate(height))
            return height;

    return zero;
}

TEMPLATE
size_t CLASS::get_last_associated() const NOEXCEPT
{
    return get_last_associated_from(get_fork());
}

TEMPLATE
size_t CLASS::get_last_associated_from(size_t height) const NOEXCEPT
{
    if (height >= height_link::terminal)
        return max_size_t;

    while (is_associated(to_candidate(++height)));
    return --height;
}

TEMPLATE
associations CLASS::get_all_unassociated() const NOEXCEPT
{
    return get_all_unassociated_above(get_fork());
}

TEMPLATE
associations CLASS::get_all_unassociated_above(size_t height) const NOEXCEPT
{
    associations out{};
    const auto top = get_top_candidate();
    while (++height <= top)
    {
        const auto header_fk = to_candidate(height);
        if (!is_associated(header_fk))
        {
            table::header::get_check_context context{};
            if (store_.header.get(header_fk, context))
            {
                // boost::multi_index_container
                out.insert(association
                {
                    context.key,
                    system::chain::context
                    {
                        context.ctx.flags,
                        context.timestamp,
                        context.ctx.mtp,
                        system::possible_wide_cast<size_t>(context.ctx.height)
                    }
                });

                // std::unordered_map
                ////out.emplace(context.key, system::chain::context
                ////{
                ////    context.ctx.flags,
                ////    context.timestamp,
                ////    context.ctx.mtp,
                ////    system::possible_wide_cast<size_t>(context.ctx.height)
                ////});
            }
        }
    }

    return out;
}

TEMPLATE
hashes CLASS::get_candidate_hashes(const heights& heights) const NOEXCEPT
{
    hashes out{};
    out.reserve(heights.size());
    for (const auto& height: heights)
    {
        const auto header_fk = to_candidate(height);
        if (!header_fk.is_terminal())
            out.push_back(get_header_key(header_fk));
    }

    // Due to reorganization, top may decrease intermittently.
    out.shrink_to_fit();
    return out;
}

TEMPLATE
hashes CLASS::get_confirmed_hashes(const heights& heights) const NOEXCEPT
{
    hashes out{};
    out.reserve(heights.size());
    for (const auto& height: heights)
    {
        const auto header_fk = to_confirmed(height);
        if (!header_fk.is_terminal())
            out.push_back(get_header_key(header_fk));
    }

    // Due to reorganization, top may decrease intermittently.
    out.shrink_to_fit();
    return out;
}

} // namespace database
} // namespace libbitcoin

#endif
