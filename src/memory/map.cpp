/**
 * Copyright (c) 2011-2022 libbitcoin developers (see AUTHORS)
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
#include <fcntl.h>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <bitcoin/system.hpp>
#include <bitcoin/database/boost.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/error.hpp>
#include <bitcoin/database/file/file.hpp>
#include <bitcoin/database/memory/accessor.hpp>

namespace libbitcoin {
namespace database {

BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

using namespace system;

// The expansion parameter is NOT IMPLEMENTED.
map::map(const path& filename, size_t minimum, size_t expansion) NOEXCEPT
  : filename_(filename),
    minimum_(minimum),
    expansion_(expansion),
    memory_map_(nullptr),
    loaded_(false),
    logical_(zero),
    capacity_(zero),
    descriptor_(file::invalid)
{
}

map::~map() NOEXCEPT
{
    // LOG STOP WARNINGS (handled if unload() and close() were called).
    BC_ASSERT_MSG(!loaded_, "file mapped at destruct");
    BC_ASSERT_MSG(is_null(memory_map_), "map defined at destruct");
    BC_ASSERT_MSG(is_zero(logical_), "logical nonzero at destruct");
    BC_ASSERT_MSG(is_zero(capacity_), "capacity nonzero at destruct");
    BC_ASSERT_MSG(descriptor_ == file::invalid, "file open at destruct");
}

const std::filesystem::path& map::file() const NOEXCEPT
{
    return filename_;
}

code map::open() NOEXCEPT
{
    std::unique_lock field_lock(field_mutex_);

    if (descriptor_ != file::invalid)
        return error::open_open;

    descriptor_ = file::open(filename_);
    if (descriptor_ == file::invalid)
        return error::open_failure;

    return file::size(logical_, descriptor_) ? error::success :
        error::size_failure;
}

code map::close() NOEXCEPT
{
    std::unique_lock field_lock(field_mutex_);

    if (loaded_)
        return error::close_loaded;

    if (descriptor_ == file::invalid)
        return error::success;

    const auto descriptor = descriptor_;
    descriptor_ = file::invalid;
    logical_ = zero;

    return file::close(descriptor) ? error::success : error::close_failure;
}

bool map::is_open() const NOEXCEPT
{
    std::shared_lock field_lock(field_mutex_);
    return descriptor_ != file::invalid;
}

// Map, flush, unmap.
// ----------------------------------------------------------------------------

// Each accessor holds a shared lock on map_mutex_ (read/write access to map).
// Exclusive lock on map_mutex_ ensures there are no open accessor objects.
// map_ field requires exclusive lock on map_mutex_ for read(flush)/write.
// Fields except map_ require exclusive lock on field_mutex_ for write.
// Fields except map_ require at least shared lock on field_mutex_ for read.

code map::load() NOEXCEPT
{
    std::unique_lock field_lock(field_mutex_);

    if (map_mutex_.try_lock())
    {
        if (loaded_)
        {
            map_mutex_.unlock();
            return error::load_loaded;
        }

        if (!map_())
        {
            map_mutex_.unlock();
            return error::load_failure;
        }

        loaded_ = true;
        map_mutex_.unlock();
        return error::success;
    }

    return error::load_locked;
}

// Suspend writes before calling (unguarded here).
code map::flush() const NOEXCEPT
{
    std::shared_lock map_lock(map_mutex_);

    if (!loaded_)
        return error::flush_unloaded;

    return flush_() ? error::success : error::flush_failure;
}

code map::unload() NOEXCEPT
{
    std::unique_lock field_lock(field_mutex_);

    if (map_mutex_.try_lock())
    {
        BC_ASSERT_MSG(logical_ <= capacity_, "logical size exceeds capacity");

        if (!loaded_)
        {
            map_mutex_.unlock();
            return error::success;
        }

        if (!unmap_())
        {
            map_mutex_.unlock();
            return error::unload_failure;
        }

        loaded_ = false;
        map_mutex_.unlock();
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

size_t map::capacity() const NOEXCEPT
{
    std::shared_lock field_lock(field_mutex_);
    return capacity_;
}

size_t map::size() const NOEXCEPT
{
    std::shared_lock field_lock(field_mutex_);
    return logical_;
}

bool map::truncate(size_t size) NOEXCEPT
{
    std::unique_lock field_lock(field_mutex_);

    if (size > logical_)
        return false;

    logical_ = size;
    return true;
}

size_t map::allocate(size_t chunk) NOEXCEPT
{
    field_mutex_.lock_upgrade();

    // log: allocate_unloaded
    if (!loaded_)
    {
        field_mutex_.unlock_upgrade();
        return storage::eof;
    }

    // log: allocate_overflow
    if (is_add_overflow(logical_, chunk))
    {
        field_mutex_.unlock_upgrade();
        return storage::eof;
    }

    const auto size = logical_ + chunk;

    if (size > capacity_)
    {
        while (!map_mutex_.try_lock_for(boost::chrono::seconds(1)))
        {
            // log: deadlock_hint
        }

        // log: remap_failure
        if (!remap_(to_capacity(size)))
        {
            map_mutex_.unlock();
            field_mutex_.unlock_upgrade();
            return storage::eof;
        }

        map_mutex_.unlock();
    }

    const auto position = logical_;
    field_mutex_.unlock_upgrade_and_lock();
    logical_ = size;
    field_mutex_.unlock();

    return position;
}

// Always returns a valid and bounded memory pointer.
memory_ptr map::get(size_t offset) const NOEXCEPT
{
    const auto ptr = std::make_shared<accessor<mutex>>(map_mutex_);

    if (!loaded_)
        return nullptr;

    // With offset > size the assignment is negative (stream is exhausted).
    ptr->assign(
        std::next(memory_map_, offset),
        std::next(memory_map_, size()));

    return ptr;
}

// private, mman wrappers, not thread safe
// ----------------------------------------------------------------------------

constexpr auto fail = -1;

bool map::flush_() const NOEXCEPT
{
    // msync should not be required on modern linux, see linus et al.
    // stackoverflow.com/questions/5902629/mmap-msync-and-linux-process-termination
#if defined(HAVE_MSC)
    // unmap (and therefore msync) must be called before ftruncate.
    // "To flush all the dirty pages plus the metadata for the file and ensure
    // that they are physically written to disk..."
    return (::msync(memory_map_, logical_, MS_SYNC) != fail) 
        && (::fsync(descriptor_) != fail);
#elif defined(F_FULLFSYNC)
    // macOS msync fails with zero logical size (but we are no longer calling).
    // non-standard macOS behavior: news.ycombinator.com/item?id=30372218
    return ::fcntl(descriptor_, F_FULLFSYNC, 0) != fail;
#else
    // Linux: fsync "transfers ("flushes") all modified in-core data of
    // (i.e., modified buffer cache pages for) the file referred to by the
    // file descriptor fd to the disk device so all changed information
    // can be retrieved even if the system crashes or is rebooted. This
    // includes writing through or flushing a disk cache if present. The
    // call blocks until the device reports that transfer has completed."
    return ::fsync(descriptor_) != fail;
#endif
}

// Trims to logical size, can be zero.
bool map::unmap_() NOEXCEPT
{
#if defined(HAVE_MSC)
    const auto success =
           (::msync(memory_map_, logical_, MS_SYNC) != fail)
        && (::munmap(memory_map_, capacity_) != fail)
        && (::ftruncate(descriptor_, logical_) != fail)
        && (::fsync(descriptor_) != fail);
#else
    const auto success = (::ftruncate(descriptor_, logical_) != fail)
    #if defined(F_FULLFSYNC)
        && (::fcntl(descriptor_, F_FULLFSYNC, 0) != fail)
    #else
        && (::fsync(descriptor_) != fail)
    #endif
        && (::munmap(memory_map_, capacity_) != fail);
#endif

    capacity_ = zero;
    memory_map_ = nullptr;
    return success;
}

// Mapping has no effect on logical size, always maps max(logical, min) size.
bool map::map_() NOEXCEPT
{
    auto size = logical_;

    // Cannot map empty file, and want mininum capacity, so expand as required.
    if (size < minimum_)
    {
        size = minimum_;
        if (::ftruncate(descriptor_, size) == fail)
            return false;
    }

    memory_map_ = pointer_cast<uint8_t>(::mmap(nullptr, size,
        PROT_READ | PROT_WRITE, MAP_SHARED, descriptor_, 0));

    return finalize_(size);
}

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

    if (::ftruncate(descriptor_, size) == fail)
        return false;

#if defined(HAVE_MSC)
    // mman-win32 mremap hack (umap/map) requires flags and file descriptor.
    memory_map_ = pointer_cast<uint8_t>(::mremap_(memory_map_, capacity_, size,
        PROT_READ | PROT_WRITE, MAP_SHARED, descriptor_));
#elif defined(MREMAP_MAYMOVE)
    memory_map_ = pointer_cast<uint8_t>(::mremap(memory_map_, capacity_, size,
        MREMAP_MAYMOVE));
#else
    // macOS: does not define mremap or MREMAP_MAYMOVE.
    // TODO: see "MREMAP_MAYMOVE" in sqlite for map extension technique.
    memory_map_ = pointer_cast<uint8_t>(::mmap(nullptr, size,
        PROT_READ | PROT_WRITE, MAP_SHARED, descriptor_, 0));
#endif

    return finalize_(size);
}

bool map::finalize_(size_t size) NOEXCEPT
{
    // TODO: madvise with large length value fails on linux, does 0 imply all?
    if (memory_map_ == MAP_FAILED ||
        ::madvise(memory_map_, 0, MADV_RANDOM) == fail)
    {
        capacity_ = zero;
        memory_map_ = nullptr;
        return false;
    }

    capacity_ = size;
    return true;
}

BC_POP_WARNING()

} // namespace database
} // namespace libbitcoin
