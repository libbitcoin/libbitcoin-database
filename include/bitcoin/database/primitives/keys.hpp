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
#ifndef LIBBITCOIN_DATABASE_PRIMITIVES_KEYS_HPP
#define LIBBITCOIN_DATABASE_PRIMITIVES_KEYS_HPP

#include <algorithm>
#include <bitcoin/system.hpp>

namespace libbitcoin {
namespace database {
namespace keys {

template <class Key>
constexpr size_t size() NOEXCEPT
{
    if constexpr (is_same_type<Key, system::chain::point>)
    {
        // Index is truncated to three bytes.
        return sub1(system::chain::point::serialized_size());
    }
    else if constexpr (is_std_array<Key>)
    {
        return array_count<Key>;
    }
}

template <class Key>
inline uint64_t hash(const Key& value) NOEXCEPT
{
    if constexpr (is_same_type<Key, system::chain::point>)
    {
        // std::hash<system::chain::point> implemented in system.
        return std::hash<system::chain::point>()(value);
    }
    else if constexpr (is_std_array<Key>)
    {
        // Assumes sufficient uniqueness in low order bytes (ok for all).
        // sequentially-valued keys should have no more buckets than values.
        constexpr auto key = size<Key>();
        constexpr auto bytes = std::min(key, sizeof(uint64_t));

        uint64_t hash{};
        std::copy_n(value.begin(), bytes, system::byte_cast(hash).begin());
        return hash;
    }
}

template <class Key>
inline uint64_t thumb(const Key& value) NOEXCEPT
{

    if constexpr (is_same_type<Key, system::chain::point>)
    {
        // Ignores index as that is already hashed in primary hash.
        return thumb(value.hash());
    }
    else if constexpr (is_std_array<Key>)
    {
        // Assumes sufficient uniqueness in second-low order bytes (ok for all).
        // The thumb(value) starts at the next byte following hash(value).
        // If key is to short then thumb aligns to the end of the key.
        // This is intended to minimize overlap to the extent possible, while
        // also avoiding the high order bits in the case of block hashes.
        constexpr auto key = size<Key>();
        constexpr auto bytes = std::min(key, sizeof(uint64_t));
        constexpr auto offset = std::min(key - bytes, bytes);

        uint64_t hash{};
        const auto start = std::next(value.begin(), offset);
        std::copy_n(start, bytes, system::byte_cast(hash).begin());
        return hash;
    }
}

template <class Key>
inline void write(writer& sink, const Key& key) NOEXCEPT
{
    if constexpr (is_same_type<Key, system::chain::point>)
    {
        sink.write_bytes(key.hash());
        sink.write_3_bytes_little_endian(key.index());
    }
    else if constexpr (is_std_array<Key>)
    {
        sink.write_bytes(key);
    }
}

template <class Array, class Key>
inline bool compare(const Array& bytes, const Key& key) NOEXCEPT
{
    using namespace system;
    if constexpr (is_same_type<Key, chain::point>)
    {
        // Index is truncated to three bytes.
        const auto index = key.index();
        return compare(array_cast<uint8_t, hash_size>(bytes), key.hash())
            && bytes.at(hash_size + 0) == byte<0>(index)
            && bytes.at(hash_size + 1) == byte<1>(index)
            && bytes.at(hash_size + 2) == byte<2>(index);
    }
    else if constexpr (is_std_array<Key>)
    {
        return std::equal(bytes.begin(), bytes.end(), key.begin());
    }
}

} // namespace keys
} // namespace database
} // namespace libbitcoin

#endif
