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
#ifndef LIBBITCOIN_DATABASE_PRIMITIVES_HEADMAP_IPP
#define LIBBITCOIN_DATABASE_PRIMITIVES_HEADMAP_IPP

#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

TEMPLATE
CLASS::headmap(storage& head, size_t buckets) NOEXCEPT
  : file_(head), initial_buckets_(buckets)
{
}

// not thread safe
// ----------------------------------------------------------------------------

TEMPLATE
bool CLASS::create() NOEXCEPT
{
    if (is_nonzero(head_size()))
        return false;

    // An empty head is a zero count.
    if (is_zero(initial_buckets_))
        return true;

    return file_.allocate_filled(link_to_position(initial_buckets_)) !=
        storage::eof;
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
    return is_zero(head_size() % bucket_size);
}

// sizing
// ----------------------------------------------------------------------------

TEMPLATE
size_t CLASS::head_size() const NOEXCEPT
{
    return file_.size();
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
    return position_to_link(head_size());
}

TEMPLATE
bool CLASS::truncate(const Link& count) NOEXCEPT
{
    if (count.is_terminal())
        return false;

    return file_.truncate(link_to_position(count));
}

TEMPLATE
bool CLASS::reserve(const Link& count) NOEXCEPT
{
    if (count.is_terminal())
        return false;

    return file_.reserve(link_to_position(count));
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
    return file_.get();
}

TEMPLATE
Value CLASS::at(size_t index) const NOEXCEPT
{
    // Buckets at or above the logical size are unallocated (count publishes).
    if (index >= count())
        return {};

    const auto ptr = file_.get(link_to_position(index));
    if (!ptr)
        return {};

    return load(ptr.data());
}

TEMPLATE
Value CLASS::at(const memory& ptr, size_t index) const NOEXCEPT
{
    using namespace system;

    // Buckets at or above the logical size are unallocated (count publishes).
    if (index >= count())
        return {};

    const auto raw = ptr.offset(link_to_position(index));
    if (is_null(raw))
        return {};

    return load(raw);
}

TEMPLATE
Value CLASS::top() const NOEXCEPT
{
    static_assert(is_link);

    const auto size = count();
    if (is_zero(size))
        return {};

    const auto ptr = file_.get(link_to_position(sub1(size)));
    if (!ptr)
        return {};

    return load(ptr.data());
}

TEMPLATE
Value CLASS::load(memory::iterator raw) const NOEXCEPT
{
    using namespace system;
    if constexpr (aligned)
    {
        // Reads full padded word (masked by to_value).
        const auto& head = *pointer_cast<std::atomic<cell>>(raw);
        return to_value(head.load(load_order));
    }
    else
    {
        const auto& head = cell_array(raw);
        cell value{};

        mutex_.lock_shared();
        cell_array(value) = head;
        mutex_.unlock_shared();

        return to_value(value);
    }
}

// slab
// ----------------------------------------------------------------------------

TEMPLATE
Link CLASS::allocate(const Link& bytes) NOEXCEPT
{
    static_assert(is_slab);

    if (bytes.is_terminal())
        return Link::terminal;

    const auto size = system::possible_narrow_cast<size_t>(bytes.value);
    const auto link = file_.allocate(size);
    if (link == storage::eof)
        return Link::terminal;

    return system::possible_narrow_cast<typename Link::integer>(link);
}

TEMPLATE
template <typename Element, if_equal<Element::size, Cell>>
bool CLASS::get(const Link& link, Element& element) const NOEXCEPT
{
    using namespace system;
    static_assert(is_slab);

    if (link.is_terminal())
        return false;

    const auto ptr = file_.get();
    if (!ptr)
        return false;

    const auto size = ptr.size();
    const auto position = possible_narrow_sign_cast<ptrdiff_t>(link.value);
    if (position >= size)
        return false;

    const auto start = possible_narrow_cast<size_t>(link.value);
    const auto offset = ptr.offset(start);
    if (is_null(offset))
        return false;

    iostream stream{ offset, size - position };
    reader source{ stream };
    return element.from_data(source);
}

// NOT WRITER-WRITER THREAD SAFE (the element is read-write).
TEMPLATE
template <typename Element, if_equal<Element::size, Cell>>
bool CLASS::put(const Link& link, const Element& element) NOEXCEPT
{
    using namespace system;
    static_assert(is_slab);

    if (link.is_terminal())
        return false;

    const auto start = possible_narrow_cast<size_t>(link.value);
    const auto ptr = file_.get(start);
    const auto bytes = possible_narrow_cast<size_t>(element.count().value);
    const auto size = possible_narrow_sign_cast<ptrdiff_t>(bytes);
    if (!ptr || (ptr.size() < size))
        return false;

    file_.prepare(start, bytes);

    iostream stream{ ptr.data(), size };
    flipper sink{ stream };
    const auto written = element.to_data(sink);
    file_.mark(start, bytes);
    return written;
}

