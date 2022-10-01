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

#include <iostream>

#if defined(HAVE_MSC)
    #include <io.h>
    #include "../mman-win32/mman.h"
#else
    #include <unistd.h>
    #include <stddef.h>
    #include <sys/mman.h>
#endif
#include <algorithm>
#include <fcntl.h>
#include <memory>
#include <shared_mutex>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/accessor.hpp>
#include <bitcoin/database/memory/memory.hpp>

// file_storage is able to support 32 bit, but because the database
// requires a larger file this is neither validated nor supported.
static_assert(sizeof(void*) == sizeof(uint64_t), "Not a 64 bit system!");

namespace libbitcoin {
namespace database {

using namespace bc::system;

#define FAIL -1
#define INVALID_HANDLE -1

// The percentage increase, e.g. 50 is 150% of the target size.
const size_t file_storage::default_expansion = 50;

// The default minimum file size.
const uint64_t file_storage::default_capacity = 1;

int file_storage::close_file(int file_handle) NOEXCEPT
{
    // TODO: it may be possible to collapse this given newer WIN32 API.
#if defined(HAVE_MSC)
    return _close(file_handle);
#else
    return ::close(file_handle);
#endif
}

size_t file_storage::file_size(int file_handle) NOEXCEPT
{
    if (file_handle == INVALID_HANDLE)
        return 0;

    // TODO: it may be possible to collapse this given newer WIN32 API.
    // This is required because off_t is defined as long, which is 32|64 bits
    // in msvc and 64 bits in linux/osx, and stat contains off_t.
#if defined(HAVE_MSC)
#if defined(HAVE_X64)
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
    BC_ASSERT_MSG(sbuf.st_size > 0, "File size cannot be 0 bytes.");
    return sign_cast<size_t>(sbuf.st_size);
}

int file_storage::open_file(const path& filename) NOEXCEPT
{
    // _wsopen_s and wstring do not throw (but are unannotated).
    BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

#if defined(HAVE_MSC)
    int handle;
    if (_wsopen_s(&handle, filename.wstring().c_str(),
        (O_RDWR | _O_BINARY | _O_RANDOM), _SH_DENYWR,
        (_S_IREAD | _S_IWRITE)) == FAIL)
        handle = INVALID_HANDLE;
#else
    int handle = ::open(filename.string().c_str(),
        (O_RDWR), (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH));
#endif
    return handle;

    BC_POP_WARNING()
}

BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

bool file_storage::handle_error(const std::string& context,
    const path& filename) NOEXCEPT
{
    // TODO: it may be possible to collapse this given newer WIN32 API.
#if defined(HAVE_MSC)
    const auto error = GetLastError();
#else
    const auto error = errno;
#endif
    LOG_FATAL(LOG_DATABASE)
        << "The file failed to " << context << ": " << filename << " : "
        << error;
    return false;
}

void file_storage::log_mapping() const NOEXCEPT
{
    LOG_DEBUG(LOG_DATABASE)
        << "Mapping: " << filename_ << " [" << capacity_ << "] ("
        << page() << ")";
}

void file_storage::log_resizing(size_t size) const NOEXCEPT
{
    LOG_DEBUG(LOG_DATABASE)
        << "Resizing: " << filename_ << " [" << size << "]";
}

void file_storage::log_flushed() const NOEXCEPT
{
    LOG_DEBUG(LOG_DATABASE)
        << "Flushed: " << filename_ << " [" << logical_size_ << "]";
}

void file_storage::log_unmapping() const NOEXCEPT
{
    LOG_DEBUG(LOG_DATABASE)
        << "Unmapping: " << filename_ << " [" << logical_size_ << "]";
}

void file_storage::log_unmapped() const NOEXCEPT
{
    LOG_DEBUG(LOG_DATABASE)
        << "Unmapped: " << filename_ << " [" << logical_size_ << ", "
        << capacity_ << "]";
}

BC_POP_WARNING()

file_storage::file_storage(const path& filename) NOEXCEPT
  : file_storage(filename, default_capacity, default_expansion)
{
}

// mmap documentation: tinyurl.com/hnbw8t5
file_storage::file_storage(const path& filename, size_t minimum,
    size_t expansion) NOEXCEPT
  : file_handle_(open_file(filename)),
    minimum_(minimum),
    expansion_(expansion),
    filename_(filename),
    closed_(true),
    data_(nullptr),
    capacity_(file_size(file_handle_)),
    logical_size_(capacity_)
{
}

// Database threads must be joined before close is called (or destruct).
file_storage::~file_storage() NOEXCEPT
{
    close();
}

// Startup and shutdown.
// ----------------------------------------------------------------------------

// mutex_ methods may throw boost::lock_error, terminating the process.
BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

// Map the database file and signal start.
// Open is not idempotent (should be called on single thread).
bool file_storage::open() NOEXCEPT
{
    // Critical Section
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
    // For unknown reason madvise(minimum_) with large value fails on linux.
    if (!map(capacity_))
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

bool file_storage::flush() const NOEXCEPT
{
    std::string error_name;

    // Critical Section
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
bool file_storage::close() NOEXCEPT
{
    std::string error_name;

    ////if (!closed_)
    ////    log_unmapping();

    // Critical Section
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

    if (logical_size_ > capacity_)
        error_name = "fit";
    else if (msync(data_, logical_size_, MS_SYNC) == FAIL)
        error_name = "msync";
    else if (munmap(data_, capacity_) == FAIL)
        error_name = "munmap";
    else if (ftruncate(file_handle_, logical_size_) == FAIL)
        error_name = "ftruncate";
    else if (fsync(file_handle_) == FAIL)
        error_name = "fsync";
    else if (close_file(file_handle_) == FAIL)
        error_name = "close";

    mutex_.unlock();
    ///////////////////////////////////////////////////////////////////////////

    // Keep logging out of the critical section.
    if (!error_name.empty())
        return handle_error(error_name, filename_);

    log_unmapped();
    return true;
}

bool file_storage::closed() const NOEXCEPT
{
    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    std::shared_lock lock(mutex_);
    return closed_;
    ///////////////////////////////////////////////////////////////////////////
}

// Operations.
// ----------------------------------------------------------------------------

size_t file_storage::capacity() const NOEXCEPT
{
    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    std::shared_lock lock(mutex_);
    return capacity_;
    ///////////////////////////////////////////////////////////////////////////
}

size_t file_storage::logical() const NOEXCEPT
{
    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    std::shared_lock lock(mutex_);
    return logical_size_;
    ///////////////////////////////////////////////////////////////////////////
}

BC_POP_WARNING()

memory_ptr file_storage::access() NOEXCEPT(false)
{
    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    auto memory = std::make_shared<accessor>(mutex_);

    memory->assign(data_);

    // The store should only have been closed after all threads terminated.
    if (closed_)
        throw runtime_exception("Access failure, store closed.");

    return memory;
}

// Throws runtime_exception if insufficient space.
memory_ptr file_storage::resize(size_t required) NOEXCEPT(false)
{
    return reserve(required, 0, 0);
}

// Throws runtime_exception if insufficient space.
memory_ptr file_storage::reserve(size_t required) NOEXCEPT(false)
{
    return reserve(required, minimum_, expansion_);
}

// Throws runtime_exception if insufficient space.
// There is no way to gracefully recover from a resize failure because there
// are integrity relationships across multiple database files. Stopping a write
// in one would require rolling back preceding write operations in others.
memory_ptr file_storage::reserve(size_t required, size_t minimum,
    size_t expansion) NOEXCEPT(false)
{
    // Internally preventing resize during close is not possible because of
    // cross-file integrity. So we must coalesce all threads before closing.

    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    auto memory = std::make_shared<accessor>(mutex_);

    // The store should only have been closed after all threads terminated.
    if (closed_)
    {
        memory->assign(data_);
        throw runtime_exception("Resize failure, store already closed.");
    }

    if (required > capacity_)
    {
        const auto resize = static_cast<size_t>(required * 
            ((expansion + 100.0) / 100.0));

        // Expansion is an integral number that represents a real number factor.
        const size_t target = std::max(minimum, resize);

        mutex_.unlock_upgrade_and_lock();
        //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

        // TODO: isolate cause and if recoverable (disk size) return nullptr.
        // All existing database pointers are invalidated by this call.
        if (!truncate_mapped(target))
        {
            handle_error("resize", filename_);
            throw runtime_exception("Resize failure, disk space may be low.");
        }

        //---------------------------------------------------------------------
        mutex_.unlock_and_lock_upgrade();
    }

    logical_size_ = required;
    memory->assign(data_);

    // Always return in shared lock state.
    // The critical section does not end until this shared pointer is freed.
    return memory;
    ///////////////////////////////////////////////////////////////////////////
}

// privates
// ----------------------------------------------------------------------------

size_t file_storage::page() const NOEXCEPT
{
#if defined(HAVE_MSC)
    SYSTEM_INFO configuration;
    GetSystemInfo(&configuration);
    return configuration.dwPageSize;
#else
    errno = 0;
    const auto page_size = sysconf(_SC_PAGESIZE);

    // -1 is both a return code and a potentially valid value, so use errno.
    if (errno != 0)
        handle_error("sysconf", filename_);

    // Check for negative value so that later we can promote to unsigned
    if (page_size == -1)
        return 0;

    BC_ASSERT(sign_cast<uint64_t>(page_size) <= max_size_t);
    return static_cast<size_t>(page_size);
#endif
}

bool file_storage::unmap() NOEXCEPT
{
    const auto success = (munmap(data_, capacity_) != FAIL);
    capacity_ = zero;
    data_ = nullptr;
    return success;
}

bool file_storage::map(size_t size) NOEXCEPT
{
    if (is_zero(size))
        return false;

    data_ = pointer_cast<uint8_t>(mmap(nullptr, size, PROT_READ | PROT_WRITE,
        MAP_SHARED, file_handle_, 0));

    return validate(size);
}

bool file_storage::remap(size_t size) NOEXCEPT
{
#ifdef MREMAP_MAYMOVE
    data_ = pointer_cast<uint8_t>(mremap(data_, capacity_, size,
        MREMAP_MAYMOVE));

    return validate(size);
#else
    return unmap() && map(size);
#endif
}

bool file_storage::truncate(size_t size) NOEXCEPT
{
    return ftruncate(file_handle_, size) != FAIL;
}

bool file_storage::truncate_mapped(size_t size) NOEXCEPT
{
    log_resizing(size);

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
}

bool file_storage::validate(size_t size) NOEXCEPT
{
    if (data_ == MAP_FAILED)
    {
        capacity_ = zero;
        data_ = nullptr;
        return false;
    }

    capacity_ = size;
    return true;
}

} // namespace database
} // namespace libbitcoin
