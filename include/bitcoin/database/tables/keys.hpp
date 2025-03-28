/**
 * Copyright (c) 2011-2025 libbitcoin developers (see AUTHORS)
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
#ifndef LIBBITCOIN_DATABASE_TABLES_KEYS_HPP
#define LIBBITCOIN_DATABASE_TABLES_KEYS_HPP

#include <algorithm>
#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {
namespace keys {

////template <typename Key>
////constexpr size_t size() NOEXCEPT
////{
////    if constexpr (is_same_type<Key, system::chain::point>)
////    {
////        // Index is truncated to three bytes.
////        return sub1(system::chain::point::serialized_size());
////    }
////    else if constexpr (is_std_array<Key>)
////    {
////        return array_count<Key>;
////    }
////}

////inline system::data_array<35> compose(const hash_digest& point_hash,
////    uint32_t point_index) NOEXCEPT
////{
////    // Index is truncated to three bytes.
////    using namespace system;
////    data_array<35> key{};
////    array_cast<uint8_t, hash_size>(key) = point_hash;
////    key.at(hash_size + 0) = byte<0>(point_index);
////    key.at(hash_size + 1) = byte<1>(point_index);
////    key.at(hash_size + 2) = byte<2>(point_index);
////    return key;
////}

////template <typename Key>
////inline size_t hash(const Key& value) NOEXCEPT
////{
////    if constexpr (is_same_type<Key, system::chain::point>)
////    {
////        // std::hash<system::chain::point> implemented in system.
////        // Use of unique_hash over the concat results in zero because index is
////        // concatenated to the high order bytes.
////        return std::hash<system::chain::point>()(value);
////    }
////    else if constexpr (is_std_array<Key>)
////    {
////        // unique_hash assumes sufficient uniqueness in low order key bytes.
////        return system::unique_hash(value);
////    }
////}
////
////template <typename Key>
////inline void write(writer& sink, const Key& key) NOEXCEPT
////{
////    if constexpr (is_same_type<Key, system::chain::point>)
////    {
////        sink.write_bytes(key.hash());
////        sink.write_3_bytes_little_endian(key.index());
////    }
////    else if constexpr (is_std_array<Key>)
////    {
////        sink.write_bytes(key);
////    }
////}
////
////template <typename Array, typename Key>
////inline bool compare(const Array& bytes, const Key& key) NOEXCEPT
////{
////    using namespace system;
////    if constexpr (is_same_type<Key, chain::point>)
////    {
////        // Index is truncated to three bytes.
////        const auto index = key.index();
////        return compare(array_cast<uint8_t, hash_size>(bytes), key.hash())
////            && bytes.at(hash_size + 0) == byte<0>(index)
////            && bytes.at(hash_size + 1) == byte<1>(index)
////            && bytes.at(hash_size + 2) == byte<2>(index);
////    }
////    else if constexpr (is_std_array<Key>)
////    {
////        return std::equal(bytes.begin(), bytes.end(), key.begin());
////    }
////}

} // namespace keys
} // namespace database
} // namespace libbitcoin

#endif
