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
#ifndef LIBBITCOIN_DATABASE_QUERY_MERKLE_IPP
#define LIBBITCOIN_DATABASE_QUERY_MERKLE_IPP

#include <algorithm>
#include <iterator>
#include <utility>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

// merkle
// ----------------------------------------------------------------------------
// server/electrum

TEMPLATE
code CLASS::get_merkle_root_and_proof(hash_digest& root, hashes& proof,
    size_t target, size_t waypoint) const NOEXCEPT
{
    if (target > waypoint)
        return error::invalid_argument;

    if (waypoint > get_top_confirmed())
        return error::not_found;

    hashes roots{};
    if (const auto ec = get_merkle_subroots(roots, waypoint))
        return ec;

    if (const auto ec = get_merkle_proof(proof, roots, target, waypoint))
        return ec;

    root = system::merkle_root(std::move(roots));
    return {};
}

TEMPLATE
hash_digest CLASS::get_merkle_root(size_t height) const NOEXCEPT
{
    hashes roots{};
    if (const auto ec = get_merkle_subroots(roots, height))
        return {};

    return system::merkle_root(std::move(roots));
}

// utilities
// ----------------------------------------------------------------------------

// static/protected
TEMPLATE
positions CLASS::merkle_branch(size_t leaf, size_t leaves,
    bool compress) NOEXCEPT
{
    using namespace system;
    BC_ASSERT(leaves <= power2(sub1(bits<size_t>)));
    BC_ASSERT(is_even(leaves) || is_one(leaves));

    positions branch{};
    if (is_zero(leaves) || leaf >= leaves)
        return branch;

    // Upper bound, actual count may be less given compression.
    branch.reserve(ceilinged_log2(leaves));

    for (auto width = one, current = leaves; current > one;)
    {
        const auto sibling = bit_xor(leaf, one);
        if (!compress || sibling < current)
            branch.emplace_back(sibling, width);

        ++current;
        shift_left_into(width);
        shift_right_into(leaf);
        shift_right_into(current);
    }

    return branch;
}

// protected
TEMPLATE
hash_option CLASS::create_interval(header_link link,
    size_t height) const NOEXCEPT
{
    // Interval ends at nth block where n is a multiple of span.
    // One is a functional but undesirable case (no optimization).
    const auto span = interval_span();
    BC_ASSERT(!is_zero(span));

    // If valid link is provided then empty return implies non-interval height.
    if (link.is_terminal() || !system::is_multiple(add1(height), span))
        return {};

    // Generate the leaf nodes for the span.
    hashes leafs(span);
    for (auto& leaf: std::views::reverse(leafs))
    {
        leaf = get_header_key(link);
        link = to_parent(link);
    }

    // Generate the merkle root of the interval ending on link header.
    return system::merkle_root(std::move(leafs));
}

// protected
TEMPLATE
hash_option CLASS::get_confirmed_interval(size_t height) const NOEXCEPT
{
    const auto span = interval_span();
    BC_ASSERT(!is_zero(span));

    if (!system::is_multiple(add1(height), span))
        return {};

    table::txs::get_interval txs{};
    if (!store_.txs.at(to_confirmed(height), txs))
        return {};

    return txs.interval;
}

// static/protected
TEMPLATE
void CLASS::merge_merkle(hashes& path, hashes&& leaves, size_t first,
    size_t lift) NOEXCEPT
{
    auto size = leaves.size();
    if (!is_one(size) && is_odd(size))
    {
        // leaves is either even or has +1 element of reserved space.
        leaves.push_back(leaves.back());
        ++size;
    }

    for (const auto& row: merkle_branch(first, size + lift))
    {
        hashes subroot{};
        if (const auto leaf = row.sibling * row.width; leaf < size)
        {
            const auto count = std::min(row.width, size - leaf);
            const auto next = std::next(leaves.begin(), leaf);
            const auto it = std::make_move_iterator(next);
            subroot = { it, std::next(it, count) };
        }
        else
        {
            subroot = { std::move(leaves.back()) };
        }

        path.push_back(partial_subroot(std::move(subroot), row.width));
    }
}

// protected
TEMPLATE
code CLASS::get_merkle_proof(hashes& proof, hashes roots, size_t target,
    size_t waypoint) const NOEXCEPT
{
    const auto span = interval_span();
    BC_ASSERT(!is_zero(span));

    const auto first = (target / span) * span;
    const auto close = sub1(first + span);
    const auto last  = std::min(waypoint, close);
    const auto size  = add1(last - first);
    auto parts = get_confirmed_hashes(first, size);
    if (is_zero(parts.size()))
        return error::merkle_proof;

    const auto count = parts.size();
    const auto pad   = to_int<size_t>(!is_one(count) && is_odd(count));
    const auto lift  = is_zero(first) ? zero : (span - (count + pad));

    using namespace system;
    proof.clear();
    proof.reserve(ceilinged_log2(parts.size()) + ceilinged_log2(roots.size()));
    merge_merkle(proof, std::move(parts), target % span, lift);
    merge_merkle(proof, std::move(roots), target / span, zero);
    return error::success;
}

// static/private
TEMPLATE
hash_digest CLASS::partial_subroot(hashes&& tree, size_t span) NOEXCEPT
{
    // Tree cannot be empty or exceed span (a power of 2).
    if (tree.empty() || tree.size() > span)
        return {};

    // Zero depth implies single tree element, which is the root.
    using namespace system;
    const auto depth = ceilinged_log2(span);
    if (is_zero(depth))
        return tree.front();

    // merkle_root() treats a single hash as top/complete, but for a partial
    // depth subtree, an odd leaf (including a single) requires duplication.
    if (is_odd(tree.size()))
        tree.push_back(tree.back());

    // Log2 of the evened breadth gives the elevation by merkle_root.
    // Partial cannot exceed depth, since tree.size() <= span (a power of 2).
    auto partial = ceilinged_log2(tree.size());
    if (is_subtract_overflow(depth, partial))
        return {};

    // Elevate hashes to partial level, and then from partial to depth.
    auto hash = merkle_root(std::move(tree));
    for (; partial < depth; ++partial)
        hash = sha256::double_hash(hash, hash);

    return hash;
}

// protected
TEMPLATE
code CLASS::get_merkle_subroots(hashes& roots, size_t waypoint) const NOEXCEPT
{
    const auto span = interval_span();
    BC_ASSERT(!is_zero(span));

    using namespace system;
    const auto leafs = add1(waypoint);
    const auto limit = ceilinged_divide(leafs, span);
    const auto count = limit + to_int<size_t>(!is_one(limit) && is_odd(limit));

    // Roots is even-size-except-one-reserved for merkle root push.
    roots.reserve(count);

    // Either all subroots elevated to same level, or there is a single root.
    for (size_t first{}; first < leafs; first += span)
    {
        const auto last = std::min(sub1(first + span), waypoint);
        const auto size = add1(last - first);

        if (size == span)
        {
            auto interval = get_confirmed_interval(last);
            if (!interval.has_value()) return error::merkle_interval;
            roots.push_back(std::move(interval.value()));
        }
        else if (is_zero(first))
        {
            // Single hash, is the complete merkle root.
            auto complete = get_confirmed_hashes(zero, size);
            roots.push_back(merkle_root(std::move(complete)));
        }
        else
        {
            // Roots is even-size-except-one-reserved for merkle root push.
            auto partial = get_confirmed_hashes(first, size);
            if (partial.empty()) return error::merkle_hashes;
            roots.push_back(partial_subroot(std::move(partial), span));
        }
    }

    return error::success;
}

} // namespace database
} // namespace libbitcoin

#endif
