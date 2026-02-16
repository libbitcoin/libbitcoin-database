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
#ifndef LIBBITCOIN_DATABASE_QUERY_OPTIONAL_IPP
#define LIBBITCOIN_DATABASE_QUERY_OPTIONAL_IPP

#include <atomic>
#include <algorithm>
#include <ranges>
#include <utility>
#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>

 // TODO: address table could use point keys to compress the multimap.

namespace libbitcoin {
namespace database {

// Address (natural-keyed).
// ----------------------------------------------------------------------------

// private/static
TEMPLATE
template <typename Functor>
inline code CLASS::parallel_address_transform(std::atomic_bool& cancel,
    outpoints& out, const output_links& links, Functor&& functor) NOEXCEPT
{
    constexpr auto parallel = poolstl::execution::par;

    std::atomic_bool fail{};
    std::vector<outpoint> outpoints(links.size());
    std::transform(parallel, links.begin(), links.end(), outpoints.begin(),
        [&functor, &cancel, &fail](const auto& link) NOEXCEPT
        {
            return functor(link, cancel, fail);
        });

    out.clear();
    if (fail) return error::integrity;
    if (cancel) return error::canceled;
    for (auto& outpoint: outpoints)
    {
        if (cancel) return error::canceled;
        if (outpoint.point().index() != point::null_index)
            out.insert(std::move(outpoint));
    }

    return error::success;
}

// protected
TEMPLATE
code CLASS::to_address_outputs(std::atomic_bool& cancel, output_links& out,
    const hash_digest& key) const NOEXCEPT
{
    // Pushing into the vector is more efficient than precomputation of size.
    out.clear();
    for (auto it = store_.address.it(key); it; ++it)
    {
        if (cancel)
            return error::canceled;

        table::address::record address{};
        if (!store_.address.get(it, address))
            return error::integrity;

        out.push_back(address.output_fk);
    }

    return error::success;
}

// protected
TEMPLATE
code CLASS::get_address_outputs_turbo(std::atomic_bool& cancel, outpoints& out,
    const hash_digest& key) const NOEXCEPT
{
    out.clear();
    output_links links{};
    if (const code ec = to_address_outputs(cancel, links, key))
        return ec;

    return parallel_address_transform(cancel, out, links,
        [this](const auto& link, auto& cancel, auto& fail) NOEXCEPT
        {
            if (cancel || fail) return outpoint{};
            auto outpoint = get_spent(link);
            fail = (outpoint.point().index() == point::null_index);
            return outpoint;
        });
}

TEMPLATE
code CLASS::get_address_outputs(std::atomic_bool& cancel, outpoints& out,
    const hash_digest& key, bool turbo) const NOEXCEPT
{
    if (turbo && store_.turbo())
        return get_address_outputs_turbo(cancel, out, key);

    out.clear();
    for (auto it = store_.address.it(key); it; ++it)
    {
        if (cancel)
            return error::canceled;

        table::address::record address{};
        if (!store_.address.get(it, address))
            return error::integrity;

        out.insert(get_spent(address.output_fk));
    }

    return error::success;
}

// protected
TEMPLATE
code CLASS::get_confirmed_unspent_outputs_turbo(std::atomic_bool& cancel,
    outpoints& out, const hash_digest& key) const NOEXCEPT
{
    out.clear();
    output_links links{};
    if (const code ec = to_address_outputs(cancel, links, key))
        return ec;

    return parallel_address_transform(cancel, out, links,
        [this](const auto& link, auto& cancel, auto& fail) NOEXCEPT
        {
            if (cancel || fail || !is_confirmed_unspent(link))
                return outpoint{};

            auto outpoint = get_spent(link);
            fail = (outpoint.point().index() == point::null_index);
            return outpoint;
        });
}

TEMPLATE
code CLASS::get_confirmed_unspent_outputs(std::atomic_bool& cancel,
    outpoints& out, const hash_digest& key, bool turbo) const NOEXCEPT
{
    if (turbo && store_.turbo())
        return get_confirmed_unspent_outputs_turbo(cancel, out, key);

    out.clear();
    for (auto it = store_.address.it(key); it; ++it)
    {
        if (cancel)
            return error::canceled;

        table::address::record address{};
        if (!store_.address.get(it, address))
            return error::integrity;

        if (is_confirmed_unspent(address.output_fk))
            out.insert(get_spent(address.output_fk));
    }

    return error::success;
}

// protected
TEMPLATE
code CLASS::get_minimum_unspent_outputs_turbo(std::atomic_bool& cancel,
    outpoints& out, const hash_digest& key, uint64_t minimum) const NOEXCEPT
{
    out.clear();
    output_links links{};
    if (const code ec = to_address_outputs(cancel, links, key))
        return ec;

    return parallel_address_transform(cancel, out, links,
        [this, minimum](const auto& link, auto& cancel, auto& fail) NOEXCEPT
        {
            if (cancel || fail || !is_confirmed_unspent(link))
                return outpoint{};

            uint64_t value{};
            if (!get_value(value, link))
            {
                fail = true;
                return outpoint{};
            }

            if (value < minimum)
                return outpoint{};

            auto outpoint = get_spent(link);
            fail = (outpoint.point().index() == point::null_index);
            return outpoint;
        });
}

TEMPLATE
code CLASS::get_minimum_unspent_outputs(std::atomic_bool& cancel,
    outpoints& out, const hash_digest& key, uint64_t minimum,
    bool turbo) const NOEXCEPT
{
    if (turbo && store_.turbo())
        return get_minimum_unspent_outputs_turbo(cancel, out, key, minimum);

    out.clear();
    for (auto it = store_.address.it(key); it; ++it)
    {
        if (cancel)
            return error::canceled;

        table::address::record address{};
        if (!store_.address.get(it, address))
            return error::integrity;

        if (is_confirmed_unspent(address.output_fk))
        {
            uint64_t value{};
            if (!get_value(value, address.output_fk))
                return error::integrity;

            if (value >= minimum)
                out.insert(get_spent(address.output_fk));
        }
    }

    return error::success;
}

TEMPLATE
code CLASS::get_confirmed_balance(std::atomic_bool& cancel, uint64_t& balance,
    const hash_digest& key, bool turbo) const NOEXCEPT
{
    outpoints outs{};
    if (const auto ec = get_confirmed_unspent_outputs(cancel, outs, key, turbo))
    {
        balance = zero;
        return ec;
    }

    // Use of to_confirmed_unspent_outputs() provides necessary deduplication.
    balance = std::accumulate(outs.begin(), outs.end(), zero,
        [](size_t total, const outpoint& out) NOEXCEPT
        {
            return system::ceilinged_add(total, out.value());
        });

    return error::success;
}

// merkle
// ----------------------------------------------------------------------------

// protected
TEMPLATE
size_t CLASS::interval_span() const NOEXCEPT
{
    // span of zero (overflow) is disallowed (division by zero).
    // span of one (2^0) caches every block (no optimization, wasted storage).
    // span greater than top height eliminates caching (no optimization).
    const auto span = system::power2(store_.interval_depth());
    return is_zero(span) ? max_size_t : span;
}

// protected
TEMPLATE
CLASS::hash_option CLASS::create_interval(header_link link,
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
    hashes leaves(span);
    for (auto& leaf: std::views::reverse(leaves))
    {
        leaf = get_header_key(link);
        link = to_parent(link);
    }

    // Generate the merkle root of the interval ending on link header.
    return system::merkle_root(std::move(leaves));
}

// protected
TEMPLATE
CLASS::hash_option CLASS::get_confirmed_interval(size_t height) const NOEXCEPT
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
void CLASS::merge_merkle(hashes& to, hashes&& from, size_t first) NOEXCEPT
{
    using namespace system;
    for (const auto& row: block::merkle_branch(first, from.size()))
    {
        BC_ASSERT(row.sibling * add1(row.width) <= from.size());
        const auto it = std::next(from.begin(), row.sibling * row.width);
        const auto mover = std::make_move_iterator(it);
        to.push_back(merkle_root({ mover, std::next(mover, row.width) }));
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
    const auto last = std::min(sub1(first + span), waypoint);
    auto other = get_confirmed_hashes(first, add1(last - first));
    if (other.empty())
        return error::merkle_proof;

    using namespace system;
    proof.reserve(ceilinged_log2(other.size()) + ceilinged_log2(roots.size()));
    merge_merkle(proof, std::move(other), target % span);
    merge_merkle(proof, std::move(roots), target / span);
    return error::success;
}

// protected
TEMPLATE
code CLASS::get_merkle_tree(hashes& tree, size_t waypoint) const NOEXCEPT
{
    const auto span = interval_span();
    BC_ASSERT(!is_zero(span));

    const auto range = add1(waypoint);
    tree.reserve(system::ceilinged_divide(range, span));
    for (size_t first{}; first < range; first += span)
    {
        const auto last = std::min(sub1(first + span), waypoint);
        const auto size = add1(last - first);

        if (size == span)
        {
            auto interval = get_confirmed_interval(last);
            if (!interval.has_value()) return error::merkle_interval;
            tree.push_back(std::move(interval.value()));
        }
        else
        {
            auto confirmed = get_confirmed_hashes(first, size);
            if (confirmed.empty()) return error::merkle_hashes;
            tree.push_back(system::merkle_root(std::move(confirmed)));
        }
    }

    return error::success;
}

TEMPLATE
code CLASS::get_merkle_root_and_proof(hash_digest& root, hashes& proof,
    size_t target, size_t waypoint) const NOEXCEPT
{
    if (target > waypoint)
        return error::merkle_arguments;

    if (waypoint > get_top_confirmed())
        return error::merkle_not_found;

    hashes tree{};
    if (const auto ec = get_merkle_tree(tree, waypoint))
        return ec;

    proof.clear();
    if (const auto ec = get_merkle_proof(proof, tree, target, waypoint))
        return ec;

    root = system::merkle_root(std::move(tree));
    return {};
}

////TEMPLATE
////bool CLASS::set_address_output(const output& output,
////    const output_link& link) NOEXCEPT
////{
////    return set_address_output(output.script().hash(), link);
////}
////
////TEMPLATE
////bool CLASS::set_address_output(const hash_digest& key,
////    const output_link& link) NOEXCEPT
////{
////    if (link.is_terminal())
////        return false;
////
////    // ========================================================================
////    const auto scope = store_.get_transactor();
////
////    // Clean single allocation failure (e.g. disk full).
////    return store_.address.put(key, table::address::record
////    {
////        {},
////        link
////    });
////    // ========================================================================
////}

// filter_tx (surrogate-keyed).
// ----------------------------------------------------------------------------

TEMPLATE
bool CLASS::is_filtered_body(const header_link& link) const NOEXCEPT
{
    return store_.filter_tx.exists(to_filter_tx(link));
}

TEMPLATE
bool CLASS::get_filter_body(filter& out, const header_link& link) const NOEXCEPT
{
    table::filter_tx::get_filter filter_tx{};
    if (!store_.filter_tx.at(to_filter_tx(link), filter_tx))
        return false;

    out = std::move(filter_tx.filter);
    return true;
}

// filter_bk (surrogate-keyed).
// ----------------------------------------------------------------------------

TEMPLATE
bool CLASS::is_filtered_head(const header_link& link) const NOEXCEPT
{
    return store_.filter_bk.exists(to_filter_bk(link));
}

TEMPLATE
bool CLASS::get_filter_head(hash_digest& out,
    const header_link& link) const NOEXCEPT
{
    table::filter_bk::get_head_only filter_bk{};
    if (!store_.filter_bk.at(to_filter_bk(link), filter_bk))
        return false;

    out = std::move(filter_bk.head);
    return true;
}

TEMPLATE
bool CLASS::get_filter_hash(hash_digest& out,
    const header_link& link) const NOEXCEPT
{
    table::filter_bk::get_hash_only filter_bk{};
    if (!store_.filter_bk.at(to_filter_bk(link), filter_bk))
        return false;

    out = std::move(filter_bk.hash);
    return true;
}

TEMPLATE
bool CLASS::get_filter_hashes(hashes& filter_hashes,
    hash_digest& previous_header, const header_link& stop_link,
    size_t count) const NOEXCEPT
{
    size_t height{};
    if (!get_height(height, stop_link))
        return false;

    count = std::min(add1(height), count);
    filter_hashes.resize(count);
    auto link = stop_link;

    // Reversal allows ancenstry population into forward vector.
    for (auto& hash: std::views::reverse(filter_hashes))
    {
        // Implies that stop_link is not a filtered block.
        if (!get_filter_hash(hash, link))
            return false;

        // Ancestry from stop link (included) ensures continuity without locks.
        link = to_parent(link);
    }

    // There's no trailing increment without at least one loop iteration.
    if (is_zero(count))
        link = to_parent(link);

    // link is genesis, previous is null.
    if (link.is_terminal())
    {
        previous_header = system::null_hash;
        return true;
    }

    // Obtaining previous from ancestry eansures its continuity as well.
    return get_filter_head(previous_header, link);
}

TEMPLATE
bool CLASS::get_filter_heads(hashes& filter_heads,
    size_t stop_height, size_t interval) const NOEXCEPT
{
    size_t height{};
    filter_heads.resize(system::floored_divide(stop_height, interval));
    for (auto& head: filter_heads)
        if (!get_filter_head(head, to_confirmed((height += interval))))
            return false;

    return true;
}

// set_filter_body
// ----------------------------------------------------------------------------

TEMPLATE
bool CLASS::set_filter_body(const header_link& link,
    const block& block) NOEXCEPT
{
    using namespace system::neutrino;
    if (!filter_enabled())
        return true;

    // Compute the current filter from the block and store under the link.
    filter body{};
    return compute_filter(body, block) && set_filter_body(link, body);
}

TEMPLATE
bool CLASS::set_filter_body(const header_link& link,
    const filter& filter) NOEXCEPT
{
    if (!filter_enabled())
        return true;

    // ========================================================================
    const auto scope = store_.get_transactor();

    // Clean single allocation failure (e.g. disk full).
    return store_.filter_tx.put(to_filter_tx(link), table::filter_tx::put_ref
    {
        {},
        filter
    });
    // ========================================================================
}

// set_filter_head
// ----------------------------------------------------------------------------

TEMPLATE
bool CLASS::set_filter_head(const header_link& link) NOEXCEPT
{
    using namespace system::neutrino;
    if (!filter_enabled())
        return true;

    // The filter body must have been previously stored under the block link.
    filter body{};
    if (!get_filter_body(body, link))
        return false;

    // If genesis then parent is terminal (returns null_hash).
    hash_digest previous{};
    const auto parent = to_parent(link);
    if (!parent.is_terminal())
        if (!get_filter_head(previous, parent))
            return false;

    // Use previous head and current body to compute current hash and head.
    hash_digest hash{};
    return set_filter_head(link, compute_header(hash, previous, body), hash);
}

TEMPLATE
bool CLASS::set_filter_head(const header_link& link, const hash_digest& head,
    const hash_digest& hash) NOEXCEPT
{
    if (!filter_enabled())
        return true;

    // ========================================================================
    const auto scope = store_.get_transactor();

    // Clean single allocation failure (e.g. disk full).
    return store_.filter_bk.put(to_filter_bk(link), table::filter_bk::put_ref
    {
        {},
        hash,
        head
    });
    // ========================================================================
}

} // namespace database
} // namespace libbitcoin

#endif
