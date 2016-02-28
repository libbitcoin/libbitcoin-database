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
#ifndef LIBBITCOIN_DATABASE_HTDB_RECORD_LIST_ITEM_IPP
#define LIBBITCOIN_DATABASE_HTDB_RECORD_LIST_ITEM_IPP

namespace libbitcoin {
namespace database {

/**
 * Item for htdb_record. A chained list with the key included.
 *
 * Stores the key, next index and user data.
 * With the starting item, we can iterate until the end using the
 * next_index() method.
 */
template <typename HashType>
class htdb_record_list_item
{
public:
    static BC_CONSTEXPR size_t index_size = sizeof(array_index);
    static BC_CONSTEXPR size_t hash_size = std::tuple_size<HashType>::value;
    static BC_CONSTEXPR file_offset value_begin = hash_size + index_size;

    htdb_record_list_item(record_manager& allocator, array_index index);

    array_index create(const HashType& key, const array_index next);

    // Does this match?
    bool compare(const HashType& key) const;

    // The actual user data.
    uint8_t* data1() const;

    // Position of next item in the chained list.
    array_index next_index() const;

    // Write a new next index.
    void write_next_index(array_index next);

private:
    uint8_t* raw_next_data() const;
    uint8_t* raw_data(file_offset offset) const;

    array_index index_;
    record_manager& manager_;
};

template <typename HashType>
htdb_record_list_item<HashType>::htdb_record_list_item(record_manager& manager,
    const array_index index)
  : manager_(manager), index_(index)
{
    static_assert(index_size == 4, "Invalid array_index size.");
}

template <typename HashType>
array_index htdb_record_list_item<HashType>::create(const HashType& key,
    const array_index next)
{
    // Create new record.
    //   [ HashType ]
    //   [ next:4   ]
    //   [ value... ]
    index_ = manager_.new_record();

    // Write record.
    auto serial = make_serializer(raw_data(0));
    serial.write_data(key);

    // MUST BE ATOMIC
    serial.write_4_bytes_little_endian(next);
    return index_;
}

template <typename HashType>
bool htdb_record_list_item<HashType>::compare(const HashType& key) const
{
    // Key data is at the start.
    const auto key_data = raw_data(0);
    return std::equal(key.begin(), key.end(), key_data);
}

template <typename HashType>
uint8_t* htdb_record_list_item<HashType>::data1() const
{
    // Value data is at the end.
    return raw_data(value_begin);
}

template <typename HashType>
array_index htdb_record_list_item<HashType>::next_index() const
{
    const auto next_data = raw_next_data();
    return from_little_endian_unsafe<array_index>(next_data);
}

template <typename HashType>
void htdb_record_list_item<HashType>::write_next_index(array_index next)
{
    const auto next_data = raw_next_data();
    auto serial = make_serializer(next_data);

    // MUST BE ATOMIC
    serial.write_4_bytes_little_endian(next);
}

template <typename HashType>
uint8_t* htdb_record_list_item<HashType>::raw_data(file_offset offset) const
{
    return manager_.get_record(index_) + offset;
}

template <typename HashType>
uint8_t* htdb_record_list_item<HashType>::raw_next_data() const
{
    // Next position is after key data.
    return raw_data(hash_size);
}

} // namespace database
} // namespace libbitcoin

#endif
