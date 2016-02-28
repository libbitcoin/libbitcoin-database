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
#ifndef LIBBITCOIN_DATABASE_MMFILE_HPP
#define LIBBITCOIN_DATABASE_MMFILE_HPP

#ifndef _WIN32
#include <sys/mman.h>
#endif
#include <cstddef>
#include <cstdint>
#include <string>
#include <boost/filesystem.hpp>
#include <boost/thread.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/disk/accessor.hpp>
#include <bitcoin/database/disk/allocator.hpp>

namespace libbitcoin {
namespace database {

/// This class is thread safe, allowing concurent read and write.
/// A change to the size of the memory map waits on and locks read and write.
class BCD_API mmfile
{
public:
    mmfile(const boost::filesystem::path& filename);
    ~mmfile();

    /// This class is not copyable.
    mmfile(const mmfile&) = delete;
    void operator=(const mmfile&) = delete;

    /// These methods are thread safe.
    size_t size() const;
    void resize(size_t size);
    accessor::ptr access();
    allocator::ptr allocate(size_t size);
    bool stop();

private:
    static size_t file_size(int file_handle);
    static int open_file(const boost::filesystem::path& filename);
    static void handle_error(const char* context,
        const boost::filesystem::path& filename);

    bool map(size_t size);
#ifdef MREMAP_MAYMOVE
    bool remap(size_t new_size);
#endif
    bool unmap();
    bool reserve(size_t size);
    bool validate(size_t size);
    void upgrade(size_t size, allocator::ptr accessor);

    mutable boost::shared_mutex mutex_;
    const boost::filesystem::path filename_;
    const int file_handle_;

    // Protected by reader/writer locks.
    size_t size_;
    uint8_t* data_;
    bool stopped_;
};

} // namespace database
} // namespace libbitcoin

#endif
