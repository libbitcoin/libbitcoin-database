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

#include <fcntl.h>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/mman.hpp>
#include <bitcoin/database/memory/mstage.hpp>
#include <bitcoin/database/memory/utilities.hpp>

#if defined(MANAGE_STAGING)

namespace libbitcoin {
namespace database {

// staging backend (anonymous reservation), not thread safe.
// ----------------------------------------------------------------------------
// private

// Never results in unmapped.
TEMPLATE
template <size_t Column>
bool CLASS::flush_(size_t rows) NOEXCEPT
{
    const auto success = persist_<Column>(to_width<Column>(rows))
        && sync_<Column>();

    if (!success)
        set_first_code(error::fsync_failure);

    return success;
}

// Persist rows below to: settled rows are already on disk (staged appends
// the remainder), a shared head synchronizes its mapping (writes through),
// an anonymous head transfers its dirty pages.
TEMPLATE
template <size_t Column>
bool CLASS::persist_(size_t to) NOEXCEPT
{
    const auto from = to_width<Column>(settled_.load());
    return staged_ ? ((from >= to) || pwrite_all(opened_[Column],
        std::next(memory_map_[Column], from), to - from, from)) :
        (head_shared || shared_.load()) ?
            (::msync(memory_map_[Column], to, MS_SYNC) != fail) :
            transfer_<Column>(to);
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

// Mapping failure results in unmapped.
// Mapping has no effect on logical size, always maps max(logical, min) size.
TEMPLATE
template <size_t Column>
bool CLASS::map_() NOEXCEPT
{
    return stage_<Column>();
}

// Always results in unmapped, trims to logical (can be zero).
TEMPLATE
template <size_t Column>
bool CLASS::unmap_(size_t) NOEXCEPT
{
    const auto logical = to_width<Column>(logical_.load());

    // Persist unflushed rows, trim preallocation to logical, sync to disk.
    const auto transferred = persist_<Column>(logical)
        && (::ftruncate(opened_[Column], logical) != fail)
        && sync_<Column>();

    // Order ensures release of the reservation in case of transfer failure.
    const auto success = (::munmap(memory_map_[Column],
        reserved_[Column]) != fail) && transferred;

    memory_map_[Column] = {};
    reserved_[Column] = zero;
    loaded_.store(false);

    if (!success)
        set_first_code(error::munmap_failure);

    return success;
}

// Remap failure results in unmapped.
// Remapping has no effect on logical size, sets map_/capacity_.
TEMPLATE
template <size_t Column>
bool CLASS::remap_(size_t size, bool final) NOEXCEPT
{
    BC_ASSERT(size >= logical_.load());

    // Cannot remap empty file, so expand to minimum capacity if zero.
    if (is_zero(size))
        size = minimum_;

    // The file is preallocated to capacity, preserving disk full detection at
    // allocation, and growth commits reserved anonymous pages in place, so no
    // mapping is released and the map base is stable within the reservation.
    if (!resize_<Column>(size, final))
        return false;

    return commit_<Column>(size, final);
}

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

    // Managed heads load released: the file holds the content and pages
    // restore to anonymous per segment on first write, so load contributes
    // no residency (an eager head population is the store's largest
    // memory transient, the full head set at open).
    if (managed_)
    {
        if (!lazy_install_())
        {
            teardown_<Column>(error::mmap_failure);
            return false;
        }

        loaded_.store(true);
        return true;
    }

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

    const auto size = to_commitment();
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
        settled), target - settled, headroom_) == fail))
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

// Commit failure results in unmapped when final (the default); a non-final
// refusal (in-reservation commit, replacement reservation, or replacement
// commit) returns false with the standing mapping untouched, so the caller
// may iterate a reduced request (admission is evaluated per request, so a
// refused amortization step does not imply exhaustion).
// Growth within the reservation commits pages in place (stable map base); an
// exhausted reservation is replaced and its unsettled content copied, under
// the exclusive remap lock held by the caller.
TEMPLATE
template <size_t Column>
bool CLASS::commit_(size_t size, bool final) NOEXCEPT
{
    const auto target = to_width<Column>(size);

    if (target <= reserved_[Column])
    {
        // A shared head extends its file mapping over the growth (the file is
        // provisioned to capacity, so the extension is already allocated).
        if (!staged_ && (head_shared || shared_.load()))
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
            target - from, headroom_) == fail))
        {
            if (final)
                teardown_<Column>(error::mmap_failure);

            return false;
        }

        if (wired_ && (target > from))
            mmap_wire(std::next(memory_map_[Column], from), target - from);

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
        if (final)
            teardown_<Column>(error::mmap_failure);

        return false;
    }

    using namespace system;
    const auto base = pointer_cast<uint8_t>(replace);
    const auto settled = page_floor(to_width<Column>(settled_.load()));

    // A shared head remaps its file onto the replacement (the file is the
    // content, so migration copies nothing).
    if (!staged_ && (head_shared || shared_.load()))
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

    // The replacement commit spans the unsettled prefix, transiently charged
    // over the standing reservation, so its refusal is the largest single
    // admission of the design: non-final refusal leaves the standing mapping
    // untouched for the caller's reduced retry (a reduced ask fits within
    // the standing reservation), and settle drainage shrinks the span, so a
    // necessity refusal pauses recoverable rather than tearing down.
    if (mmap_commit(std::next(base, settled), target - settled,
        headroom_) == fail)
    {
        ::munmap(replace, reserved);
        if (final)
            teardown_<Column>(error::mmap_failure);

        return false;
    }

    // Copy unsettled content (writes are excluded by the remap lock).
    const auto logical = to_width<Column>(logical_.load());

    if (settled < logical)
        std::copy_n(std::next(memory_map_[Column], settled),
            logical - settled, std::next(base, settled));

    if (wired_)
        mmap_wire(std::next(base, settled), target - settled);

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
    if (managed_)
    {
        const auto pages = ceilinged_divide(reserved, page_);
        const auto words = ceilinged_divide(pages, page_bound);
        auto grown = std::make_unique<dirty_bitmaps>(words);

        for (size_t word{}; word < std::min(words_, words); ++word)
            grown[word].store(dirty_[word].load(relaxed), relaxed);

        dirty_ = std::move(grown);
        words_ = words;

        // The replacement reservation is fully anonymous (content was
        // copied above), so released and intent page state resets.
        intent_ = std::make_unique<dirty_bitmaps>(words);
        released_ = std::make_unique<dirty_bitmaps>(words);
        sweep_ = std::make_unique<uint64_t[]>(words);
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
    return ceilinged_multiply(to_capacity(std::max(rows, minimum_)),
        reserve_factor);
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

} // namespace database
} // namespace libbitcoin

#endif // MANAGE_STAGING

#endif
