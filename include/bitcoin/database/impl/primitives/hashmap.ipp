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
#ifndef LIBBITCOIN_DATABASE_PRIMITIVES_HASHMAP_IPP
#define LIBBITCOIN_DATABASE_PRIMITIVES_HASHMAP_IPP

#include <atomic>
#include <algorithm>
#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

TEMPLATE
CLASS::hashmap(storage& header, storage& body, const Link& buckets) NOEXCEPT
  : head_(header, buckets), body_(body)
{
}

// not thread safe
// ----------------------------------------------------------------------------

TEMPLATE
bool CLASS::create() NOEXCEPT
{
    Link count{};
    return head_.create() && head_.get_body_count(count) &&
        body_.truncate(count);
}

TEMPLATE
bool CLASS::close() NOEXCEPT
{
    return head_.set_body_count(body_.count());
}

TEMPLATE
bool CLASS::backup() NOEXCEPT
{
    return head_.set_body_count(body_.count());
}

TEMPLATE
bool CLASS::restore() NOEXCEPT
{
    Link count{};
    return head_.verify() && head_.get_body_count(count) &&
        body_.truncate(count);
}

TEMPLATE
bool CLASS::verify() const NOEXCEPT
{
    Link count{};
    return head_.verify() && head_.get_body_count(count) &&
        (count == body_.count());
}

// sizing
// ----------------------------------------------------------------------------

TEMPLATE
bool CLASS::enabled() const NOEXCEPT
{
    return head_.buckets() > one;
}

TEMPLATE
size_t CLASS::buckets() const NOEXCEPT
{
    return head_.buckets();
}

TEMPLATE
size_t CLASS::head_size() const NOEXCEPT
{
    return head_.size();
}

TEMPLATE
size_t CLASS::body_size() const NOEXCEPT
{
    return body_.size();
}

TEMPLATE
Link CLASS::count() const NOEXCEPT
{
    return body_.count();
}

TEMPLATE
size_t CLASS::capacity() const NOEXCEPT
{
    return body_.capacity();
}

TEMPLATE
bool CLASS::expand(const Link& count) NOEXCEPT
{
    return body_.expand(count);
}

// diagnostic counters
// ----------------------------------------------------------------------------

TEMPLATE
size_t CLASS::positive_search_count() const NOEXCEPT
{
    return positive_.load(std::memory_order_relaxed);
}

TEMPLATE
size_t CLASS::negative_search_count() const NOEXCEPT
{
    return negative_.load(std::memory_order_relaxed);
}

// query interface
// ----------------------------------------------------------------------------

TEMPLATE
code CLASS::get_fault() const NOEXCEPT
{
    return body_.get_fault();
}

TEMPLATE
size_t CLASS::get_space() const NOEXCEPT
{
    return body_.get_space();
}

TEMPLATE
code CLASS::reload() NOEXCEPT
{
    return body_.reload();
}

// query interface
// ----------------------------------------------------------------------------

TEMPLATE
inline Link CLASS::top(const Link& link) const NOEXCEPT
{
    if (link >= head_.buckets())
        return {};

    return head_.top(link);
}

TEMPLATE
inline bool CLASS::exists(const memory_ptr& ptr, const Key& key) const NOEXCEPT
{
    return !first(ptr, key).is_terminal();
}

TEMPLATE
inline bool CLASS::exists(const Key& key) const NOEXCEPT
{
    return !first(key).is_terminal();
}

TEMPLATE
inline Link CLASS::first(const memory_ptr& ptr, const Key& key) const NOEXCEPT
{
    return first(ptr, head_.top(key), key);
}

TEMPLATE
inline Link CLASS::first(const Key& key) const NOEXCEPT
{
    return first(get_memory(), key);
}

TEMPLATE
inline typename CLASS::iterator CLASS::it(Key&& key) const NOEXCEPT
{
    const auto top = head_.top(key);
    return { get_memory(), top, std::forward<Key>(key) };
}

TEMPLATE
inline typename CLASS::iterator CLASS::it(const Key& key) const NOEXCEPT
{
    return { get_memory(), head_.top(key), key };
}

TEMPLATE
inline Link CLASS::allocate(const Link& size) NOEXCEPT
{
    return body_.allocate(size);
}

TEMPLATE
inline memory_ptr CLASS::get_memory() const NOEXCEPT
{
    return body_.get();
}

TEMPLATE
Key CLASS::get_key(const Link& link) NOEXCEPT
{
    using namespace system;
    const auto ptr = body_.get(link);
    if (!ptr || is_lesser(ptr->size(), index_size))
        return {};

    return unsafe_array_cast<uint8_t, key_size>(std::next(ptr->begin(),
        Link::size));
}

