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

/// This class is not thread safe.
/// Size non-max implies record manager (ordinal record links).
template <typename Link, typename Key, size_t Size = max_size_t>
class iterator
{
public:
    DEFAULT_COPY_MOVE_DESTRUCT(iterator);

    /// This advances to first match (or terminal).
    INLINE iterator(const memory_ptr& data, const Link& start,
        const Key& key) NOEXCEPT;

    /// Advance to and return next iterator.
    INLINE bool advance() NOEXCEPT;

    /// Advance to next match and return false if terminal (not found).
    INLINE const Link& self() const NOEXCEPT;

protected:
    INLINE bool is_match() const NOEXCEPT;
    INLINE Link get_next() const NOEXCEPT;

private:
    static constexpr auto is_slab = (Size == max_size_t);
    static constexpr size_t link_to_position(const Link& link) NOEXCEPT;

    // These are thread safe.
    const memory_ptr memory_;
    const Key key_;

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
