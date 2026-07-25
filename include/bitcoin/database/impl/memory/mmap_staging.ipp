/**
 * Copyright (c) 2011-2026 libbitcoin developers (see AUTHORS)
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
#ifndef LIBBITCOIN_DATABASE_MEMORY_MMAP_STAGING_IPP
#define LIBBITCOIN_DATABASE_MEMORY_MMAP_STAGING_IPP

#include <algorithm>
#include <fcntl.h>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/mstage.hpp>

namespace libbitcoin {
namespace database {

// Write-completion accounting (staged instances). Extents chase completions,
// closing the gaps, settling the completed prefix.

TEMPLATE
void CLASS::complete(size_t
    #if defined(MANAGE_STAGING)
    offset
    #endif
    , size_t
    #if defined(MANAGE_STAGING)
    count
    #endif
) NOEXCEPT
{
#if defined(MANAGE_STAGING)
    if (!staged_ || is_zero(count))
        return;

    // Acquire-ordered window snapshot (published entries are immobile).
    const auto head = ring_head_.load(std::memory_order_acquire);
    const auto size = ring_size_.load(std::memory_order_acquire);

    // Lock-free binary search of the sorted window for the covering extent.
    size_t low{};
    auto high = size;
    while (low < high)
    {
        const auto middle = to_half(low + high);
        auto& record = ring_.at((head + middle) % extents);
        const auto start = record.start.load(std::memory_order_relaxed);

        if (offset < start)
        {
            high = middle;
            continue;
        }

        if (offset >= (start + record.count))
        {
            low = add1(middle);
            continue;
        }

        // Wrap from over-completion stalls the frontier (conservative).
        const auto previous = record.outstanding.fetch_sub(count,
            std::memory_order_relaxed);

        // Repair a decrement landed on a recycled slot (start moved).
        if (record.start.load(std::memory_order_relaxed) != start)
        {
            record.outstanding.fetch_add(count, std::memory_order_relaxed);
            return;
        }

        // Extent completion is allocation-coarse, so maintenance is cheap
        // here and the element write fast path otherwise takes no lock.
        if (previous == count)
        {
            std::unique_lock extent_lock(extent_mutex_, std::try_to_lock);
            if (extent_lock.owns_lock())
                maintain_();
        }

        return;
    }
#endif
}

TEMPLATE
size_t CLASS::frontier() const NOEXCEPT
{
#if defined(MANAGE_STAGING)
    if (staged_)
        return frontier_.load();
#endif

    return size();
}

#if defined(MANAGE_STAGING)

TEMPLATE
void CLASS::record_(size_t start, size_t count) NOEXCEPT
{
    if (!staged_ || is_zero(count))
        return;

    std::unique_lock extent_lock(extent_mutex_);
    maintain_();

    // Saturation stalls the frontier (conservative, safe); asserts in debug.
    const auto size = ring_size_.load(std::memory_order_relaxed);
    BC_ASSERT(size < extents);
    if (size == extents)
        return;

    const auto head = ring_head_.load(std::memory_order_relaxed);
    auto& record = ring_.at((head + size) % extents);
    record.start.store(start, std::memory_order_relaxed);
    record.count = count;
    record.outstanding.store(count * columns, std::memory_order_relaxed);

    // Publish the extent (pairs with the acquire window snapshot).
    ring_size_.store(add1(size), std::memory_order_release);

    if (is_zero(size))
        frontier_.store(start);
}

// Pop completed extents from the head, advancing the frontier (locked).
TEMPLATE
void CLASS::maintain_() NOEXCEPT
{
    auto head = ring_head_.load(std::memory_order_relaxed);
    auto size = ring_size_.load(std::memory_order_relaxed);

    while (!is_zero(size) &&
        is_zero(ring_.at(head).outstanding.load(std::memory_order_relaxed)))
    {
        head = add1(head) % extents;
        --size;
    }

    ring_head_.store(head, std::memory_order_relaxed);
    ring_size_.store(size, std::memory_order_release);
    frontier_.store(is_zero(size) ? logical_.load() :
        ring_.at(head).start.load(std::memory_order_relaxed));
}

// staging dispatch, not thread safe.
// ----------------------------------------------------------------------------
// private

TEMPLATE
template <size_t... Index>
bool CLASS::settle_all_(size_t rows, std::index_sequence<Index...>) NOEXCEPT
{
    const auto from = settled_.load();
    if (!(settle_<Index>(from, rows) && ...))
        return false;

    settled_.store(rows);
    return true;
}

TEMPLATE
template <size_t... Index>
bool CLASS::unsettle_all_(size_t rows, std::index_sequence<Index...>) NOEXCEPT
{
    if (!(unsettle_<Index>(rows) && ...))
        return false;

    settled_.store(rows);
    return true;
}

// staging wrappers, not thread safe.
// ----------------------------------------------------------------------------
// private

// Stage failure results in unmapped.
// Staging has no effect on logical size, commits max(logical, min) capacity.
TEMPLATE
template <size_t Column>
bool CLASS::stage_() NOEXCEPT
{
    auto size = logical_.load();

    // Cannot map empty file, and want minimum capacity, so expand as required.
    // disk_full: space is set but no code is set with false return.
    if ((size < minimum_) && !resize_<Column>(size = minimum_))
        return false;

    // Reserve address space with generous multiple of capacity (costless).
    const auto reserved = page_ceiling(to_width<Column>(to_reservation(size)));
    const auto base = mmap_reserve(reserved);

    if (base == MAP_FAILED)
    {
        set_first_code(error::mmap_failure);
        return false;
    }

    memory_map_[Column] = system::pointer_cast<uint8_t>(base);
    reserved_[Column] = reserved;

    // Commit anonymous pages above the settle boundary page floor.
    const auto settled = page_floor(to_width<Column>(settled_.load()));
    const auto target = to_width<Column>(size);

    if ((target > settled) && (mmap_commit(std::next(memory_map_[Column],
        settled), target - settled) == fail))
    {
        teardown_<Column>(error::mmap_failure);
        return false;
    }

    // Populate anonymous memory from the file (unstaged content in full,
    // staged only the settle boundary page remainder).
    const auto logical = to_width<Column>(logical_.load());

    if ((settled < logical) && !pread_all(opened_[Column],
        std::next(memory_map_[Column], settled), logical - settled, settled))
    {
        teardown_<Column>(error::fsync_failure);
        return false;
    }

    // Convert the settled prefix to a read-only file mapping.
    if (!settle_<Column>(zero, settled_.load()))
        return false;

    loaded_.store(true);
    return true;
}

// Commit failure results in unmapped.
// Growth within the reservation commits pages in place (stable map base); an
// exhausted reservation is replaced and its unsettled content copied, under
// the exclusive remap lock held by the caller.
TEMPLATE
template <size_t Column>
bool CLASS::commit_(size_t size) NOEXCEPT
{
    const auto target = to_width<Column>(size);

    if (target <= reserved_[Column])
    {
        // Never commit below the settle boundary page floor (the settled
        // prefix is a read-only file mapping); recommit is idempotent.
        const auto settled = page_floor(to_width<Column>(settled_.load()));
        const auto current = page_floor(to_width<Column>(capacity_.load()));
        const auto from = std::max(settled, current);

        if ((target > from) && (mmap_commit(std::next(memory_map_[Column],
            from), target - from) == fail))
        {
            teardown_<Column>(error::mmap_failure);
            return false;
        }

        return true;
    }

    // Reservation exhausted, so reserve larger and migrate (rare by sizing).
    const auto reserved = page_ceiling(to_width<Column>(to_reservation(size)));
    const auto replace = mmap_reserve(reserved);

    if (replace == MAP_FAILED)
    {
        teardown_<Column>(error::mmap_failure);
        return false;
    }

    const auto base = system::pointer_cast<uint8_t>(replace);
    const auto settled = page_floor(to_width<Column>(settled_.load()));

    if (mmap_commit(std::next(base, settled), target - settled) == fail)
    {
        ::munmap(replace, reserved);
        teardown_<Column>(error::mmap_failure);
        return false;
    }

    // Copy unsettled content (writes are excluded by the remap lock).
    const auto logical = to_width<Column>(logical_.load());

    if (settled < logical)
        std::copy_n(std::next(memory_map_[Column], settled),
            logical - settled, std::next(base, settled));

    // Convert the settled prefix on the replacement reservation.
    if (!is_zero(settled) &&
        (mmap_settle(replace, settled, opened_[Column], zero) == fail))
    {
        ::munmap(replace, reserved);
        teardown_<Column>(error::mmap_failure);
        return false;
    }

#if !defined(WITHOUT_MADVISE)
    if (!is_zero(settled) && !advise_(base, settled))
    {
        ::munmap(replace, reserved);
        teardown_<Column>(error::madvise_failure);
        return false;
    }
#endif

    // Release the exhausted reservation and adopt the replacement.
    const auto released = ::munmap(memory_map_[Column],
        reserved_[Column]) != fail;

    memory_map_[Column] = base;
    reserved_[Column] = reserved;

    if (!released)
    {
        set_first_code(error::munmap_failure);
        return false;
    }

    return true;
}

// Convert flushed rows [from, to) to a read-only shared file mapping, page
// floored so the settle boundary page remains anonymous with its settled
// bytes retained. Releases the covered anonymous pages. Failure results in
// unmapped.
TEMPLATE
template <size_t Column>
bool CLASS::settle_(size_t from, size_t to) NOEXCEPT
{
    if (!staged_)
        return true;

    const auto begin = page_floor(to_width<Column>(from));
    const auto end = page_floor(to_width<Column>(to));

    if (begin == end)
        return true;

    const auto address = std::next(memory_map_[Column], begin);

    if (mmap_settle(address, end - begin, opened_[Column], begin) == fail)
    {
        teardown_<Column>(error::mmap_failure);
        return false;
    }

#if !defined(WITHOUT_MADVISE)
    if (!advise_(address, end - begin))
    {
        teardown_<Column>(error::madvise_failure);
        return false;
    }
#endif

    return true;
}

// Revert settled pages at/above rows to committed anonymous memory and
// restore the retained bytes below rows from the file (truncation below the
// settle boundary). Failure results in unmapped.
TEMPLATE
template <size_t Column>
bool CLASS::unsettle_(size_t rows) NOEXCEPT
{
    const auto bytes = to_width<Column>(rows);
    const auto begin = page_floor(bytes);
    const auto end = page_floor(to_width<Column>(settled_.load()));

    if (begin == end)
        return true;

    const auto address = std::next(memory_map_[Column], begin);

    if (mmap_unsettle(address, end - begin) == fail)
    {
        teardown_<Column>(error::mmap_failure);
        return false;
    }

    if ((begin < bytes) && !pread_all(opened_[Column], address, bytes - begin,
        begin))
    {
        teardown_<Column>(error::fsync_failure);
        return false;
    }

    return true;
}

// Teardown results in unmapped (release failure is not further reported).
TEMPLATE
template <size_t Column>
void CLASS::teardown_(const error::error_t& ec) NOEXCEPT
{
    set_first_code(ec);

    if (!is_null(memory_map_[Column]))
        ::munmap(memory_map_[Column], reserved_[Column]);

    memory_map_[Column] = {};
    reserved_[Column] = zero;
    loaded_.store(false);
}

// staging utilities, not thread safe.
// ----------------------------------------------------------------------------
// private

#if !defined(WITHOUT_MADVISE)
TEMPLATE
bool CLASS::advise_(uint8_t* map, size_t size) const NOEXCEPT
{
    // Use 1GB chunks to avoid large-length issues.
    constexpr auto chunk = system::power2(30u);
    const auto advice = random_ ? MADV_RANDOM : MADV_SEQUENTIAL;

    for (auto offset = zero; offset < size; offset += chunk)
    {
        const auto length = std::min(chunk, size - offset);
        if (::madvise(std::next(map, offset), length, advice) == fail)
            return false;
    }

    return true;
}
#endif // WITHOUT_MADVISE

// Reservation is address space only (costless), so multiply for headroom;
// exhaustion is handled by reservation replacement.
TEMPLATE
size_t CLASS::to_reservation(size_t rows) const NOEXCEPT
{
    constexpr size_t headroom = 4;
    return system::ceilinged_multiply(to_capacity(std::max(rows, minimum_)),
        headroom);
}

TEMPLATE
size_t CLASS::page_floor(size_t bytes) const NOEXCEPT
{
    return system::bit_and(bytes, system::bit_not(sub1(page_)));
}

TEMPLATE
size_t CLASS::page_ceiling(size_t bytes) const NOEXCEPT
{
    return page_floor(system::ceilinged_add(bytes, sub1(page_)));
}

#endif // MANAGE_STAGING

} // namespace database
} // namespace libbitcoin

#endif
