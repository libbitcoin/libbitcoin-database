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
#ifndef LIBBITCOIN_DATABASE_HASH_TABLE_HEADER_IPP
#define LIBBITCOIN_DATABASE_HASH_TABLE_HEADER_IPP

#include <cstring>
#include <functional>
#include <shared_mutex>
#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/memory.hpp>
#include <bitcoin/database/memory/storage.hpp>

namespace libbitcoin {
namespace database {

TEMPLATE
CLASS::hash_table_header(storage& file, Index buckets) NOEXCEPT
  : file_(file), buckets_(buckets)
{
}

TEMPLATE
bool CLASS::create() NOEXCEPT
{
    constexpr auto fill = system::narrow_cast<uint8_t>(empty);
    const auto file_size = size(buckets_);

    // The accessor must remain in scope until the end of the block.
    const auto memory = file_.resize(file_size);

    // Speed-optimized fill implementation.
    std::memset(memory->buffer(), fill, file_size);

    // Overwrite the start of the buffer with the bucket count.
    system::unsafe_to_little_endian<Index>(memory->buffer(), buckets_);
    return true;
}

TEMPLATE
bool CLASS::start() NOEXCEPT
{
    // File is too small for the number of buckets in the header.
    if (file_.capacity() < link(buckets_))
        return false;

    // The accessor must remain in scope until the end of the block.
    const auto memory = file_.access();

    // Does not require atomicity (no concurrency during start).
    return system::unsafe_from_little_endian<Index>(memory->buffer()) == buckets_;
}

TEMPLATE
Link CLASS::read(Index index) const NOEXCEPT
{
    BC_ASSERT(index < buckets_);

    // The accessor must remain in scope until the end of the block.
    const auto memory = file_.access();
    memory->increment(link(index));

    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    std::shared_lock lock(mutex_);
    return system::unsafe_from_little_endian<Link>(memory->buffer());
    ///////////////////////////////////////////////////////////////////////////
}

TEMPLATE
void CLASS::write(Index index, Link value) NOEXCEPT
{
    BC_ASSERT(index < buckets_);

    // The accessor must remain in scope until the end of the block.
    const auto memory = file_.access();
    memory->increment(link(index));

    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    std::unique_lock lock(mutex_);
    system::unsafe_to_little_endian<Link>(memory->buffer(), value);
    ///////////////////////////////////////////////////////////////////////////
}

TEMPLATE
Index CLASS::buckets() const NOEXCEPT
{
    return buckets_;
}

TEMPLATE
size_t CLASS::size() NOEXCEPT
{
    return size(buckets_);
}

// static
TEMPLATE
size_t CLASS::size(Index buckets) NOEXCEPT
{
    // Header byte size is file link of last bucket + 1:
    //
    //  [  size:buckets        ]
    //  [ [ row[0]           ] ]
    //  [ [      ...         ] ]
    //  [ [ row[buckets - 1] ] ] <=
    //
    return link(buckets);
}

// static, private
TEMPLATE
file_offset CLASS::link(Index index) NOEXCEPT
{
    // File link of indexed bucket is:
    //
    //     [  size       :Index  ]
    //     [ [ row[0]    :Link ] ]
    //     [ [      ...        ] ]
    //  => [ [ row[index]:Link ] ]
    //
    return sizeof(Index) + index * sizeof(Link);
}

} // namespace database
} // namespace libbitcoin

#endif
