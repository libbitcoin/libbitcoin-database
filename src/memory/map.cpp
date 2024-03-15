/**
 * Copyright (c) 2011-2023 libbitcoin developers (see AUTHORS)
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
#include <bitcoin/database/memory/map.hpp>

#if defined(HAVE_MSC)
    #include "mman-win32/mman.h"
#else
    #include <sys/mman.h>
    #include <sys/stat.h>
    #include <sys/types.h>
#endif
#include <algorithm>
#include <chrono>
#include <fcntl.h>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/error.hpp>
#include <bitcoin/database/file/file.hpp>

namespace libbitcoin {
namespace database {

BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

using namespace system;

map::map(const path& filename, size_t minimum, size_t expansion) NOEXCEPT
  : filename_(filename),
    minimum_(minimum),
    expansion_(expansion),
    memory_map_(nullptr),
    opened_(file::invalid),
    loaded_(false),
    capacity_(zero),
    logical_(zero)
{
}

map::~map() NOEXCEPT
{
    BC_ASSERT_MSG(!loaded_, "file mapped at destruct");
    BC_ASSERT_MSG(is_null(memory_map_), "map defined at destruct");
    BC_ASSERT_MSG(is_zero(logical_), "logical nonzero at destruct");
    BC_ASSERT_MSG(is_zero(capacity_), "capacity nonzero at destruct");
    BC_ASSERT_MSG(opened_ == file::invalid, "file open at destruct");
}

const std::filesystem::path& map::file() const NOEXCEPT
{
    return filename_;
}

code map::open() NOEXCEPT
{
    std::unique_lock field_lock(field_mutex_);

    if (opened_ != file::invalid)
        return error::open_open;

    opened_ = file::open(filename_);
    if (opened_ == file::invalid)
        return error::open_failure;

    return file::size(logical_, opened_) ? error::success :
        error::size_failure;
}

code map::close() NOEXCEPT
{
    std::unique_lock field_lock(field_mutex_);

    if (loaded_)
        return error::close_loaded;

    if (opened_ == file::invalid)
        return error::success;

    const auto descriptor = opened_;
    opened_ = file::invalid;
    logical_ = zero;

    return file::close(descriptor) ? error::success : error::close_failure;
}

bool map::is_open() const NOEXCEPT
{
    std::shared_lock field_lock(field_mutex_);
    return opened_ != file::invalid;
}

// map, flush, unmap.
// ----------------------------------------------------------------------------
// Each accessor holds a shared lock on remap_mutex_ (read/write access to map).
// Exclusive lock on remap_mutex_ ensures there are open accessor objects,
// which allows for safely remapping the memory map.

code map::load() NOEXCEPT
{
    std::unique_lock field_lock(field_mutex_);

    if (remap_mutex_.try_lock())
    {
        if (loaded_)
        {
            remap_mutex_.unlock();
            return error::load_loaded;
        }

        // Updates fields.
        if (!map_())
        {
            remap_mutex_.unlock();
            return error::load_failure;
        }

        loaded_ = true;
        remap_mutex_.unlock();
        return error::success;
    }

    return error::load_locked;
}

// Suspend writes before calling.
code map::flush() const NOEXCEPT
{
    // Prevent unload, resize, remap.
    std::shared_lock map_lock(remap_mutex_);
    std::shared_lock field_lock(field_mutex_);

    if (!loaded_)
        return error::flush_unloaded;

    // Reads fields and the memory map.
    return flush_() ? error::success : error::flush_failure;
}

code map::unload() NOEXCEPT
{
    std::unique_lock field_lock(field_mutex_);

    if (remap_mutex_.try_lock())
    {
        if (!loaded_)
        {
            remap_mutex_.unlock();
            return error::success;
        }

        BC_ASSERT_MSG(logical_ <= capacity_, "logical size exceeds capacity");

        // Updates fields.
        if (!unmap_())
        {
            remap_mutex_.unlock();
            return error::unload_failure;
        }

        loaded_ = false;
        remap_mutex_.unlock();
        return error::success;
    }

    return error::unload_locked;
}

bool map::is_loaded() const NOEXCEPT
{
    std::shared_lock field_lock(field_mutex_);
    return loaded_;
}

// Interface.
// ----------------------------------------------------------------------------

size_t map::size() const NOEXCEPT
{
    std::shared_lock field_lock(field_mutex_);
    return logical_;
}

size_t map::capacity() const NOEXCEPT
{
    std::shared_lock field_lock(field_mutex_);
    return capacity_;
}

bool map::truncate(size_t size) NOEXCEPT
{
    std::unique_lock field_lock(field_mutex_);

    if (size > logical_)
        return false;

    logical_ = size;
    return true;
}

// Waits until all access pointers are destructed. Will deadlock if any access
// pointer is waiting on allocation. Lock safety requires that access pointers
// are short-lived and do not block on allocation.
// TODO: Could loop over a try lock here and log deadlock warning.
size_t map::allocate(size_t chunk) NOEXCEPT
{
    std::unique_lock field_lock(field_mutex_);

    if (!loaded_ || is_add_overflow(logical_, chunk))
        return storage::eof;

    auto end = logical_ + chunk;
    if (end > capacity_)
    {
        const auto size = to_capacity(end);

        std::unique_lock remap_lock(remap_mutex_);

        if (!remap_(size))
            return storage::eof;
    }

    std::swap(logical_, end);
    return end;
}

memory_ptr map::get(size_t offset) const NOEXCEPT
{
    // Obtaining logical before access prevents mutual mutex wait (deadlock).
    // The store could remap between here and next line, but logical size
    // only increases. Close zeroizes logical but must be unloaded to do so.
    // Truncate can reduce logical, but capacity is not affected. It is always
    // the case that ptr may write past current logical, so long as it never
    // writes past current capacity. Truncation is managed by callers.
    const auto logical = size();

    // Takes a shared lock on remap_mutex_ until destruct, blocking remap.
    const auto ptr = std::make_shared<access>(remap_mutex_);

    // loaded_ update is precluded by remap_mutex_, making this read atomic.
    if (!loaded_ || is_null(ptr))
        return nullptr;

    // With offset > size the assignment is negative (stream is exhausted).
    BC_PUSH_WARNING(NO_POINTER_ARITHMETIC)
    ptr->assign(memory_map_ + offset, memory_map_ + logical);
    BC_POP_WARNING()
    return ptr;
}

// private, mman wrappers, not thread safe
// ----------------------------------------------------------------------------

constexpr auto fail = -1;

// Never results in unmapped.
bool map::flush_() const NOEXCEPT
{
    // msync should not be required on modern linux, see linus et al.
    // stackoverflow.com/questions/5902629/mmap-msync-and-linux-process-termination
#if defined(HAVE_MSC)
    // unmap (and therefore msync) must be called before ftruncate.
    // "To flush all the dirty pages plus the metadata for the file and ensure
    // that they are physically written to disk..."
    return (::msync(memory_map_, logical_, MS_SYNC) != fail) 
        && (::fsync(opened_) != fail);
#elif defined(F_FULLFSYNC)
    // macOS msync fails with zero logical size (but we are no longer calling).
    // non-standard macOS behavior: news.ycombinator.com/item?id=30372218
    return ::fcntl(opened_, F_FULLFSYNC, 0) != fail;
#else
    // Linux: fsync "transfers ("flushes") all modified in-core data of
    // (i.e., modified buffer cache pages for) the file referred to by the
    // file descriptor fd to the disk device so all changed information
    // can be retrieved even if the system crashes or is rebooted. This
    // includes writing through or flushing a disk cache if present. The
    // call blocks until the device reports that transfer has completed."
    return ::fsync(opened_) != fail;
#endif
}

// Always results in unmapped.
// Trims to logical size, can be zero.
bool map::unmap_() NOEXCEPT
{
#if defined(HAVE_MSC)
    const auto success =
           (::msync(memory_map_, logical_, MS_SYNC) != fail)
        && (::munmap(memory_map_, capacity_) != fail)
        && (::ftruncate(opened_, logical_) != fail)
        && (::fsync(opened_) != fail);
#else
    const auto success = (::ftruncate(opened_, logical_) != fail)
    #if defined(F_FULLFSYNC)
        && (::fcntl(opened_, F_FULLFSYNC, 0) != fail)
    #else
        && (::fsync(opened_) != fail)
    #endif
        && (::munmap(memory_map_, capacity_) != fail);
#endif

    capacity_ = zero;
    memory_map_ = nullptr;
    return success;
}

// Mapping failure results in unmapped.
// Mapping has no effect on logical size, always maps max(logical, min) size.
bool map::map_() NOEXCEPT
{
    auto size = logical_;

    // Cannot map empty file, and want mininum capacity, so expand as required.
    if (size < minimum_)
    {
        size = minimum_;
        if (::ftruncate(opened_, size) == fail)
        {
            unmap_();
            return false;
        }
    }

    memory_map_ = pointer_cast<uint8_t>(::mmap(nullptr, size,
        PROT_READ | PROT_WRITE, MAP_SHARED, opened_, 0));

    return finalize_(size);
}

// Remap failure results in unmapped.
// Remapping has no effect on logical size, sets map_/capacity_.
bool map::remap_(size_t size) NOEXCEPT
{
    // Cannot remap empty file, so expand to minimum capacity if zero.
    if (is_zero(size))
        size = minimum_;

#if !defined(HAVE_MSC) && !defined(MREMAP_MAYMOVE)
    // macOS: unmap before ftruncate sets new size.
    if (!unmap_())
        return false;
#endif

    if (::ftruncate(opened_, size) == fail)
    {
        unmap_();
        return false;
    }

#if defined(HAVE_MSC)
    // mman-win32 mremap hack (umap/map) requires flags and file descriptor.
    memory_map_ = pointer_cast<uint8_t>(::mremap_(memory_map_, capacity_, size,
        PROT_READ | PROT_WRITE, MAP_SHARED, opened_));
#elif defined(MREMAP_MAYMOVE)
    memory_map_ = pointer_cast<uint8_t>(::mremap(memory_map_, capacity_, size,
        MREMAP_MAYMOVE));
#else
    // macOS: does not define mremap or MREMAP_MAYMOVE.
    // TODO: see "MREMAP_MAYMOVE" in sqlite for map extension technique.
    memory_map_ = pointer_cast<uint8_t>(::mmap(nullptr, size,
        PROT_READ | PROT_WRITE, MAP_SHARED, opened_, 0));
#endif

    return finalize_(size);
}

// Finalize failure results in unmapped.
bool map::finalize_(size_t size) NOEXCEPT
{
    if (memory_map_ == MAP_FAILED)
    {
        capacity_ = zero;
        memory_map_ = nullptr;
        return false;
    }

    // TODO: madvise with large length value fails on linux, does 0 imply all?
    if (::madvise(memory_map_, 0, MADV_RANDOM) == fail)
    {
        unmap_();
        return false;
    }

    capacity_ = size;
    return true;
}

BC_POP_WARNING()

} // namespace database
} // namespace libbitcoin
