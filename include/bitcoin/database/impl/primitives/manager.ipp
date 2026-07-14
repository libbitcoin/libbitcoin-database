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
#ifndef LIBBITCOIN_DATABASE_PRIMITIVES_MANAGER_IPP
#define LIBBITCOIN_DATABASE_PRIMITIVES_MANAGER_IPP

#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

TEMPLATE
template <size_t Column>
inline memory_ptr CLASS::get() const NOEXCEPT
{
    if constexpr (is_one(columns))
        return files_.get();
    else
        return files_.get_at(Column);
}

TEMPLATE
template <size_t Column>
inline memory CLASS::get1() const NOEXCEPT
{
    if constexpr (is_one(columns))
        return files_.get1();
    else
        return files_.get_at1(Column);
}

TEMPLATE
template <size_t Column>
inline memory_ptr CLASS::get(const Link& link) const NOEXCEPT
{
    if (link.is_terminal())
        return {};

    const auto position = link_to_position<Column>(link);

    // memory.size() may be negative (stream treats as exhausted).
    if constexpr (is_one(columns))
        return files_.get(position);
    else
        return files_.get_at(Column, position);
}

TEMPLATE
template <size_t Column>
inline memory CLASS::get1(const Link& link) const NOEXCEPT
{
    if (link.is_terminal())
        return {};

    const auto position = link_to_position<Column>(link);

    // memory.size() may be negative (stream treats as exhausted).
    if constexpr (is_one(columns))
        return files_.get1(position);
    else
        return files_.get_at1(Column, position);
}

TEMPLATE
template <size_t Column>
inline memory::iterator CLASS::get_raw1(const Link& link) const NOEXCEPT
{
    if (link.is_terminal())
        return {};

    const auto position = link_to_position<Column>(link);

    // memory.size() may be negative (stream treats as exhausted).
    if constexpr (is_one(columns))
        return files_.get_raw(position);
    else
        return files_.get_at_raw(Column, position);
}

TEMPLATE
template <size_t Columns, if_equal<Columns, one>>
inline memory_ptr CLASS::get_capacity(const Link& link) const NOEXCEPT
{
    if (link.is_terminal())
        return {};

    return files_.get_capacity(link_to_position(link));
}

TEMPLATE
CLASS::managers(storage& body) NOEXCEPT
  : files_(body)
{
}

TEMPLATE
inline size_t CLASS::size() const NOEXCEPT
{
    if constexpr (is_one(columns))
        return files_.size();
    else
        return files_.size() * strides(std::make_index_sequence<columns>{});
}

TEMPLATE
inline size_t CLASS::capacity() const NOEXCEPT
{
    if constexpr (is_one(columns))
        return files_.capacity();
    else
        return files_.capacity() * strides(std::make_index_sequence<columns>{});
}

TEMPLATE
inline Link CLASS::count() const NOEXCEPT
{
    return elements_to_link(files_.size());
}

TEMPLATE
bool CLASS::truncate(const Link& count) NOEXCEPT
{
    if (count.is_terminal())
        return false;

    // Truncate to count visible records (absolute, shared row count).
    return files_.truncate(link_to_elements(count));
}

TEMPLATE
bool CLASS::expand(const Link& count) NOEXCEPT
{
    if (count.is_terminal())
        return false;

    // Expand to count visible records (absolute, shared row count).
    return files_.expand(link_to_elements(count));
}

TEMPLATE
bool CLASS::reserve(const Link& count) NOEXCEPT
{
    if (count.is_terminal())
        return false;

    // Expand by count invisible records (relative, shared row count).
    return files_.reserve(link_to_elements(count));
}

TEMPLATE
Link CLASS::allocate(const Link& count) NOEXCEPT
{
    if (count.is_terminal())
        return count;

    // One shared allocation across all columns (one lock). Expand by count
    // visible records (relative), return absolute record link of first.
    const auto start = files_.allocate(link_to_elements(count));

    // Guards addition overflow in cast_link (start must be valid). The eof
    // sentinel is in file denomination, so must be detected before conversion.
    if (start == storage::eof)
        return Link::terminal;

    // Callers (nomaps) handle terminal allocation.
    return elements_to_link(start);
}

// Errors.
// ----------------------------------------------------------------------------

TEMPLATE
code CLASS::get_fault() const NOEXCEPT
{
    return files_.get_fault();
}

TEMPLATE
size_t CLASS::get_space() const NOEXCEPT
{
    return files_.get_space();
}

TEMPLATE
code CLASS::reload() NOEXCEPT
{
    return files_.load();
}

// static
// ----------------------------------------------------------------------------

// private
TEMPLATE
template <size_t Column>
constexpr size_t CLASS::stride() NOEXCEPT
{
    using namespace system;
    constexpr auto size = std::get<Column>(sizes);

    if constexpr (size == max_size_t)
    {
        // Slab: link/key incorporated into size (byte-addressed map).
        return size;
    }
    else if constexpr (is_zero(Column) && is_nonzero(key_size))
    {
        // Spine of a keyed map: link/key precede the record.
        return Link::size + key_size + size;
    }
    else
    {
        // Keyless (nomap/arraymap) or non-spine column: pure record.
        static_assert(is_nonzero(size));
        return size;
    }
}

// private
TEMPLATE
template <size_t... Index>
constexpr size_t CLASS::strides(std::index_sequence<Index...>) NOEXCEPT
{
    return (stride<Index>() + ...);
}

TEMPLATE
template <size_t Column>
constexpr size_t CLASS::link_to_position(const Link& link) NOEXCEPT
{
    using namespace system;
    const auto value = possible_narrow_cast<size_t>(link.value);
    constexpr auto element_size = stride<Column>();

    if constexpr (is_slab)
    {
        // Slab implies link/key incorporated into size.
        return value;
    }
    else
    {
        // Record (keyed spine or keyless) implies fixed element stride.
        BC_ASSERT(!is_multiply_overflow(value, element_size));
        return value * element_size;
    }
}

TEMPLATE
template <size_t Column>
constexpr Link CLASS::position_to_link(size_t position) NOEXCEPT
{
    using namespace system;
    constexpr auto element_size = stride<Column>();

    if constexpr (is_slab)
    {
        // Slab implies link/key incorporated into size.
        return { cast_link(position) };
    }
    else
    {
        static_assert(is_nonzero(element_size));
        return { cast_link(position / element_size) };
    }
}

// private
TEMPLATE
constexpr size_t CLASS::link_to_elements(const Link& link) NOEXCEPT
{
    using namespace system;

    // Single column: file elements are bytes (width one). Records convert
    // by stride (slab passes through, slab links are already byte offsets).
    // Aggregate: file rows are the shared record count (no conversion).
    if constexpr (is_one(columns))
        return link_to_position<zero>(link);
    else
        return possible_narrow_cast<size_t>(link.value);
}

// private
TEMPLATE
constexpr Link CLASS::elements_to_link(size_t elements) NOEXCEPT
{
    // Inverse of link_to_elements (slab and aggregate pass through).
    if constexpr (is_one(columns))
        return position_to_link<zero>(elements);
    else
        return { cast_link(elements) };
}

TEMPLATE
constexpr CLASS::integer CLASS::cast_link(size_t link) NOEXCEPT
{
    using namespace system;
    constexpr auto terminal = Link::terminal;

    // link limit is sub1(terminal), where terminal is 2^((8*Link::bytes)-1).
    // It is ok for the payload to exceed link limit (link is identity only).
    return link >= terminal ? terminal : possible_narrow_cast<integer>(link);
}

} // namespace database
} // namespace libbitcoin

#endif
