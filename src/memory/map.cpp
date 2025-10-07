/**
 * Copyright (c) 2011-2025 libbitcoin developers (see AUTHORS)
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
    #include "mman-win32/mman.hpp"
#else
    #include <sys/mman.h>
    #include <sys/stat.h>
    #include <sys/types.h>
    #include <unistd.h>
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

map::map(const path& filename, size_t minimum, size_t expansion,
    bool random) NOEXCEPT
  : filename_(filename),
    minimum_(minimum),
    expansion_(expansion),
    random_(random)
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

    // Windows doesn't use madvise, instead infers map access from file open.
    if (const auto ec = file::open_ex(opened_, filename_, random_))
        return ec;

    return file::size_ex(logical_, opened_);
}

code map::close() NOEXCEPT
{
    std::unique_lock map_lock(remap_mutex_);
    std::unique_lock field_lock(field_mutex_);

    if (loaded_)
        return error::close_loaded;

    if (opened_ == file::invalid)
        return error::success;

    const auto descriptor = opened_;
    opened_ = file::invalid;
    logical_ = zero;

    return file::close_ex(descriptor);
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

        remap_mutex_.unlock();
        return error::success;
    }

    return error::load_locked;
}

// Suspend writes before calling.
code map::reload() NOEXCEPT
{
    std::unique_lock field_lock(field_mutex_);

    if (remap_mutex_.try_lock())
    {
        if (!loaded_)
        {
            remap_mutex_.unlock();
            return error::reload_unloaded;
        }

        // Allow resume from disk full.
        set_disk_space(zero);

        remap_mutex_.unlock();
        return error::success;
    }

    return error::reload_locked;
}

// Suspend writes before calling.
code map::flush() NOEXCEPT
{
    // Prevent unload, resize, remap.
    std::shared_lock map_lock(remap_mutex_);
    std::shared_lock field_lock(field_mutex_);

    if (!loaded_)
        return error::flush_unloaded;

    // Reads fields and the memory map.
    return flush_() ? error::success : error::flush_failure;
}

// Suspend writes before calling.
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

bool map::expand(size_t size) NOEXCEPT
{
    std::unique_lock field_lock(field_mutex_);

    if (fault_ || !loaded_)
        return false;

    if (size <= logical_)
        return true;

    if (size > capacity_)
    {
        const auto capacity = to_capacity(size);
        std::unique_lock remap_lock(remap_mutex_);
        if (!remap_(capacity))
            return false;
    }

    logical_ = size;
    return true;
}

bool map::reserve(size_t chunk) NOEXCEPT
{
    std::unique_lock field_lock(field_mutex_);

    if (fault_ || !loaded_ || is_add_overflow(logical_, chunk))
        return false;

    const auto end = logical_ + chunk;
    if (end > capacity_)
    {
        const auto capacity = to_capacity(end);
        std::unique_lock remap_lock(remap_mutex_);
        if (!remap_(capacity))
            return false;
    }

    // Same as allocate except logical does not change.
    return true;
}

// Waits until all access pointers are destructed. Will deadlock if any access
// pointer is waiting on allocation. Lock safety requires that access pointers
// are short-lived and do not block on allocation.
size_t map::allocate(size_t chunk) NOEXCEPT
{
    std::unique_lock field_lock(field_mutex_);

    if (fault_ || !loaded_ || is_add_overflow(logical_, chunk))
        return storage::eof;

    auto end = logical_ + chunk;
    if (end > capacity_)
    {
        const auto capacity = to_capacity(end);

        // TODO: Could loop over a try lock here and log deadlock warning.
        std::unique_lock remap_lock(remap_mutex_);

        // Disk full condition leaves store in valid state despite eof return.
        if (!remap_(capacity))
            return storage::eof;
    }

    std::swap(logical_, end);
    return end;
}

memory_ptr map::set(size_t offset, size_t size, uint8_t backfill) NOEXCEPT
{
    {
        std::unique_lock field_lock(field_mutex_);

        if (fault_ || !loaded_ || is_add_overflow(offset, size))
            return {};

        const auto end = std::max(logical_, offset + size);
        if (end > capacity_)
        {
            const auto capacity = to_capacity(end);

            // TODO: Could loop over a try lock here and log deadlock warning.
            std::unique_lock remap_lock(remap_mutex_);

            // Disk full condition leaves store in valid state despite null.
            if (!remap_(capacity))
                return {};

            // Fill new capacity as offset may not be at end due to expansion.
            BC_PUSH_WARNING(NO_POINTER_ARITHMETIC)
            std::fill_n(memory_map_ + logical_, capacity - logical_, backfill);
            BC_POP_WARNING()
        }

        logical_ = end;
    }

    return get(offset);
}

memory_ptr map::get(size_t offset) const NOEXCEPT
{
    // Obtaining size before access prevents mutual mutex wait (deadlock).
    // The store could remap between here and next line, but capacity only
    // increases. Close zeroizes capacity but file must be unloaded to do so.
    // Truncate can reduce logical, but capacity is not affected. It is always
    // safe to write past current logical within current capacity.
    const auto allocated = size();

    // Takes a shared lock on remap_mutex_ until destruct, blocking remap.
    const auto ptr = std::make_shared<access>(remap_mutex_);

    // loaded_ update is precluded by remap_mutex_, making this read atomic.
    if (!loaded_ || is_null(ptr))
        return nullptr;

    // With offset > size the assignment is negative (stream is exhausted).
    BC_PUSH_WARNING(NO_POINTER_ARITHMETIC)
    ptr->assign(memory_map_ + offset, memory_map_ + allocated);
    BC_POP_WARNING()
    return ptr;
}

memory_ptr map::get_capacity(size_t offset) const NOEXCEPT
{
    // Same as get() but limited by capacity() vs. size().
    const auto allocated = capacity();
    const auto ptr = std::make_shared<access>(remap_mutex_);
    if (!loaded_ || is_null(ptr))
        return nullptr;

    BC_PUSH_WARNING(NO_POINTER_ARITHMETIC)
    ptr->assign(memory_map_ + offset, memory_map_ + allocated);
    BC_POP_WARNING()
    return ptr;
}

memory::iterator map::get_raw(size_t offset) const NOEXCEPT
{
    // Pointer is otherwise unguarded, not remap safe (use for table heads).
    if (offset > size())
        return nullptr;

    BC_PUSH_WARNING(NO_POINTER_ARITHMETIC)
    return memory_map_ + offset;
    BC_POP_WARNING()
}

code map::get_fault() const NOEXCEPT
{
    return error_.load();
}

size_t map::get_space() const NOEXCEPT
{
    return space_.load();
}

// protected
// ----------------------------------------------------------------------------

size_t map::to_capacity(size_t required) const NOEXCEPT
{
    BC_PUSH_WARNING(NO_STATIC_CAST)
    const auto resize = required * ((expansion_ + 100.0) / 100.0);
    const auto target = std::max(minimum_, static_cast<size_t>(resize));
    BC_POP_WARNING()

    BC_ASSERT(target >= required);
    return target;
}

// Read-write protected by atomic, write-write protected by remap_mutex.
void map::set_first_code(const error::error_t& ec) NOEXCEPT
{
    if (!fault_)
    {
        // fault is not exposed so requires no atomic (fast read).
        fault_ = true;

        // error is atomic for public read exposure.
        error_.store(ec);
    }
}

void map::set_disk_space(size_t required) NOEXCEPT
{
    space_.store(required);
}

// private, mman wrappers, not thread safe
// ----------------------------------------------------------------------------

// TODO: map_, flush_, unmap_, remap_ (resize_, finalize_) return codes.

constexpr auto fail = -1;

// Never results in unmapped.
bool map::flush_() NOEXCEPT
{
#if defined(HAVE_MSC)
    // unmap (and therefore msync) must be called before ftruncate.
    // "To flush all the dirty pages plus the metadata for the file and ensure
    // that they are physically written to disk..."
    const auto success = (::msync(memory_map_, logical_, MS_SYNC) != fail) 
        && (::fsync(opened_) != fail);
#elif defined(F_FULLFSYNC)
    // macOS msync fails with zero logical size (but we are no longer calling).
    // non-standard macOS behavior: news.ycombinator.com/item?id=30372218
    const auto success = ::fcntl(opened_, F_FULLFSYNC, 0) != fail;
#else
    // msync should not be required on modern linux, see linus et al.
    // stackoverflow.com/questions/5902629/mmap-msync-and-linux-process-termination
    // Linux: fsync "transfers ("flushes") all modified in-core data of
    // (i.e., modified buffer cache pages for) the file referred to by the
    // file descriptor fd to the disk device so all changed information
    // can be retrieved even if the system crashes or is rebooted. This
    // includes writing through or flushing a disk cache if present. The
    // call blocks until the device reports that transfer has completed."
    const auto success = ::fsync(opened_) != fail;
#endif

    if (!success)
        set_first_code(error::fsync_failure);

    return success;
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
    if (!success)
        set_first_code(error::munmap_failure);

    loaded_ = false;
    capacity_ = zero;
    memory_map_ = {};
    return success;
}

// Mapping failure results in unmapped.
// Mapping has no effect on logical size, always maps max(logical, min) size.
bool map::map_() NOEXCEPT
{
    auto size = logical_;

    // Cannot map empty file, and want mininum capacity, so expand as required.
    // disk_full: space is set but no code is set with false return.
    if ((size < minimum_) && !resize_((size = minimum_)))
      return false;

    memory_map_ = pointer_cast<uint8_t>(::mmap(nullptr, size,
        PROT_READ | PROT_WRITE, MAP_SHARED, opened_, 0));

    return finalize_(size);
}

// Remap failure results in unmapped.
// Remapping has no effect on logical size, sets map_/capacity_.
bool map::remap_(size_t size) NOEXCEPT
{
    BC_ASSERT(size >= logical_);

    // Cannot remap empty file, so expand to minimum capacity if zero.
    if (is_zero(size))
        size = minimum_;

#if !defined(HAVE_MSC) && !defined(MREMAP_MAYMOVE)
    // macOS: unmap before ftruncate sets new size.
    if (!unmap_())
        return false;

    // disk_full: unmap(ok), resize(fail for space), map(ok), return false.
    // disk_full: if second unmap fails then code is set, and false return.
    if (!resize_(size))
    {
        /* bool */ map::map_();
        return false;
    }
