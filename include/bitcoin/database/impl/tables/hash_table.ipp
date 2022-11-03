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

namespace libbitcoin {
namespace database {

// Obtaining memory object is considered const access despite the fact that
// memory is writeable. Non-const manager access implies memory map modify.

TEMPLATE
CLASS::hash_table(storage& header, storage& body, const link& buckets) NOEXCEPT
  : header_(header, buckets), body_(body)
{
}

TEMPLATE
bool CLASS::create() NOEXCEPT
{
    return header_.create() && verify();
}

TEMPLATE
bool CLASS::verify() const NOEXCEPT
{
    return header_.verify() && header_.get_body_count() == body_.count();
}

TEMPLATE
reader_ptr CLASS::at(const link& record) const NOEXCEPT
{
    if (record.is_terminal())
        return {};

    const auto ptr = body_.get(record);

    BC_ASSERT_MSG(!is_null(ptr), "guarded by is_terminal");
    const auto source = std::make_shared<reader>(ptr);
    source->skip_bytes(link_size);
    if constexpr (!slab) { source->set_limit(record_size); }
    return source;
}

TEMPLATE
reader_ptr CLASS::find(const key& key) const NOEXCEPT
{
    Element element{ body_, header_.head(key) };
    while (!element.is_terminal() && !element.is_match(key))
        element.advance();

    if (element.is_terminal())
        return {};

    const auto ptr = body_.get(element.self());

    BC_ASSERT_MSG(!is_null(ptr), "guarded by is_terminal");
    const auto source = std::make_shared<reader>(ptr);
    source->skip_bytes(link_size);
    if constexpr (!slab) { source->set_limit(record_size); }
    source->skip_bytes(key_size);
    return source;
}

TEMPLATE
writer_ptr CLASS::push(const key& key, const link& size) NOEXCEPT
{
    const auto item = body_.allocate(size);

    if (item.is_terminal())
        return {};

    const auto ptr = body_.get(item);
    const auto index = header_.index(key);
    const auto fin = [this, item, index](uint8_t& data) NOEXCEPT
    {
        BC_PUSH_WARNING(NO_REINTERPRET_CAST)
        header_.push(item, reinterpret_cast<link&>(data), index);
        BC_POP_WARNING()
    };

    BC_ASSERT_MSG(!is_null(ptr), "guarded by is_terminal");
    const auto sink = std::make_shared<writer>(sinker{ ptr, fin });
    if constexpr (slab) { sink->set_limit(size); }
    sink->skip_bytes(link_size);
    if constexpr (!slab) { sink->set_limit(record_size); }
    sink->write_bytes(key);
    return sink;
}

} // namespace database
} // namespace libbitcoin

#endif
