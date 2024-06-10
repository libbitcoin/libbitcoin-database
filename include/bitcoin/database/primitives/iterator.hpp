/**
 * Copyright (c) 2011-2023 libbitcoin developers (see AUTHORS)
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
#ifndef LIBBITCOIN_DATABASE_PRIMITIVES_ITERATOR_HPP
#define LIBBITCOIN_DATABASE_PRIMITIVES_ITERATOR_HPP

#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/memory.hpp>

namespace libbitcoin {
namespace database {

/// THIS HOLDS A memory_ptr WHICH HOLDS A SHARED REMAP LOCK. IT SHOULD NOT BE
/// HELD WHILE THE HOLDING CODE EXECUTES READS AGAINST THE SAME TABLE.
/// OTHERWISE A DEADLOCK WILL OCCUR WHEN THE TABLE'S FILE IS EXPANDED, WHICH
/// WAITS ON THE RELEASE OF THE SHARED LOCK (REMAP REQUIRES EXCLUSIVE ACCESS).
/// THE hashmap.get(const iterator& it, ...) METHOD EXISTS TO PREVENT A CALL TO
/// manager.get(), WHICH DESPITE BEING A READ WOULD CAUSE A DEADLOCK. THIS IS
/// BECAUSE IT CANNOT COMPLETE ITS READ WHILE REMAP IS WAITING ON ACCESS.

/// This class is not thread safe.
/// Size non-max implies record manager (ordinal record links).
template <typename Link, typename Key, size_t Size = max_size_t>
class iterator
{
public:
    DEFAULT_COPY_MOVE_DESTRUCT(iterator);

    static constexpr size_t link_to_position(const Link& link) NOEXCEPT;

    /// This advances to first match (or terminal).
    /// Key must be passed as an l-value as it is held by reference.
    INLINE iterator(const memory_ptr& data, const Link& start,
        const Key& key) NOEXCEPT;

    /// Advance to and return next iterator.
    INLINE bool advance() NOEXCEPT;

    /// Advance to next match and return false if terminal (not found).
    INLINE const Link& self() const NOEXCEPT;

    /// Access the underlying memory pointer.
    // TODO: for use by hashmap, make exclusive via friend.
    INLINE const memory_ptr& get() const NOEXCEPT;

protected:
    INLINE bool is_match() const NOEXCEPT;
    INLINE Link get_next() const NOEXCEPT;

private:
    static constexpr auto is_slab = (Size == max_size_t);

    // This is not thread safe, but it's object is not modified here and the
    // memory that it refers to is not addressable until written, and writes
    // are guarded by allocator, which is protected by mutex.
    const memory_ptr memory_;

    // This is thread safe.
    const Key& key_;

    // This is not thread safe.
    Link link_;
};

} // namespace database
} // namespace libbitcoin

#define TEMPLATE \
template <typename Link, typename Key, size_t Size>
#define CLASS iterator<Link, Key, Size>

#include <bitcoin/database/impl/primitives/iterator.ipp>

#undef CLASS
#undef TEMPLATE

#endif
