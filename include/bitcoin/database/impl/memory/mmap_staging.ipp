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
#include <chrono>
#include <fcntl.h>
#if defined(STAGING_TELEMETRY)
    #include <iostream>
    #include <sstream>
#endif
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/mstage.hpp>
#include <bitcoin/database/memory/utilities.hpp>

namespace libbitcoin {
namespace database {

// Write-completion accounting (staged instances). Extents chase completions,
// closing the gaps, settling the completed prefix.

TEMPLATE
void CLASS::complete(size_t STAGING_ONLY(offset),
    size_t STAGING_ONLY(count)) NOEXCEPT
{
#if defined(MANAGE_STAGING)
    if (!staged_ || is_zero(count))
        return;

    // Acquire-ordered window snapshot (published entries are immobile; the
    // packed word precludes observing a torn head/size pair across pops).
    const auto window = window_.load(std::memory_order_acquire);
    const auto [head, size] = system::unpack_word<uint64_t>(window);

    // Lock-free binary search of the sorted window for the covering extent.
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

        if (offset >= (start + record.count))
        {
            low = add1(middle);
            continue;
        }

        // Wrap from over-completion stalls the frontier (conservative).
        const auto previous = record.outstanding.fetch_sub(count, relaxed);

        // Repair a decrement landed on a recycled slot (start moved).
        if (record.start.load(relaxed) != start)
        {
            record.outstanding.fetch_add(count, relaxed);
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

    // Contention can record extents out of start order (allocation claim and
    // recording are not one atomic step), transiently breaking search order.
    // Contiguity guarantees a unique cover, so fall back to a linear scan.
    for (size_t index{}; index < size; ++index)
    {
        auto& record = ring_.at((head + index) % extents);
        const auto start = record.start.load(relaxed);
        if ((offset < start) || (offset >= (start + record.count)))
            continue;

        const auto previous = record.outstanding.fetch_sub(count, relaxed);
        if (record.start.load(relaxed) != start)
        {
            record.outstanding.fetch_add(count, relaxed);
            return;
        }

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

    using namespace system;
    auto [head, size] = unpack_word<uint64_t>(window_.load(relaxed));

    // A full ring waits on completions (extents are allocation-coarse, so
    // saturation implies extreme concurrency). An unrecorded extent would be
    // unsafe: an emptied ring advances the frontier to logical, so untracked
    // incomplete writes could settle. Completions are lock-free, so waiting
    // needs only this thread's own maintenance; fault or disk full releases
    // the wait (recording is then moot, as recovery discards the ring).
    while (size == extents)
    {
        if (fault_.load() || !is_zero(space_.load()))
            return;

        std::this_thread::yield();
        maintain_();
        std::tie(head, size) = unpack_word<uint64_t>(window_.load(relaxed));
    }

    auto& record = ring_.at((head + size) % extents);
    record.start.store(start, relaxed);
    record.count = count;
    record.outstanding.store(count * columns, relaxed);

    // Publish the extent (pairs with the acquire window snapshot).
    window_.store(pack_word<uint64_t>(head, add1(size)), release);
    if (is_zero(size))
        frontier_.store(start);
}

// Pop completed extents from the head, advancing the frontier (locked).
TEMPLATE
void CLASS::maintain_() NOEXCEPT
{
    using namespace system;
    auto [head, size] = unpack_word<uint64_t>(window_.load(relaxed));
    while (!is_zero(size) && is_zero(ring_.at(head).outstanding.load(relaxed)))
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

// Delay the caller while staging exceeds its memory bound (write throttle).
// Signaled as the settler drains; a fault or settler stop releases the caller
// into the normal allocation path (which fails on fault/unload). The relieved
// state is lock-free for the common (unparked) case; notifiers synchronize on
// throttle_mutex_ so a parked caller cannot miss its signal.
TEMPLATE
void CLASS::throttle_() NOEXCEPT
{
    const auto relieved = [this]() NOEXCEPT
    {
        using namespace system;
        const auto rows = floored_subtract(logical_.load(), settled_.load());
        const auto debt = ceilinged_multiply(rows, stride);
        return (debt <= limit_) || !settling_.load() || fault_.load();
    };

    if (!staged_ || relieved())
        return;

    std::unique_lock throttle_lock(throttle_mutex_);

    throttle_cv_.wait(throttle_lock, relieved);
}

// Wake throttled callers, synchronized so a parking caller cannot miss the
// signal between its relieved check and its wait.
TEMPLATE
void CLASS::signal_() NOEXCEPT
{
    std::unique_lock throttle_lock(throttle_mutex_);

    throttle_cv_.notify_all();
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
    signal_();
    check_invariants_();
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

TEMPLATE
template <size_t... Index>
bool CLASS::evict_all_(size_t from, size_t to,
    std::index_sequence<Index...>) NOEXCEPT
{
    return (evict_<Index>(from, to) && ...);
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
    // Provision the file in full (disk reserved, so allocation within cannot
    // fail for space), but commit only what is in use; committing provisioned
    // rows would charge that memory before anything is written.
    // disk_full: space is set but no code is set with false return.
    const auto provision = to_provision();
    if (!resize_<Column>(provision))
        return false;

    const auto size = to_commitment();

    // Reserve address space with generous multiple of capacity (costless), so
    // that commitment growth never migrates the mapping (base is stable).
    const auto reserved = page_ceiling(to_width<Column>(
        to_reservation(provision)));
    const auto base = mmap_reserve(reserved);

    if (base == MAP_FAILED)
    {
        set_first_code(error::mmap_failure);
        return false;
    }

    using namespace system;
    memory_map_[Column] = pointer_cast<uint8_t>(base);
    reserved_[Column] = reserved;

    // Page-dirty tracking for unstaged (rewrite-in-place head) instances,
    // sized to the reservation (value-initialized to clean). Single column
    // only (byte-offset marks); aggregates transfer in full. Shared heads
    // write through their mapping, so they track and transfer nothing.
    if constexpr (is_zero(Column) && is_one(columns))
    {
        if (!staged_ && !head_shared)
        {
            const auto pages = ceilinged_divide(reserved, page_);
            words_ = ceilinged_divide(pages, page_bound);
            dirty_ = std::make_unique<dirty_bitmaps>(words_);
            intent_ = std::make_unique<dirty_bitmaps>(words_);
            released_ = std::make_unique<dirty_bitmaps>(words_);
            sweep_ = std::make_unique<uint64_t[]>(words_);
            writers_.store(zero);
        }
    }

    const auto target = to_width<Column>(size);

    // A shared head maps its file in place of commitment (content is the
    // mapping, so there is nothing to populate).
    if (!staged_ && head_shared)
    {
        if (mmap_share(memory_map_[Column], target, opened_[Column],
            zero) == fail)
        {
            teardown_<Column>(error::mmap_failure);
            return false;
        }

#if !defined(WITHOUT_MADVISE)
        // Heads read randomly, so the mapping advises as the file opens.
        if (!advise_(memory_map_[Column], target))
        {
            teardown_<Column>(error::madvise_failure);
            return false;
        }
#endif

        loaded_.store(true);
        return true;
    }

    // Commit anonymous pages above the settle boundary page floor.
    const auto settled = page_floor(to_width<Column>(settled_.load()));

    if ((target > settled) && (mmap_commit(std::next(memory_map_[Column],
        settled), target - settled) == fail))
    {
        teardown_<Column>(error::mmap_failure);
        return false;
    }

    // Populate anonymous memory from the file (unstaged content in full,
    // staged only the settle boundary page remainder).
    const auto logical = to_width<Column>(logical_.load());

    // The read streams its own page cache discard: a whole-column read
    // caches the column (the head resident twice), and that transient
    // stacked over the growing anonymous set forces reclaim during staging
    // (measured: the eldest staged columns swap before the store is open).
    // Chunking caps the duplicate at settle_chunk.
    for (auto at = settled; at < logical; )
    {
        const auto chunk = std::min(logical - at, settle_chunk);
        if (!pread_all(opened_[Column],
            std::next(memory_map_[Column], at), chunk, at))
        {
            teardown_<Column>(error::fsync_failure);
            return false;
        }

        at += chunk;
        file_discard(opened_[Column]);
    }

    // Convert the settled prefix to a read-only file mapping.
    if (!settle_<Column>(zero, settled_.load()))
        return false;

    // Attribute the anonymous span (above the settled file mapping) for
    // diagnostics: smaps then decomposes anonymous memory by table.
    mmap_name(std::next(memory_map_[Column], settled),
        reserved_[Column] - settled,
        filenames_[Column].filename().string().c_str());

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
        // A shared head extends its file mapping over the growth (the file is
        // provisioned to capacity, so the extension is already allocated).
        if (!staged_ && head_shared)
        {
            const auto grown = page_floor(to_width<Column>(capacity_.load()));

            if ((target > grown) && (mmap_share(std::next(
                memory_map_[Column], grown), target - grown, opened_[Column],
                grown) == fail))
            {
                teardown_<Column>(error::mmap_failure);
                return false;
            }

            return true;
        }

        // Never commit below the settle boundary page floor (the settled
        // prefix is a read-only file mapping); recommit is idempotent.
        const auto settled = page_floor(to_width<Column>(settled_.load()));
        const auto current = page_floor(to_width<Column>(capacity_.load()));
        const auto from = std::max(settled, current);

        if ((target > from) && (mmap_commit(std::next(memory_map_[Column], from),
            target - from) == fail))
        {
            teardown_<Column>(error::mmap_failure);
            return false;
        }

        // Committed growth is a new (unnamed) vma; reattribute it.
        if (target > from)
            mmap_name(std::next(memory_map_[Column], from), target - from,
                filenames_[Column].filename().string().c_str());

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

    using namespace system;
    const auto base = pointer_cast<uint8_t>(replace);
    const auto settled = page_floor(to_width<Column>(settled_.load()));

    // A shared head remaps its file onto the replacement (the file is the
    // content, so migration copies nothing).
    if (!staged_ && head_shared)
    {
        if (mmap_share(replace, target, opened_[Column], zero) == fail)
        {
            ::munmap(replace, reserved);
            teardown_<Column>(error::mmap_failure);
            return false;
        }

        const auto stale = ::munmap(memory_map_[Column], reserved_[Column])
            != fail;

        memory_map_[Column] = base;
        reserved_[Column] = reserved;

        if (!stale)
        {
            set_first_code(error::munmap_failure);
            return false;
        }

        return true;
    }

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

    // Regrow dirty tracking with the reservation (marks are excluded by the
    // remap lock, as unstaged mutations hold protected accessors).
    if constexpr (is_zero(Column))
    {
        if (!staged_ && dirty_)
        {
            const auto pages = ceilinged_divide(reserved, page_);
            const auto words = ceilinged_divide(pages, page_bound);
            auto grown = std::make_unique<dirty_bitmaps>(words);

            for (size_t word{}; word < std::min(words_, words); ++word)
                grown[word].store(dirty_[word].load(relaxed),
                    relaxed);

            dirty_ = std::move(grown);
            words_ = words;

            // The replacement reservation is fully anonymous (content was
            // copied above), so released and intent page state resets.
            intent_ = std::make_unique<dirty_bitmaps>(words);
            released_ = std::make_unique<dirty_bitmaps>(words);
            sweep_ = std::make_unique<uint64_t[]>(words);
        }
    }

    // Release the exhausted reservation and adopt the replacement.
    const auto released = ::munmap(memory_map_[Column], reserved_[Column]) 
        != fail;

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

    // Demote the settled extent to reclaim-first order: cached for imminent
    // re-read (validation follows archival) but reclaimed under pressure
    // before active pages and anonymous heads (head residency priority).
    // Ample hosts skip demotion: with nothing contesting memory it only
    // converts warm cache into re-read faults (see demote_memory).
    static const auto ample = system_memory() > demote_memory;
    if (!ample && (mmap_cold(address, end - begin) == fail))
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

// Release cached pages of settled rows [from, to), page floored within the
// converted read-only mapping (its pages cannot be dirtied, and any pending
// settle write-back is synced by the release). Cost follows residency, so
// re-eviction is idempotent and free. The mapping is unaffected (a later
// read faults the page back from the file). Failure faults the store
// (advisory-class failures have no benign modes) but leaves it mapped, as
// the shared-lock caller cannot tear down under live accessors.
TEMPLATE
template <size_t Column>
bool CLASS::evict_(size_t from, size_t to) NOEXCEPT
{
    const auto begin = page_floor(to_width<Column>(from));
    const auto end = page_floor(to_width<Column>(to));
    if (begin == end)
        return true;

    if (mmap_evict(std::next(memory_map_[Column], begin), end - begin) == fail)
    {
        set_first_code(error::fsync_failure);
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
    // Advice is elective (normal is the kernel default) and configured from
    // the read pattern (see database::advice); random_ is structural.
    if (access_ == advice::normal)
        return true;

    // Order follows the advice enumeration (scattered is random without the
    // preload, which this path does not apply in any case).
    static constexpr std::array<int, 4> advices
    {
        MADV_NORMAL, MADV_RANDOM, MADV_SEQUENTIAL, MADV_RANDOM
    };

    const auto behavior = advices.at(static_cast<uint8_t>(access_));
    for (size_t offset{}; offset < size; offset += advise_chunk)
    {
        const auto length = std::min(advise_chunk, size - offset);
        if (::madvise(std::next(map, offset), length, behavior) == fail)
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
    using namespace system;
    return ceilinged_multiply(to_capacity(std::max(rows, minimum_)), headroom);
}

TEMPLATE
size_t CLASS::page_floor(size_t bytes) const NOEXCEPT
{
    using namespace system;
    return bit_and(bytes, bit_not(sub1(page_)));
}

TEMPLATE
size_t CLASS::page_ceiling(size_t bytes) const NOEXCEPT
{
    using namespace system;
    return page_floor(ceilinged_add(bytes, sub1(page_)));
}

// settle scheduler, instance-owned (drains completed writes to clean cache).
// ----------------------------------------------------------------------------
// private

// Dirty marking without the writer uncount (no paired prepare), for internal
// mark restoration (mark() pairs with prepare() and uncounts its writer).
TEMPLATE
void CLASS::remark_(size_t offset, size_t size) NOEXCEPT
{
    auto page = offset / page_;
    const auto end = (offset + sub1(size)) / page_;
    while ((page <= end) && ((page / page_bound) < words_))
    {
        const auto bit = system::bit_right<uint64_t>(page % page_bound);
        dirty_[page++ / page_bound].fetch_or(bit, relaxed);
    }

    marks_.fetch_add(one, relaxed);
}

// Transfer dirty pages within [0, bytes), clearing marks before content
// reads so that concurrently remarked pages transfer on the next pass.
// Marks beyond bytes are retained (backfill above logical transfers when
// logical grows over it). Adjacent dirty pages coalesce into single writes.
TEMPLATE
template <size_t Column>
bool CLASS::transfer_(size_t bytes) NOEXCEPT
{
    using namespace system;
    if (is_zero(bytes))
        return true;

    // One pass at a time: concurrent passes split the claimed dirty set.
    std::unique_lock transfer_lock(transfer_mutex_);

    // Untracked (multi-column unstaged) instances transfer in full.
    if (!dirty_)
        return pwrite_all(opened_[Column], memory_map_[Column], bytes, zero);

    size_t from{};
    size_t to{};
    const auto write = [&]() NOEXCEPT
    {
        return (from >= to) || pwrite_all(opened_[Column],
            std::next(memory_map_[Column], from), to - from, from);
    };

    const auto pages = ceilinged_divide(bytes, page_);
    const auto bound = std::min(words_, ceilinged_divide(pages, page_bound));
    for (size_t word{}; word < bound; ++word)
    {
        auto bits = dirty_[word].exchange(zero, relaxed);

        // Retain marks at and above the page bound (boundary word only).
        const auto first = word * page_bound;
        if (pages < (first + page_bound))
        {
            const auto keep = mask_right<uint64_t>(pages - first);
            dirty_[word].fetch_or(bit_and(bits, keep), relaxed);
            bits = bit_and(bits, bit_not(keep));
        }

        for (size_t bit{}; !is_zero(bits) && (bit < page_bound); ++bit)
        {
            if (!get_right(bits, bit))
                continue;

            bits = set_right(bits, bit, false);
            const auto start = (first + bit) * page_;
            const auto end = std::min(start + page_, bytes);
            if (start == to)
            {
                to = end;
                continue;
            }

            if (!write())
            {
                // Restore claimed marks (the failed range, the page that
                // ended it, and this word's unwritten remainder) so failure
                // is retryable, not lossy.
                remark_(from, to - from);
                remark_(start, end - start);
                dirty_[word].fetch_or(bits, relaxed);
                return false;
            }

            from = start;
            to = end;
        }
    }

    if (!write())
    {
        remark_(from, to - from);
        return false;
    }

    return true;
}

// Durability barrier for the column file.
TEMPLATE
template <size_t Column>
bool CLASS::sync_() NOEXCEPT
{
#if defined(F_FULLFSYNC)
    // non-standard macOS behavior: news.ycombinator.com/item?id=30372218
    return ::fcntl(opened_[Column], F_FULLFSYNC, 0) != fail;
#else
    return ::fsync(opened_[Column]) != fail;
#endif
}

TEMPLATE
void CLASS::settler_start_() NOEXCEPT
{
    // A shared head has no lazy writer (the kernel writes its mapping back).
    if (!staged_ && head_shared)
        return;

    limit_ = system_memory() / throttle_factor;
    evicted_ = zero;
    settling_.store(true);
    settler_ = std::thread([this]() NOEXCEPT
    {
        staged_ ? settler_run_() : head_run_();
    });
}

TEMPLATE
void CLASS::settler_stop_() NOEXCEPT
{
    if (!settling_.exchange(false))
        return;

    signal_();
    settler_cv_.notify_all();
    if (settler_.joinable())
        settler_.join();
}

// Pressure-paced draining (the windows model): drain intensity follows memory
// conditions, settled pages remain cached, writers delay only at the staging
// memory bound (write throttle), sustained stillness drains the residual (the
// lazy writer) so a quiescent map converges to fully settled.
TEMPLATE
void CLASS::settler_run_() NOEXCEPT
{
    // Tiers derive from physical memory, pressure and compression occupancy
    // (a pressure precursor: the kernel compresses without raising level).
    const auto memory  = system_memory();
    const auto urgent  = memory / urgent_factor;
    const auto active  = memory / active_factor;
    const auto squeeze = memory / compress_factor;
    const auto scarce  = memory / sweep_factor;
    const auto chunk   = std::max(one, settle_chunk / stride);
    const auto sweep   = std::max(one, evict_chunk / stride);

    // Ticks without allocation before idle draining (settling is not writing,
    // so draining does not hold its own clock).
    auto mark = logical_.load();
    size_t still{};

    const auto backlog = [this]() NOEXCEPT
    {
        using namespace system;
        return ceilinged_multiply(floored_subtract(frontier_.load(),
            settled_.load()), stride);
    };

    while (settling_.load())
    {
        std::unique_lock settler_lock(settler_mutex_);
        settler_cv_.wait_for(settler_lock, std::chrono::seconds(1));
        settler_lock.unlock();

        if (!settling_.load())
            return;

        const auto top = logical_.load();
        still = (top == mark) ? std::min(add1(still), idle_seconds) : zero;
        mark = top;

#if defined(STAGING_TELEMETRY)
        // The settler drains to the frontier while the throttle measures debt
        // to logical, so a pinned frontier stops settling while debt grows
        // (unbounded). lag exposes the pin, debt the throttle pressure. One
        // string per line, as settler threads share the stream.
        if (is_zero(++telemetry_ % telemetry_seconds))
        {
            using namespace system;
            const auto settled = settled_.load();
            const auto frontier = frontier_.load();
            const auto [head_, size_] = unpack_word<uint64_t>(
                window_.load(relaxed));

            std::ostringstream line{};
            line << "staging " << filenames_.front().filename().string()
                << " logical=" << top
                << " frontier=" << frontier
                << " settled=" << settled
                << " lag=" << floored_subtract(top, frontier)
                << " debt=" << floored_subtract(top, settled)
                << " window=" << size_
                << " head=" << head_
                << std::endl;

            std::cerr << line.str() << std::flush;
        }
#endif

        // Scarcity is read directly: clean cache is reclaimable, so the
        // kernel pressure level does not raise while free memory exhausts.
        if (system_free() < scarce)
            evict_next_(sweep);

        auto bytes = backlog();
        if (is_zero(bytes))
            continue;

        // Urgency drains continuously, activity/stillness one chunk per tick.
        const auto driven =
            (system_pressure() > one) ||
            (system_compressed() > squeeze) ||
            (bytes > urgent);

        if (!driven && 
            (bytes <= active) && 
            (still < idle_seconds))
            continue;

        do
        {
            if (!settle_next_(chunk))
                break;

            bytes = backlog();
        }
        while (settling_.load() && driven && (bytes > active));
    }
}

// Idle transfer for unstaged (rewrite-in-place head) instances: sustained
// mark stillness drains dirty pages (the lazy writer) so a quiescent map
// converges to persisted and snapshot/close transfer approximately nothing.
// Requires no write exclusion: marks follow writes and transfer clears
// before reading, so racing writes remark and transfer on the next pass
// (torn disk pages are unreachable, as live heads are only trusted
// following a clean close).
//
// Memory scarcity additionally pages the head to its own file (the windows
// model): transfer, then release cold clean pages to read-only file mappings
// (reclaimable cache), restored to anonymous by prepare() before any write.
// Release waits one pass after engagement so all writers declare intent.
TEMPLATE
void CLASS::head_run_() NOEXCEPT
{
    const auto scarce = system_memory() / evict_factor;

    // Ticks without a mark before idle draining.
    auto mark = marks_.load();
    auto transferred = mark;
    size_t still{};
    size_t touched{};

    while (settling_.load())
    {
        std::unique_lock settler_lock(settler_mutex_);
        settler_cv_.wait_for(settler_lock, std::chrono::seconds(1));
        settler_lock.unlock();

        if (!settling_.load())
            return;

        const auto top = marks_.load();
        const auto writes = top - mark;
        still = (top == mark) ? std::min(add1(still), idle_seconds) : zero;
        mark = top;

        // Touch pass: assert working-set residency at a bounded rate.
        // Hash-uniform probing is per-page sparse in every phase, so the
        // kernel ages head pages cold and swaps them under cache pressure,
        // though the set is the process working set (and head misses are
        // unshieldable serial faults on the probe path). A read sets the
        // page-table accessed bit without dirtying the page; volatile
        // prevents elision. The unprivileged equivalent of mlock, and soft:
        // under true extremity the kernel can still take the pages. The
        // per-tick budget revisits every page each touch_seconds regardless
        // of instance size (a whole-instance lap gave the largest head a
        // proportionally longer revisit, which lost the aging race first).
        // Residency-guarded so the touch never faults: a swapped page that
        // nothing probes rests in swap, one the workload needs returns by
        // its own fault and is defended thereafter.
        // Residency is only contested under scarcity, so at plenty the tick
        // costs a counter test (the aging race has no other runner).
        if (system_available() < (system_memory() / sweep_factor))
        {
            using namespace system;
            std::shared_lock touch_lock(remap_mutex_);
            if (loaded_.load() && !fault_.load())
            {
                const auto pages = to_width<zero>(logical_.load()) / page_;
                auto budget = ceilinged_divide(pages, touch_seconds);
#if !defined(HAVE_APPLE)
                // Probe buffer and map alias, declared with the touch they
                // serve (excluded on darwin below), as they are otherwise
                // unused there.
                unsigned char resident[touch_span];
                const volatile auto* map = memory_map_[zero];
#endif
                while (!is_zero(pages) && !is_zero(budget))
                {
                    if (touched >= pages)
                        touched = zero;

                    const auto count = std::min(
                        { touch_span, pages - touched, budget });

#if !defined(HAVE_APPLE)
                    // Darwin excluded: mincore hides compressed pages from
                    // the guard, and unguarded touching measured as pure
                    // decompression churn; release is the darwin mechanism.
                    const auto at = touched * page_;
                    if (mmap_resident(std::next(memory_map_[zero], at),
                        count * page_, resident) == 0)
                        for (size_t page{}; page < count; ++page)
                            if (is_odd(resident[page]))
                                (void)map[at + page * page_];
#endif

                    touched += count;
                    budget = floored_subtract(budget, count);
                }
            }
        }

        // Available includes reclaimable file cache, which a loaded store
        // keeps large while the kernel swaps cold anonymous pages, so free
        // exhaustion also signals scarcity (anon is being displaced).
        const auto scarcity = head_release && dirty_ &&
            ((system_available() < scarce) || (system_free() < scarce));

        // Once engaged, a quiet instance converts independent of momentary
        // scarcity: the signal clears as swap absorbs the hot set, but the
        // swapped pages remain anonymous, so reads fault them back one at a
        // time (a serial swap-in per probe). Conversion instead settles
        // clean pages without read-back (the file holds their content),
        // freeing swap and routing reads through the file mapping.
        const auto engaged = engaged_.load();
        const auto quiet = writes < release_quiet;
        const auto draining = engaged && quiet;
        if (!scarcity && !draining &&
            ((still < idle_seconds) || (transferred == top)))
            continue;

        // A write-hot instance neither transfers nor releases under
        // scarcity: transferred pages re-dirty immediately (write
        // amplification without release payoff, as hash-scattered writes
        // into released pages each cost a segment restore, and the sweep
        // otherwise re-releases restored segments every pass). Its
        // anonymous set is left to swap (dirty-exempt) until quiescence,
        // typically the phase change.
#if !defined(HAVE_APPLE)
        // EXPERIMENT: darwin releases while write-hot (page-level intent and
        // dirty filters remain) to price segment restores against the
        // measured compressed-hot crawl.
        if (scarcity && !quiet)
            continue;
#endif

        {
            std::shared_lock map_lock(remap_mutex_);

            if (!loaded_.load() || fault_.load())
                continue;

            if constexpr (head_release)
                if (scarcity && !engaged)
                    engaged_.store(true);

            // Scarcity passes pace writeback for release: settle maps page
            // cache content, and durability remains a clean close property,
            // so sync applies only to idle draining.
            if (!transfer_<zero>(to_width<zero>(logical_.load())) ||
                (!scarcity && !sync_<zero>()))
            {
                set_first_code(error::fsync_failure);
                continue;
            }

            // Discard the page cache copy of the transfer: the anonymous
            // head is the live copy, so caching the file doubles it.
            file_discard(opened_[zero]);

            // Quiet is assured here (hot scarcity skipped above, and idle
            // draining implies sixty still seconds).
            if (engaged && !release_pages_())
                continue;
        }

        transferred = top;
        still = zero;
    }
}

// Release cold clean head page runs to read-only file mappings (reclaimable),
// full pages below logical only. Conversion is run-granular (release_chunk
// minimum) as each conversion splits a mapping: page granularity fragments
// the address space beyond what host memory management tolerates. Writer
// synchronization is a per-page bit protocol: prepare() declares intent then
// loads released; release stores released then loads intent (both
// sequentially consistent), so a run converts only when no write can land on
// it unrestored. A wrong release costs one restore. Conversion and restore
// serialize on restore_mutex_.
TEMPLATE
bool CLASS::release_pages_() NOEXCEPT
{
    using namespace system;
    const auto bytes = to_width<zero>(logical_.load());
    const auto pages = bytes / page_;
    const auto bound = std::min(words_, ceilinged_divide(pages, page_bound));
    const auto chunk = std::max(one, release_chunk / page_);

    std::unique_lock restore_lock(restore_mutex_);

    // Materialize candidacy (hot aging clears only the snapshot bits, so a
    // concurrent declaration on a candidate page is retained for the live
    // rechecks below).
    for (size_t word{}; word < bound; ++word)
    {
        const auto hot = intent_[word].load();
        intent_[word].fetch_and(bit_not(hot));
        sweep_[word] = release_below(release_candidates(
            dirty_[word].load(relaxed), hot, released_[word].load(relaxed)),
            pages, word * page_bound);
    }

    // Convert maximal candidate runs of at least chunk pages, clamped to
    // chunk bounds. Restore is chunk-aligned and chunk-granular, so a
    // partially released chunk places unreleased pages inside a restored
    // span. The bit protocol guards released pages only: a writer to an
    // unreleased page observes released clear, so it neither restores nor
    // takes restore_mutex_, and its write is dropped by the copy-install
    // window. Whole chunks make the restored span exactly the released set.
    for (auto run = next_run(sweep_.get(), pages, zero); run.first < pages;
        run = next_run(sweep_.get(), pages, run.second))
    {
        const auto first = ceilinged_divide(run.first, chunk) * chunk;
        const auto second = (run.second / chunk) * chunk;

        if ((second <= first) || ((second - first) < chunk))
            continue;

        const auto begin = first / page_bound;
        const auto end = sub1(second) / page_bound;
        const auto mask = [&](size_t word) NOEXCEPT
        {
            return page_mask(first, second, word * page_bound);
        };

        // Declare the release (prepare() restores from this point).
        for (auto word = begin; word <= end; ++word)
            released_[word].fetch_or(mask(word));

        // An in-flight writer (counted, unaged) or raced intent or mark
        // invalidates the conversion (whole run). The count loads first: a
        // writer counted later observes released and restores, one drained
        // earlier has published its marks (both sequentially consistent).
        auto raced = is_nonzero(writers_.load());
        for (auto word = begin; (word <= end) && !raced; ++word)
            raced = !is_zero(bit_and(mask(word),
                bit_or(intent_[word].load(), dirty_[word].load())));

        if (raced || (mmap_settle(
            std::next(memory_map_[zero], first * page_),
            (second - first) * page_, opened_[zero],
            first * page_) == fail))
        {
            for (auto word = begin; word <= end; ++word)
                released_[word].fetch_and(bit_not(mask(word)));

            if (!raced)
            {
                set_first_code(error::mmap_failure);
                return false;
            }
        }
    }

    return true;
}

// Restore released pages overlapping [offset, offset+size) to writable
// anonymous memory (content preserved by atomic installation), before a
// declared write. Restoration is segmented at release_chunk alignment: the
// containing segment restores whole (unifying any fragmentation within it)
// but never more, as released runs consolidate without bound and restoring
// a maximal run copies gigabytes per scattered write (a restore convoy).
TEMPLATE
void CLASS::restore_(size_t offset, size_t size) NOEXCEPT
{
    using namespace system;
    std::unique_lock restore_lock(restore_mutex_);

    // Segments clamp to full pages below logical (as does release candidacy):
    // the reservation above commitment is inaccessible (installation reads).
    const auto pages = to_width<zero>(logical_.load()) / page_;
    const auto span = std::max(one, release_chunk / page_);
    const auto last = (offset + sub1(size)) / page_;
    auto page = offset / page_;
    page -= (page % span);

    for (; (page <= last) && (page < pages); page += span)
    {
        const auto stop = std::min(page + span, pages);
        const auto mask = [&](size_t word) NOEXCEPT
        {
            return page_mask(page, stop, word * page_bound);
        };

        const auto begin = page / page_bound;
        const auto end = sub1(stop) / page_bound;
        auto any = false;
        for (auto word = begin; (word <= end) && !any; ++word)
            any = !is_zero(bit_and(released_[word].load(), mask(word)));

        if (!any)
            continue;

        if (mmap_restore(std::next(memory_map_[zero], page * page_),
            (stop - page) * page_) == fail)
        {
            set_first_code(error::mmap_failure);
            return;
        }

        for (auto word = begin; word <= end; ++word)
            released_[word].fetch_and(bit_not(mask(word)));
    }
}

// Settle up to chunk completed rows: write under the shared remap lock
// (completed extents are immutable, writers proceed), convert under a brief
// exclusive. Durability remains a snapshot property (no sync here).
TEMPLATE
bool CLASS::settle_next_(size_t chunk) NOEXCEPT
{
    size_t from{};
    size_t target{};

    {
        std::shared_lock map_lock(remap_mutex_);

        if (!loaded_.load() || fault_.load())
            return false;

        from = settled_.load();
        target = std::min(frontier_.load(),
            system::ceilinged_add(from, chunk));

        if (target <= from)
            return false;

        if (!settle_write_(from, target, sequence{}))
        {
            set_first_code(error::fsync_failure);
            return false;
        }
    }

    std::unique_lock map_lock(remap_mutex_);

    if (!loaded_.load())
        return false;

    // A raced settle boundary (flush) invalidates only this conversion.
    if (settled_.load() != from)
        return true;

    return settle_all_(target, sequence{});
}

// Evict up to chunk settled rows under memory scarcity: a wrapping sweep
// from the oldest offset (offset is write age in an append-only body, and
// re-read probability decays with it). Scarcity-driven release keeps the
// machine out of the exhausted-cache regime, where the kernel periodic
// sync walk (cost follows resident pages, not dirty) otherwise degrades
// fault service. A wrong eviction costs one cold read, so the sweep needs
// no precision. Runs under the shared remap lock (readers proceed).
TEMPLATE
bool CLASS::evict_next_(size_t chunk) NOEXCEPT
{
    std::shared_lock map_lock(remap_mutex_);

    if (!loaded_.load() || fault_.load())
        return false;

    // A truncated boundary self-heals here (the cursor clamps to a lap).
    const auto end = settled_.load();

    // Confine the lap to the recently settled tail: cache holds what was
    // just written (and what validation just read), while the body below
    // is long evicted, so a full-body lap sweeps unresident rows for most
    // of its length and the tail refills faster than the cursor returns.
    // A lap can hold no more than physical memory, as nothing else is
    // resident to evict.
    const auto span = system::greater(chunk, system_memory() / stride);
    const auto floor = (end > span) ? (end - span) : zero;
    const auto from = ((evicted_ > floor) && (evicted_ < end)) ? evicted_ :
        floor;
    const auto target = std::min(end, system::ceilinged_add(from, chunk));

    if (target <= from)
        return false;

    if (!evict_all_(from, target, sequence{}))
        return false;

    // Wrap at the lap end so the next sweep resumes at the tail floor.
    evicted_ = (target == end) ? zero : target;
    return true;
}

TEMPLATE
template <size_t... Index>
bool CLASS::settle_write_(size_t from, size_t to,
    std::index_sequence<Index...>) NOEXCEPT
{
    return (pwrite_all(opened_[Index],
        std::next(memory_map_[Index], to_width<Index>(from)),
        to_width<Index>(to) - to_width<Index>(from),
        to_width<Index>(from)) && ...);
}

#endif // MANAGE_STAGING

} // namespace database
} // namespace libbitcoin

#endif
