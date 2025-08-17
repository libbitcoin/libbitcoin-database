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
#ifndef LIBBITCOIN_DATABASE_PRIMITIVES_ARRAYMAP_IPP
#define LIBBITCOIN_DATABASE_PRIMITIVES_ARRAYMAP_IPP

#include <algorithm>
#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

TEMPLATE
CLASS::arraymap(storage& header, storage& body, const Link& buckets) NOEXCEPT
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
bool CLASS::clear() NOEXCEPT
{
    // Head is nullified and its body reference is zeroized. Body memory
    // recovery requires truncate/unload/load, which should be preceded by a
    // snapshot as all existing snapshots will be invalidated by the truncate.
    // An intervening snapshot will capture the zero count body reference (and
    // null head links) and will therefore be recoverable whether or not the
    // truncation succeeds.
    return head_.clear();
}

TEMPLATE
bool CLASS::backup(bool prune) NOEXCEPT
{
    return head_.set_body_count(prune ? Link{ 0 } : body_.count());
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
    return head_.enabled();
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
inline Link CLASS::at(size_t key) const NOEXCEPT
{
    return head_.at(key);
}

TEMPLATE
ELEMENT_CONSTRAINT
inline bool CLASS::at(size_t key, Element& element) const NOEXCEPT
{
    return get(at(key), element);
}

TEMPLATE
inline Link CLASS::exists(size_t key) const NOEXCEPT
{
    return !at(key).is_terminal();
}

TEMPLATE
ELEMENT_CONSTRAINT
inline bool CLASS::get(const Link& link, Element& element) const NOEXCEPT
{
    const auto ptr = body_.get(link);
    if (!ptr)
        return false;

    using namespace system;
    iostream stream{ *ptr };
    reader source{ stream };

    if constexpr (!is_slab) { BC_DEBUG_ONLY(source.set_limit(RowSize * element.count());) }
    return element.from_data(source);
}

TEMPLATE
ELEMENT_CONSTRAINT
bool CLASS::put(size_t key, const Element& element) NOEXCEPT
{
    // Avoid setting at/above terminal sentinel into a bucket position.
    if (key >= Link::terminal)
        return false;

    const auto link = body_.allocate(element.count());
    const auto ptr = body_.get(link);
    if (!ptr)
        return false;

    // iostream.flush is a nop (direct copy).
    using namespace system;
    iostream stream{ *ptr };
    finalizer sink{ stream };

    if constexpr (!is_slab) { BC_DEBUG_ONLY(sink.set_limit(RowSize * element.count());) }
    return element.to_data(sink) && head_.push(link, head_.index(key));
}

} // namespace database
} // namespace libbitcoin

#endif
