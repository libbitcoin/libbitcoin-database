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
#ifndef LIBBITCOIN_DATABASE_MEMORY_MMAP_EXTENT_IPP
#define LIBBITCOIN_DATABASE_MEMORY_MMAP_EXTENT_IPP

#include <mutex>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/mstage.hpp>

namespace libbitcoin {
namespace database {

// Write-completion accounting (staged instances). Extents chase completions,
// closing the gaps, settling the completed prefix.
// ----------------------------------------------------------------------------

TEMPLATE
void CLASS::complete(size_t STAGING_ONLY(offset),
    size_t STAGING_ONLY(count)) NOEXCEPT
{
#if defined(MANAGE_STAGING)
    if (!staged_ || is_zero(count))
        return;

    // The covering extent is immobile while this completion is pending and
    // cannot refuse a consistent claim, so a failed pass raced a recycle
    // (torn navigation or stale snapshot): rescan against a fresh window.
    for (;;)
    {
        // Acquire-ordered window snapshot (published entries are immobile;
        // the packed word precludes observing a torn head/size pair).
        const auto window = window_.load(std::memory_order_acquire);
        const auto [head, size] = system::unpack_word<uint64_t>(window);

        // Lock-free binary search of the sorted window (relaxed navigation
        // is heuristic only; claim_ verifies cover under the generation).
        auto high = size;
        for (size_t low{}; low < high;)
        {
            const auto middle = to_half(low + high);
            auto& record = ring_.at((head + middle) % extents);
            const auto start = record.start.load(relaxed);

            if (offset < start)
            {
                high = middle;
                continue;
            }

            if (offset >= (start + record.count.load(relaxed)))
            {
                low = add1(middle);
                continue;
            }

            if (claim_(record, offset, count))
                return;

            // The matched slot recycled under the search: scan below.
            break;
        }

        // Contention can record extents out of start order (allocation claim
        // and recording are not one atomic step), transiently breaking search
        // order. Contiguity guarantees a unique cover: scan linearly.
        for (size_t index{}; index < size; ++index)
            if (claim_(ring_.at((head + index) % extents), offset, count))
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

// extent ring, locked except claim_ (lock-free).
// ----------------------------------------------------------------------------
// private

// Claim completion of count rows against the extent, by cas decrement of the
// packed state under a cover check ordered by that state's own acquire: the
// publishing release sequences start/count before state, so both read after
// it belong to the observed generation (a live outstanding precludes a
// recycle in progress, as slots re-record only after retirement). A range
// match read before the acquire can be torn across a recycle and debit the
// slot's successor extent, pinning the true cover's frontier forever. False
// implies the slot does not (or no longer does) cover the offset, or is
// drained (recycle in flight): the caller rescans. The true covering extent
// cannot refuse: its generation is stable and its outstanding covers every
// pending completion while any remains.
TEMPLATE
bool CLASS::claim_(extent& record, size_t offset, size_t count) NOEXCEPT
{
    using namespace system;
    auto state = record.state.load(std::memory_order_acquire);

    for (;;)
    {
        const auto generation = shift_right<uint64_t>(state, generation_shift);
        const auto outstanding = bit_and<uint64_t>(state, outstanding_mask);
        const auto start = record.start.load(relaxed);

        // An outstanding below the claim is a recycle in flight or a drained
        // slot (never the covering extent), refused rather than wrapped.
        if ((outstanding < count) || (offset < start) ||
            (offset >= (start + record.count.load(relaxed))))
            return false;

        if (record.state.compare_exchange_weak(state,
            pack_extent_(generation, outstanding - count),
            std::memory_order_acq_rel, std::memory_order_acquire))
        {
            // Extent completion is allocation-coarse, so maintenance is
            // cheap here and the element write fast path takes no lock.
            if (outstanding == count)
            {
                std::unique_lock extent_lock(extent_mutex_, std::try_to_lock);

                if (extent_lock.owns_lock())
                    maintain_();
            }

            return true;
        }

        // The failed cas reloaded state (acquire): recheck under it.
    }
}

// Claim and record an extent under one lock: a claim never exists outside
// the ring and the ring is start-ordered, so the frontier can never pass an
// unwritten extent (claim-then-record raced the frontier past the claim).
// Returns eof (unclaimed) on insufficient capacity, fault, or disk full.
TEMPLATE
size_t CLASS::record_(size_t count) NOEXCEPT
{
    std::unique_lock extent_lock(extent_mutex_);

    if (is_zero(count))
        return logical_.load();

    maintain_();

    using namespace system;
    auto [head, size] = unpack_word<uint64_t>(window_.load(relaxed));

    // A full ring waits on completions (extents are allocation-coarse, so
    // saturation implies extreme concurrency). Completions are lock-free, so
    // waiting needs only this thread's own maintenance; fault or disk full
    // releases the wait unclaimed (the write then fails fast).
    while (size == extents)
    {
        if (fault_.load() || !is_zero(space_.load()))
            return storage::eof;

        std::this_thread::yield();
        maintain_();
        std::tie(head, size) = unpack_word<uint64_t>(window_.load(relaxed));
    }

    const auto start = logical_.load();
    if (is_add_overflow(start, count) ||
        ((start + count) > capacity_.load()))
        return storage::eof;

    auto& record = ring_.at((head + size) % extents);
    const auto generation = bit_and<uint64_t>(add1(shift_right<uint64_t>(
        record.state.load(relaxed), generation_shift)), generation_mask);
    BC_ASSERT((count * columns) <= outstanding_mask);
    record.start.store(start, relaxed);
    record.count.store(count, relaxed);

    // Publish the state (pairs with the acquire in claim_): a claim against
    // the stale generation now fails its cas, and one against this
    // generation observes the new start and count through the release.
    record.state.store(pack_extent_(generation, count * columns),
        std::memory_order_release);

    // Publish the extent (pairs with the acquire window snapshot).
    window_.store(pack_word<uint64_t>(head, add1(size)), release);
    if (is_zero(size))
        frontier_.store(start);

    logical_.store(start + count);
    check_invariants_();
    return start;
}

// Pop completed extents from the head, advancing the frontier (locked).
TEMPLATE
void CLASS::maintain_() NOEXCEPT
{
    using namespace system;
    auto [head, size] = unpack_word<uint64_t>(window_.load(relaxed));
    while (!is_zero(size) && is_zero(bit_and<uint64_t>(
        ring_.at(head).state.load(relaxed), outstanding_mask)))
    {
        head = add1(head) % extents;
        --size;
    }

    window_.store(pack_word<uint64_t>(head, size), release);
    frontier_.store(is_zero(size) ? logical_.load() :
        ring_.at(head).start.load(relaxed));
    check_invariants_();
}

// Discard all extents, requires quiescent writers (locked). Any extent then
// outstanding is an abandoned write (unreferenced), safe to settle as is.
TEMPLATE
void CLASS::discard_() NOEXCEPT
{
    if (!staged_)
        return;

    std::unique_lock extent_lock(extent_mutex_);

    window_.store(zero, release);
    frontier_.store(logical_.load());
    check_invariants_();
}

// Discard extents above the truncation and clamp any overlap, requires
// quiescent writers (locked).
TEMPLATE
void CLASS::trim_(size_t count) NOEXCEPT
{
    if (!staged_)
        return;

    std::unique_lock extent_lock(extent_mutex_);

    using namespace system;
    auto [head, size] = unpack_word<uint64_t>(window_.load(relaxed));
    while (!is_zero(size))
    {
        auto& tail = ring_.at((head + sub1(size)) % extents);
        const auto start = tail.start.load(relaxed);
        if (start >= count)
        {
            --size;
            continue;
        }

        if (ceilinged_add(start, tail.count.load(relaxed)) > count)
        {
            const auto trimmed = count - start;
            tail.count.store(trimmed, relaxed);

            // Clamp outstanding within the state, generation unchanged
            // (the extent is trimmed, not recycled; writers quiescent).
            const auto limit = trimmed * columns;
            const auto state = tail.state.load(relaxed);
            if (bit_and<uint64_t>(state, outstanding_mask) > limit)
                tail.state.store(pack_extent_(shift_right<uint64_t>(
                    state, generation_shift), limit), relaxed);
        }

        break;
    }

    window_.store(pack_word<uint64_t>(head, size), release);
    frontier_.store(is_zero(size) ? count :
        ring_.at(head).start.load(relaxed));
}

#endif // MANAGE_STAGING

} // namespace database
} // namespace libbitcoin

#endif
