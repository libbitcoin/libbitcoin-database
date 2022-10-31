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
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/accessor.hpp>
#include <bitcoin/database/memory/file.hpp>

namespace libbitcoin {
namespace database {

BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

using namespace system;

map::map(const path& filename, size_t minimum, size_t expansion) NOEXCEPT
  : filename_(filename),
    minimum_(minimum),
    expansion_(expansion),
    memory_map_(nullptr),
    mapped_(false),
    logical_(zero),
    capacity_(zero),
    descriptor_(file::invalid)
{
}

map::~map() NOEXCEPT
{
    // LOG STOP WARNINGS (handled if unload() and close() were called).
    BC_ASSERT_MSG(!mapped_, "file mapped at destruct");
    BC_ASSERT_MSG(is_null(memory_map_), "map defined at destruct");
    BC_ASSERT_MSG(is_zero(logical_), "logical nonzero at destruct");
    BC_ASSERT_MSG(is_zero(capacity_), "capacity nonzero at destruct");
    BC_ASSERT_MSG(descriptor_ == file::invalid, "file open at destruct");
}

bool map::open() NOEXCEPT
{
    std::unique_lock field_lock(field_mutex_);

    // open_open
    if (descriptor_ != file::invalid)
        return false;

    descriptor_ = file::open(filename_);
    logical_ = file::size(descriptor_);

    // open_failure
    return descriptor_ != file::invalid;
}

bool map::close() NOEXCEPT
{
    std::unique_lock field_lock(field_mutex_);

    // close_mapped
    if (mapped_)
        return false;

    // idempotent (close-closed is ok)
    if (descriptor_ == file::invalid)
        return true;

    const auto descriptor = descriptor_;
    descriptor_ = file::invalid;
    logical_ = zero;

    // close_failure
    return file::close(descriptor);
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

bool map::load() NOEXCEPT
{
    std::unique_lock field_lock(field_mutex_);

    if (map_mutex_.try_lock())
    {
        if (mapped_ || !map_())
        {
            // map_mapped | map_failure
            map_mutex_.unlock();
            return false;
        }

        mapped_ = true;
        map_mutex_.unlock();
        return true;
    }

    // map_locked
    return false;
}

bool map::flush() const NOEXCEPT
{
    std::shared_lock field_lock(field_mutex_);

    if (!mapped_)
    {
        // flush_unmapped
        return false;
    }

    // Join store threads before calling, ensuring no outstanding memory object
    // or reservation in process.
    std::unique_lock map_lock(map_mutex_);

    // flush_failure
    return flush_();
}

bool map::unload() NOEXCEPT
{
    std::unique_lock field_lock(field_mutex_);

    if (map_mutex_.try_lock())
    {
        BC_ASSERT_MSG(logical_ <= capacity_, "logical size exceeds capacity");

        if (!mapped_ || !unmap_())
        {
            // unmap_unmapped | unmap_failure
            map_mutex_.unlock();
            return false;
        }

        map_mutex_.unlock();
        mapped_ = false;
        return true;
    }

    // unmap_locked
    return false;
}

bool map::is_mapped() const NOEXCEPT
{
    std::shared_lock field_lock(field_mutex_);
    return mapped_;
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

// Cannot resize beyond capactity, intended for truncation.
bool map::resize(size_t size) NOEXCEPT
{
    std::unique_lock field_lock(field_mutex_);

    // resize_overflow
    if (size > capacity_)
        return false;

    logical_ = size;
    return true;
}

size_t map::allocate(size_t chunk) NOEXCEPT
{
    field_mutex_.lock_upgrade();

    // allocate_unmapped
    if (!mapped_)
    {
        field_mutex_.unlock_upgrade();
        return storage::eof;
    }

    // allocate_overflow
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
            // TODO: log deadlock_hint
        }

        // remap_failure
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

// The memory pointer can be encapsulated into a read/write/flip stream.
// This requires a mutable byte pointer and end pointer.
memory_ptr map::get(size_t offset) NOEXCEPT
{
    auto memory = std::make_shared<accessor<mutex>>(map_mutex_);

    // data_unmapped
    if (!mapped_)
        return nullptr;

    // Because end is not record-bound it only guards file access, not records.
    memory->assign(
        std::next(memory_map_, offset),
        std::next(memory_map_, size()));

    return memory;
}

// private, mman wrappers, not thread safe
// ----------------------------------------------------------------------------

constexpr auto mman_fail = -1;

// Flush failure does not halt process, flush does not ensure file integrity.
bool map::flush_() const NOEXCEPT
{
    // macos msync fails with zero logical size.
    return is_zero(logical_) ||
        ::msync(memory_map_, logical_, MS_SYNC) != mman_fail;
}

// Trims to logical size, can be zero.
bool map::unmap_() NOEXCEPT
{
    // fsync is required to ensure file integrity (cache cleared).
    const auto success = flush_()
        && (::munmap(memory_map_, capacity_) != mman_fail)
        && (::ftruncate(descriptor_, logical_) != mman_fail)
        && (::fsync(descriptor_) != mman_fail);

    capacity_ = zero;
    memory_map_ = nullptr;
    return success;
}

// Mapping has no effect on logical size, always maps max(logical/min) size.
bool map::map_() NOEXCEPT
{
    auto size = logical_;

    // Cannot map empty file, and want mininum capacity, so expand as required.
    if (size < minimum_)
    {
        size = minimum_;
        if (::ftruncate(descriptor_, size) == mman_fail)
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

    if (::ftruncate(descriptor_, size) == mman_fail)
        return false;

// mman-win32 mremap hack (umap/map) requires flags and file descriptor.
#if defined(HAVE_MSC)
    memory_map_ = pointer_cast<uint8_t>(::mremap_(memory_map_, capacity_, size,
        PROT_READ | PROT_WRITE, MAP_SHARED, descriptor_));
#elif defined(MREMAP_MAYMOVE)
    memory_map_ = pointer_cast<uint8_t>(::mremap(memory_map_, capacity_, size,
        MREMAP_MAYMOVE));
#else
    // macOS: does not define mremap or MREMAP_MAYMOVE.
    memory_map_ = pointer_cast<uint8_t>(::mmap(nullptr, size,
        PROT_READ | PROT_WRITE, MAP_SHARED, descriptor_, 0));
#endif

    return finalize_(size);
}

bool map::finalize_(size_t size) NOEXCEPT
{
    // TODO: madvise with large length value fails on linux, is 0 sufficient?
    if (memory_map_ == MAP_FAILED ||
        ::madvise(memory_map_, 0, MADV_RANDOM) == mman_fail)
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
