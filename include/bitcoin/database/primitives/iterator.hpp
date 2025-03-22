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
#ifndef LIBBITCOIN_DATABASE_PRIMITIVES_ITERATOR_HPP
#define LIBBITCOIN_DATABASE_PRIMITIVES_ITERATOR_HPP

#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/memory.hpp>
#include <bitcoin/database/primitives/keys.hpp>
#include <bitcoin/database/primitives/manager.hpp>

namespace libbitcoin {
namespace database {

/// THIS HOLDS A memory_ptr WHICH HOLDS A SHARED REMAP LOCK. IT SHOULD NOT BE
/// HELD WHILE THE HOLDING CODE EXECUTES READS AGAINST THE SAME TABLE.
/// OTHERWISE A DEADLOCK WILL OCCUR WHEN THE TABLE'S FILE IS EXPANDED, WHICH
/// WAITS ON THE RELEASE OF THE SHARED LOCK (REMAP REQUIRES EXCLUSIVE ACCESS).
/// THE hashmap.get(const iterator& it, ...) METHOD EXISTS TO PREVENT A CALL TO
/// manager.get(), WHICH DESPITE BEING A READ WOULD CAUSE A DEADLOCK. THIS IS
/// BECAUSE IT CANNOT COMPLETE ITS READ WHILE REMAP IS WAITING ON ACCESS.
/// A SIMILAR RISK ARISES FROM HOLDING iterator WHILE READING/WRITING ANY OTHER
/// TABLE AS A CYCLE CAUSING THE ABOVE WILL OCCUR. USE THE ITERATOR TO COLLECT
/// A SET FROM ITS TABLE AND THEN CALL iterator.release() TO FREE THE POINTER.

/// This class is not thread safe.
/// Size non-max implies record manager (ordinal record links).
template <class Link, class Key, size_t Size = max_size_t>
class iterator
{
public:
    DEFAULT_COPY_MOVE_DESTRUCT(iterator);

    /// This advances to first match (or terminal).
    iterator(memory_ptr&& data, const Link& start, Key&& key) NOEXCEPT;
    iterator(memory_ptr&& data, const Link& start, const Key& key) NOEXCEPT;

    /// Advance to next and return false if none found.
    inline bool advance() NOEXCEPT;

    /// Expose the search key.
    inline const Key& key() const NOEXCEPT;

    /// Return current link, terminal if not found.
    inline const Link& self() const NOEXCEPT;

    /// Access the underlying memory pointer.
    inline const memory_ptr& get() const NOEXCEPT;

    /// Release the memory pointer, invalidates iterator.
    inline void reset() NOEXCEPT;

    /// True if the iterator is not terminal.
    inline operator bool() const NOEXCEPT;

protected:
    Link to_match(Link link) const NOEXCEPT;
    Link to_next(Link link) const NOEXCEPT;

private:
    using manager = database::manager<Link, Key, Size>;
    static constexpr auto key_size = keys::size<Key>();

    // This is not thread safe, but it's object is not modified here and the
    // memory that it refers to is not addressable until written, and writes
    // are guarded by allocator, which is protected by mutex.
    memory_ptr memory_;

    // This is thread safe.
    const Key key_;

    // This is not thread safe.
    Link link_;
};

} // namespace database
} // namespace libbitcoin

#define TEMPLATE \
template <class Link, class Key, size_t Size>
#define CLASS iterator<Link, Key, Size>

#include <bitcoin/database/impl/primitives/iterator.ipp>

#undef CLASS
#undef TEMPLATE

#endif
