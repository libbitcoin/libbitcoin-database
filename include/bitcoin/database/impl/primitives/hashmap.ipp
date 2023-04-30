/**
 * Copyright (c) 2011-2022 libbitcoin developers (see AUTHORS)
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

#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

TEMPLATE
CLASS::hashmap(storage& header, storage& body, const Link& buckets) NOEXCEPT
  : head_(header, buckets), manager_(body)
{
}

// not thread safe
// ----------------------------------------------------------------------------

TEMPLATE
bool CLASS::create() NOEXCEPT
{
    Link count{};
    return head_.create() &&
        head_.get_body_count(count) && manager_.truncate(count);
}

TEMPLATE
bool CLASS::close() NOEXCEPT
{
    return head_.set_body_count(manager_.count());
}

TEMPLATE
bool CLASS::backup() NOEXCEPT
{
    return head_.set_body_count(manager_.count());
}

TEMPLATE
bool CLASS::restore() NOEXCEPT
{
    Link count{};
    return head_.verify() &&
        head_.get_body_count(count) && manager_.truncate(count);
}

TEMPLATE
bool CLASS::verify() const NOEXCEPT
{
    Link count{};
    return head_.verify() &&
        head_.get_body_count(count) && count == manager_.count();
}

// sizing
// ----------------------------------------------------------------------------

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
    return manager_.size();
}

TEMPLATE
Link CLASS::count() const NOEXCEPT
{
    return manager_.count();
}

// query interface
// ----------------------------------------------------------------------------

TEMPLATE
Link CLASS::top(const Link& link) const NOEXCEPT
{
    return link < head_.buckets() ? head_.top(link) : Link{};
}

TEMPLATE
bool CLASS::exists(const Key& key) const NOEXCEPT
{
    return !first(key).is_terminal();
}

TEMPLATE
Link CLASS::first(const Key& key) const NOEXCEPT
{
    return it(key).self();
}

TEMPLATE
typename CLASS::iterator CLASS::it(const Key& key) const NOEXCEPT
{
    // TODO: due to iterator design, key is copied into iterator.
    return { manager_.get(), head_.top(key), key };
}

TEMPLATE
Link CLASS::allocate(const Link& size) NOEXCEPT
{
    return manager_.allocate(size);
}

TEMPLATE
Key CLASS::get_key(const Link& link) NOEXCEPT
{
    constexpr auto key_size = array_count<Key>;
    const auto ptr = manager_.get(link);

    // As with link, search key is presumed valid (otherwise null array).
    if (!ptr || system::is_lesser(ptr->size(), Link::size + key_size))
        return {};

    return system::unsafe_array_cast<uint8_t, key_size>(std::next(
        ptr->begin(), Link::size));
}

// query interface (iostreams)
// ----------------------------------------------------------------------------

TEMPLATE
template <typename Element, if_equal<Element::size, Size>>
bool CLASS::get(const Link& link, Element& element) const NOEXCEPT
{
    auto source = streamer<reader>(link);
    return source && element.from_data(*source);
}

TEMPLATE
template <typename Element, if_equal<Element::size, Size>>
bool CLASS::set(const Link& link, const Element& element) NOEXCEPT
{
    auto sink = streamer<finalizer>(link);
    return sink && element.to_data(*sink);
}

TEMPLATE
template <typename Element, if_equal<Element::size, Size>>
Link CLASS::set_link(const Element& element) NOEXCEPT
{
    Link link{};
    return set_link(link, element) ? link : Link{};
}

TEMPLATE
template <typename Element, if_equal<Element::size, Size>>
bool CLASS::set_link(Link& link, const Element& element) NOEXCEPT
{
    link = allocate(element.count());
    return set(link, element);
}

TEMPLATE
template <typename Element, if_equal<Element::size, Size>>
Link CLASS::put_link(const Key& key, const Element& element) NOEXCEPT
{
    Link link{};
    return put_link(link, key, element) ? link : Link{};
}

TEMPLATE
template <typename Element, if_equal<Element::size, Size>>
bool CLASS::put_link(Link& link, const Key& key,
    const Element& element) NOEXCEPT
{
    // Reusing put() here would cause a second invocation of element.count().
    const auto size = element.count();
    link = allocate(size);
    auto sink = putter(link, key, size);
    return sink && element.to_data(*sink) && sink->finalize();
}

TEMPLATE
template <typename Element, if_equal<Element::size, Size>>
bool CLASS::put(const Key& key, const Element& element) NOEXCEPT
{
    return !put_link(key, element).is_terminal();
}

TEMPLATE
template <typename Element, if_equal<Element::size, Size>>
bool CLASS::put(const Link& link, const Key& key,
    const Element& element) NOEXCEPT
{
    auto sink = putter(link, key, element.count());
    return sink && element.to_data(*sink) && sink->finalize();
}

// query interface (memory)
// ============================================================================

TEMPLATE
template <typename Element, if_equal<Element::size, Size>>
bool CLASS::get1(const Link& link, Element& element) const NOEXCEPT
{
    const auto ptr = manager_.get(link);
    if (!ptr) return false;
    simple_reader reader{ *ptr };
    reader.skip_bytes(Link::size + array_count<Key>);
    return element.from_data(reader);
}

TEMPLATE
template <typename Element, if_equal<Element::size, Size>>
bool CLASS::set1(const Link& link, const Element& element) NOEXCEPT
{
    const auto ptr = manager_.get(link);
    if (!ptr) return false;
    simple_finalizer writer{ *ptr };
    writer.skip_bytes(Link::size + array_count<Key>);
    return element.to_data(writer);
}

TEMPLATE
template <typename Element, if_equal<Element::size, Size>>
Link CLASS::set_link1(const Element& element) NOEXCEPT
{
    Link link{};
    return set_link1(link, element) ? link : Link{};
}

TEMPLATE
template <typename Element, if_equal<Element::size, Size>>
bool CLASS::set_link1(Link& link, const Element& element) NOEXCEPT
{
    link = allocate(element.count());
    return set1(link, element);
}

TEMPLATE
template <typename Element, if_equal<Element::size, Size>>
Link CLASS::put_link1(const Key& key, const Element& element) NOEXCEPT
{
    Link link{};
    return put_link1(link, key, element) ? link : Link{};
}

TEMPLATE
template <typename Element, if_equal<Element::size, Size>>
bool CLASS::put_link1(Link& link, const Key& key,
    const Element& element) NOEXCEPT
{
    link = allocate(element.count());
    const auto ptr = manager_.get(link);
    if (!ptr) return false;
    simple_finalizer writer{ *ptr };

    // finalizing
    using namespace system;
    writer.skip_bytes(Link::size);
    writer.write_bytes(key);
    writer.set_finalizer([this, link, index = head_.index(key), ptr]() NOEXCEPT
    {
        auto& next = unsafe_array_cast<uint8_t, Link::size>(ptr->begin());
        return head_.push(link, next, index);
    });

    return element.to_data(writer) && writer.finalize();
}

TEMPLATE
template <typename Element, if_equal<Element::size, Size>>
bool CLASS::put1(const Key& key, const Element& element) NOEXCEPT
{
    return !put_link1(key, element).is_terminal();
}

TEMPLATE
template <typename Element, if_equal<Element::size, Size>>
bool CLASS::put1(const Link& link, const Key& key,
    const Element& element) NOEXCEPT
{
    const auto ptr = manager_.get(link);
    if (!ptr) return false;
    simple_finalizer writer{ *ptr };

    // finalizing
    using namespace system;
    writer.skip_bytes(Link::size);
    writer.write_bytes(key);
    writer.set_finalizer([this, link, index = head_.index(key), ptr]() NOEXCEPT
    {
        auto& next = unsafe_array_cast<uint8_t, Link::size>(ptr->begin());
        return head_.push(link, next, index);
    });

    return element.to_data(writer) && writer.finalize();
}

// ============================================================================

TEMPLATE
bool CLASS::commit(const Link& link, const Key& key) NOEXCEPT
{
    const auto ptr = manager_.get(link);
    if (!ptr) return false;

    // Set element search key.
    system::unsafe_array_cast<uint8_t, array_count<Key>>(std::next(
        ptr->begin(), Link::size)) = key;

    // Commit element to search index.
    auto& next = system::unsafe_array_cast<uint8_t, Link::size>(ptr->begin());
    return head_.push(link, next, head_.index(key));
}

TEMPLATE
Link CLASS::commit_link(const Link& link, const Key& key) NOEXCEPT
{
    return commit(link, key) ? link : Link{};
}

// protected
// ----------------------------------------------------------------------------

TEMPLATE
template <typename Streamer>
typename Streamer::ptr CLASS::streamer(const Link& link) const NOEXCEPT
{
    const auto ptr = manager_.get(link);
    if (!ptr) return {};

    const auto stream = std::make_shared<Streamer>(ptr);
    stream->skip_bytes(Link::size + array_count<Key>);

    // Limits to single record or eof for slab (caller can remove limit).
    if constexpr (!is_slab) { stream->set_limit(Size); }
    return stream;
}

TEMPLATE
finalizer_ptr CLASS::creater(Link& link, const Key& key,
    const Link& size) NOEXCEPT
{
    link = allocate(size);
    return putter(link, key, size);
}

TEMPLATE
finalizer_ptr CLASS::putter(const Link& link, const Key& key,
    const Link& size) NOEXCEPT
{
    using namespace system;
    const auto ptr = manager_.get(link);
    if (!ptr) return {};

    const auto sink = std::make_shared<finalizer>(ptr);
    sink->skip_bytes(Link::size);
    sink->write_bytes(key);
    sink->set_finalizer([this, link, index = head_.index(key), ptr]() NOEXCEPT
    {
        auto& next = unsafe_array_cast<uint8_t, Link::size>(ptr->begin());
        return head_.push(link, next, index);
    });

    // Limits to size records or eof for slab.
    if constexpr (!is_slab) { sink->set_limit(Size * size); }
    return sink;
}

} // namespace database
} // namespace libbitcoin

#endif
