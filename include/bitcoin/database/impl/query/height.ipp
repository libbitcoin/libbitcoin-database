/**
 * Copyright (c) 2011-2025 libbitcoin developers (see AUTHORS)
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
#ifndef LIBBITCOIN_DATABASE_QUERY_HEIGHT_IPP
#define LIBBITCOIN_DATABASE_QUERY_HEIGHT_IPP

#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

TEMPLATE
size_t CLASS::get_candidate_size() const NOEXCEPT
{
    // If the store is not opened this will be a max_size loop.
    return get_candidate_size(get_top_candidate());
}

TEMPLATE
size_t CLASS::get_candidate_size(size_t top) const NOEXCEPT
{
    size_t wire{};
    for (auto height = zero; height <= top; ++height)
        wire += get_block_size(to_candidate(height));

    return wire;
}

TEMPLATE
size_t CLASS::get_confirmed_size() const NOEXCEPT
{
    // If the store is not opened this will be a max_size loop.
    return get_confirmed_size(get_top_confirmed());
}

TEMPLATE
size_t CLASS::get_confirmed_size(size_t top) const NOEXCEPT
{
    size_t wire{};
    for (auto height = zero; height <= top; ++height)
        wire += get_block_size(to_confirmed(height));

    return wire;
}

TEMPLATE
bool CLASS::initialize(const block& genesis) NOEXCEPT
{
    BC_ASSERT(!is_initialized());
    BC_ASSERT(is_one(genesis.transactions_ptr()->size()));

    // TODO: add genesis block neutrino head and body when neutrino is enabled.

    // ========================================================================
    const auto scope = store_.get_transactor();

    if (!set(genesis, context{}, false, false))
        return false;

    const auto link = to_header(genesis.hash());

    // Unsafe for allocation failure, but only used in store creation.
    return set_strong(link)
        && push_candidate(link)
        && push_confirmed(link);
    // ========================================================================
}

TEMPLATE
bool CLASS::push_candidate(const header_link& link) NOEXCEPT
{
    if (link.is_terminal())
        return false;

    // ========================================================================
    const auto scope = store_.get_transactor();

    // Clean single allocation failure (e.g. disk full).
    const table::height::record candidate{ {}, link };
    return store_.candidate.put(candidate);
    // ========================================================================
}

TEMPLATE
bool CLASS::push_confirmed(const header_link& link) NOEXCEPT
{
    if (link.is_terminal())
        return false;

    // ========================================================================
    const auto scope = store_.get_transactor();

    // Clean single allocation failure (e.g. disk full).
    const table::height::record confirmed{ {}, link };
    return store_.confirmed.put(confirmed);
    // ========================================================================
}

TEMPLATE
bool CLASS::pop_candidate() NOEXCEPT
{
    using ix = table::transaction::ix::integer;
    const auto top = system::possible_narrow_cast<ix>(get_top_candidate());
    if (is_zero(top))
        return false;

    // ========================================================================
    const auto scope = store_.get_transactor();

    // Clean single allocation failure (e.g. disk full).
    return store_.candidate.truncate(top);
    // ========================================================================
}

TEMPLATE
bool CLASS::pop_confirmed() NOEXCEPT
{
    using ix = table::transaction::ix::integer;
    const auto top = system::possible_narrow_cast<ix>(get_top_confirmed());
    if (is_zero(top))
        return false;

    // ========================================================================
    const auto scope = store_.get_transactor();

    // Clean single allocation failure (e.g. disk full).
    return store_.confirmed.truncate(top);
    // ========================================================================
}

} // namespace database
} // namespace libbitcoin

#endif
