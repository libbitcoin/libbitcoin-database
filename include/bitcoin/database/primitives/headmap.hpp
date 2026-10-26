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
#ifndef LIBBITCOIN_DATABASE_PRIMITIVES_HEADMAP_HPP
#define LIBBITCOIN_DATABASE_PRIMITIVES_HEADMAP_HPP

#include <atomic>
#include <shared_mutex>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/locks/locks.hpp>
#include <bitcoin/database/memory/memory.hpp>
#include <bitcoin/database/primitives/linkage.hpp>

namespace libbitcoin {
namespace database {

/// A head-only array of values indexed by position (no body, no count cell).
/// The storage logical size is the count, providing count atomicity and
/// bucket publication as with bodies. Content restores with the heads.
/// A value is a link (masked to terminal) or an opaque integral, of no more
/// bytes than the cell or the link.
/// A cell size of max_size_t is a slab, in which the link is a byte offset
/// and elements are serialized as with bodies (cell accessors do not apply).
template <class Link, size_t Cell = Link::size, class Value = Link,
    if_not_greater<value_size<Value>(), std::min(Cell, Link::size)> = true>
class headmap
{
public:
    DELETE_COPY_MOVE_DESTRUCT(headmap);

    using link = Link;

    /// Buckets are filled at create, a push head has none.
    headmap(storage& head, size_t buckets=zero) NOEXCEPT;

    /// Setup, not thread safe.
    /// -----------------------------------------------------------------------

    bool create() NOEXCEPT;
    bool close() NOEXCEPT;
    bool backup(bool=false) NOEXCEPT;
    bool restore() NOEXCEPT;
    bool verify() const NOEXCEPT;

    /// Sizing.
    /// -----------------------------------------------------------------------

    /// Head file bytes.
    size_t head_size() const NOEXCEPT;

    /// Body file bytes (zero, no body).
    size_t body_size() const NOEXCEPT;

    /// Count of buckets (links).
    Link count() const NOEXCEPT;

    /// Reduce count as specified (truncated buckets are rewritten by push).
    bool truncate(const Link& count) NOEXCEPT;

    /// Reserve additional count buckets to guard against disk full.
    /// Not writer-writer thread safe.
    bool reserve(const Link& count) NOEXCEPT;

    /// Errors.
    /// -----------------------------------------------------------------------

    /// Get the fault condition.
    code get_fault() const NOEXCEPT;

    /// Get the space required to clear the disk full condition.
    size_t get_space() const NOEXCEPT;

    /// Resume from disk full condition (nothing to reload, no body).
    code reload() NOEXCEPT;

    /// Query interface.
    /// -----------------------------------------------------------------------

    /// Head accessor (shared lock), for at(ptr, index) over many indexes.
    memory get_memory() const NOEXCEPT;

    /// Return value at index, default (terminal link) if not allocated.
    Value at(size_t index) const NOEXCEPT;

    /// Return value at index using accessor, default if not allocated.
    Value at(const memory& ptr, size_t index) const NOEXCEPT;

    /// Return link at the top index, terminal if empty or unloaded.
    Value top() const NOEXCEPT;

    /// Append link at count into reserved capacity (single writer).
    bool push(const Link& link) NOEXCEPT;

    /// Get element at index, false if not allocated or deserialize error.
    template <typename Element, if_equal<Element::size, Cell> = true,
        if_not_equal<Element::size, max_size_t> = true>
    bool get(size_t index, Element& element) const NOEXCEPT;

    /// Set element at index, extending count as necessary (filled empty).
    /// The element reads the current cell and writes its replacement, and is
    /// rerun from a copy for each failed exchange.
    template <typename Element, if_equal<Element::size, Cell> = true,
        if_not_equal<Element::size, max_size_t> = true>
    bool set(size_t index, const Element& element) NOEXCEPT;

    /// Slab.
    /// -----------------------------------------------------------------------

    /// Allocate slab bytes at returned link (follow with put).
    Link allocate(const Link& bytes) NOEXCEPT;

    /// Get element at link, false if deserialize error.
    template <typename Element, if_equal<Element::size, Cell> = true>
    bool get(const Link& link, Element& element) const NOEXCEPT;

    /// Put element at allocated link, false if serialize error.
    template <typename Element, if_equal<Element::size, Cell> = true>
    bool put(const Link& link, const Element& element) NOEXCEPT;

private:
    // A slab is byte-addressed, so the cell is unused and a byte wide.
    static constexpr bool is_slab = (Cell == max_size_t);
    using cell = unsigned_type<is_slab ? one : Cell>;
    static constexpr size_t bucket_size = is_slab ? one : Cell;
    static constexpr bool aligned = (bucket_size == sizeof(cell));
    static_assert(is_nonzero(Link::size));
    static_assert(std::atomic<cell>::is_always_lock_free);

    // Links are ordered by count publication, opaque values by the cell.
    static constexpr auto is_link = is_linkage<Value>;
    static constexpr auto cell_size = system::possible_narrow_sign_cast<
        ptrdiff_t>(bucket_size);
    static constexpr auto load_order = is_link ? std::memory_order_relaxed :
        std::memory_order_acquire;
    static constexpr auto store_order = is_link ? std::memory_order_relaxed :
        std::memory_order_release;

    INLINE static auto& cell_array(memory::iterator it) NOEXCEPT
    {
        return system::unsafe_array_cast<uint8_t, bucket_size>(it);
    }

    template <typename Integral>
    INLINE static auto& cell_array(Integral& value) NOEXCEPT
    {
        return cell_array(system::pointer_cast<uint8_t>(&value));
    }

    INLINE static constexpr Value to_value(cell value) NOEXCEPT
    {
        using namespace system;
        if constexpr (is_link)
            return possible_narrow_cast<typename Value::integer>(
                bit_and<cell>(Value::terminal, value));
        else
            return possible_narrow_cast<Value>(value);
    }

    // Byte offset of bucket index within head file.
    // [[bucket[0]...bucket[count-1]]]
    INLINE static constexpr size_t link_to_position(size_t index) NOEXCEPT
    {
        BC_ASSERT(!system::is_multiply_overflow(index, bucket_size));
        return index * bucket_size;
    }

    INLINE static constexpr Link position_to_link(size_t position) NOEXCEPT
    {
        using namespace system;
        return possible_narrow_cast<typename Link::integer>(
            floored_divide(position, bucket_size));
    }

    Value load(memory::iterator raw) const NOEXCEPT;

    // These are thread safe.
    storage& file_;
    const size_t initial_buckets_;
    mutable shared_mutex mutex_{};
};

template <typename Schema>
using head_map = headmap<typename Schema::link, Schema::cell>;

template <typename Schema>
using value_map = headmap<typename Schema::link, Schema::cell,
    typename Schema::value>;

} // namespace database
} // namespace libbitcoin

#define TEMPLATE template <class Link, size_t Cell, class Value, \
    if_not_greater<value_size<Value>(), std::min(Cell, Link::size)> If>
#define CLASS headmap<Link, Cell, Value, If>

#include <bitcoin/database/impl/primitives/headmap.ipp>

#undef CLASS
#undef TEMPLATE

#endif
