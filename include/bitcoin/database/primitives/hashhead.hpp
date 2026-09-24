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
#ifndef LIBBITCOIN_DATABASE_PRIMITIVES_HEAD_HPP
#define LIBBITCOIN_DATABASE_PRIMITIVES_HEAD_HPP

#include <atomic>
#include <shared_mutex>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/memory.hpp>
#include <bitcoin/database/primitives/keys.hpp>
#include <bitcoin/database/primitives/linkage.hpp>

namespace libbitcoin {
namespace database {

/// Fixed size hashmap header.
template <class Link, class Key, size_t CellSize = Link::size,
    if_not_greater<Link::size, CellSize> = true>
class hashhead
{
public:
    DELETE_COPY_MOVE_DESTRUCT(hashhead);

    using bytes = typename Link::bytes;

    /// A hash head is disabled if it has no buckets.
    hashhead(storage& head, size_t buckets, size_t expected=zero) NOEXCEPT;

    /// Sizing (thread safe).
    inline size_t size() const NOEXCEPT;
    inline size_t buckets() const NOEXCEPT;

    /// Filter selections, derived at construct, overridden by set_filter_k.
    inline size_t filter_k() const NOEXCEPT;

    /// Set filter selections from stored envelope (not thread safe).
    bool set_filter_k(size_t k) NOEXCEPT;

    /// Set bucket count from stored envelope (not thread safe).
    bool set_buckets(size_t buckets) NOEXCEPT;

    /// Optimal filter k-value for expected load factor.
    static size_t optimal_k(size_t count, size_t buckets) NOEXCEPT;

    /// Buckets for expected elements, scaled by memory between load factors.
    static uint32_t derive_buckets(uint64_t expected, uint32_t low,
        uint32_t high) NOEXCEPT;

    /// Create from empty head file (not thread safe).
    bool create() NOEXCEPT;

    /// Clear the existing index of all links.
    bool clear() NOEXCEPT;

    /// The first fault code recorded by the head storage.
    code get_fault() const NOEXCEPT;

    /// The space required by a failed head storage allocation.
    size_t get_space() const NOEXCEPT;

    /// False if head file size incorrect (not thread safe).
    bool verify() const NOEXCEPT;

    /// Unsafe if verify false (not thread safe).
    bool get_body_count(Link& count) const NOEXCEPT;
    bool set_body_count(const Link& count) NOEXCEPT;

    /// Convert natural key to head bucket index (all keys are valid).
    /// Terminal is a valid bucket index (just not a valid bucket value).
    inline Link index(const Key& key) const NOEXCEPT;

    /// Unsafe if verify false.
    inline Link top(const Key& key) const NOEXCEPT;
    inline Link top(const Link& index) const NOEXCEPT;
    inline bool push(const Link& current, bytes& next, const Key& key) NOEXCEPT;
    inline bool push(bool& collision, const Link& current, bytes& next,
        const Key& key) NOEXCEPT;

protected:

    // filtering
    // ------------------------------------------------------------------------

    static constexpr size_t cell_size = CellSize;
    static constexpr size_t link_size = Link::size;
    static constexpr size_t link_bits = Link::bits;
    static constexpr size_t m = to_bits(cell_size) - link_bits;
    static constexpr size_t k_default = system::floored_log2(m);
    static constexpr size_t k_max = (m < two) ? zero : std::min(m,
        bits<uint64_t> / system::ceilinged_log2(m));

    using filter_t = system::bloom<m, k_max>;
    using cell = unsigned_type<cell_size>;
    using filter = filter_t::type;
    using link = Link::integer;

    static constexpr cell terminal = system::bit_all<cell>;
    static constexpr bool aligned = (cell_size == sizeof(cell));
    static_assert(link_bits + m == to_bits(cell_size));
    static_assert(std::atomic<cell>::is_always_lock_free);
    static_assert(is_nonzero(Link::size));

    static_assert(is_zero(k_max) || (k_default <= k_max));

    INLINE static constexpr filter to_filter(cell value) NOEXCEPT;
    INLINE static constexpr link to_link(cell value) NOEXCEPT;
    INLINE bool screened(cell value, uint64_t entropy) const NOEXCEPT;
    INLINE cell next_cell(bool& collision, cell previous,
        link current, uint64_t entropy) const NOEXCEPT;

    static double bloom_false_positive(size_t k, size_t n) NOEXCEPT;
    static size_t poisson_span(double load_factor) NOEXCEPT;
    static double expected_walk(size_t k, double load_factor) NOEXCEPT;

    inline cell get_cell(const Link& index) const NOEXCEPT;
    inline bool set_cell(bool& collision, bytes& next, const Link& current,
        const Key& key) NOEXCEPT;

    // ------------------------------------------------------------------------

private:
    INLINE static auto& cell_array(memory::iterator it) NOEXCEPT
    {
        return system::unsafe_array_cast<uint8_t, cell_size>(it);
    }

    template <typename Integral, if_integral<Integral> = true>
    INLINE static auto& cell_array(Integral& value) NOEXCEPT
    {
        return cell_array(system::pointer_cast<uint8_t>(&value));
    }

    INLINE static auto& link_array(memory::iterator it) NOEXCEPT
    {
        return system::unsafe_array_cast<uint8_t, link_size>(it);
    }

    template <typename Integral, if_integral<Integral> = true>
    INLINE static auto& link_array(Integral& value) NOEXCEPT
    {
        return link_array(system::pointer_cast<uint8_t>(&value));
    }

    // Byte offset of bucket index within head file.
    // [body_size][[bucket[0]...bucket[buckets-1]]]
    INLINE static constexpr size_t link_to_position(const Link& index) NOEXCEPT
    {
        using namespace system;
        BC_ASSERT(!is_multiply_overflow<size_t>(index, cell_size));
        BC_ASSERT(!is_add_overflow<size_t>(cell_size, index * cell_size));
        return possible_narrow_cast<size_t>(add1<size_t>(index) * cell_size);
    }

    // These are thread safe.
    storage& file_;
    mutable std::shared_mutex mutex_{};

    // Protected by order - derived at construct, envelope overrides at open.
    Link buckets_;
    size_t k_;
};

} // namespace database
} // namespace libbitcoin

#define TEMPLATE template <class Link, class Key, size_t CellSize, \
    if_not_greater<Link::size, CellSize> If>
#define CLASS hashhead<Link, Key, CellSize, If>

#include <bitcoin/database/impl/primitives/hashhead.ipp>

#undef CLASS
#undef TEMPLATE

#endif
