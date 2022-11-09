/**
 * Copyright (c) 2011-2022 libbitcoin developers (see AUTHORS)
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
#ifndef LIBBITCOIN_DATABASE_PRIMITIVES_ARRAYMAP_IPP
#define LIBBITCOIN_DATABASE_PRIMITIVES_ARRAYMAP_IPP

#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {
    
TEMPLATE
CLASS::arraymap(storage& body) NOEXCEPT
  : body_(body)
{
}

TEMPLATE
Record CLASS::get(const link& link) const NOEXCEPT
{
    return { at(link) };
}

TEMPLATE
bool CLASS::insert(const Record& record) NOEXCEPT
{
    // record.size() is slab/byte or record allocation.
    return record.to_data(push(record.size()));
}

// protected
// ----------------------------------------------------------------------------

TEMPLATE
reader_ptr CLASS::at(const link& record) const NOEXCEPT
{
    if (record.is_terminal())
        return {};

    const auto ptr = body_.get(record);
    if (!ptr)
        return {};

    const auto source = std::make_shared<reader>(ptr);
    if constexpr (!slab) { source->set_limit(payload_size); }
    return source;
}

TEMPLATE
writer_ptr CLASS::push(const link& size) NOEXCEPT
{
    BC_ASSERT(!size.is_terminal());
    BC_ASSERT(!system::is_multiply_overflow<size_t>(size, payload_size));

    const auto item = body_.allocate(size);
    if (item == storage::eof)
        return {};

    const auto ptr = body_.get(item);
    if (!ptr)
        return {};

    const auto sink = std::make_shared<writer>(ptr);
    if constexpr (slab) { sink->set_limit(size); }
    if constexpr (!slab) { sink->set_limit(size * payload_size); }
    return sink;
}

} // namespace database
} // namespace libbitcoin

#endif
