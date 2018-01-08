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

// TODO: rename to records_row.
template <typename LinkType, typename RecordManager>
class record_list
  : noncopyable
{
public:
    typedef serializer<uint8_t*>::functor write_function;

    static const LinkType not_found = (LinkType)bc::max_uint64;

    /// Construct for a new record.
    record_list(RecordManager& manager);

    /// Construct for an existing record.
    record_list(RecordManager& manager, LinkType index);

    /// Allocate and populate a new record.
    LinkType create(write_function write);

    /// Allocate a record to the existing next record.
    void link(LinkType next);

    /// The actual user data for this record.
    memory_ptr data() const;

    /// Index of the next record.
    LinkType next_index() const;

private:
    memory_ptr raw_data(file_offset offset) const;

    LinkType index_;
    RecordManager& manager_;
};

} // namespace database
} // namespace libbitcoin

#include <bitcoin/database/impl/record_list.ipp>

#endif
