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
    buckets_(system::power2<bucket_integer>(bits)),
    mask_(system::unmask_right<bucket_integer>(bits))
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

    const auto index = possible_narrow_cast<bucket_integer>(keys::hash<Key>(key));
    return bit_and<bucket_integer>(mask_, index);
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

    if constexpr (aligned)
    {
        // Reads full padded word.
        // xcode clang++16 does not support C++20 std::atomic_ref.
        ////const std::atomic_ref<bucket_integer> head(unsafe_byte_cast<bucket_integer>(raw));
        const auto& head = *pointer_cast<std::atomic<bucket_integer>>(raw);

        // Acquire is necessary to synchronize with push release.
        // Relaxed would miss next updates, so acquire is optimal.
        return head.load(std::memory_order_acquire);
    }
    else
    {
        const auto& head = to_array<bucket_size>(raw);
        mutex_.lock_shared();
        const auto top = head;
        mutex_.unlock_shared();
        return top;
    }

    // TODO: return terminal if filtered.
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
    bool collision{};
    return push(collision, current, next, index);
}

TEMPLATE
inline bool CLASS::push(bool& collision, const Link& current, bytes& next,
    const Link& index) NOEXCEPT
{
    using namespace system;
    const auto raw = file_.get_raw(link_to_position(index));
    if (is_null(raw))
        return false;

    if constexpr (aligned)
    {
        // Writes full padded word (0x00 fill).
        // xcode clang++16 does not support C++20 std::atomic_ref.
        ////const std::atomic_ref<bucket_integer> head(unsafe_byte_cast<bucket_integer>(raw));
        auto& head = *pointer_cast<std::atomic<bucket_integer>>(raw);
        auto top = head.load(std::memory_order_acquire);
        do
        {
            // Compiler could order this after head.store, which would expose key
            // to search before next entry is linked. Thread fence imposes order.
            // A release fence ensures that all prior writes (like next) are
            // completed before any subsequent atomic store.
            next = Link{ top };
            std::atomic_thread_fence(std::memory_order_release);
        }
        while (!head.compare_exchange_weak(top, current,
            std::memory_order_release, std::memory_order_acquire));
    }
    else
    {
        auto& head = to_array<bucket_size>(raw);
        mutex_.lock();
        next = head;
        head = current;
        mutex_.unlock();
    }

    // TODO: set collision when unfiltered or fingerprint matches filter.
    collision = false;

    // The returned next is set to prevous head, which is where collisions may
    // be resolved to duplicate or not, when 'collision' is set to true.
    return true;
}

} // namespace database
} // namespace libbitcoin

#endif
