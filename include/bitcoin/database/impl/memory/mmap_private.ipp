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
#ifndef LIBBITCOIN_DATABASE_MEMORY_MMAP_PRIVATE_IPP
#define LIBBITCOIN_DATABASE_MEMORY_MMAP_PRIVATE_IPP

#include <algorithm>
#include <fcntl.h>
#include <tuple>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/mman.hpp>

namespace libbitcoin {
namespace database {

// mman dispatch, not thread safe.
// ----------------------------------------------------------------------------
// private

TEMPLATE
template <size_t... Index>
bool CLASS::flush_all_(size_t rows, std::index_sequence<Index...>) NOEXCEPT
{
    return (flush_<Index>(rows) && ...);
}

TEMPLATE
template <size_t... Index>
bool CLASS::map_all_(std::index_sequence<Index...>) NOEXCEPT
{
#if defined(HAVE_STAGING)
    // Get page size (usually 4KB), one bit ensures a power of two as required.
    const int page_size = ::sysconf(_SC_PAGESIZE);
    const auto page = system::possible_narrow_sign_cast<size_t>(page_size);

    if (page_size == fail || !is_one(system::ones_count(page)))
    {
        set_first_code(error::sysconf_failure);
        capacity_.store(zero);
        return false;
    }

    page_ = page;

    // File content is fully flushed by definition at load.
    settled_.store(staged_ ? logical_.load() : zero);
#endif

    if (!(map_<Index>() && ...))
    {
        capacity_.store(zero);
        return false;
    }

    capacity_.store(std::max(logical_.load(), minimum_));
    return true;
}

TEMPLATE
template <size_t... Index>
bool CLASS::unmap_all_(std::index_sequence<Index...>) NOEXCEPT
{
    const auto capacity = capacity_.load();
    const auto success = (unmap_<Index>(capacity) && ...);
    capacity_.store(zero);

#if defined(HAVE_STAGING)
    settled_.store(zero);
#endif

    return success;
}

TEMPLATE
template <size_t... Index>
bool CLASS::remap_all_(size_t capacity, std::index_sequence<Index...>) NOEXCEPT
{
    if (!(remap_<Index>(capacity) && ...))
    {
        capacity_.store(zero);
        return false;
    }

    capacity_.store(capacity);
    return true;
}

// mman wrappers, not thread safe.
// ----------------------------------------------------------------------------
// private

// Never results in unmapped.
TEMPLATE
template <size_t Column>
bool CLASS::flush_(size_t
    #if defined(HAVE_STAGING) || defined(HAVE_MSC)
    rows
    #endif
) NOEXCEPT
{
#if defined(HAVE_STAGING)
    // Transfer unflushed rows from anonymous memory to the file. Settled rows
    // are already on disk (staged); unstaged content is written in full.
    const auto from = to_width<Column>(staged_ ? settled_.load() : zero);
    const auto to = to_width<Column>(rows);

    const auto success =
           ((from >= to) || pwrite_all(opened_[Column],
               std::next(memory_map_[Column], from), to - from, from))
#if defined(F_FULLFSYNC)
        // non-standard macOS behavior: news.ycombinator.com/item?id=30372218
        && (::fcntl(opened_[Column], F_FULLFSYNC, 0) != fail);
#else
        && (::fsync(opened_[Column]) != fail);
#endif
#elif defined(HAVE_MSC)
    // unmap (and therefore msync) must be called before ftruncate.
    // "To flush all the dirty pages plus the metadata for the file and ensure
    // that they are physically written to disk..."
    const auto size = to_width<Column>(rows);
    const auto success =
           (::msync(memory_map_[Column], size, MS_SYNC) != fail)
        && (::fsync(opened_[Column]) != fail);
#elif defined(F_FULLFSYNC)
    // macOS msync fails with zero logical size (but we are no longer calling).
    // non-standard macOS behavior: news.ycombinator.com/item?id=30372218
    const auto success = ::fcntl(opened_[Column], F_FULLFSYNC, 0) != fail;
#else
    // msync should not be required on modern linux, see linus et al.
    // stackoverflow.com/questions/5902629/mmap-msync-and-linux-process-termination
    // Linux: fsync "transfers ("flushes") all modified in-core data of
    // (i.e., modified buffer cache pages for) the file referred to by the
    // file descriptor fd to the disk device so all changed information
    // can be retrieved even if the system crashes or is rebooted. This
    // includes writing through or flushing a disk cache if present. The
    // call blocks until the device reports that transfer has completed."
    const auto success = ::fsync(opened_[Column]) != fail;
#endif

    if (!success)
        set_first_code(error::fsync_failure);

    return success;
}

// Always results in unmapped, file is unchanged.
TEMPLATE
template <size_t Column>
bool CLASS::release_(size_t size) NOEXCEPT
{
    const auto success =
        ::munmap(memory_map_[Column], to_width<Column>(size)) != fail;

    if (!success)
        set_first_code(error::munmap_failure);

    // loaded_ is caller-owned: unmap_ publishes unloaded, remap_ remains
    // loaded across replacement (lock-free allocate guards must not observe
    // a transient unload).
    memory_map_[Column] = {};
    return success;
}

// Always results in unmapped, trims to logical (can be zero).
TEMPLATE
template <size_t Column>
bool CLASS::unmap_(size_t
    #if !defined(HAVE_STAGING)
    size
    #endif
) NOEXCEPT
{
    const auto logical = to_width<Column>(logical_.load());

#if defined(HAVE_STAGING)
    // Persist unflushed rows, trim preallocation to logical, sync to disk.
    const auto from = to_width<Column>(staged_ ? settled_.load() : zero);
    const auto transferred =
           ((from >= logical) || pwrite_all(opened_[Column],
               std::next(memory_map_[Column], from), logical - from, from))
        && (::ftruncate(opened_[Column], logical) != fail)
#if defined(F_FULLFSYNC)
        && (::fcntl(opened_[Column], F_FULLFSYNC, 0) != fail);
#else
        && (::fsync(opened_[Column]) != fail);
#endif

    // Order ensures release of the reservation in case of transfer failure.
    const auto success = (::munmap(memory_map_[Column],
        reserved_[Column]) != fail) && transferred;

    memory_map_[Column] = {};
    reserved_[Column] = zero;
#elif defined(HAVE_MSC)
    // Windows cannot resize a mapped file.
    // msync requires the live mapping, ftruncate requires it gone.
    const auto synced =
           (::msync(memory_map_[Column], logical, MS_SYNC) != fail);

    // Order ensures release in case of sync failure.
    const auto success = release_<Column>(size) && synced
        && (::ftruncate(opened_[Column], logical) != fail)
        && (::fsync(opened_[Column]) != fail);
#else
    // POSIX permits resizing a mapped file.
    const auto truncated =
           (::ftruncate(opened_[Column], logical) != fail)
#if defined(F_FULLFSYNC)
        && (::fcntl(opened_[Column], F_FULLFSYNC, 0) != fail);
#else
        && (::fsync(opened_[Column]) != fail);
#endif

    // Order ensures release in case of truncate failure.
    const auto success = release_<Column>(size) && truncated;
#endif

    loaded_.store(false);

    if (!success)
        set_first_code(error::munmap_failure);

    return success;
}

// Mapping failure results in unmapped.
// Mapping has no effect on logical size, always maps max(logical, min) size.
TEMPLATE
template <size_t Column>
bool CLASS::map_() NOEXCEPT
{
#if defined(HAVE_STAGING)
    return stage_<Column>();
#else
    auto size = logical_.load();

    // Cannot map empty file, and want minimum capacity, so expand as required.
    // disk_full: space is set but no code is set with false return.
    if ((size < minimum_) && !resize_<Column>(size = minimum_))
        return false;

    memory_map_[Column] = system::pointer_cast<uint8_t>(
        ::mmap(nullptr, to_width<Column>(size), PROT_READ | PROT_WRITE,
            MAP_SHARED, opened_[Column], 0));

    return finalize_<Column>(size);
#endif
}

// Remap failure results in unmapped.
// Remapping has no effect on logical size, sets map_/capacity_.
TEMPLATE
template <size_t Column>
bool CLASS::remap_(size_t size) NOEXCEPT
{
    BC_ASSERT(size >= logical_.load());

    // Cannot remap empty file, so expand to minimum capacity if zero.
    if (is_zero(size))
        size = minimum_;

#if defined(HAVE_STAGING)
    // The file is preallocated to capacity, preserving disk full detection at
    // allocation, and growth commits reserved anonymous pages in place, so no
    // mapping is released and the map base is stable within the reservation.
    if (!resize_<Column>(size))
        return false;

    return commit_<Column>(size);
#else
#if !defined(HAVE_MSC) && !defined(MREMAP_MAYMOVE)
    // macOS cannot remap in place, so release the mapping without trimming and
    // the file remains at capacity_ bytes and resize_'s fallocate delta (and
    // the fallocate shim's preallocation window) are exact by construction.
    if (!release_<Column>(capacity_.load()))
        return false;

    if (!resize_<Column>(size))
    {
        map_<Column>();
        return false;
    }
#else
    if (!resize_<Column>(size))
        return false;
#endif

#if defined(HAVE_MSC)

    // mman-win32 mremap hack (umap/map) requires flags and file descriptor.
    memory_map_[Column] = system::pointer_cast<uint8_t>(
        ::mremap_(memory_map_[Column], to_width<Column>(capacity_.load()),
            to_width<Column>(size), PROT_READ | PROT_WRITE, MAP_SHARED,
            opened_[Column]));

#elif defined(MREMAP_MAYMOVE)

    memory_map_[Column] = system::pointer_cast<uint8_t>(
        ::mremap(memory_map_[Column], to_width<Column>(capacity_.load()),
            to_width<Column>(size), MREMAP_MAYMOVE));

#else

    // macOS does not define mremap or MREMAP_MAYMOVE. The prior mapping was
    // released above and the resized file is mapped fresh.
    // TODO: see "MREMAP_MAYMOVE" in sqlite for map extension technique.
    memory_map_[Column] = system::pointer_cast<uint8_t>(
        ::mmap(nullptr, to_width<Column>(size), PROT_READ | PROT_WRITE,
            MAP_SHARED, opened_[Column], 0));

#endif

    return finalize_<Column>(size);
#endif // HAVE_STAGING
}

// disk_full: space is set but no code is set with false return.
TEMPLATE
template <size_t Column>
bool CLASS::resize_(size_t size) NOEXCEPT
{
    const auto target = to_width<Column>(size);
    const auto capacity = to_width<Column>(capacity_.load());

    // Disk full detection, any other failure is an abort.
#if !defined(WITHOUT_FALLOCATE)
    if (::fallocate(opened_[Column], 0, capacity, target - capacity) == fail)
#else
    if (::ftruncate(opened_[Column], target) == fail)
#endif
    {
        // Disk full is the only restartable store failure (leave mapped).
        if (errno == ENOSPC)
        {
            set_disk_space(system::ceilinged_multiply(system::floored_subtract(
                size, capacity_.load()), stride));
            return false;
        }

        set_first_code(error::ftruncate_failure);
        unmap_<Column>(capacity_.load());
        return false;
    }

    return true;
}

// Finalize failure results in unmapped.
TEMPLATE
template <size_t Column>
bool CLASS::finalize_(size_t
    #if !defined(HAVE_MSC) && !defined(WITHOUT_MADVISE)
    size
    #endif
) NOEXCEPT
{
    if (memory_map_[Column] == MAP_FAILED)
    {
        loaded_.store(false);
        memory_map_[Column] = {};

        // mmap or mremap failure (not mapped).
        set_first_code(error::mmap_failure);
        return false;
    }

#if !defined(HAVE_MSC) && !defined(WITHOUT_MADVISE)
    // Get page size (usually 4KB).
    const int page_size = ::sysconf(_SC_PAGESIZE);
    const auto page = system::possible_narrow_sign_cast<size_t>(page_size);

    // If not one bit then page size is not a power of two as required.
    if (page_size == fail || !is_one(system::ones_count(page)))
    {
        set_first_code(error::sysconf_failure);
        unmap_<Column>(size);
        return false;
    }

    // Align mapped bytes up to page boundary.
    using namespace system;
    const auto max = sub1(page);
    const auto target = to_width<Column>(size);
    const auto align = bit_and(ceilinged_add(target, max), bit_not(max));

    // Use 1GB chunks to avoid large-length issues.
    constexpr auto chunk = power2(30u);
    const auto advice = random_ ? MADV_RANDOM : MADV_SEQUENTIAL;

    for (auto offset = zero; offset < align; offset += chunk)
    {
        const auto length = std::min(chunk, align - offset);
        const auto start = std::next(memory_map_[Column], offset);

        if (::madvise(start, length, advice) == fail || (random_ &&
            ::madvise(start, length, MADV_WILLNEED) == fail))
        {
            set_first_code(error::madvise_failure);
            unmap_<Column>(size);
            return false;
        }
    }
#endif // !HAVE_MSC && !WITHOUT_MADVISE

    loaded_.store(true);
    return true;
}

#if defined(HAVE_STAGING)

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

#endif // HAVE_STAGING

} // namespace database
} // namespace libbitcoin

#endif
