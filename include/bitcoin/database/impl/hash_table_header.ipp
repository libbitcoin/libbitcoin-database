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
#ifndef LIBBITCOIN_DATABASE_HASH_TABLE_HEADER_IPP
#define LIBBITCOIN_DATABASE_HASH_TABLE_HEADER_IPP

#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/memory/memory.hpp>
#include <bitcoin/database/memory/storage.hpp>

namespace libbitcoin {
namespace database {

template <typename Index, typename Link>
template <typename Key>
inline Index hash_table_header<Index, Link>::remainder(const Key& key,
    Index divisor)
{
    // TODO: implement std::hash replacement to prevent store drift.
    return divisor == 0 ? 0 : std::hash<Key>()(key) % divisor;
}

template <typename Index, typename Link>
hash_table_header<Index, Link>::hash_table_header(storage& file, Index buckets)
  : file_(file), buckets_(buckets)
{
    static_assert(std::is_unsigned<Link>::value,
        "Hash table header requires unsigned value type.");

    static_assert(std::is_unsigned<Index>::value,
        "Hash table header requires unsigned index type.");
}

template <typename Index, typename Link>
bool hash_table_header<Index, Link>::create()
{
    const auto file_size = size(buckets_);

    // The accessor must remain in scope until the end of the block.
    const auto memory = file_.resize(file_size);

    // Speed-optimized fill implementation.
    memset(memory->buffer(), (uint8_t)empty, file_size);

    // Overwrite the start of the buffer with the bucket count.
    auto serial = make_unsafe_serializer(memory->buffer());
    serial.template write_little_endian<Index>(buckets_);
    return true;
}

template <typename Index, typename Link>
bool hash_table_header<Index, Link>::start()
{
    // File is too small for the number of buckets in the header.
    if (file_.size() < link(buckets_))
        return false;

    // The accessor must remain in scope until the end of the block.
    const auto memory = file_.access();

    // Does not require atomicity (no concurrency during start).
    auto deserial = make_unsafe_deserializer(memory->buffer());
    return deserial.template read_little_endian<Index>() == buckets_;
}

template <typename Index, typename Link>
Link hash_table_header<Index, Link>::read(Index index) const
{
    BITCOIN_ASSERT(index < buckets_);

    // The accessor must remain in scope until the end of the block.
    const auto memory = file_.access();
    memory->increment(link(index));
    auto deserial = make_unsafe_deserializer(memory->buffer());

    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    shared_lock lock(mutex_);
    return deserial.template read_little_endian<Link>();
    ///////////////////////////////////////////////////////////////////////////
}

template <typename Index, typename Link>
void hash_table_header<Index, Link>::write(Index index, Link value)
{
    BITCOIN_ASSERT(index < buckets_);

    // The accessor must remain in scope until the end of the block.
    const auto memory = file_.access();
    memory->increment(link(index));
    auto serial = make_unsafe_serializer(memory->buffer());

    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    unique_lock lock(mutex_);
    serial.template write_little_endian<Link>(value);
    ///////////////////////////////////////////////////////////////////////////
}

template <typename Index, typename Link>
Index hash_table_header<Index, Link>::buckets() const
{
    return buckets_;
}

template <typename Index, typename Link>
size_t hash_table_header<Index, Link>::size()
{
    return size(buckets_);
}

// static
template <typename Index, typename Link>
size_t hash_table_header<Index, Link>::size(Index buckets)
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

// static
template <typename Index, typename Link>
file_offset hash_table_header<Index, Link>::link(Index index)
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
