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
#ifndef LIBBITCOIN_DATABASE_PRIMITIVES_HEADMAPS_IPP
#define LIBBITCOIN_DATABASE_PRIMITIVES_HEADMAPS_IPP

#include <atomic>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/memory.hpp>

namespace libbitcoin {
namespace database {

TEMPLATE
CLASS::headmaps(storage& head) NOEXCEPT
  : file_(head)
{
}

// not thread safe
// ----------------------------------------------------------------------------

TEMPLATE
bool CLASS::create() NOEXCEPT
{
    // An empty head is a zero count.
    return is_zero(head_size());
}

TEMPLATE
bool CLASS::drop() NOEXCEPT
{
    return truncate(0);
}

TEMPLATE
bool CLASS::close() NOEXCEPT
{
    // Count is the head logical size.
    return true;
}

TEMPLATE
bool CLASS::backup(bool) NOEXCEPT
{
    // Count is the head logical size.
    return true;
}

TEMPLATE
bool CLASS::restore() NOEXCEPT
{
    // Content is fully restored with the head, so there is nothing to prune.
    return verify();
}

TEMPLATE
bool CLASS::verify() const NOEXCEPT
{
    // Columns share the row count, so there is no size to verify.
    return true;
}

// sizing
// ----------------------------------------------------------------------------

TEMPLATE
size_t CLASS::head_size() const NOEXCEPT
{
    return file_.size() * file_.width();
}

TEMPLATE
size_t CLASS::body_size() const NOEXCEPT
{
    // There is no body (all content is in the head).
    return zero;
}

TEMPLATE
Link CLASS::count() const NOEXCEPT
{
    using namespace system;
    return possible_narrow_cast<typename Link::integer>(file_.size());
}

TEMPLATE
bool CLASS::truncate(const Link& count) NOEXCEPT
{
    if (count.is_terminal())
        return false;

    return file_.truncate(system::possible_narrow_cast<size_t>(count.value));
}

TEMPLATE
bool CLASS::expand(const Link& count) NOEXCEPT
{
    if (count.is_terminal())
        return false;

    return file_.expand(system::possible_narrow_cast<size_t>(count.value));
}

// error condition
// ----------------------------------------------------------------------------

TEMPLATE
code CLASS::get_fault() const NOEXCEPT
{
    return file_.get_fault();
}

TEMPLATE
size_t CLASS::get_space() const NOEXCEPT
{
    return file_.get_space();
}

TEMPLATE
code CLASS::reload() NOEXCEPT
{
    return error::success;
}

// query interface
// ----------------------------------------------------------------------------

TEMPLATE
memory CLASS::get_memory() const NOEXCEPT
{
    return file_.get_at(zero);
}

TEMPLATE
template <size_t Column>
inline memory::iterator CLASS::get_raw() const NOEXCEPT
{
    if (is_zero(file_.size()))
        return {};

    return file_.get_raw_at(Column);
}

TEMPLATE
template <size_t Column>
inline auto CLASS::at(size_t index) const NOEXCEPT -> value<Column>
{
    return at<Column>(file_.get_at(Column), index);
}

TEMPLATE
template <size_t Column>
inline auto CLASS::at(const memory& ptr, size_t index) const NOEXCEPT
    -> value<Column>
{
    using namespace system;
    using cell = value<Column>;

    // Rows at or above the logical count are unallocated (count publishes).
    if (!ptr || (index >= file_.size()))
        return {};

    const auto raw = file_.get_raw_at(Column, link_to_position<Column>(index));
    const auto& head = *pointer_cast<std::atomic<cell>>(raw);
    return native_from_little_end(head.load(std::memory_order_acquire));
}

TEMPLATE
template <size_t Column, typename Element>
bool CLASS::set(size_t index, const Element& element) NOEXCEPT
{
    // Avoid setting the terminal sentinel into a row position.
    if (index == Link::terminal)
        return false;

    // Allocate as necessary (allocations are filled).
    if ((index >= file_.size()) && !file_.expand(add1(index)))
        return false;

    return set<Column>(file_.get_at(Column), index, element);
}

TEMPLATE
template <size_t Column, typename Element>
bool CLASS::set(const memory& ptr, size_t index,
    const Element& element) NOEXCEPT
{
    if (!ptr || (index >= file_.size()))
        return false;

    return exchange<Column>(file_.get_raw_at(Column,
        link_to_position<Column>(index)), element);
}

// private
TEMPLATE
template <size_t Column, typename Element>
bool CLASS::exchange(memory::iterator raw, const Element& element) NOEXCEPT
{
    using namespace system;
    using cell = value<Column>;
    constexpr auto size = possible_narrow_sign_cast<ptrdiff_t>(sizeof(cell));
    static_assert(Element::size == sizeof(cell));

    auto& top = *pointer_cast<std::atomic<cell>>(raw);
    auto head = top.load(std::memory_order_acquire);
    auto written = true;
    cell update{};
    do
    {
        auto copy = element;
        iostream current{ pointer_cast<uint8_t>(&head), size };
        reader source{ current };
        iostream replacement{ pointer_cast<uint8_t>(&update), size };
        flipper sink{ replacement };
        written = copy.from_data(source) && copy.to_data(sink);
    }
    while (written && !top.compare_exchange_weak(head, update,
        std::memory_order_release, std::memory_order_acquire));

    return written;
}

} // namespace database
} // namespace libbitcoin

#endif
