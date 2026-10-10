/**
 * Copyright (c) 2011-2026 libbitcoin developers
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
#ifndef LIBBITCOIN_DATABASE_TABLES_INDEXES_WTXID_HPP
#define LIBBITCOIN_DATABASE_TABLES_INDEXES_WTXID_HPP

#include <bitcoin/database/define.hpp>
#include <bitcoin/database/primitives/primitives.hpp>
#include <bitcoin/database/tables/schema.hpp>

namespace libbitcoin {
namespace database {
namespace table {

/// wtxid is a record hashmap indexing the pool by witness hash. A record holds
/// only its search chain link, and its link is that of the pool record whose
/// id columns hold the key (a search verifies the candidate pool record).
struct wtxid
  : public hash_map<schema::wtxid>
{
    using hash_map<schema::wtxid>::hashmap;

    /// Allocate through the pool record link and commit that record under the
    /// witness hash. Records skipped by a failed write are completed unlinked.
    inline bool put(const link& record,
        const system::hash_digest& hash) NOEXCEPT
    {
        const auto end = count();
        if (end.is_terminal() || record.is_terminal() ||
            end.value > record.value)
            return false;

        using namespace system;
        const auto skipped = record.value - end.value;
        if (allocate(add1(skipped)).is_terminal())
            return false;

        abandon(end, skipped);
        return commit(record, key{ hash });
    }
};

} // namespace table
} // namespace database
} // namespace libbitcoin

#endif