// NOT WRITER-WRITER THREAD SAFE (the logical top is read-write).
TEMPLATE
bool CLASS::push(const Link& link) NOEXCEPT
{
    using namespace system;
    if (link.is_terminal())
        return false;

    const auto position = head_size();

    // Dispose accessor (lock) before allocate.
    {
        const auto ptr = file_.get_capacity(position);
        if (!ptr || (ptr.size() < possible_narrow_sign_cast<ptrdiff_t>(
            bucket_size)))
            return false;

        // Writes into unpublished capacity.
        file_.prepare(position, bucket_size);
        const auto raw = ptr.data();
        cell value = link.value;
        if constexpr (aligned)
        {
            // Writes full padded word.
            auto& head = *pointer_cast<std::atomic<cell>>(raw);
            head.store(value, std::memory_order_relaxed);
        }
        else
        {
            auto& head = cell_array(raw);

            mutex_.lock();
            head = cell_array(value);
            mutex_.unlock();
        }

        file_.mark(position, bucket_size);
    }

    // Publication of the pre-written bucket is its allocation.
    return file_.allocate(bucket_size) != storage::eof;
}

// cell
// ----------------------------------------------------------------------------

TEMPLATE
template <typename Element, if_equal<Element::size, Cell>,
    if_not_equal<Element::size, max_size_t>>
bool CLASS::get(size_t index, Element& element) const NOEXCEPT
{
    using namespace system;
    static_assert(!is_slab);

    // Buckets at or above the logical size are unallocated (count publishes).
    if (index >= count())
        return false;

    const auto ptr = file_.get(link_to_position(index));
    if (!ptr)
        return false;

    const auto raw = ptr.data();
    cell value{};
    if constexpr (aligned)
    {
        const auto& head = *pointer_cast<std::atomic<cell>>(raw);
        value = head.load(load_order);
    }
    else
    {
        const auto& head = cell_array(raw);

        mutex_.lock_shared();
        cell_array(value) = head;
        mutex_.unlock_shared();
    }

    iostream stream{ pointer_cast<uint8_t>(&value), cell_size };
    reader source{ stream };
    return element.from_data(source);
}

TEMPLATE
template <typename Element, if_equal<Element::size, Cell>,
    if_not_equal<Element::size, max_size_t>>
bool CLASS::set(size_t index, const Element& element) NOEXCEPT
{
    using namespace system;
    static_assert(!is_slab);
    static_assert(aligned, "set requires an aligned cell");

    // Avoid setting at/above terminal sentinel into a bucket position.
    if (index >= Link::terminal)
        return false;

    // Allocate as necessary (allocations are filled).
    const auto position = link_to_position(index);
    const auto ptr = file_.get_filled(position, bucket_size);
    if (!ptr)
        return false;

    return exchange(ptr.data(), position, element);
}

TEMPLATE
template <typename Element, if_equal<Element::size, Cell>,
    if_not_equal<Element::size, max_size_t>>
bool CLASS::set(const std::vector<typename Link::integer>& indexes,
    const Element& element) NOEXCEPT
{
    using namespace system;
    static_assert(!is_slab);
    static_assert(aligned, "set requires an aligned cell");

    if (indexes.empty())
        return true;

    // Avoid setting at/above terminal sentinel into a bucket position.
    const auto last = std::ranges::max(indexes);
    if (last >= Link::terminal)
        return false;

    // Allocate as necessary (allocations are filled), guard across the set.
    const auto guard = file_.get_filled(link_to_position(last), bucket_size);
    if (!guard)
        return false;

    for (const auto index: indexes)
    {
        const auto position = link_to_position(index);
        const auto raw = file_.get_raw(position);
        if (is_null(raw) || !exchange(raw, position, element))
            return false;
    }

    return true;
}

// private
TEMPLATE
template <typename Element>
bool CLASS::exchange(memory::iterator raw, size_t position,
    const Element& element) NOEXCEPT
{
    using namespace system;
    file_.prepare(position, bucket_size);
    auto& top = *pointer_cast<std::atomic<cell>>(raw);
    auto head = top.load(std::memory_order_acquire);
    auto written = true;
    cell update{};
    do
    {
        auto copy = element;
        iostream current{ pointer_cast<uint8_t>(&head), cell_size };
        reader source{ current };
        iostream replacement{ pointer_cast<uint8_t>(&update), cell_size };
        flipper sink{ replacement };
        written = copy.from_data(source) && copy.to_data(sink);
    }
    while (written && !top.compare_exchange_weak(head, update,
        std::memory_order_release, std::memory_order_acquire));

    file_.mark(position, bucket_size);
    return written;
}

} // namespace database
} // namespace libbitcoin

#endif
