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
#ifndef LIBBITCOIN_DATABASE_DISK_ARRAY_IPP
#define LIBBITCOIN_DATABASE_DISK_ARRAY_IPP

#include <bitcoin/bitcoin.hpp>

namespace libbitcoin {
namespace database {

template <typename IndexType, typename ValueType>
disk_array<IndexType, ValueType>::disk_array(mmfile& file,
    file_offset sector_start)
  : file_(file), size_(0), sector_start_(sector_start)
{
    static_assert(std::is_unsigned<ValueType>::value,
        "disk_array only works with unsigned types");
}

template <typename IndexType, typename ValueType>
void disk_array<IndexType, ValueType>::create(IndexType size)
{
    // Calculate the minimum file size.
    const auto minimum_file_size = sector_start_ + item_position(size);

    // The writer must remain in scope until the end of the block.
    auto allocated = file_.allocate(minimum_file_size);
    const auto write_position = allocated->buffer() + sector_start_;

    // MUST BE ATOMIC
    auto serial = make_serializer(write_position);
    serial.write_little_endian(size);

    for (IndexType index = 0; index < size; ++index)
        serial.write_little_endian(empty);
}

template <typename IndexType, typename ValueType>
void disk_array<IndexType, ValueType>::start()
{
    BITCOIN_ASSERT(sizeof(IndexType) <= file_.size());

    // The reader must remain in scope until the end of the block.
    const auto reader = file_.access();
    const auto read_position = reader->buffer();

    size_ = from_little_endian_unsafe<IndexType>(read_position);
}

template <typename IndexType, typename ValueType>
ValueType disk_array<IndexType, ValueType>::read(IndexType index) const
{
    BITCOIN_ASSERT_MSG(size_ != 0, "disk_array::start() wasn't called.");
    BITCOIN_ASSERT(index < size_);

    // Find the item in the file.
    const auto offset = sector_start_ + item_position(index);

    // The reader must remain in scope until the end of the block.
    const auto reader = file_.access();
    const auto read_position = reader->buffer() + offset;

    // Deserialize value.
    return from_little_endian_unsafe<ValueType>(read_position);
}

template <typename IndexType, typename ValueType>
void disk_array<IndexType, ValueType>::write(IndexType index, ValueType value)
{
    BITCOIN_ASSERT_MSG(size_ > 0, "disk_array::start() wasn't called.");
    BITCOIN_ASSERT(index < size_);

    // Find the item in the file.
    const auto position = sector_start_ + item_position(index);

    // Calculate the minimum file size.
    const auto minimum_file_size = position + sizeof(value);

    // The writer must remain in scope until the end of the block.
    auto allocated = file_.allocate(minimum_file_size);
    auto write_position = allocated->buffer() + position;

    // MUST BE ATOMIC
    auto serial = make_serializer(write_position);
    serial.write_little_endian(value);
}

template <typename IndexType, typename ValueType>
IndexType disk_array<IndexType, ValueType>::size() const
{
    return size_;
}

template <typename IndexType, typename ValueType>
file_offset disk_array<IndexType, ValueType>::item_position(
    IndexType index) const
{
    return sizeof(IndexType) + index * sizeof(ValueType);
}

} // namespace database
} // namespace libbitcoin

#endif
