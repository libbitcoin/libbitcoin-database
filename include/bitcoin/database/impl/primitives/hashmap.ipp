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
#ifndef LIBBITCOIN_DATABASE_PRIMITIVES_HASHMAP_IPP
#define LIBBITCOIN_DATABASE_PRIMITIVES_HASHMAP_IPP

#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

TEMPLATE
CLASS::hashmap(storage& header, storage& body, const Link& buckets) NOEXCEPT
  : header_(header, buckets), body_(body)
{
}

// not thread safe
// ----------------------------------------------------------------------------

TEMPLATE
bool CLASS::create() NOEXCEPT
{
    return header_.create() && verify();
}

TEMPLATE
bool CLASS::verify() const NOEXCEPT
{
    Link count{};
    return header_.verify() && header_.get_body_count(count) &&
        count == body_.count();
}

// query interface
// ----------------------------------------------------------------------------

TEMPLATE
bool CLASS::exists(const Key& key) const NOEXCEPT
{
    return !it(key).self().is_terminal();
}

TEMPLATE
Record CLASS::get(const Key& key) const NOEXCEPT
{
    return { it(key).self() };
}

TEMPLATE
Record CLASS::get(const Link& link) const NOEXCEPT
{
    return { at(link) };
}

TEMPLATE
typename CLASS::iterable CLASS::it(const Key& key) const NOEXCEPT
{
    return { body_.get(), header_.top(key), key };
}

TEMPLATE
bool CLASS::insert(const Key& key, const Record& record) NOEXCEPT
{
    // record.size() is slab/byte or record allocation.
    return record.to_data(push(key, record.size()));
}

// protected
// ----------------------------------------------------------------------------

TEMPLATE
reader_ptr CLASS::find(const Key& key) const NOEXCEPT
{
    const auto record = it(key).self();
    if (record.is_terminal())
        return {};

    const auto source = at(record);
    if (!source)
        return {};

    source->skip_bytes(array_count<Key>);
    return source;
}

TEMPLATE
reader_ptr CLASS::at(const Link& link) const NOEXCEPT
{
    if (link.is_terminal())
        return {};

    const auto ptr = body_.get(link);
    if (!ptr)
        return {};

    const auto source = std::make_shared<reader>(ptr);
    source->skip_bytes(Link::size);
    if constexpr (!is_slab) { source->set_limit(Record::size); }
    return source;
}

TEMPLATE
finalizer_ptr CLASS::push(const Key& key, const Link& size) NOEXCEPT
{
    using namespace system;
    BC_ASSERT(!size.is_terminal());
    BC_ASSERT(!is_multiply_overflow<size_t>(size, Record::size));

    const auto item = body_.allocate(size);
    if (item.is_terminal())
        return {};

    const auto ptr = body_.get(item);
    if (!ptr)
        return {};

    const auto sink = std::make_shared<finalizer>(ptr);
    const auto index = header_.index(key);

    sink->set_finalizer([this, item, index, ptr]() NOEXCEPT
    {
        auto& next = unsafe_array_cast<uint8_t, Link::size>(ptr->begin());
        return header_.push(item, next, index);
    });

    if constexpr (is_slab) { sink->set_limit(size); }
    sink->skip_bytes(Link::size);
    if constexpr (!is_slab) { sink->set_limit(size * Record::size); }
    sink->write_bytes(key);
    return sink;
}

} // namespace database
} // namespace libbitcoin

#endif
