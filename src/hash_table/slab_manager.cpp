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
#include <bitcoin/database/hash_table/slab_manager.hpp>

#include <cstddef>
#include <boost/thread.hpp>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/memory/memory.hpp>
#include <bitcoin/database/memory/memory_map.hpp>

namespace libbitcoin {
namespace database {

slab_manager::slab_manager(memory_map& file, file_offset sector_start)
  : file_(file), start_(sector_start), size_(0)
{
}

// This method is not thread safe.
// Write the byte size of the allocated space to the file.
void slab_manager::create()
{
    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    boost::shared_lock<boost::shared_mutex> unique_lock(mutex_);

    size_.store(sizeof(file_offset));
    write_size();
    ///////////////////////////////////////////////////////////////////////////
}

void slab_manager::start()
{
    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    boost::shared_lock<boost::shared_mutex> unique_lock(mutex_);

    read_size();
    ///////////////////////////////////////////////////////////////////////////
}

void slab_manager::sync()
{
    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    boost::unique_lock<boost::shared_mutex> unique_lock(mutex_);

    write_size();
    ///////////////////////////////////////////////////////////////////////////
}

// new record allocation
file_offset slab_manager::new_slab(size_t bytes_needed)
{
    BITCOIN_ASSERT_MSG(size_ > 0, "slab_manager::start() wasn't called.");

    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    boost::unique_lock<boost::shared_mutex> unique_lock(mutex_);

    const auto slab_position = size_.load();
    reserve(bytes_needed);
    return slab_position;
    ///////////////////////////////////////////////////////////////////////////
}

const memory::ptr slab_manager::get(file_offset position) const
{
    BITCOIN_ASSERT_MSG(size_ > 0, "slab_manager::start() wasn't called.");
    BITCOIN_ASSERT(position < size_);

    const auto reader = file_.access();
    reader->increment(start_ + position);
    return reader;
}

// privates

void slab_manager::reserve(size_t bytes_needed)
{
    const size_t required_size = start_ + size_ + bytes_needed;
    file_.resize(required_size);
    size_ += bytes_needed;
}

// Read the size value from the first chunk of the file.
void slab_manager::read_size()
{
    BITCOIN_ASSERT(start_ + sizeof(file_offset) <= file_.size());

    // The reader must remain in scope until the end of the block.
    const auto reader = file_.access();
    const auto read_position = reader->buffer() + start_;

    const auto size = from_little_endian_unsafe<file_offset>(read_position);
    size_.store(size);
}

// Write the size value to the first chunk of the file.
void slab_manager::write_size()
{
    BITCOIN_ASSERT(start_ + sizeof(file_offset) <= file_.size());

    const auto minimum_file_size = start_ + sizeof(file_offset);

    // The writer must remain in scope until the end of the block.
    auto allocated = file_.allocate(minimum_file_size);
    auto write_position = allocated->buffer() + start_;

    auto serial = make_serializer(write_position);
    serial.write_little_endian(size_.load());
}

} // namespace database
} // namespace libbitcoin
