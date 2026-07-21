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
bool CLASS::flush_all_(std::index_sequence<Index...>) NOEXCEPT
{
    return (flush_<Index>() && ...);
}

TEMPLATE
template <size_t... Index>
bool CLASS::map_all_(std::index_sequence<Index...>) NOEXCEPT
{
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
bool CLASS::flush_() NOEXCEPT
{
#if defined(HAVE_MSC)
    // unmap (and therefore msync) must be called before ftruncate.
    // "To flush all the dirty pages plus the metadata for the file and ensure
    // that they are physically written to disk..."
    const auto size = to_width<Column>(logical_.load());
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

    loaded_.store(false);
    memory_map_[Column] = {};
    return success;
}

// Always results in unmapped, trims to logical (can be zero).
TEMPLATE
template <size_t Column>
bool CLASS::unmap_(size_t size) NOEXCEPT
{
    const auto logical = to_width<Column>(logical_.load());

#if defined(HAVE_MSC)
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
    auto size = logical_.load();

    // Cannot map empty file, and want minimum capacity, so expand as required.
    // disk_full: space is set but no code is set with false return.
    if ((size < minimum_) && !resize_<Column>(size = minimum_))
        return false;

    memory_map_[Column] = system::pointer_cast<uint8_t>(
        ::mmap(nullptr, to_width<Column>(size), PROT_READ | PROT_WRITE,
            MAP_SHARED, opened_[Column], 0));

    return finalize_<Column>(size);
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

    // macOS does not define mremap or MREMAP_MAYMOVE.
    // TODO: see "MREMAP_MAYMOVE" in sqlite for map extension technique.
    memory_map_[Column] = system::pointer_cast<uint8_t>(
        ::mmap(nullptr, to_width<Column>(size), PROT_READ | PROT_WRITE,
            MAP_SHARED, opened_[Column], 0));

#endif

    return finalize_<Column>(size);
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

} // namespace database
} // namespace libbitcoin

#endif
