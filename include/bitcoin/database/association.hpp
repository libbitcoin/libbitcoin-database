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
#ifndef LIBBITCOIN_DATABASE_ASSOCIATION_HPP
#define LIBBITCOIN_DATABASE_ASSOCIATION_HPP

#include <bitcoin/system.hpp>
#include <bitcoin/database/boost.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/tables/tables.hpp>

namespace libbitcoin {
namespace database {

/// Association between block hash and context.
struct association
{
    table::height::link link;
    system::hash_digest hash;
    system::chain::context context;

    struct key{};
    struct pos{};

    struct name_extractor
    {
        using result_type = size_t;

        inline const result_type& operator()(
            const association& item) const NOEXCEPT
        {
            return item.context.height;
        }

        inline result_type& operator()(
            association* item) const NOEXCEPT
        {
            BC_ASSERT_MSG(item, "null pointer");
            return item->context.height;
        }
    };
};

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
> boost_associations;

// TODO: move to own .h/.cpp.
/// Collection of association between block hash and context.
/// Indexed by unique block hash and by unique block height.
class associations
  : public boost_associations
{
public:
    /// Use base class constructors.
    using boost_associations::boost_associations;

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
};

} // namespace database
} // namespace libbitcoin

#endif
