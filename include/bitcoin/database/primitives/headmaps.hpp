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
#ifndef LIBBITCOIN_DATABASE_PRIMITIVES_HEADMAPS_HPP
#define LIBBITCOIN_DATABASE_PRIMITIVES_HEADMAPS_HPP

#include <array>
#include <atomic>
#include <tuple>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/memory.hpp>
#include <bitcoin/database/primitives/linkage.hpp>

namespace libbitcoin {
namespace database {

/// SoA aggregate head-only array: value columns indexed by position (no body,
/// no count cell). Storage logical row count is the count, common across
/// columns, with allocation unified across all columns (one lock). One storage
/// backs all columns, so accessor guards every column against remap.
/// Value is opaque integral of its column's cell size.
template <class Link, size_t... Cells>
class headmaps
{
public:
    DELETE_COPY_MOVE_DESTRUCT(headmaps);

    using link = Link;

    static constexpr size_t columns = sizeof...(Cells);
    static constexpr std::array<size_t, columns> widths{ Cells... };

    template <size_t Column>
    using value = std::tuple_element_t<Column,
        std::tuple<unsigned_type<Cells>...>>;

    headmaps(storage& head) NOEXCEPT;

    /// Setup, not thread safe.
    /// -----------------------------------------------------------------------

    bool create() NOEXCEPT;
    bool drop() NOEXCEPT;
    bool close() NOEXCEPT;
    bool backup(bool=false) NOEXCEPT;
    bool restore() NOEXCEPT;
    bool verify() const NOEXCEPT;

    /// Sizing.
    /// -----------------------------------------------------------------------

    /// Aggregate head bytes across all columns.
    size_t head_size() const NOEXCEPT;

    /// Body file bytes (zero, no body).
    size_t body_size() const NOEXCEPT;

    /// Count of rows (common across columns).
    Link count() const NOEXCEPT;

    /// Reduce count as specified.
    bool truncate(const Link& count) NOEXCEPT;

    /// Increase count as specified (filled empty), must not hold accessor.
    bool expand(const Link& count) NOEXCEPT;

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

    /// Head accessor (shared lock), guards all columns.
    memory get_memory() const NOEXCEPT;

    /// Column base iterator, null if empty (guard required).
    template <size_t Column>
    inline memory::iterator get_raw() const NOEXCEPT;

    /// Return value at index, default if not allocated.
    template <size_t Column>
    inline auto at(size_t index) const NOEXCEPT -> value<Column>;

    /// Return value at index using accessor, default if not allocated.
    template <size_t Column>
    inline auto at(const memory& ptr, size_t index) const NOEXCEPT
        -> value<Column>;

    /// Set element at index, extending count as necessary (filled empty).
    /// Element reads current cell and writes its replacement, and is rerun from
    /// a copy for each failed exchange.
    template <size_t Column, typename Element>
    bool set(size_t index, const Element& element) NOEXCEPT;

    /// Set element at allocated index using accessor.
    template <size_t Column, typename Element>
    bool set(const memory& ptr, size_t index, const Element& element) NOEXCEPT;

private:
    static_assert(columns > one, "requires more than one column");
    static_assert(((sizeof(unsigned_type<Cells>) == Cells) && ...));
    static_assert((std::atomic<unsigned_type<Cells>>::is_always_lock_free
        && ...));
    static_assert(is_nonzero(Link::size));

    // Byte offset of row index within the column file.
    template <size_t Column>
    INLINE static constexpr size_t link_to_position(size_t index) NOEXCEPT
    {
        BC_ASSERT(!system::is_multiply_overflow(index, widths.at(Column)));
        return index * widths.at(Column);
    }

    template <size_t Column, typename Element>
    static bool exchange(memory::iterator raw, const Element& element) NOEXCEPT;

    // This is thread safe.
    storage& file_;
};

/// Schema is the row link schema; Columns are the cell column tables.
template <typename Schema, typename... Columns>
using head_maps = headmaps<typename Schema::link, Columns::width...>;

/// Named column forwarder: binds a column index onto an aggregate head.
template <typename Table, size_t Column>
class head_column
{
public:
    using value = typename Table::template value<Column>;

    INLINE head_column(Table& table) NOEXCEPT
      : table_(table)
    {
    }

    INLINE memory::iterator get_raw() const NOEXCEPT
    {
        return table_.template get_raw<Column>();
    }

    INLINE value at(size_t index) const NOEXCEPT
    {
        return table_.template at<Column>(index);
    }

    INLINE value at(const memory& ptr, size_t index) const NOEXCEPT
    {
        return table_.template at<Column>(ptr, index);
    }

    template <typename Element>
    INLINE bool set(size_t index, const Element& element) NOEXCEPT
    {
        return table_.template set<Column>(index, element);
    }

    template <typename Element>
    INLINE bool set(const memory& ptr, size_t index,
        const Element& element) NOEXCEPT
    {
        return table_.template set<Column>(ptr, index, element);
    }

private:
    Table& table_;
};

} // namespace database
} // namespace libbitcoin

#define TEMPLATE template <class Link, size_t... Cells>
#define CLASS headmaps<Link, Cells...>

#include <bitcoin/database/impl/primitives/headmaps.ipp>

#undef CLASS
#undef TEMPLATE

#endif
