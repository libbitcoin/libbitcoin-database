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
  : header_(header, buckets), manager_(body)
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
        count == manager_.count();
}

TEMPLATE
bool CLASS::snap() NOEXCEPT
{
    return header_.set_body_count(manager_.count());
}

// query interface
// ----------------------------------------------------------------------------

TEMPLATE
bool CLASS::exists(const Key& key) const NOEXCEPT
{
    return !it(key).self().is_terminal();
}

TEMPLATE
// ITERATOR LIFETIME MUST NOT BE EXTENDED BEYOND ENUMERATION - DEADLOCK RISK.
typename CLASS::iterator CLASS::it(const Key& key) const NOEXCEPT
{
    return { manager_.get(), header_.top(key), key };
}

TEMPLATE
template <typename Record, if_equal<Record::size, Size>>
bool CLASS::get(const Key& key, Record& record) const NOEXCEPT
{
    return get(it(key).self(), record);
}

TEMPLATE
template <typename Record, if_equal<Record::size, Size>>
bool CLASS::get(const Link& link, Record& record) const NOEXCEPT
{
    auto source = at(link);
    if (!source)
        return false;

    // Use of stream pointer can be eliminated by cloning at() here.
    // RECORD.FROM_DATA MUST NOT EXTEND SOURCE LIFETIME - DEADLOCK RISK.
    return record.from_data(*source);
}

TEMPLATE
template <typename Record, if_equal<Record::size, Size>>
bool CLASS::put(const Key& key, const Record& record) NOEXCEPT
{
    auto sink = push(key, record.count());
    if (!sink)
        return false;

    // Use of stream pointer can be eliminated by cloning push() here.
    // RECORD.TO_DATA MUST NOT EXTEND SOURCE LIFETIME - DEADLOCK RISK.
    const auto result = record.to_data(*sink);
    sink->finalize();
    return result;
}

// protected
// ----------------------------------------------------------------------------

TEMPLATE
reader_ptr CLASS::at(const Link& link) const NOEXCEPT
{
    if (link.is_terminal())
        return {};

    const auto ptr = manager_.get(link);
    if (!ptr)
        return {};

    const auto source = std::make_shared<reader>(ptr);
    source->skip_bytes(Link::size + array_count<Key>);
    if constexpr (!is_slab) { source->set_limit(Size); }
    return source;
}

TEMPLATE
reader_ptr CLASS::find(const Key& key) const NOEXCEPT
{
    const auto record = it(key).self();
    if (record.is_terminal())
        return {};

    const auto source = at(record);
    if (!source)
        return {};

    return source;
}

TEMPLATE
finalizer_ptr CLASS::push(const Key& key, const Link& size) NOEXCEPT
{
    const auto value = system::possible_narrow_cast<size_t>(size.value);
    BC_ASSERT(is_slab || !system::is_multiply_overflow(value, Size));
    BC_ASSERT(!size.is_terminal());

    const auto item = manager_.allocate(size);
    if (item.is_terminal())
        return {};

    const auto ptr = manager_.get(item);
    if (!ptr)
        return {};

    const auto sink = std::make_shared<finalizer>(ptr);
    const auto index = header_.index(key);

    // Finalization activates the record by updating header and next.
    sink->set_finalizer([this, item, index, ptr]() NOEXCEPT
    {
        auto& next = system::unsafe_array_cast<uint8_t, Link::size>(ptr->begin());
        return header_.push(item, next, index);
    });

    // Link (next) commit is deferred until finalize.
    if constexpr (is_slab) { sink->set_limit(value); }
    sink->skip_bytes(Link::size);
    sink->write_bytes(key);
    if constexpr (!is_slab) { sink->set_limit(value * Size); }
    return sink;
}

} // namespace database
} // namespace libbitcoin

#endif
