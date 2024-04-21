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
#ifndef LIBBITCOIN_DATABASE_ASSOCIATIONS_HPP
#define LIBBITCOIN_DATABASE_ASSOCIATIONS_HPP

#include <bitcoin/system.hpp>
#include <bitcoin/database/association.hpp>
#include <bitcoin/database/boost.hpp>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

typedef boost::multi_index_container
<
    association,
    boost::multi_index::indexed_by
    <
        boost::multi_index::hashed_unique
        <
            boost::multi_index::tag<association::key>,
            boost::multi_index::key<&association::hash>
        >,
        boost::multi_index::ordered_unique
        <
            boost::multi_index::tag<association::pos>,
            association::name_extractor
        >
    >
> associations_;

/// Collection of association between block hash and context.
/// Indexed by unique block hash and by unique block height.
class associations
  : public associations_
{
public:
    /// Use base class constructors.
    using associations_::associations_;

    /// Forward iterator for the ordered_unique index.
    inline auto pos_begin() const NOEXCEPT
    {
        return get<association::pos>().begin();
    }

    /// Forward end iterator for the ordered_unique index.
    inline auto pos_end() const NOEXCEPT
    {
        return get<association::pos>().end();
    }

    /// Returns a pointer to an association instance, or .end().
    inline auto find(const system::hash_digest& hash) const NOEXCEPT
    {
        BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
        return get<association::key>().find(hash);
        BC_POP_WARNING()
    }

    /// Returns a pointer to an association instance, or .pos_end().
    inline auto find(size_t height) const NOEXCEPT
    {
        BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
        return get<association::pos>().find(height);
        BC_POP_WARNING()
    }

    /// Hash identifies an element in the hashed_unique index.
    inline bool exists(const system::hash_digest& hash) const NOEXCEPT
    {
        return find(hash) != end();
    }

    /// Height identifies an element in the ordered_unique index.
    inline bool exists(size_t height) const NOEXCEPT
    {
        return find(height) != pos_end();
    }

    /// The context of the maximum ordered_unique index (must not be empty).
    inline const system::chain::context& top() const NOEXCEPT
    {
        BC_ASSERT(!empty());

        BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
        return get<association::pos>().rbegin()->context;
        BC_POP_WARNING()
    }
};

} // namespace database
} // namespace libbitcoin

#endif