TEMPLATE
ELEMENT_CONSTRAINT
inline bool CLASS::find(const Key& key, Element& element) const NOEXCEPT
{
    return !find_link(key, element).is_terminal();
}

TEMPLATE
ELEMENT_CONSTRAINT
inline Link CLASS::find_link(const Key& key, Element& element) const NOEXCEPT
{
    // This override avoids duplicated memory_ptr construct in get(first()).
    const auto ptr = get_memory();
    const auto link = first(ptr, head_.top(key), key);
    if (link.is_terminal())
        return {};

    return read(ptr, link, element);
}

TEMPLATE
ELEMENT_CONSTRAINT
inline bool CLASS::get(const Link& link, Element& element) const NOEXCEPT
{
    // This override is the normal form.
    return read(get_memory(), link, element);
}

// static
TEMPLATE
ELEMENT_CONSTRAINT
inline bool CLASS::get(const memory_ptr& ptr, const Link& link,
    Element& element) NOEXCEPT
{
    return read(ptr, link, element);
}

// static
TEMPLATE
ELEMENT_CONSTRAINT
inline bool CLASS::get(const iterator& it, Element& element) NOEXCEPT
{
    // This override avoids deadlock when holding iterator to the same table.
    return read(it.ptr(), *it, element);
}

// static
TEMPLATE
ELEMENT_CONSTRAINT
inline bool CLASS::get(const iterator& it, const Link& link,
    Element& element) NOEXCEPT
{
    // This override avoids deadlock when holding iterator to the same table.
    return read(it.ptr(), link, element);
}

// static
TEMPLATE
ELEMENT_CONSTRAINT
bool CLASS::set(const memory_ptr& ptr, const Link& link, const Key& key,
    const Element& element) NOEXCEPT
{
    using namespace system;
    if (!ptr)
        return false;

    const auto start = body::link_to_position(link);
    if (is_limited<ptrdiff_t>(start))
        return false;

    const auto size = ptr->size();
    const auto position = possible_narrow_and_sign_cast<ptrdiff_t>(start);
    if (position >= size)
        return false;

    // Stream starts at record and the index is skipped for reader convenience.
    const auto offset = ptr->offset(start);
    if (is_null(offset))
        return false;

    // Set element search key.
    unsafe_array_cast<uint8_t, key_size>(std::next(offset,
        Link::size)) = key;

    iostream stream{ offset, size - position };
    finalizer sink{ stream };
    sink.skip_bytes(index_size);

    if constexpr (!is_slab) { BC_DEBUG_ONLY(sink.set_limit(RowSize * element.count());) }
    return element.to_data(sink);
}

TEMPLATE
ELEMENT_CONSTRAINT
bool CLASS::set(const Link& link, const Key& key,
    const Element& element) NOEXCEPT
{
    return set(get_memory(), link, key, element);
}

TEMPLATE
ELEMENT_CONSTRAINT
inline Link CLASS::set_link(const Key& key, const Element& element) NOEXCEPT
{
    Link link{};
    if (!set_link(link, key, element))
        return {};
    
    return link;
}

TEMPLATE
ELEMENT_CONSTRAINT
inline bool CLASS::set_link(Link& link, const Key& key,
    const Element& element) NOEXCEPT
{
    link = allocate(element.count());
    return set(link, key, element);
}

TEMPLATE
ELEMENT_CONSTRAINT
inline Link CLASS::put_link(const Key& key, const Element& element) NOEXCEPT
{
    Link link{};
    if (!put_link(link, key, element))
        return {};

    return link;
}

TEMPLATE
ELEMENT_CONSTRAINT
inline bool CLASS::put_link(Link& link, const Key& key,
    const Element& element) NOEXCEPT
{
    link = allocate(element.count());
    return put(link, key, element);
}

TEMPLATE
ELEMENT_CONSTRAINT
inline bool CLASS::put(const Key& key, const Element& element) NOEXCEPT
{
    return !put_link(key, element).is_terminal();
}

TEMPLATE
ELEMENT_CONSTRAINT
inline bool CLASS::put(const Link& link, const Key& key,
    const Element& element) NOEXCEPT
{
    // This override is the normal form.
    return write(get_memory(), link, key, element);
}

TEMPLATE
ELEMENT_CONSTRAINT
inline bool CLASS::put(const memory_ptr& ptr, const Link& link, const Key& key,
    const Element& element) NOEXCEPT
{
    return write(ptr, link, key, element);
}

