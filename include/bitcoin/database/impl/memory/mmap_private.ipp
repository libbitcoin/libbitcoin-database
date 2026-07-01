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
template <size_t Column>
size_t CLASS::space_one_(size_t rows) const NOEXCEPT
{
    return system::floored_subtract(to_capacity(to_bytes<Column>(rows)),
        std::get<Column>(capacity_));
}

TEMPLATE
template <size_t... Index>
size_t CLASS::space_all_(size_t rows, 
    std::index_sequence<Index...>) const NOEXCEPT
{
    return (space_one_<Index>(rows) + ...);
}

TEMPLATE
template <size_t... Index>
bool CLASS::map_all_(std::index_sequence<Index...>) NOEXCEPT
{
    return (map_<Index>() && ...);
}

TEMPLATE
template <size_t... Index>
bool CLASS::unmap_all_(std::index_sequence<Index...>) NOEXCEPT
{
    return (unmap_<Index>() && ...);
}

TEMPLATE
template <size_t... Index>
bool CLASS::flush_all_(std::index_sequence<Index...>) NOEXCEPT
{
    return (flush_<Index>() && ...);
}

TEMPLATE
template <size_t... Index>
bool CLASS::remap_all_(size_t logical, std::index_sequence<Index...>) NOEXCEPT 
{
    const auto space = space_all_(logical, std::index_sequence<Index...>{});
    return (remap_<Index>(to_bytes<Index>(logical), space) && ...);
}

// mman wrappers, not thread safe.
// ----------------------------------------------------------------------------
// private

// Never results in unmapped.
TEMPLATE
template <size_t Column>
bool CLASS::flush_() NOEXCEPT
{
#if defined(HAVE_MSC)
    // unmap (and therefore msync) must be called before ftruncate.
    // "To flush all the dirty pages plus the metadata for the file and ensure
    // that they are physically written to disk..."
    const auto size = to_bytes<Column>(logical_);
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

// Always results in unmapped.
// Trims to logical size, can be zero.
TEMPLATE
template <size_t Column>
bool CLASS::unmap_() NOEXCEPT
{
    const auto logical = to_bytes<Column>(logical_);

#if defined(HAVE_MSC)
    const auto success =
           (::msync(memory_map_[Column], logical, MS_SYNC) != fail)
        && (::munmap(memory_map_[Column], capacity_[Column]) != fail)
        && (::ftruncate(opened_[Column], logical) != fail)
        && (::fsync(opened_[Column]) != fail);
#else
    const auto success = 
           (::ftruncate(opened_[Column], logical) != fail)
    #if defined(F_FULLFSYNC)
        && (::fcntl(opened_[Column], F_FULLFSYNC, 0) != fail)
    #else
        && (::fsync(opened_[Column]) != fail)
    #endif
        && (::munmap(memory_map_[Column], capacity_[Column]) != fail);
#endif
    if (!success)
        set_first_code(error::munmap_failure);

    loaded_ = false;
    capacity_[Column] = zero;
    memory_map_[Column] = {};
    return success;
}

// Mapping failure results in unmapped.
// Mapping has no effect on logical size, always maps max(logical, min) size.
TEMPLATE
template <size_t Column>
bool CLASS::map_() NOEXCEPT
{
    auto size = to_bytes<Column>(logical_);
    const auto minimum = to_bytes<Column>(minimum_);

    if ((size < minimum) && !resize_<Column>((size = minimum),
        space_one_<Column>(logical_)))
        return false;

    memory_map_[Column] = system::pointer_cast<uint8_t>(
        ::mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED,
            opened_[Column], 0));

    return finalize_<Column>(size);
}

