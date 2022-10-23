/**
 * Copyright (c) 2011-2019 libbitcoin developers (see AUTHORS)
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
#ifndef LIBBITCOIN_DATABASE_ELEMENTS_KEYED_RECORD_IPP
#define LIBBITCOIN_DATABASE_ELEMENTS_KEYED_RECORD_IPP

#include <algorithm>
#include <iterator>
#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {
    
// Size excludes Link and Key.
TEMPLATE
CLASS::keyed_record(record_manager<Link, Size>& manager) NOEXCEPT
  : keyed_record(manager, base::eof)
{
}

TEMPLATE
CLASS::keyed_record(record_manager<Link, Size>& manager, Link link) NOEXCEPT
  : element<record_manager<Link, Size>>(manager, link)
{
}

TEMPLATE
Link CLASS::create(Link next, const Key& key, auto& write) NOEXCEPT
{
    // Size includes Key but not Link (Key is treated as part of element value).
    constexpr auto size = sizeof(Link) + Size;
    const auto memory = base::allocate(one);
    auto start = memory->data();
    system::write::bytes::copy writer({ start, std::next(start, size) });
    writer.write_little_endian<Link>(next);
    writer.write_bytes(key);
    write(writer);
    return base::self();
}

TEMPLATE
void CLASS::read(auto& read) const NOEXCEPT
{
    constexpr auto size = Size - key_size;
    const auto memory = base::get(sizeof(Link) + key_size);
    const auto start = memory->data();
    system::read::bytes::copy reader({ start, std::next(start, size) });
    read(reader);
}

TEMPLATE
bool CLASS::match(const Key& key) const NOEXCEPT
{
    const auto memory = base::get(sizeof(Link));
    return std::equal(key.begin(), key.end(), memory->data());
}

TEMPLATE
Key CLASS::key() const NOEXCEPT
{
    const auto memory = base::get(sizeof(Link));
    return system::unsafe_array_cast<uint8_t, key_size>(memory->data());
}

} // namespace database
} // namespace libbitcoin

#endif