#else
    // disk_full: space is set but no code is set with false return.
    if (!resize_(size))
        return false;
#endif

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

// disk_full: space is set but no code is set with false return.
bool map::resize_(size_t size) NOEXCEPT
{
    // Disk full detection is platform common, any other failure is an abort.
    if (::fallocate(opened_, 0, capacity_, size - capacity_) == fail)
    {
        // Disk full is the only restartable store failure (leave mapped).
        if (errno == ENOSPC)
        {
            set_disk_space(size - capacity_);
            return false;
        }

        set_first_code(error::ftruncate_failure);
        unmap_();
        return false;
    }

    return true;
}

// Finalize failure results in unmapped.
bool map::finalize_(size_t size) NOEXCEPT
{
    if (memory_map_ == MAP_FAILED)
    {
        loaded_ = false;
        capacity_ = zero;
        memory_map_ = {};

        // mmap or mremap failure (not mapped).
        set_first_code(error::mmap_failure);
        return false;
    }

#if !defined(HAVE_MSC)
    // Get page size (usually 4KB).
    const int page_size = ::sysconf(_SC_PAGESIZE);
    const auto page = possible_narrow_sign_cast<size_t>(page_size);

    // If not one bit then page size is not a power of two as required.
    if (page_size == fail || !is_one(ones_count(page)))
    {
        set_first_code(error::sysconf_failure);
        unmap_();
        return false;
    }
    
    // Align size up to page boundary.
    const auto max = sub1(page);
    const auto align = bit_and(ceilinged_add(size, max), bit_not(max));

    // Use 1GB chunks to avoid large-length issues.
    constexpr auto chunk = power2(30u);
    const auto advice = random_ ? MADV_RANDOM : MADV_SEQUENTIAL;

    for (auto offset = zero; offset < align; offset += chunk)
    {
        BC_PUSH_WARNING(NO_POINTER_ARITHMETIC)
        const auto start = memory_map_ + offset;
        BC_POP_WARNING()

        if (::madvise(start, std::min(chunk, align - offset), advice) == fail)
        {
            set_first_code(error::madvise_failure);
            unmap_();
            return false;
        }
    }
#endif

    loaded_ = true;
    capacity_ = size;
    return true;
}

BC_POP_WARNING()

} // namespace database
} // namespace libbitcoin
