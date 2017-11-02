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
#ifndef LIBBITCOIN_DATABASE_RECORD_LIST_HPP
#define LIBBITCOIN_DATABASE_RECORD_LIST_HPP

#include <cstddef>
#include <cstdint>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/memory.hpp>
#include <bitcoin/database/primitives/record_manager.hpp>

namespace libbitcoin {
namespace database {

class BCD_API record_list
{
public:
    static BC_CONSTEXPR array_index empty = bc::max_uint32;
    static BC_CONSTEXPR size_t index_size = sizeof(array_index);

    typedef serializer<uint8_t*>::functor write_function;

    /// Construct for a new or existing record.
    record_list(record_manager& manager, array_index index=empty);

    /// Allocate and populate a new record.
    array_index create(write_function write);

    /// Allocate a record to the existing next record.
    void link(array_index next);

    /// The actual user data for this record.
    memory_ptr data() const;

    /// Index of the next record.
    array_index next_index() const;

private:
    memory_ptr raw_data(file_offset offset) const;

    array_index index_;
    record_manager& manager_;
};

} // namespace database
} // namespace libbitcoin

#endif
