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

constexpr uint64_t fnv1a_combine(uint64_t left, uint64_t right)
{
    constexpr uint64_t fnv_prime = 0x100000001b3;
    constexpr uint64_t fnv_offset = 0xcbf29ce484222325;

    auto hash = fnv_offset;
    hash ^= left;
    hash *= fnv_prime;
    hash ^= right;
    hash *= fnv_prime;
    return hash;
}

template <class Key>
constexpr size_t size() NOEXCEPT
{
    using namespace system;
    if constexpr (is_same_type<Key, chain::point>)
    {
        // Index is truncated to three bytes.
        return sub1(chain::point::serialized_size());
    }
    else if constexpr (is_std_array<Key>)
    {
        return array_count<Key>;
    }
}

template <class Key>
inline uint64_t hash(const Key& value) NOEXCEPT
{
    using namespace system;
    if constexpr (is_same_type<Key, chain::point>)
    {
        // Simplify the null point test for performance.
        // Ensure bucket zero if and only if coinbase tx.
        if (value.index() == chain::point::null_index)
            return zero;

        // Simple combine is sufficient for bucket selection.
        // Given the uniformity of sha256 this produces a Poisson distribution.
        const auto result = bit_xor(hash(value.hash()),
            shift_left<uint64_t>(value.index()));

        // Bump any zero hash result into bucket one to avoid coinbase bucket.
        return is_zero(result) ? one : result;
    }
    else if constexpr (is_std_array<Key>)
    {
        // Assumes sufficient uniqueness in low order bytes (ok for all).
        // sequentially-valued keys should have no more buckets than values.
        constexpr auto key = size<Key>();
        constexpr auto bytes = std::min(key, sizeof(uint64_t));

        uint64_t hash{};
        std::copy_n(value.begin(), bytes, byte_cast(hash).begin());
        return hash;
    }
}

template <class Key>
inline uint64_t thumb(const Key& value) NOEXCEPT
{
    using namespace system;
    if constexpr (is_same_type<Key, system::chain::point>)
    {
        // spread point.index across point.hash extraction, as otherwise the
        // unlikely bucket collisions of points of the same hash will not be
        // differentiated by filters. This has a very small impact on false
        // positives (-5,789 out of ~2.6B) but w/o material computational cost.
        return fnv1a_combine(thumb(value.hash()), value.index());
        ////return thumb(value.hash());
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
        std::copy_n(start, bytes, byte_cast(hash).begin());
        return hash;
    }
}

template <class Key>
inline void write(writer& sink, const Key& key) NOEXCEPT
{
    using namespace system;
    if constexpr (is_same_type<Key, chain::point>)
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
