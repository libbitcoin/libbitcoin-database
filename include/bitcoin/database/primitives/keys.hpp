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

#include <bitcoin/database.hpp>

namespace libbitcoin {
namespace database {
namespace keys {

/// en.wikipedia.org/wiki/Fowler–Noll–Vo_hash_function
constexpr uint64_t fnv1a_combine(uint64_t left, uint64_t right);

/// Key size in bytes.
template <class Key>
constexpr size_t size() NOEXCEPT;

/// Hash of Key for hashmap bucket selection.
template <class Key>
inline uint64_t hash(const Key& value) NOEXCEPT;

/// Hash of Key for hashmap filter entropy.
template <class Key>
inline uint64_t thumb(const Key& value) NOEXCEPT;

/// Write size() bytes of key to current sink location.
template <class Key>
inline void write(writer& sink, const Key& key) NOEXCEPT;

/// Compare size() bytes of key to bytes.
template <class Array, class Key>
inline bool compare(const Array& bytes, const Key& key) NOEXCEPT;

} // namespace keys
} // namespace database
} // namespace libbitcoin

#include <bitcoin/database/impl/primitives/keys.ipp>

#endif
