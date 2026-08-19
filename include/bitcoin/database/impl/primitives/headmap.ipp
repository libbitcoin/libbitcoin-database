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
CLASS::headmap(storage& head) NOEXCEPT
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
Link CLASS::at(size_t index) const NOEXCEPT
{
    using namespace system;

    // Buckets at or above the logical size are unallocated (count publishes).
    if (index >= count())
        return {};

    const auto ptr = file_.get(link_to_position(index));
    if (!ptr)
        return {};

    // Reads full padded word.
    const auto raw = ptr.data();
    const auto& head = *pointer_cast<std::atomic<integer>>(raw);

    // Aligned values must be masked to match terminal.
    return bit_and(Link::terminal, head.load(std::memory_order_relaxed));
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

        // Writes full padded word (into unpublished capacity).
        file_.prepare(position, bucket_size);
        const auto raw = ptr.data();
        auto& head = *pointer_cast<std::atomic<integer>>(raw);
        head.store(link, std::memory_order_relaxed);
        file_.mark(position, bucket_size);
    }

    // Publication of the pre-written bucket is its allocation.
    return file_.allocate(bucket_size) != storage::eof;
}

} // namespace database
} // namespace libbitcoin

#endif
