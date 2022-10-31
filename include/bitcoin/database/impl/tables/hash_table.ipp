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
#ifndef LIBBITCOIN_DATABASE_TABLES_HASH_TABLE_IPP
#define LIBBITCOIN_DATABASE_TABLES_HASH_TABLE_IPP

#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/memory.hpp>
#include <bitcoin/database/memory/storage.hpp>

namespace libbitcoin {
namespace database {

// Obtaining memory object is considered const access despite the fact that
// memory is writeable. Non-const manager access implies memory map modify.

TEMPLATE
CLASS::hash_table(storage& header, storage& body, link buckets) NOEXCEPT
  : header_(header, buckets), body_(body)
{
}

// Return istream that starts [at] key and terminates at Size.
// Store deserializer accepts istream, using reader to construct object.
// istream can either be kept alive by retention of memory_ptr or can obtain a
// memory_ptr on each read call, using retained manager/link.
TEMPLATE
Element CLASS::at(link link) const NOEXCEPT
{
    // TODO: turn element into reader.
    // TODO: stream fault if not found.
    return { body_, link };
}

// Return istream that starts [after] key and terminates at Size.
// Store deserializer accepts istream, using reader to construct object.
TEMPLATE
Element CLASS::find(const key& key) const NOEXCEPT
{
    // TODO: turn element into reader.
    // TODO: stream fault if not found.
    auto element = at(header_.head(key));
    for (; element && !element.match(key); element.advance());
    return element;
}

// Return ostream that starts [after] key and terminates at Size.
// Store serializer accepts ostream, using writer to emit object.
// Write the key to the stream before returning.
TEMPLATE
Element CLASS::push(const key& key, link size) NOEXCEPT
{
    const auto current = body_.allocate(size);
    auto memory = body_.get(current);

    // TODO: turn element into writer, capture memory.
    // TODO: stream fault.
    if (!memory) return at(link::eof);

    // TODO: capture void push(...) in writer closure, execute at flush.
    auto& next = system::unsafe_byte_cast<link>(memory->data());
    header_.push(current, next, key);

}

} // namespace database
} // namespace libbitcoin

#endif