// Remap failure results in unmapped.
// Remapping has no effect on logical size, sets map_/capacity_.
TEMPLATE
template <size_t Column>
bool CLASS::remap_(size_t size, size_t space) NOEXCEPT
{
    BC_ASSERT(size >= to_bytes<Column>(logical_));

    // Cannot remap empty file, so expand to minimum capacity if zero.
    if (is_zero(size))
        size = to_bytes<Column>(minimum_);

#if !defined(HAVE_MSC) && !defined(MREMAP_MAYMOVE)
    // macOS: unmap before ftruncate sets new size.
    if (!unmap_<Column>())
        return false;

    if (!resize_<Column>(size, space))
    {
        /* bool */ map_<Column>();
        return false;
    }
#else
    if (!resize_<Column>(size, space))
        return false;
#endif

#if defined(HAVE_MSC)

    // mman-win32 mremap hack (umap/map) requires flags and file descriptor.
    memory_map_[Column] = system::pointer_cast<uint8_t>(
        ::mremap_(memory_map_[Column], capacity_[Column], size,
            PROT_READ | PROT_WRITE, MAP_SHARED, opened_[Column]));

#elif defined(MREMAP_MAYMOVE)

    memory_map_[Column] = system::pointer_cast<uint8_t>(
        ::mremap(memory_map_[Column], capacity_[Column], size,
            MREMAP_MAYMOVE));

#else

    // macOS: does not define mremap or MREMAP_MAYMOVE.
    // TODO: see "MREMAP_MAYMOVE" in sqlite for map extension technique.
    memory_map_[Column] = system::pointer_cast<uint8_t>(
        ::mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED,
            opened_[Column], 0));

#endif

    return finalize_<Column>(size);
}

// disk_full: space is set but no code is set with false return.
TEMPLATE
template <size_t Column>
bool CLASS::resize_(size_t size, size_t space) NOEXCEPT
{
    // Disk full detection, any other failure is an abort.
#if !defined (WITHOUT_FALLOCATE)
    if (::fallocate(opened_[Column], 0, capacity_[Column],
        size - capacity_[Column]) == fail)
#else
    if (::ftruncate(opened_[Column], size) == fail)
#endif
    {
        // Disk full is the only restartable store failure (leave mapped).
        if (errno == ENOSPC)
        {
            set_disk_space(space);
            return false;
        }

        set_first_code(error::ftruncate_failure);
        unmap_<Column>();
        return false;
    }

    return true;
}

// Finalize failure results in unmapped.
TEMPLATE
template <size_t Column>
bool CLASS::finalize_(size_t size) NOEXCEPT
{
    if (memory_map_[Column] == MAP_FAILED)
    {
        loaded_ = false;
        capacity_[Column] = zero;
        memory_map_[Column] = {};

        // mmap or mremap failure (not mapped).
        set_first_code(error::mmap_failure);
        return false;
    }

#if !defined (WITHOUT_MADVISE)
#if !defined(HAVE_MSC)
    // Get page size (usually 4KB).
    const int page_size = ::sysconf(_SC_PAGESIZE);
    const auto page = system::possible_narrow_sign_cast<size_t>(page_size);

    // If not one bit then page size is not a power of two as required.
    if (page_size == fail || !is_one(system::ones_count(page)))
    {
        set_first_code(error::sysconf_failure);
        unmap_<Column>();
        return false;
    }
    
    // Align size up to page boundary.
    using namespace system;
    const auto max = sub1(page);
    const auto align = bit_and(ceilinged_add(size, max), bit_not(max));

    // Use 1GB chunks to avoid large-length issues.
    constexpr auto chunk = power2(30u);
    const auto advice = (random_ ? MADV_RANDOM : MADV_SEQUENTIAL) |
        MADV_WILLNEED;

    for (auto offset = zero; offset < align; offset += chunk)
    {
        BC_PUSH_WARNING(NO_POINTER_ARITHMETIC)
        const auto start = memory_map_[Column] + offset;
        BC_POP_WARNING()

        if (::madvise(start, std::min(chunk, align - offset), advice) == fail)
        {
            set_first_code(error::madvise_failure);
            unmap_<Column>();
            return false;
        }
    }
#endif // !HAVE_MSC
#endif // !WITHOUT_MADVISE

    loaded_ = true;
    capacity_[Column] = size;
    return true;
}

} // namespace database
} // namespace libbitcoin

#endif
