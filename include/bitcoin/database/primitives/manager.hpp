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
#ifndef LIBBITCOIN_DATABASE_PRIMITIVES_MANAGER_HPP
#define LIBBITCOIN_DATABASE_PRIMITIVES_MANAGER_HPP

#include <utility>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/memory.hpp>
#include <bitcoin/database/primitives/keys.hpp>

namespace libbitcoin {
namespace database {

template <class Link, class Key, size_t... Sizes>
class managers
{
public:
    using integer = typename Link::integer;

    template <size_t Column = zero>
    static constexpr Link position_to_link(size_t position) NOEXCEPT;
    template <size_t Column = zero>
    static constexpr size_t link_to_position(const Link& link) NOEXCEPT;
    static constexpr integer cast_link(size_t link) NOEXCEPT;

public:
    DEFAULT_COPY_MOVE_DESTRUCT(managers);

    /// Return memory object for column full map (null only if oom or unloaded).
    template <size_t Column = zero>
    inline memory_ptr get() const NOEXCEPT;
    template <size_t Column = zero>
    inline memory get1() const NOEXCEPT;

    /// Return memory object for column record (null only if oom or unloaded).
    /// Pointer is constrained to starting write within logical allocation.
    template <size_t Column = zero>
    inline memory_ptr get(const Link& link) const NOEXCEPT;
    template <size_t Column = zero>
    inline memory get1(const Link& link) const NOEXCEPT;
    template <size_t Column = zero>
    inline memory::iterator get_raw1(const Link& link) const NOEXCEPT;

    /// Return memory object (limited to AoS) within capacity.
    template <size_t Columns = sizeof...(Sizes), if_equal<Columns, one> = true>
    inline memory_ptr get_capacity(const Link& link) const NOEXCEPT;

    /// Manage shared multi-backed byte storage device (caller owns storage).
    managers(storage& body) NOEXCEPT;

    /// The aggregate logical byte size (cold size) across all columns.
    inline size_t size() const NOEXCEPT;

    /// The aggregate byte capacity (hot size) across all columns.
    inline size_t capacity() const NOEXCEPT;

    /// The logical record count (common across columns).
    inline Link count() const NOEXCEPT;

    /// Reduce logical size to count records (false if exceeds logical).
    bool truncate(const Link& count) NOEXCEPT;

    /// Increase logical size to count records as required (false if fails).
    bool expand(const Link& count) NOEXCEPT;

    /// Thread safe but reservations do not accumulate (effectively unsafe).
    /// Increase capacity by count records (false only if fails).
    bool reserve(const Link& count) NOEXCEPT;

    /// Unified allocation across all columns (one lock).
    /// Increase logical size by count records, return offset to first (or eof).
    Link allocate(const Link& count) NOEXCEPT;

    /// Get the unified fault condition.
    code get_fault() const NOEXCEPT;

    /// Get the unified space required to clear the disk full condition.
    size_t get_space() const NOEXCEPT;

    /// Unified resume from disk full condition.
    code reload() NOEXCEPT;

private:
    static constexpr auto columns = sizeof...(Sizes);
    static constexpr auto key_size = keys::size<Key>();
    static constexpr std::array<size_t, columns> sizes{ Sizes... };
    static constexpr auto is_slab = (std::get<zero>(sizes) == max_size_t);
    static_assert(!is_slab || is_one(columns), "slab implies single column");
    static_assert(!is_zero(columns), "requires at least one column");

    template <size_t Column>
    static constexpr size_t stride() NOEXCEPT;
    template <size_t... Index>
    static constexpr size_t strides(std::index_sequence<Index...>) NOEXCEPT;

    /// Convert between record links and the file's native denomination
    /// (elements). Single column file elements are BYTES (width one), so
    /// records convert by stride (slabs pass through, links are byte offsets).
    /// Aggregate file elements are ROWS, which equal records (pass through).
    static constexpr size_t link_to_elements(const Link& link) NOEXCEPT;
    static constexpr Link elements_to_link(size_t elements) NOEXCEPT;

    // Thread and remap safe.
    storage& files_;
};

template <class Link, class Key, size_t Size>
using manager = managers<Link, Key, Size>;

} // namespace database
} // namespace libbitcoin

#define TEMPLATE template <class Link, class Key, size_t... Sizes>
#define CLASS managers<Link, Key, Sizes...>

#include <bitcoin/database/impl/primitives/manager.ipp>

#undef CLASS
#undef TEMPLATE

#endif
