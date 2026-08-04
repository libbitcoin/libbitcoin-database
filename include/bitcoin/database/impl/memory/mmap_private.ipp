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
#ifndef LIBBITCOIN_DATABASE_MEMORY_MMAP_PRIVATE_IPP
#define LIBBITCOIN_DATABASE_MEMORY_MMAP_PRIVATE_IPP

#include <algorithm>
#include <fcntl.h>
#include <tuple>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/mman.hpp>
#include <bitcoin/database/memory/mstage.hpp>
#include <bitcoin/database/memory/utilities.hpp>

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
#if defined(MANAGE_STAGING)
    using namespace system;
    const auto page = page_size();

    // Page size must be a power of two.
    if (is_zero(page) || !is_one(ones_count(page)))
    {
        set_first_code(error::sysconf_failure);
        capacity_.store(zero);
        return false;
    }

    page_ = page;
    window_.store(zero);
    settled_.store(staged_ ? logical_.load() : zero);
    frontier_.store(staged_ ? logical_.load() : zero);
    marks_.store(zero);
#endif

    if (!(map_<Index>() && ...))
    {
        capacity_.store(zero);
        file_.store(zero);
        return false;
    }

    // The file is provisioned in full; capacity publishes committed rows only.
    file_.store(to_provision());
    capacity_.store(to_commitment());
    check_invariants_();
    return true;
}

TEMPLATE
template <size_t... Index>
bool CLASS::unmap_all_(std::index_sequence<Index...>) NOEXCEPT
{
    const auto capacity = capacity_.load();
    const auto success = (unmap_<Index>(capacity) && ...);
    capacity_.store(zero);

    // Unmapping truncates the file to logical, which is then its provisioning.
    file_.store(logical_.load());

#if defined(MANAGE_STAGING)
    window_.store(zero);
    settled_.store(zero);
    frontier_.store(zero);
    marks_.store(zero);
    dirty_.reset();
    intent_.reset();
    released_.reset();
    sweep_.reset();
    words_ = zero;
    engaged_.store(false);
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

    // Growth beyond the provisioned file extends it (resize_ is a no-op
    // within), so the extent tracks the high water of provisioning.
    file_.store(std::max(file_.load(), capacity));
    capacity_.store(capacity);
    check_invariants_();
    return true;
}

// mman wrappers, not thread safe.
// ----------------------------------------------------------------------------
// private

