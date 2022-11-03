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
    link count{};
    return header_.verify() && header_.get_body_count(count) &&
        (body_.count() == count);
}

TEMPLATE
reader_ptr CLASS::at(const link& record) const NOEXCEPT
{
    // Directly access element.
    using namespace system;
    const auto source = to_shared<reader>(body_.get(record));

    // Skip over link, positioning reader at key.
    source->skip_bytes(link_size);

    // Caller must not exceed logical slab size.
    // All elements constrained to file end, records limited to record size.
    if constexpr (!slab) { source->set_limit(key_size + record_size); }
    return source;
}

TEMPLATE
reader_ptr CLASS::find(const key& key) const NOEXCEPT
{
    // Search for element.
    using namespace system;
    Element element{ body_, header_.head(key) };
    while (!element.is_terminal() && !element.match(key))
        element.advance();

    if (element.is_terminal())
        return {};

    const auto source = to_shared<reader>(body_.get(element.self()));

    // Skip over link and key, positioning reader at data.
    source->skip_bytes(link_size + key_size);

    // Caller must not exceed logical slab size.
    // All elements constrained to file end, records limited to record size.
    if constexpr (!slab) { source->set_limit(record_size); }
    return source;
}

TEMPLATE
writer_ptr CLASS::push(const key& key, const link& size) NOEXCEPT
{
    // Create element.
    using namespace system;
    const auto record = body_.allocate(size);

    if (record.is_terminal())
        return {};

    const auto index = header_.hash(key);
    const auto finalize = [this, record, index](uint8_t* data) NOEXCEPT
    {
        // This can only return false if file is unmapped (ignore return).
        header_.push(record, unsafe_byte_cast<link>(data), index);
    };

    const auto sink = to_shared<writer>({ body_.get(record), finalize });

    // size (slab) includes link/key.
    if constexpr (slab) { sink->set_limit(size); }
    sink->skip_bytes(link_size);

    // record_size includes key (not link).
    if constexpr (!slab) { sink->set_limit(record_size); }
    sink->skip_bytes(key_size);

    // Skipped over link and key, positioning reader at data.
    return sink;
}

} // namespace database
} // namespace libbitcoin

#endif
