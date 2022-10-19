/**
 * Copyright (c) 2011-2019 libbitcoin developers (see AUTHORS)
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
#include <bitcoin/database/memory/file_storage.hpp>

#if defined(HAVE_MSC)
    #include "../mman-win32/mman.h"
#else
    #include <sys/mman.h>
    #include <sys/stat.h>
    #include <sys/types.h>
#endif
#include <fcntl.h>
#include <memory>
#include <shared_mutex>
#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/accessor.hpp>
#include <bitcoin/database/memory/file.hpp>
#include <bitcoin/database/memory/memory.hpp>

// file_storage is able to support 32 bit, but because the database
// requires a larger file this is neither validated nor supported.
static_assert(sizeof(void*) == sizeof(uint64_t), "Not a 64 bit system!");

namespace libbitcoin {
namespace database {

BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

using namespace system;

#define FAIL -1
#define INVALID_DESCRIPTOR -1

file_storage::file_storage(const path& filename, size_t minimum,
    size_t expansion) NOEXCEPT
  : filename_(filename),
    minimum_(minimum),
    expansion_(expansion),
    map_(nullptr),
    mapped_(false),
    logical_(zero),
    capacity_(zero),
    file_descriptor_(INVALID_DESCRIPTOR)
{
}

file_storage::~file_storage() NOEXCEPT
{
    // LOG STOP WARNINGS (handled if unload_map() and close() were called).
    BC_ASSERT_MSG(!mapped_, "file mapped at destruct");
    BC_ASSERT_MSG(is_null(map_), "map defined at destruct");
    BC_ASSERT_MSG(is_zero(logical_), "logical nonzero at destruct");
    BC_ASSERT_MSG(is_zero(capacity_), "capacity nonzero at destruct");
    BC_ASSERT_MSG(file_descriptor_ == INVALID_DESCRIPTOR, "file open at destruct");
}

bool file_storage::open() NOEXCEPT
{
    std::unique_lock field_lock(field_mutex_);

    // SEQUENCE ERROR (open_while_open)
    if (file_descriptor_ != INVALID_DESCRIPTOR)
        return false;

    file_descriptor_ = open_file(filename_);
    logical_ = file_size(file_descriptor_);

    // STORE CORRUPTION (open_failure)
    return file_descriptor_ != INVALID_DESCRIPTOR;
}

bool file_storage::close() NOEXCEPT
{
    std::unique_lock field_lock(field_mutex_);

    // SEQUENCE ERROR (close_while_mapped)
    if (mapped_)
        return false;

    // idempotent
    if (file_descriptor_ == INVALID_DESCRIPTOR)
        return true;

    const auto descriptor = file_descriptor_;
    file_descriptor_ = INVALID_DESCRIPTOR;
    logical_ = zero;

    // STORE CORRUPTION (close_failure)
    return close_file(descriptor);
}

bool file_storage::is_open() const NOEXCEPT
{
    std::shared_lock field_lock(field_mutex_);
    return file_descriptor_ != INVALID_DESCRIPTOR;
}

// Map, flush, unmap.
// ----------------------------------------------------------------------------

// Each accessor holds a shared lock on map_mutex_ (read/write access to map).
// Exclusive lock on map_mutex_ ensures there are no open accessor objects.
// map_ field requires exclusive lock on map_mutex_ for read(flush)/write.
// Fields except map_ require exclusive lock on field_mutex_ for write.
// Fields except map_ require at least shared lock on field_mutex_ for read.

bool file_storage::load_map() NOEXCEPT
{
    std::unique_lock field_lock(field_mutex_);

    if (map_mutex_.try_lock())
    {
        if (mapped_ || !map())
        {
            // SEQUENCE ERROR (map_while_mapped) | STORE CORRUPTION (map_failure)
            map_mutex_.unlock();
            return false;
        }

        mapped_ = true;
        map_mutex_.unlock();
        return true;
    }

    // SEQUENCE ERROR (map_while_locked)
    return false;
}

bool file_storage::unload_map() NOEXCEPT
{
    std::unique_lock field_lock(field_mutex_);

    if (map_mutex_.try_lock())
    {
        BC_ASSERT_MSG(logical_ <= capacity_, "logical size exceeds capacity");

        if (!mapped_ || !unmap())
        {
            // SEQUENCE ERROR (unmap_while_unmapped) | STORE CORRUPTION (unmap_failure)
            map_mutex_.unlock();
            return false;
        }

        map_mutex_.unlock();
        mapped_ = false;
        return true;
    }

    // SEQUENCE ERROR (unmap_while_locked)
    return false;
}

bool file_storage::is_mapped() const NOEXCEPT
{
    std::shared_lock field_lock(field_mutex_);
    return mapped_;
}

// Operations.
// ----------------------------------------------------------------------------

bool file_storage::flush_map() const NOEXCEPT
{
    std::shared_lock field_lock(field_mutex_);

    if (!mapped_)
    {
        // SEQUENCE ERROR (flush_while_unmapped)
        return false;
    }

    // Join store threads before calling, ensuring no outstanding memory object
    // or reservation in process.
    std::unique_lock map_lock(map_mutex_);

    // STORE CORRUPTION (flush_failure)
    return flush();
}

// private
memory_ptr file_storage::reserve(size_t required, size_t minimum,
    size_t expansion) NOEXCEPT
{
    field_mutex_.lock_upgrade();

    if (!mapped_)
    {
        // SEQUENCE ERROR (reserve_while_unmapped)
        field_mutex_.unlock_upgrade();
        return nullptr;
    }

    if (required > capacity_)
    {
        // TODO: use timed lock and have caller loop with message.
        // Block until all memory objects are freed (potential deadlock).
        std::unique_lock map_lock(map_mutex_);

        if (!remap(get_resize(required, minimum, expansion)))
        {
            // STORE CORRUPTION (remap_failure)
            field_mutex_.unlock_upgrade();
            return nullptr;
        }
    }

    // No thread can intervene here because of the field_mutex_ upgrade lock.
    // This ensures that the returned object is of the required map size.
    auto memory = std::make_shared<accessor>(map_mutex_);
    memory->assign(map_);

    field_mutex_.unlock_upgrade_and_lock();
    logical_ = required;
    field_mutex_.unlock();
    return memory;
}

memory_ptr file_storage::get() NOEXCEPT
{
    auto memory = std::make_shared<accessor>(map_mutex_);

    if (!mapped_)
    {
        // SEQUENCE ERROR (get_while_unmapped)
        memory.reset();
        return nullptr;
    }

    memory->assign(map_);
    return memory;
}

memory_ptr file_storage::reserve(size_t required) NOEXCEPT
{
    return reserve(required, minimum_, expansion_);
}

memory_ptr file_storage::resize(size_t required) NOEXCEPT
{
    return reserve(required, zero, zero);
}

size_t file_storage::logical() const NOEXCEPT
{
    std::shared_lock field_lock(field_mutex_);
    return logical_;
}

size_t file_storage::capacity() const NOEXCEPT
{
    std::shared_lock field_lock(field_mutex_);
    return capacity_;
}

size_t file_storage::size() const NOEXCEPT
{
    std::shared_lock field_lock(field_mutex_);
    return is_open() ? file_size(file_descriptor_) : zero;
}

// private, mman wrappers, not thread safe
// ----------------------------------------------------------------------------

// Flush failure does not halt process, flush does not ensure file integrity.
bool file_storage::flush() const NOEXCEPT
{
    // macos msync fails with zero logical size.
    return is_zero(logical_) ||
        ::msync(map_, logical_, MS_SYNC) != FAIL;
}

// Trims to logical size, can be zero.
bool file_storage::unmap() NOEXCEPT
{
    // fsync is required to ensure file integrity (cache cleared).
    const auto success = flush()
        && (::munmap(map_, capacity_) != FAIL)
        && (::ftruncate(file_descriptor_, logical_) != FAIL)
        && (::fsync(file_descriptor_) != FAIL);

    capacity_ = zero;
    map_ = nullptr;
    return success;
}

// Mapping has no effect on logical size.
bool file_storage::map() NOEXCEPT
{
    auto size = logical_;

    // Cannot map empty file, so expand to minimum capacity if zero.
    if (is_zero(size))
    {
        size = minimum_;
        if (::ftruncate(file_descriptor_, minimum_) == FAIL)
            return false;

        capacity_ = size;
    }

    map_ = pointer_cast<uint8_t>(::mmap(nullptr, size, PROT_READ | PROT_WRITE,
        MAP_SHARED, file_descriptor_, 0));

    return finalize(size);
}

// Remapping has no effect on logical size, sets map_/capacity_.
bool file_storage::remap(size_t size) NOEXCEPT
{
    // Cannot remap empty file, so expand to minimum capacity if zero.
    if (is_zero(size))
        size = minimum_;

    if (::ftruncate(file_descriptor_, size) == FAIL)
        return false;
    
// mman-win32 mremap hack (umap/map) requires flags and file descriptor.
#if defined (HAVE_MSC)
    map_ = pointer_cast<uint8_t>(::mremap_(map_, capacity_, size,
        PROT_READ | PROT_WRITE, MAP_SHARED, file_descriptor_));
#else
    map_ = pointer_cast<uint8_t>(::mremap(map_, capacity_, size,
        MREMAP_MAYMOVE));
#endif

    return finalize(size);
}

bool file_storage::finalize(size_t size) NOEXCEPT
{
    // TODO: madvise with large length value fails on linux, is 0 sufficient?
    if (map_ == MAP_FAILED || ::madvise(map_, 0, MADV_RANDOM) == FAIL)
    {
        capacity_ = zero;
        map_ = nullptr;
        return false;
    }

    capacity_ = size;
    return true;
}

BC_POP_WARNING()

} // namespace database
} // namespace libbitcoin
