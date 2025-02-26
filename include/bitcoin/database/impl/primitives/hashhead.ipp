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
#ifndef LIBBITCOIN_DATABASE_PRIMITIVES_HASHHEAD_IPP
#define LIBBITCOIN_DATABASE_PRIMITIVES_HASHHEAD_IPP

#include <algorithm>
#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>

// Heads are not subject to resize/remap and therefore do not require memory
// smart pointer with shared remap lock. Using get_raw() saves that allocation.

namespace libbitcoin {
namespace database {

TEMPLATE
CLASS::hashhead(storage& head, size_t bits) NOEXCEPT
  : file_(head),
    buckets_(system::power2<integer>(bits)),
    mask_(system::unmask_right<integer>(bits))
{
}

TEMPLATE
inline size_t CLASS::size() const NOEXCEPT
{
    return link_to_position(buckets_);
}

TEMPLATE
inline size_t CLASS::buckets() const NOEXCEPT
{
    return buckets_;
}

TEMPLATE
bool CLASS::create() NOEXCEPT
{
    if (is_nonzero(file_.size()))
        return false;

    const auto allocation = size();
    const auto start = file_.allocate(allocation);

    // Guards addition overflow in file_.get (start must be valid).
    if (start == storage::eof)
        return false;

    const auto ptr = file_.get(start);
    if (!ptr)
        return false;

    BC_ASSERT_MSG(verify(), "unexpected head size");

    // std::memset/fill_n have identical performance (on win32).
    ////std::memset(ptr->data(), system::bit_all<uint8_t>, allocation);
    std::fill_n(ptr->data(), allocation, system::bit_all<uint8_t>);
    return set_body_count(zero);
}

TEMPLATE
bool CLASS::verify() const NOEXCEPT
{
    return file_.size() == size();
}

TEMPLATE
bool CLASS::get_body_count(Link& count) const NOEXCEPT
{
    const auto ptr = file_.get();
    if (!ptr)
        return false;

    count = to_array<Link::size>(ptr->data());
    return true;
}

TEMPLATE
bool CLASS::set_body_count(const Link& count) NOEXCEPT
{
    const auto ptr = file_.get();
    if (!ptr)
        return false;

    // If head is padded then last bytes are fill (0xff).
    to_array<Link::size>(ptr->data()) = count;
    return true;
}

TEMPLATE
inline Link CLASS::index(const Key& key) const NOEXCEPT
{
    using namespace system;
    BC_ASSERT_MSG(mask_ < max_size_t, "insufficient domain");
    BC_ASSERT_MSG(is_nonzero(buckets_), "hash table requires buckets");

    // unique_hash assumes sufficient uniqueness in low order key bytes.
    const auto index = possible_narrow_cast<integer>(unique_hash(key));
    return bit_and<integer>(mask_, index);
}

TEMPLATE
inline Link CLASS::top(const Key& key) const NOEXCEPT
{
    return top(index(key));
}

TEMPLATE
inline Link CLASS::top(const Link& index) const NOEXCEPT
{
    using namespace system;
    const auto raw = file_.get_raw(link_to_position(index));
    if (is_null(raw))
        return {};

    if constexpr (Align)
    {
        // Reads full padded word.
        const std::atomic_ref<integer> head(unsafe_byte_cast<integer>(raw));
        return head.load(std::memory_order_acquire);
    }
    else
    {
        const auto& head = to_array<size_>(raw);
        mutex_.lock_shared();
        const auto top = head;
        mutex_.unlock_shared();
        return top;
    }
}

TEMPLATE
inline bool CLASS::push(const Link& current, bytes& next,
    const Key& key) NOEXCEPT
{
    return push(current, next, index(key));
}

TEMPLATE
inline bool CLASS::push(const Link& current, bytes& next,
    const Link& index) NOEXCEPT
{
    using namespace system;
    const auto raw = file_.get_raw(link_to_position(index));
    if (is_null(raw))
        return false;

    if constexpr (Align)
    {
        // Writes full padded word (0x00 fill).
        const std::atomic_ref<integer> head(unsafe_byte_cast<integer>(raw));
        next = Link(head.exchange(current, std::memory_order_acq_rel));
    }
    else
    {
        auto& head = to_array<size_>(raw);
        mutex_.lock();
        next = head;
        head = current;
        mutex_.unlock();
    }

    return true;
}

} // namespace database
} // namespace libbitcoin

#endif
