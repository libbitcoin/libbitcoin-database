/**
 * Copyright (c) 2011-2015 libbitcoin developers (see AUTHORS)
 *
 * This file is part of libbitcoin.
 *
 * libbitcoin is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License with
 * additional permissions to the one published by the Free Software
 * Foundation, either version 3 of the License, or (at your option)
 * any later version. For more information see LICENSE.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef LIBBITCOIN_DATABASE_LINKED_RECORDS_HPP
#define LIBBITCOIN_DATABASE_LINKED_RECORDS_HPP

#include <bitcoin/database/define.hpp>
#include <bitcoin/database/record/record_allocator.hpp>

namespace libbitcoin {
namespace database {

// used by test
BC_CONSTEXPR size_t linked_record_offset = sizeof(array_index);

/**
 * linked_records is a one-way linked list with a next value containing
 * the index of the next record.
 * Records can be dropped by forgetting an index, and updating to the next
 * value. We can think of this as a LIFO queue.
 */
class BCD_API linked_records
{
public:
    // std::numeric_limits<array_index>::max()
    static BC_CONSTEXPR array_index empty = bc::max_uint32;

    linked_records(record_allocator& allocator);

    /**
     * Create new list with a single record.
     */
    array_index create();

    /**
     * Insert new record before index. Returns index of new record.
     */
    array_index insert(array_index next);

    /**
     * Read next index for record in list.
     */
    array_index next(array_index index) const;

    /**
     * Get underlying record data.
     */
    record_byte_pointer get(array_index index) const; 

private:
    record_allocator& allocator_;
};

} // namespace database
} // namespace libbitcoin

#endif
