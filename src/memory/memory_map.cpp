/**
 * Copyright (c) 2011-2017 libbitcoin developers (see AUTHORS)
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
#include <bitcoin/database/memory/memory_map.hpp>

#include <iostream>

#ifdef _WIN32
    #include <io.h>
    #include "../mman-win32/mman.h"
#else
    #include <unistd.h>
    #include <stddef.h>
    #include <sys/mman.h>
#endif
#include <cstddef>
#include <cstdint>
#include <fcntl.h>
#include <memory>
#include <stdexcept>
#include <string>
#include <sys/types.h>
#include <sys/stat.h>
#include <boost/filesystem.hpp>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/memory/accessor.hpp>
#include <bitcoin/database/memory/allocator.hpp>
#include <bitcoin/database/memory/memory.hpp>

// memory_map is able to support 32 bit, but because the database
// requires a larger file this is neither validated nor supported.
static_assert(sizeof(void*) == sizeof(uint64_t), "Not a 64 bit system!");

namespace libbitcoin {
namespace database {

#define FAIL -1
#define INVALID_HANDLE -1

// The percentage increase, e.g. 50 is 150% of the target size.
const size_t memory_map::default_expansion = 50;

size_t memory_map::file_size(int file_handle)
{
    if (file_handle == INVALID_HANDLE)
        return 0;

    // This is required because off_t is defined as long, which is 32 bits in
    // msvc and 64 bits in linux/osx, and stat contains off_t.
#ifdef _WIN32
#ifdef _WIN64
    struct _stat64 sbuf;
    if (_fstat64(file_handle, &sbuf) == FAIL)
        return 0;
#else
    struct _stat32 sbuf;
    if (_fstat32(file_handle, &sbuf) == FAIL)
        return 0;
#endif
#else
    // Limited to 32 bit files on 32 bit systems, see linux.die.net/man/2/open
    struct stat sbuf;
    if (fstat(file_handle, &sbuf) == FAIL)
        return 0;
#endif

    // Convert signed to unsigned size.
    BITCOIN_ASSERT_MSG(sbuf.st_size > 0, "File size cannot be 0 bytes.");
    return static_cast<size_t>(sbuf.st_size);
}

int memory_map::open_file(const path& filename)
{
#ifdef _WIN32
    int handle = _wopen(filename.wstring().c_str(),
        (O_RDWR | _O_BINARY | _O_RANDOM), (_S_IREAD | _S_IWRITE));
#else
    int handle = ::open(filename.string().c_str(),
        (O_RDWR), (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH));
#endif
    return handle;
}

bool memory_map::handle_error(const std::string& context,
    const path& filename)
{
#ifdef _WIN32
    const auto error = GetLastError();
#else
    const auto error = errno;
#endif
    LOG_FATAL(LOG_DATABASE)
        << "The file failed to " << context << ": " << filename << " : "
        << error;
    return false;
}

void memory_map::log_mapping() const
{
    LOG_DEBUG(LOG_DATABASE)
        << "Mapping: " << filename_ << " [" << file_size_ << "] ("
        << page() << ")";
}

void memory_map::log_resizing(size_t size) const
{
    LOG_DEBUG(LOG_DATABASE)
        << "Resizing: " << filename_ << " [" << size << "]";
}

void memory_map::log_flushed() const
{
    LOG_DEBUG(LOG_DATABASE)
        << "Flushed: " << filename_ << " [" << logical_size_ << "]";
}

void memory_map::log_unmapping() const
{
    LOG_DEBUG(LOG_DATABASE)
        << "Unmapping: " << filename_ << " [" << logical_size_ << "]";
}

void memory_map::log_unmapped() const
{
    LOG_DEBUG(LOG_DATABASE)
        << "Unmapped: " << filename_ << " [" << logical_size_ << ", "
        << file_size_ << "]";
}

memory_map::memory_map(const path& filename)
  : memory_map(filename, nullptr, default_expansion)
{
}

memory_map::memory_map(const path& filename, mutex_ptr mutex)
  : memory_map(filename, mutex, default_expansion)
{
}

// mmap documentation: tinyurl.com/hnbw8t5
memory_map::memory_map(const path& filename, mutex_ptr mutex, size_t expansion)
  : file_handle_(open_file(filename)),
    expansion_(expansion),
    filename_(filename),
    data_(nullptr),
    file_size_(file_size(file_handle_)),
    logical_size_(file_size_),
    closed_(true),
    remap_mutex_(mutex)
{
}

// Database threads must be joined before close is called (or destruct).
memory_map::~memory_map()
{
    close();
}

// Startup and shutdown.
// ----------------------------------------------------------------------------

// Map the database file and signal start.
// Open is not idempotent (should be called on single thread).
bool memory_map::open()
{
    // Critical Section (internal/unconditional)
    ///////////////////////////////////////////////////////////////////////////
    mutex_.lock_upgrade();

    if (!closed_)
    {
        mutex_.unlock_upgrade();
        //---------------------------------------------------------------------
        return false;
    }

    mutex_.unlock_upgrade_and_lock();
    //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    std::string error_name;

    // Initialize data_.
    if (!map(file_size_))
        error_name = "map";
    else if (madvise(data_, 0, MADV_RANDOM) == FAIL)
        error_name = "madvise";
    else
        closed_ = false;

    mutex_.unlock();
    ///////////////////////////////////////////////////////////////////////////

    // Keep logging out of the critical section.
    if (!error_name.empty())
        return handle_error(error_name, filename_);

    log_mapping();
    return true;
}

bool memory_map::flush() const
{
    std::string error_name;

    // Critical Section (internal/unconditional)
    ///////////////////////////////////////////////////////////////////////////
    mutex_.lock_upgrade();

    if (closed_)
    {
        mutex_.unlock_upgrade();
        //---------------------------------------------------------------------
        return true;
    }

    mutex_.unlock_upgrade_and_lock();
    //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

    if (msync(data_, logical_size_, MS_SYNC) == FAIL)
        error_name = "flush";

    mutex_.unlock();
    ///////////////////////////////////////////////////////////////////////////

    // Keep logging out of the critical section.
    if (!error_name.empty())
        return handle_error(error_name, filename_);

    ////log_flushed();
    return true;
}

// Close is idempotent and thread safe.
bool memory_map::close()
{
    std::string error_name;

    ////if (!closed_)
    ////    log_unmapping();

    // Critical Section (internal/unconditional)
    ///////////////////////////////////////////////////////////////////////////
    mutex_.lock_upgrade();

    if (closed_)
    {
        mutex_.unlock_upgrade();
        //---------------------------------------------------------------------
        return true;
    }

    mutex_.unlock_upgrade_and_lock();
    //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

    closed_ = true;

    if (logical_size_ > file_size_)
        error_name = "fit";
    else if (msync(data_, logical_size_, MS_SYNC) == FAIL)
        error_name = "msync";
    else if (munmap(data_, file_size_) == FAIL)
        error_name = "munmap";
    else if (ftruncate(file_handle_, logical_size_) == FAIL)
        error_name = "ftruncate";
    else if (fsync(file_handle_) == FAIL)
        error_name = "fsync";
    else if (::close(file_handle_) == FAIL)
        error_name = "close";

    mutex_.unlock();
    ///////////////////////////////////////////////////////////////////////////

    // Keep logging out of the critical section.
    if (!error_name.empty())
        return handle_error(error_name, filename_);

    log_unmapped();
    return true;
}

bool memory_map::closed() const
{
    // Critical Section (internal/unconditional)
    ///////////////////////////////////////////////////////////////////////////
    REMAP_READ(mutex_);

    return closed_;
    ///////////////////////////////////////////////////////////////////////////
}

// Operations.
// ----------------------------------------------------------------------------

size_t memory_map::size() const
{
    // Critical Section (internal)
    ///////////////////////////////////////////////////////////////////////////
    REMAP_READ(mutex_);

    return file_size_;
    ///////////////////////////////////////////////////////////////////////////
}

memory_ptr memory_map::access()
{
    return REMAP_ACCESSOR(data_, mutex_);
}

// throws runtime_error
memory_ptr memory_map::resize(size_t size)
{
    return reserve(size, 0);
}

// throws runtime_error
memory_ptr memory_map::reserve(size_t size)
{
    return reserve(size, expansion_);
}

// throws runtime_error
// There is no way to gracefully recover from a resize failure because there
// are integrity relationships across multiple database files. Stopping a write
// in one would require rolling back preceding write operations in others.
// To handle this situation without database corruption would require predicting
// the required allocation and all resizing before writing a block.
memory_ptr memory_map::reserve(size_t size, size_t expansion)
{
    // Internally preventing resize during close is not possible because of
    // cross-file integrity. So we must coalesce all threads before closing.

    // Critical Section (internal)
    ///////////////////////////////////////////////////////////////////////////
    const auto memory = REMAP_ALLOCATOR(mutex_);

    // The store should only have been closed after all threads terminated.
    if (closed_)
        throw std::runtime_error("Resize failure, store already closed.");

    if (size > file_size_)
    {
        // TODO: manage overflow (requires ceiling_multiply).
        // Expansion is an integral number that represents a real number factor.
        const size_t target = size * ((expansion + 100.0) / 100.0);

        mutex_.unlock_upgrade_and_lock();
        //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

        // All existing database pointers are invalidated by this call.
        if (!truncate_mapped(target))
        {
            handle_error("resize", filename_);
            throw std::runtime_error("Resize failure, disk space may be low.");
        }

        //---------------------------------------------------------------------
        mutex_.unlock_and_lock_upgrade();
    }

    logical_size_ = size;
    REMAP_ASSIGN(memory, data_);

    // Always return in shared lock state.
    // The critical section does not end until this shared pointer is freed.
    return memory;
    ///////////////////////////////////////////////////////////////////////////
}

// privates
// ----------------------------------------------------------------------------

size_t memory_map::page() const
{
#ifdef _WIN32
    SYSTEM_INFO configuration;
    GetSystemInfo(&configuration);
    return configuration.dwPageSize;
#else
    errno = 0;
    const auto page_size = sysconf(_SC_PAGESIZE);

    // -1 is both a return code and a potentially valid value, so use errno.
    if (errno != 0)
        handle_error("sysconf", filename_);

    BITCOIN_ASSERT(page_size <= max_size_t);
    return static_cast<size_t>(page_size == -1 ? 0 : page_size);
#endif
}

bool memory_map::unmap()
{
    const auto success = (munmap(data_, file_size_) != FAIL);
    file_size_ = 0;
    data_ = nullptr;
    return success;
}

bool memory_map::map(size_t size)
{
    if (size == 0)
        return false;

    data_ = reinterpret_cast<uint8_t*>(mmap(0, size, PROT_READ | PROT_WRITE,
        MAP_SHARED, file_handle_, 0));

    return validate(size);
}

bool memory_map::remap(size_t size)
{
#ifdef MREMAP_MAYMOVE
    data_ = reinterpret_cast<uint8_t*>(mremap(data_, file_size_, size,
        MREMAP_MAYMOVE));

    return validate(size);
#else
    return unmap() && map(size);
#endif
}

bool memory_map::truncate(size_t size)
{
    return ftruncate(file_handle_, size) != FAIL;
}

bool memory_map::truncate_mapped(size_t size)
{
    log_resizing(size);

    // Critical Section (conditional/external)
    ///////////////////////////////////////////////////////////////////////////
    conditional_lock lock(remap_mutex_);

#ifndef MREMAP_MAYMOVE
    if (!unmap())
        return false;
#endif

    if (!truncate(size))
        return false;

#ifndef MREMAP_MAYMOVE
    return map(size);
#else
    return remap(size);
#endif
    ///////////////////////////////////////////////////////////////////////////
}

bool memory_map::validate(size_t size)
{
    if (data_ == MAP_FAILED)
    {
        file_size_ = 0;
        data_ = nullptr;
        return false;
    }

    file_size_ = size;
    return true;
}

} // namespace database
} // namespace libbitcoin
