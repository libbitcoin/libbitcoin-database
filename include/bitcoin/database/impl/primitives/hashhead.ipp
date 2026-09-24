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
#ifndef LIBBITCOIN_DATABASE_PRIMITIVES_HASHHEAD_IPP
#define LIBBITCOIN_DATABASE_PRIMITIVES_HASHHEAD_IPP

#include <cmath>
#include <bitcoin/database/define.hpp>

// Heads are not subject to resize/remap and therefore do not require memory
// smart pointer with shared remap lock. Using get_raw() saves that allocation.

namespace libbitcoin {
namespace database {

// configuration
// ----------------------------------------------------------------------------

TEMPLATE
CLASS::hashhead(storage& head, size_t buckets, size_t expected) NOEXCEPT
  : file_(head),
    buckets_(system::possible_narrow_cast<link>(buckets)),
    k_(optimal_k(expected, buckets))
{
    BC_ASSERT(buckets <= Link::terminal);
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
inline size_t CLASS::filter_k() const NOEXCEPT
{
    return k_;
}

TEMPLATE
bool CLASS::create() NOEXCEPT
{
    if (is_nonzero(file_.size()))
        return false;

    // The fill is file content: a managed head writes it to the file and
    // maps it released, so creation contributes no memory residency.
    if (file_.allocate(size(), system::bit_all<uint8_t>) == storage::eof)
        return false;

    BC_ASSERT_MSG(verify(), "unexpected head size");
    return set_body_count(zero);
}

TEMPLATE
bool CLASS::clear() NOEXCEPT
{
    const auto ptr = file_.get();
    if (!ptr)
        return false;

    file_.prepare(zero, size());
    std::fill_n(ptr.data(), size(), system::bit_all<uint8_t>);
    file_.mark(zero, size());
    return set_body_count(zero);
}

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
bool CLASS::verify() const NOEXCEPT
{
    return file_.size() == size();
}

TEMPLATE
bool CLASS::set_filter_k(size_t k) NOEXCEPT
{
    if constexpr (is_zero(k_max))
    {
        return is_zero(k);
    }
    else
    {
        if (is_zero(k) || (k > k_max))
            return false;

        k_ = k;
        return true;
    }
}

TEMPLATE
bool CLASS::set_buckets(size_t buckets) NOEXCEPT
{
    if (buckets > Link::terminal)
        return false;

    buckets_ = system::possible_narrow_cast<link>(buckets);
    return true;
}

TEMPLATE
uint32_t CLASS::derive_buckets(uint64_t expected, uint32_t low,
    uint32_t high) NOEXCEPT
{
    using namespace system;
    constexpr auto gigabyte = power2<uint64_t>(30u);
    constexpr auto megabyte = power2<uint64_t>(20u);
    constexpr auto uncontested = power2<uint64_t>(35u);
#if defined(HAVE_APPLE)
    constexpr auto contested = 10u * gigabyte;
#else
    constexpr auto contested = 8u * gigabyte;
#endif

    if (is_zero(expected) || is_zero(low) || is_zero(high))
        return {};

    const auto scaled = ceilinged_multiply<uint64_t>(expected, 10);
    const auto floored = scaled / low;
    const auto ceiled = scaled / high;
    const auto memory = system_memory();

    if (memory <= contested)
        return possible_narrow_cast<uint32_t>(floored);

    if (memory >= uncontested)
        return possible_narrow_cast<uint32_t>(ceiled);

    // Megabytes, as the byte product overflows the word.
    const auto over = (memory - contested) / megabyte;
    const auto rise = floored_subtract(ceiled, floored);
    constexpr auto span = (uncontested - contested) / megabyte;
    return limit<uint32_t>(floored + (ceilinged_multiply(rise, over) / span));
}

TEMPLATE
size_t CLASS::optimal_k(size_t count, size_t buckets) NOEXCEPT
{
    if constexpr (is_zero(k_max))
    {
        return k_default;
    }
    else
    {
        if (is_zero(count) || is_zero(buckets))
            return k_default;

        const auto load_factor = system::to_floating(count) /
            system::to_floating(buckets);

        auto optimum = one;
        auto minimum = expected_walk(one, load_factor);
        for (auto k = two; k <= k_max; ++k)
        {
            const auto cost = expected_walk(k, load_factor);
            if (cost < minimum)
            {
                minimum = cost;
                optimum = k;
            }
        }

        return optimum;
    }
}

// Bloom false positive rate for n keys of k selections over m bits.
TEMPLATE
double CLASS::bloom_false_positive(size_t k, size_t n) NOEXCEPT
{
    using namespace system;
    const auto retain = 1.0 - (1.0 / to_floating(m));
    const auto value = std::pow(retain, to_floating(k * n));
    return std::pow(1.0 - value, to_floating(k));
}

// Poisson mass is negligible past ten deviations of the mean. The minimum
// floors the bound for small means where deviations are near zero.
TEMPLATE
size_t CLASS::poisson_span(double load_factor) NOEXCEPT
{
    constexpr auto deviations = 10.0;
    constexpr auto minimum = 30.0;
    return system::to_integer<size_t>(std::ceil(load_factor +
        deviations * std::sqrt(load_factor) + minimum));
}

// False positive over poisson bucket occupancy, weighted by the occupancy.
TEMPLATE
double CLASS::expected_walk(size_t k, double load_factor) NOEXCEPT
{
    using namespace system;
    auto walk = 0.0;
    auto poisson = std::exp(-load_factor);
    const auto span = poisson_span(load_factor);
    for (auto n = one; n <= span; ++n)
    {
        poisson *= load_factor / to_floating(n);
        walk += poisson * to_floating(n) * bloom_false_positive(k, n);
    }

    return walk;
}

TEMPLATE
bool CLASS::get_body_count(Link& count) const NOEXCEPT
{
    const auto ptr = file_.get();
    if (!ptr)
        return false;

    // Body count is written as the first value in link size, but since
    // offsetting is a multiple of sell size, a full cell is consumed for it.
    // In case of disabled there are no cells, so file is link size.
    link_array(count.value) = link_array(ptr.data());
    return true;
}

TEMPLATE
bool CLASS::set_body_count(const Link& count) NOEXCEPT
{
    const auto ptr = file_.get();
    if (!ptr)
        return false;

    // Body count is written as the first value in link size, but since
    // offsetting is a multiple of sell size, a full cell is consumed for it.
    // In case of disabled there are no cells, so file is link size.
    auto value = count.value;
    file_.prepare(zero, Link::size);
    link_array(ptr.data()) = link_array(value);
    file_.mark(zero, Link::size);
    return true;
}

// operation
// ----------------------------------------------------------------------------

TEMPLATE
inline Link CLASS::index(const Key& key) const NOEXCEPT
{
    return keys::bucket(key, buckets_.value);
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
    if (screened(value, keys::thumb(key)))
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
    const auto position = link_to_position(index(key));
    const auto raw = file_.get_raw(position);
    if (is_null(raw))
        return false;

    file_.prepare(position, cell_size);
    const auto entropy = keys::thumb(key);
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
            update = next_cell(collision, head, current, entropy);
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
        auto update = next_cell(collision, head, current, entropy);
        top = cell_array(update);
        mutex_.unlock();
    }

    file_.mark(position, cell_size);
    return true;
}

// protected
// ----------------------------------------------------------------------------
// filters

TEMPLATE
INLINE constexpr CLASS::filter CLASS::to_filter(cell value) NOEXCEPT
{
    using namespace system;
    return possible_narrow_cast<filter>(shift_right(value, link_bits));
}

TEMPLATE
INLINE constexpr CLASS::link CLASS::to_link(cell value) NOEXCEPT
{
    using namespace system;
    constexpr auto mask = unmask_right<cell>(link_bits);
    return possible_narrow_cast<link>(bit_and(value, mask));
}

TEMPLATE
INLINE CLASS::cell CLASS::next_cell(bool& collision, cell previous,
    link current, uint64_t entropy) const NOEXCEPT
{
    if constexpr (filter_t::disabled)
    {
        collision = true;
        return current;
    }
    else
    {
        using namespace system;
        const auto prev = to_filter(previous);
        const auto next = filter_t::screen(prev, entropy, k_);
        collision = filter_t::is_collision(prev, next);
        return bit_or<cell>(shift_left<cell>(next, link_bits), current);
    }
}

TEMPLATE
INLINE bool CLASS::screened(cell value, uint64_t entropy) const NOEXCEPT
{
    if constexpr (filter_t::disabled)
    {
        return true;
    }
    else
    {
        return filter_t::is_screened(to_filter(value), entropy, k_);
    }
}

} // namespace database
} // namespace libbitcoin

#endif
