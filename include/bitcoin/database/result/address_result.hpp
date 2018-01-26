/**
 * Copyright (c) 2011-2017 libbitcoin developers (see AUTHORS)
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
#ifndef LIBBITCOIN_DATABASE_ADDRESS_RESULT_HPP
#define LIBBITCOIN_DATABASE_ADDRESS_RESULT_HPP

#include <cstddef>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/primitives/hash_table_multimap.hpp>

namespace libbitcoin {
namespace database {

/// Partially-deferred address result.
class BCD_API address_result
{
public:
    typedef short_hash key_type;
    typedef array_index index_type;
    typedef array_index link_type;
    typedef hash_table_multimap<key_type, index_type, link_type> multimap;

    address_result(multimap::list list, const short_hash& hash, size_t limit,
        size_t from_height);

    /// The address hash of the query.
    const short_hash& hash() const;

    /// The count limit of the query.
    size_t limit() const;

    /// The height start of the query.
    size_t from_height() const;

private:
    short_hash hash_;
    size_t limit_;
    size_t from_height_;

    // This class is thread safe.
    multimap::list list_;
};

} // namespace database
} // namespace libbitcoin

#endif
