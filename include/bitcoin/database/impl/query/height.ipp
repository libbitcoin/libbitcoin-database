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

#include <algorithm>
#include <ranges>
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

// shared_lock readers
// ----------------------------------------------------------------------------
// Protected against index pop (low contention) to ensure branch consistency.

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

TEMPLATE
size_t CLASS::get_fork() const NOEXCEPT
{
    ///////////////////////////////////////////////////////////////////////////
    std::shared_lock interlock1{ candidate_reorganization_mutex_ };
    std::shared_lock interlock2{ confirmed_reorganization_mutex_ };
    return get_fork_();
    ///////////////////////////////////////////////////////////////////////////
}

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

TEMPLATE
hashes CLASS::get_candidate_hashes(const heights& heights) const NOEXCEPT
{
    hashes out{};
    out.reserve(heights.size());

    ///////////////////////////////////////////////////////////////////////////
    std::shared_lock interlock{ candidate_reorganization_mutex_ };

    for (const auto& height: heights)
    {
        const auto link = to_candidate(height);
        if (!link.is_terminal())
            out.push_back(get_header_key(link));
    }

    return out;
    ///////////////////////////////////////////////////////////////////////////
}

TEMPLATE
hashes CLASS::get_confirmed_hashes(const heights& heights) const NOEXCEPT
{
    hashes out{};
    out.reserve(heights.size());

    ///////////////////////////////////////////////////////////////////////////
    std::shared_lock interlock{ confirmed_reorganization_mutex_ };

    for (const auto& height: heights)
    {
        const auto link = to_confirmed(height);
        if (!link.is_terminal())
            out.push_back(get_header_key(link));
    }

    return out;
    ///////////////////////////////////////////////////////////////////////////
}

TEMPLATE
hashes CLASS::get_confirmed_hashes(size_t first, size_t count) const NOEXCEPT
{
    using namespace system;
    const auto size = is_odd(count) && count > one ? add1(count) : count;
    if (is_add_overflow(count, one) || is_add_overflow(first, size))
        return {};

    auto link = to_confirmed(first + count);
    if (link.is_terminal())
        return {};

    // Extra allocation for odd count optimizes for merkle root.
    // Vector capacity is never reduced when resizing to smaller size.
    hashes out(size);
    out.resize(count);

    for (auto& hash: std::views::reverse(out))
    {
        hash = get_header_key(link);
        link = to_parent(link);
    }

    return out;
}

TEMPLATE
header_links CLASS::get_confirmed_headers(size_t first,
    size_t limit) const NOEXCEPT
{
    // Empty is always a successful/valid result for this method.
    if (is_zero(limit))
        return {};

    // First requested height is currently above top.
    const auto top = get_top_confirmed();
    if (first > top)
        return {};

    // add1(top) cannot overflow, as indexed block maximum cannot exceed size_t.
    limit = std::min(limit, add1(top) - first);
    auto last = first + sub1(limit);

    // Due to reorganization it is possible for this height to now be terminal.
    auto link = to_confirmed(last);

    // Walk link back to first indexed header (for reorg safety).
    while (link.is_terminal() && last > first)
        link = to_confirmed(--last);

    // No headers are currently confirmed at/above first.
    if (link.is_terminal())
        return {};

    // Compiler should optimize out last to_parent() call.
    header_links out(add1(last - first));
    for (auto& value: std::views::reverse(out))
        link = to_parent(value = link);

    return out;
}

// writers
// ----------------------------------------------------------------------------

TEMPLATE
bool CLASS::initialize(const block& genesis) NOEXCEPT
{
    BC_ASSERT(!is_initialized());
    BC_ASSERT(is_one(genesis.transactions_ptr()->size()));

    // TODO: add genesis block filter_tx head and body when filter_tx is enabled.

    // ========================================================================
    const auto scope = store_.get_transactor();

    if (!set(genesis, context{}, false, false))
        return false;

    const auto link = to_header(genesis.hash());

    // Unsafe for allocation failure, but only used in store creation.
    return set_filter_body(link, genesis)
        && set_filter_head(link)
        && push_candidate(link)
        && push_confirmed(link, true);
    // ========================================================================
}

TEMPLATE
bool CLASS::push_candidate(const header_link& link) NOEXCEPT
{
    if (link.is_terminal())
        return false;

    // Reserve-commit commit for deferred access.
    if (!store_.candidate.reserve(one))
        return false;

    // ========================================================================
    const auto scope = store_.get_transactor();

    // Clean single allocation failure (e.g. disk full).
    const table::height::record candidate{ {}, link };
    return store_.candidate.put(candidate);
    // ========================================================================
}

TEMPLATE
bool CLASS::pop_candidate() NOEXCEPT
{
    using ix = table::transaction::ix::integer;
    const auto top = system::possible_narrow_cast<ix>(get_top_candidate());
    if (is_zero(top))
        return false;

    // Clean single allocation failure (e.g. disk full).
    // ========================================================================
    const auto scope = store_.get_transactor();

    ///////////////////////////////////////////////////////////////////////////
    std::unique_lock interlock{ candidate_reorganization_mutex_ };
    return store_.candidate.truncate(top);
    ///////////////////////////////////////////////////////////////////////////
    // ========================================================================
}

TEMPLATE
bool CLASS::push_confirmed(const header_link& link, bool strong) NOEXCEPT
{
    table::txs::get_coinbase_and_count txs{};
    if (strong && !store_.txs.at(to_txs(link), txs))
        return false;

    // Reserve-commit to ensure disk full safety and deferred access.
    if (!store_.confirmed.reserve(one))
        return false;

    // ========================================================================
    const auto scope = store_.get_transactor();

    // This reservation guard assumes no concurrent writes to the table.
    if (strong && !set_strong(link, txs.number, txs.coinbase_fk, true))
        return false;

    const table::height::record confirmed{ {}, link };
    return store_.confirmed.commit(confirmed);
    // ========================================================================
}

TEMPLATE
bool CLASS::pop_confirmed() NOEXCEPT
{
    using ix = table::transaction::ix::integer;
    const auto top = system::possible_narrow_cast<ix>(get_top_confirmed());
    if (is_zero(top))
        return false;

    const auto link = to_confirmed(top);
    table::txs::get_coinbase_and_count txs{};
    if (!store_.txs.at(to_txs(link), txs))
        return {};

    // ========================================================================
    const auto scope = store_.get_transactor();

    // Clean single allocation failure.
    if (!set_strong(link, txs.number, txs.coinbase_fk, false))
        return false;

    // Truncate cannot fail for disk full.
    // This truncate assumes no concurrent writes to the table.
    ///////////////////////////////////////////////////////////////////////////
    std::unique_lock interlock{ confirmed_reorganization_mutex_ };
    return store_.confirmed.truncate(top);
    ///////////////////////////////////////////////////////////////////////////
    // ========================================================================
}

} // namespace database
} // namespace libbitcoin

#endif
