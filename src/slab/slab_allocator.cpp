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
#include <bitcoin/database/slab/slab_allocator.hpp>

#include <cstddef>
#include <boost/thread.hpp>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/disk/mmfile.hpp>

namespace libbitcoin {
namespace database {

slab_allocator::slab_allocator(mmfile& file, file_offset sector_start)
  : file_(file), start_(sector_start), size_(0)
{
}

// This method is not thread safe.
// Write the byte size of the allocated space to the file.
void slab_allocator::create()
{
    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    boost::shared_lock<boost::shared_mutex> unique_lock(mutex_);

    size_.store(sizeof(file_offset));
    write_size();
    ///////////////////////////////////////////////////////////////////////////
}

void slab_allocator::start()
{
    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    boost::shared_lock<boost::shared_mutex> unique_lock(mutex_);

    read_size();
    ///////////////////////////////////////////////////////////////////////////
}

void slab_allocator::sync()
{
    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    boost::unique_lock<boost::shared_mutex> unique_lock(mutex_);

    write_size();
    ///////////////////////////////////////////////////////////////////////////
}

// new record allocation
file_offset slab_allocator::new_slab(size_t bytes_needed)
{
    BITCOIN_ASSERT_MSG(size_ > 0, "slab_allocator::start() wasn't called.");

    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    boost::unique_lock<boost::shared_mutex> unique_lock(mutex_);

    const auto slab_position = size_.load();
    reserve(bytes_needed);
    return slab_position;
    ///////////////////////////////////////////////////////////////////////////
}

// logical record access
const slab_byte_pointer slab_allocator::get_slab(file_offset position) const
{
    BITCOIN_ASSERT_MSG(size_ > 0, "slab_allocator::start() wasn't called.");
    BITCOIN_ASSERT(position < size_);

    const auto reader = file_.reader();
    const auto offset = start_ + position;
    const auto read_position = reader.buffer() + offset;
    return const_cast<uint8_t*>(read_position);
}

// retrieve minimum eof/memory boundary (ok to grow before use).
uint64_t slab_allocator::to_eof(slab_byte_pointer slab) const
{
    // Hold the reader in scope to prevent file.size change until complete.
    const auto reader = file_.reader();
    const auto buffer = reader.buffer();

    if (slab <= buffer)
        return 0;

    const auto size = file_.size();
    const ptrdiff_t offset = slab - buffer;
    const auto unsigned_offset = static_cast<size_t>(offset);
    const auto valid = 0 <= offset && unsigned_offset < size;
    return valid ? size - unsigned_offset : 0;
}

// privates

void slab_allocator::reserve(size_t bytes_needed)
{
    // See comment in hsdb_shard::reserve()
    const size_t required_size = start_ + size_ + bytes_needed;
    file_.resize(required_size);
    size_ += bytes_needed;
}

// Read the size value from the first chunk of the file.
void slab_allocator::read_size()
{
    BITCOIN_ASSERT(start_ + sizeof(file_offset) <= file_.size());

    const auto reader = file_.reader();
    const auto read_position = reader.buffer() + start_;
    size_.store(from_little_endian_unsafe<file_offset>(read_position));
}

// Write the size value to the first chunk of the file.
void slab_allocator::write_size()
{
    BITCOIN_ASSERT(start_ + sizeof(file_offset) <= file_.size());

    const auto minimum_file_size = start_ + sizeof(file_offset);
    auto writer = file_.writer(minimum_file_size);
    auto write_position = writer.buffer() + start_;
    auto serial = make_serializer(write_position);
    serial.write_little_endian(size_.load());
}

} // namespace database
} // namespace libbitcoin