// Never results in unmapped.
TEMPLATE
template <size_t Column>
bool CLASS::flush_(size_t
    #if defined(MANAGE_STAGING) || defined(HAVE_MSC)
    rows
    #endif
) NOEXCEPT
{
#if defined(MANAGE_STAGING)
    // Transfer unflushed rows from anonymous memory to the file. Settled rows
    // are already on disk (staged); unstaged transfers dirty pages only, or
    // synchronizes its mapping (shared head, which writes through).
    const auto from = to_width<Column>(settled_.load());
    const auto to = to_width<Column>(rows);

    const auto success =
           (staged_ ? ((from >= to) || pwrite_all(opened_[Column],
               std::next(memory_map_[Column], from), to - from, from)) :
            head_shared ? (::msync(memory_map_[Column], to, MS_SYNC) != fail) :
               transfer_<Column>(to))
        && sync_<Column>();
#elif defined(HAVE_MSC)
    // unmap (and therefore msync) must be called before ftruncate.
    // "To flush all the dirty pages plus the metadata for the file and ensure
    // that they are physically written to disk..."
    const auto size = to_width<Column>(rows);
    const auto success =
           (::msync(memory_map_[Column], size, MS_SYNC) != fail)
        && (::fsync(opened_[Column]) != fail);
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
    #if !defined(MANAGE_STAGING)
    size
    #endif
) NOEXCEPT
{
    const auto logical = to_width<Column>(logical_.load());

#if defined(MANAGE_STAGING)
    // Persist unflushed rows, trim preallocation to logical, sync to disk.
    const auto from = to_width<Column>(settled_.load());
    const auto transferred =
           (staged_ ? ((from >= logical) || pwrite_all(opened_[Column],
               std::next(memory_map_[Column], from), logical - from, from)) :
               transfer_<Column>(logical))
        && (::ftruncate(opened_[Column], logical) != fail)
        && sync_<Column>();

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
        && (::fsync(opened_[Column]) != fail);

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
#if defined(MANAGE_STAGING)
    return stage_<Column>();
#else
    // Cannot map empty file, and want minimum capacity, so expand as required.
    // The classic mapping is file-backed, so commitment is provisioning.
    // disk_full: space is set but no code is set with false return.
    const auto size = to_provision();
    if (!resize_<Column>(size))
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

#if defined(MANAGE_STAGING)
    // The file is preallocated to capacity, preserving disk full detection at
    // allocation, and growth commits reserved anonymous pages in place, so no
    // mapping is released and the map base is stable within the reservation.
    if (!resize_<Column>(size))
        return false;

    return commit_<Column>(size);
#else
    if (!resize_<Column>(size))
        return false;

#if defined(HAVE_MSC)
    // mman-win32 mremap hack (umap/map) requires flags and file descriptor.
    memory_map_[Column] = system::pointer_cast<uint8_t>(
        ::mremap_(memory_map_[Column], to_width<Column>(capacity_.load()),
            to_width<Column>(size), PROT_READ | PROT_WRITE, MAP_SHARED,
            opened_[Column]));
#else
    memory_map_[Column] = system::pointer_cast<uint8_t>(
        ::mremap(memory_map_[Column], to_width<Column>(capacity_.load()),
            to_width<Column>(size), MREMAP_MAYMOVE));
#endif

    return finalize_<Column>(size);
#endif // MANAGE_STAGING
}

// disk_full: space is set but no code is set with false return.
TEMPLATE
template <size_t Column>
bool CLASS::resize_(size_t size) NOEXCEPT
{
    // The file is provisioned ahead of commitment, so growth within the
    // provisioned extent requires no disk operation (the space is reserved).
    const auto extent = file_.load();
    if (size <= extent)
        return true;

    const auto target = to_width<Column>(size);
    const auto capacity = to_width<Column>(extent);

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
            using namespace system;
            set_disk_space(ceilinged_multiply(floored_subtract(size, extent),
                stride));
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
    using namespace system;
    const int page_size = ::sysconf(_SC_PAGESIZE);
    const auto page = possible_narrow_sign_cast<size_t>(page_size);

    // If not one bit then page size is not a power of two as required.
    if (page_size == fail || !is_one(ones_count(page)))
    {
        set_first_code(error::sysconf_failure);
        unmap_<Column>(size);
        return false;
    }

    // Align mapped bytes up to page boundary.
    const auto max = sub1(page);
    const auto target = to_width<Column>(size);
    const auto align = bit_and(ceilinged_add(target, max), bit_not(max));

    // Advice is elective (normal is the kernel default) and configured from
    // the read pattern (see database::advice), as advising from the write
    // pattern (structural) invites fault read amplification on random reads.
    // Random access preloads (small heads, avoiding initial fault stalls).
    if (access_ != advice::normal)
    {
        const auto random = (access_ != advice::sequential);
        const auto preload = (access_ == advice::random);
        const auto behavior = random ? MADV_RANDOM : MADV_SEQUENTIAL;

        for (size_t offset{}; offset < align; offset += advise_chunk)
        {
            const auto length = std::min(advise_chunk, align - offset);
            const auto start = std::next(memory_map_[Column], offset);

            if (::madvise(start, length, behavior) == fail || (preload &&
                ::madvise(start, length, MADV_WILLNEED) == fail))
            {
                set_first_code(error::madvise_failure);
                unmap_<Column>(size);
                return false;
            }
        }
    }
#endif // !HAVE_MSC && !WITHOUT_MADVISE

    loaded_.store(true);
    return true;
}


} // namespace database
} // namespace libbitcoin

#endif
