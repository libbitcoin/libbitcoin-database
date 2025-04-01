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

// configuration
// ----------------------------------------------------------------------------

TEMPLATE
CLASS::hashhead(storage& head, size_t buckets) NOEXCEPT
  : file_(head),
    buckets_(system::possible_narrow_cast<link>(buckets))
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

    link_array(count.value) = link_array(ptr->data());
    return true;
}

TEMPLATE
bool CLASS::set_body_count(const Link& count) NOEXCEPT
{
    const auto ptr = file_.get();
    if (!ptr)
        return false;

    // If head is padded then last bytes are fill (0xff).
    auto value = count.value;
    link_array(ptr->data()) = link_array(value);
    return true;
}

// operation
// ----------------------------------------------------------------------------

TEMPLATE
inline Link CLASS::index(const Key& key) const NOEXCEPT
{
    return system::possible_narrow_cast<link>(keys::hash(key)) % buckets_;
}

TEMPLATE
inline Link CLASS::top(const Link& index) const NOEXCEPT
{
    return to_link(get_cell(index));
}

TEMPLATE
inline Link CLASS::top(const Key& key) const NOEXCEPT
{
    const auto value = get_cell(index(key));
    if (screened(value, key))
        return to_link(value);

    // Conflict (body) search is bypassed by filter when key is not screened.
    // If terminal here it is assured that table does not contain the key.
    return {};
}

TEMPLATE
inline bool CLASS::push(const Link& current, bytes& next,
    const Key& key) NOEXCEPT
{
    bool unused{};
    return push(unused, current, next, key);
}

TEMPLATE
inline bool CLASS::push(bool& collision, const Link& current, bytes& next,
    const Key& key) NOEXCEPT
{
    // next holds previous top and can searched for dups if collision is true.
    return set_cell(collision, next, current, key);
}

// protected
// ----------------------------------------------------------------------------
// read/write

TEMPLATE
inline CLASS::cell CLASS::get_cell(const Link& index) const NOEXCEPT
{
    using namespace system;
    const auto raw = file_.get_raw(link_to_position(index));
    if (is_null(raw))
        return terminal;

    if constexpr (aligned)
    {
        // Reads full padded word.
        // xcode clang++16 does not support C++20 std::atomic_ref.
        ////const std::atomic_ref<cell> top(unsafe_byte_cast<cell>(raw));
        const auto& top = *pointer_cast<std::atomic<cell>>(raw);

        // Acquire is necessary to synchronize with set_cell release.
        // Relaxed would miss next updates, so acquire is optimal.
        return top.load(std::memory_order_acquire);
    }
    else
    {
        const auto& top = cell_array(raw);
        cell head{};

        mutex_.lock_shared();
        cell_array(head) = top;
        mutex_.unlock_shared();

        return head;
    }
}

TEMPLATE
inline bool CLASS::set_cell(bool& collision, bytes& next, const Link& current,
    const Key& key) NOEXCEPT
{
    using namespace system;
    const auto raw = file_.get_raw(link_to_position(index(key)));
    if (is_null(raw))
        return false;

    if constexpr (aligned)
    {
        // Writes full padded word (0x00 fill).
        // xcode clang++16 does not support C++20 std::atomic_ref.
        ////const std::atomic_ref<cell> head(unsafe_byte_cast<cell>(raw));
        auto& top = *pointer_cast<std::atomic<cell>>(raw);
        auto head = top.load(std::memory_order_acquire);
        cell update{};
        do
        {
            // Compiler could order this after top.store, which would expose key
            // to search before next entry is linked. Thread fence imposes order.
            // A release fence ensures that all prior writes (like next) are
            // completed before any subsequent atomic store.
            auto masked = bit_and<cell>(Link::terminal, head);
            next = link_array(masked);
            update = to_cell(collision, head, current, key);
            std::atomic_thread_fence(std::memory_order_release);
        }
        while (!top.compare_exchange_weak(head, update,
            std::memory_order_release, std::memory_order_acquire));
    }
    else
    {
        auto& top = cell_array(raw);
        cell head{};

        mutex_.lock();
        cell_array(head) = top;
        auto masked = bit_and<cell>(Link::terminal, head);
        next = link_array(masked);
        auto update = to_cell(collision, head, current, key);
        top = cell_array(update);
        mutex_.unlock();
    }

    return true;
}

// protected
// ----------------------------------------------------------------------------
// filters

TEMPLATE
INLINE constexpr bool CLASS::screened(cell value, const Key& key) NOEXCEPT
{
    if constexpr (sieve_t::disabled)
    {
        return true;
    }
    else
    {
        return sieve_t::is_screened(to_filter(value), fingerprint(key));
    }
}

TEMPLATE
INLINE constexpr CLASS::filter CLASS::fingerprint(const Key& key) NOEXCEPT
{
    using namespace system;
    return possible_narrow_cast<filter>(keys::thumb(key));
}

TEMPLATE
INLINE constexpr CLASS::filter CLASS::to_filter(cell value) NOEXCEPT
{
    using namespace system;
    return possible_narrow_cast<filter>(shift_right(value, link_bits));
}

TEMPLATE
INLINE constexpr CLASS::link CLASS::to_link(cell value) NOEXCEPT
{
    if constexpr (sieve_t::disabled)
    {
        return system::possible_narrow_cast<link>(value);
    }
    else
    {
        using namespace system;
        if (value == terminal)
            return {};

        constexpr auto mask = unmask_right<cell>(link_bits);
        return possible_narrow_cast<link>(bit_and(value, mask));
    }
}

TEMPLATE
INLINE constexpr CLASS::cell CLASS::to_cell(bool& collision, cell previous,
    link current, const Key& key) NOEXCEPT
{
    if constexpr (sieve_t::disabled)
    {
        collision = true;
        return current;
    }
    else
    {
        using namespace system;
        const auto sieve = to_filter(previous);
        const auto next = sieve_t::screen(sieve, fingerprint(key));
        collision = (next == sieve || sieve_t::is_saturated(next));
        return bit_or<cell>(shift_left<cell>(next, link_bits), current);
    }
}

} // namespace database
} // namespace libbitcoin

#endif
