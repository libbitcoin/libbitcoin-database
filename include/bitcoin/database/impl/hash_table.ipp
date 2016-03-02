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
#ifndef LIBBITCOIN_DATABASE_MEMORY_ARRAY_IPP
#define LIBBITCOIN_DATABASE_MEMORY_ARRAY_IPP

#include <bitcoin/bitcoin.hpp>

namespace libbitcoin {
namespace database {

template <typename IndexType, typename ValueType>
hash_table<IndexType, ValueType>::hash_table(memory_map& file,
    file_offset header_size)
  : file_(file), size_(0), header_size_(header_size)
{
    static_assert(std::is_unsigned<ValueType>::value,
        "hash_table only works with unsigned types");
}

template <typename IndexType, typename ValueType>
void hash_table<IndexType, ValueType>::create(IndexType size)
{
    // Calculate the minimum file size.
    const auto minimum_file_size = header_size_ + item_position(size);

    // The memory object must remain in scope until the end of the block.
    const auto memory = file_.allocate(minimum_file_size);
    const auto size_address = memory->buffer() + header_size_;

    // MUST BE ATOMIC
    auto serial = make_serializer(size_address);
    serial.write_little_endian(size);

    for (IndexType index = 0; index < size; ++index)
        serial.write_little_endian(empty);
}

template <typename IndexType, typename ValueType>
void hash_table<IndexType, ValueType>::start()
{
    BITCOIN_ASSERT(sizeof(IndexType) <= file_.size());

    // The memory accessor remain in scope until the end of the block.
    const auto memory = file_.access();
    const auto size_address = memory->buffer();

    size_ = from_little_endian_unsafe<IndexType>(size_address);
}

template <typename IndexType, typename ValueType>
ValueType hash_table<IndexType, ValueType>::read(IndexType index) const
{
    BITCOIN_ASSERT_MSG(size_ != 0, "hash_table::start() wasn't called.");
    BITCOIN_ASSERT(index < size_);

    // Find the item in the file.
    const auto offset = header_size_ + item_position(index);

    // The memory accessor remain in scope until the end of the block.
    const auto memory = file_.access();
    const auto value_address = memory->buffer() + offset;

    // Deserialize value.
    return from_little_endian_unsafe<ValueType>(value_address);
}

template <typename IndexType, typename ValueType>
void hash_table<IndexType, ValueType>::write(IndexType index, ValueType value)
{
    BITCOIN_ASSERT_MSG(size_ > 0, "hash_table::start() wasn't called.");
    BITCOIN_ASSERT(index < size_);

    // Find the item in the file.
    const auto position = header_size_ + item_position(index);

    // Calculate the minimum file size.
    const auto minimum_file_size = position + sizeof(value);

    // The memory object must remain in scope until the end of the block.
    auto memory = file_.allocate(minimum_file_size);
    auto value_position = memory->buffer() + position;

    // MUST BE ATOMIC
    auto serial = make_serializer(value_position);
    serial.write_little_endian(value);
}

template <typename IndexType, typename ValueType>
IndexType hash_table<IndexType, ValueType>::size() const
{
    return size_;
}

template <typename IndexType, typename ValueType>
file_offset hash_table<IndexType, ValueType>::item_position(
    IndexType index) const
{
    return sizeof(IndexType) + index * sizeof(ValueType);
}

} // namespace database
} // namespace libbitcoin

#endif
