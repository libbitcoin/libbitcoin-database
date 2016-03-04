/**
 * Copyright (c) 2011-2015 libbitcoin developers (see AUTHORS)
 *
 * This file is part of libbitcoin.
 *
 * libbitcoin is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License with
 * additional permissions to the one published by the Free Software
 * Foundation, either version 3 of the License, or (at your option)
 * any later version. For more information see LICENSE.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#include <bitcoin/database/memory/memory_map.hpp>

#include <iostream>

#ifdef _WIN32
    #include <io.h>
    #include "../mman-win32/mman.h"
    #define FILE_OPEN_PERMISSIONS _S_IREAD | _S_IWRITE
#else
    #include <unistd.h>
    #include <stddef.h>
    #include <sys/mman.h>
    #define FILE_OPEN_PERMISSIONS S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH
#endif
#include <cstddef>
#include <cstdint>
#include <fcntl.h>
#include <memory>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/memory/accessor.hpp>
#include <bitcoin/database/memory/allocator.hpp>
#include <bitcoin/database/memory/memory.hpp>

// memory_map is be able to support 32 bit but because the database 
// requires a larger file this is not validated or supported.
static_assert(sizeof(void*) == sizeof(uint64_t), "Not a 64 bit system!");

namespace libbitcoin {
namespace database {

using boost::format;
using boost::filesystem::path;

#define EXPANSION_NUMERATOR 150
#define EXPANSION_DENOMINATOR 100

size_t memory_map::file_size(int file_handle)
{
    if (file_handle == -1)
        return 0;

    // This is required because off_t is defined as long, whcih is 32 bits in
    // msvc and 64 bits in linux/osx, and stat contains off_t.
#ifdef _WIN32
#ifdef _WIN64
    struct _stat64 sbuf;
    if (_fstat64(file_handle, &sbuf) == -1)
        return 0;
#else
    struct _stat32 sbuf;
    if (_fstat32(file_handle, &sbuf) == -1)
        return 0;
#endif
#else
    struct stat sbuf;
    if (fstat(file_handle, &sbuf) == -1)
        return 0;
#endif

    // Convert signed to unsigned size.
    BITCOIN_ASSERT_MSG(sbuf.st_size > 0, "File size cannot be 0 bytes.");
    return static_cast<size_t>(sbuf.st_size);
}

int memory_map::open_file(const path& filename)
{
#ifdef _WIN32
    int handle = _wopen(filename.wstring().c_str(), O_RDWR,
        FILE_OPEN_PERMISSIONS);
#else
    int handle = open(filename.string().c_str(), O_RDWR,
        FILE_OPEN_PERMISSIONS);
#endif
    return handle;
}

bool memory_map::handle_error(const char* context, const path& filename)
{
    static const auto form = "The file failed to %1%: %2% error: %3%";
#ifdef _WIN32
    const auto error = GetLastError();
#else
    const auto error = errno;
#endif
    const auto message = format(form) % context % filename % error;
    log::fatal(LOG_DATABASE) << message.str();
    return false;
}

memory_map::memory_map(const path& filename)
  : filename_(filename),
    stopped_(false),
    data_(nullptr),
    file_handle_(open_file(filename_)),
    file_size_(file_size(file_handle_)),
    logical_size_(file_size_)
{
    // This initializes data_.
    stopped_ = !map(file_size_);

    if (stopped_)
        handle_error("map", filename_);
    else
        log::info(LOG_DATABASE) << "Mapping: " << filename_;
}

memory_map::~memory_map()
{
    stop();
}

bool memory_map::stopped() const
{
    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    REMAP_READ(mutex_);

    return stopped_;
    ///////////////////////////////////////////////////////////////////////////
}

bool memory_map::stop()
{
    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    REMAP_WRITE(mutex_);

    if (stopped_)
        return true;

    stopped_ = true;
    log::info(LOG_DATABASE) << "Unmapping: " << filename_;

    if (munmap(data_, file_size_) == -1)
        return handle_error("munmap", filename_);

    if (ftruncate(file_handle_, logical_size_) == -1)
        return handle_error("ftruncate", filename_);

    if (fsync(file_handle_) == -1)
        return handle_error("fsync", filename_);

    if (close(file_handle_) == -1)
        return handle_error("close", filename_);

    return true;
    ///////////////////////////////////////////////////////////////////////////
}

size_t memory_map::size() const
{
    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    REMAP_READ(mutex_);

    return file_size_;
    ///////////////////////////////////////////////////////////////////////////
}

memory_ptr memory_map::access()
{
    return REMAP_ACCESSOR(data_, mutex_);
}

memory_ptr memory_map::resize(size_t size)
{
    return reserve(size, EXPANSION_DENOMINATOR);
}

memory_ptr memory_map::reserve(size_t size)
{
    return reserve(size, EXPANSION_NUMERATOR);
}

memory_ptr memory_map::reserve(size_t size, size_t expansion)
{
    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    const auto memory = REMAP_ALLOCATOR(mutex_);

    if (size > file_size_)
    {
        const auto new_size = size * expansion / EXPANSION_DENOMINATOR;

        if (!truncate(new_size))
            throw std::runtime_error("Resize failure, disk space may be low.");
    }

    logical_size_ = size;
    REMAP_DOWNGRADE(memory, data_);

    return memory;
    ///////////////////////////////////////////////////////////////////////////
}

// privates

bool memory_map::unmap()
{
    bool success = (munmap(data_, file_size_) != -1);
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
    data_ = reinterpret_cast<uint8_t*>(mremap(data_, size_, size,
        MREMAP_MAYMOVE));

    return validate(size);
#else
    return unmap() && map(size);
#endif
}

bool memory_map::truncate(size_t size)
{
#ifndef MREMAP_MAYMOVE
    if (!unmap())
        return false;
#endif

    const auto message = format("Resizing: %1% [%2%]") % filename_ % size;
    log::debug(LOG_DATABASE) << message.str();

    if (ftruncate(file_handle_, size) == -1)
    {
        handle_error("truncate", filename_);
        return false;
    }

#ifndef MREMAP_MAYMOVE
    return map(size);
#endif
    return remap(size);
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