TEMPLATE
ELEMENT_CONSTRAINT
inline bool CLASS::put(bool& duplicate, const memory_ptr& ptr,
    const Link& link, const Key& key, const Element& element) NOEXCEPT
{
    Link previous{};
    if (!write(previous, ptr, link, key, element))
        return false;

    if (previous.is_terminal())
    {
        duplicate = false;
        ////negative_.fetch_add(one, std::memory_order_relaxed);
    }
    else
    {
        // Search the previous conflicts to determine if actual duplicate.
        duplicate = !first(ptr, previous, key).is_terminal();
        ////positive_.fetch_add(one, std::memory_order_relaxed);
    }

    return true;
}

TEMPLATE
inline Link CLASS::commit_link(const Link& link, const Key& key) NOEXCEPT
{
    if (!commit(link, key))
        return {};

    return link;
}

TEMPLATE
inline bool CLASS::commit(const Link& link, const Key& key) NOEXCEPT
{
    return commit(get_memory(), link, key);
}

TEMPLATE
bool CLASS::commit(const memory_ptr& ptr, const Link& link,
    const Key& key) NOEXCEPT
{
    using namespace system;
    if (!ptr)
        return false;

    // get element offset (fault)
    const auto offset = ptr->offset(body::link_to_position(link));
    if (is_null(offset))
        return false;

    // Commit element to search index (terminal is a valid bucket index).
    auto& next = unsafe_array_cast<uint8_t, Link::size>(offset);
    return head_.push(link, next, key);
}

// protected
// ----------------------------------------------------------------------------

// static
TEMPLATE
Link CLASS::first(const memory_ptr& ptr, const Link& link,
    const Key& key) NOEXCEPT
{
    using namespace system;
    if (!ptr)
        return {};

    auto next = link;
    while (!next.is_terminal())
    {
        // get element offset (fault)
        const auto offset = ptr->offset(body::link_to_position(next));
        if (is_null(offset))
            return {};

        // element key matches (found)
        if (keys::compare(unsafe_array_cast<uint8_t, key_size>(
            std::next(offset, Link::size)), key))
            return next;

        // set next element link (loop)
        next = unsafe_array_cast<uint8_t, Link::size>(offset);
    }

    return next;
}

// static
TEMPLATE
ELEMENT_CONSTRAINT
bool CLASS::read(const memory_ptr& ptr, const Link& link,
    Element& element) NOEXCEPT
{
    using namespace system;
    if (!ptr || link.is_terminal())
        return false;

    const auto start = body::link_to_position(link);
    if (is_limited<ptrdiff_t>(start))
        return false;

    const auto size = ptr->size();
    const auto position = possible_narrow_and_sign_cast<ptrdiff_t>(start);
    if (position >= size)
        return false;

    const auto offset = ptr->offset(start);
    if (is_null(offset))
        return false;

    // Stream starts at record and the index is skipped for reader convenience.
    iostream stream{ offset, size - position };
    reader source{ stream };
    source.skip_bytes(index_size);

    if constexpr (!is_slab) { BC_DEBUG_ONLY(source.set_limit(RowSize * element.count());) }
    return element.from_data(source);
}

TEMPLATE
ELEMENT_CONSTRAINT
bool CLASS::write(const memory_ptr& ptr, const Link& link, const Key& key,
    const Element& element) NOEXCEPT
{
    Link unused{};
    return write(unused, ptr, link, key, element);
}

TEMPLATE
ELEMENT_CONSTRAINT
bool CLASS::write(Link& previous, const memory_ptr& ptr, const Link& link,
    const Key& key, const Element& element) NOEXCEPT
{
    using namespace system;
    if (!ptr || link.is_terminal())
        return false;

    const auto start = body::link_to_position(link);
    if (is_limited<ptrdiff_t>(start))
        return false;

    const auto size = ptr->size();
    const auto position = possible_narrow_and_sign_cast<ptrdiff_t>(start);
    if (position >= size)
        return false;

    const auto offset = ptr->offset(start);
    if (is_null(offset))
        return false;

    // iostream.flush is a nop (direct copy).
    iostream stream{ offset, size - position };
    finalizer sink{ stream };
    sink.skip_bytes(Link::size);
    keys::write(sink, key);

    // Commit element to body.
    if constexpr (!is_slab) { BC_DEBUG_ONLY(sink.set_limit(RowSize * element.count());) }
    auto& next = unsafe_array_cast<uint8_t, Link::size>(offset);
    if (!element.to_data(sink))
        return false;

    // Commit element to search (terminal is a valid bucket index).
    bool collision{};
    if (!head_.push(collision, link, next, key))
        return false;

    // If filter collision set previous stack head for conflict resolution.
    if (collision)
        previous = next;
    else
        previous = Link::terminal;

    return true;
}

} // namespace database
} // namespace libbitcoin

#endif
