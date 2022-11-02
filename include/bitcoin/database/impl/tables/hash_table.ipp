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

#include <memory>
#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

// Obtaining memory object is considered const access despite the fact that
// memory is writeable. Non-const manager access implies memory map modify.

TEMPLATE
CLASS::hash_table(storage& header, storage& body, link buckets) NOEXCEPT
  : header_(header, buckets), body_(body)
{
}

TEMPLATE
bool CLASS::create() NOEXCEPT
{
    return header_.create() && verify();
}

// TODO: for recovery trim body to count (fail if body.count < count).
TEMPLATE
bool CLASS::verify() NOEXCEPT
{
    link count{};
    return header_.get_body_count(count) && (body_.count() == count);
}

TEMPLATE
map_source_ptr CLASS::at(link record) const NOEXCEPT
{
    return std::make_shared<map_source>(body_.get(record));
}

TEMPLATE
map_source_ptr CLASS::find(const key& key) const NOEXCEPT
{
    // Element is a key-matching search iterator.
    Element element{ body_, header_.head(key) };
    for (; !element.is_terminal() && !element.match(key); element.advance());

    return element.is_terminal() ?
        system::to_shared<map_source>() :
        system::to_shared(body_.get(element.self()));
}

TEMPLATE
map_sink_ptr CLASS::push(const key& key, link size) NOEXCEPT
{
    const auto record = body_.allocate(size);

    //// auto& next = system::unsafe_byte_cast<link>(memory->data());
    //// header_.push(record, next, index);
    ////return { header_, header_.hash(key), current, body_.get(record) };

    return record.is_terminal() ?
        system::to_shared<map_source>() :
        system::to_shared(body_.get(record));
}

} // namespace database
} // namespace libbitcoin

#endif
