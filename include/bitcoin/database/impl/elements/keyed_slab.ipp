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
#ifndef LIBBITCOIN_DATABASE_ELEMENTS_KEYED_SLAB_IPP
#define LIBBITCOIN_DATABASE_ELEMENTS_KEYED_SLAB_IPP

#include <algorithm>
#include <iterator>
#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

TEMPLATE
CLASS::keyed_slab(slab_manager<Link>& manager) NOEXCEPT
  : keyed_slab(manager, base::eof)
{
}

TEMPLATE
CLASS::keyed_slab(slab_manager<Link>& manager, Link link) NOEXCEPT
  : element<slab_manager<Link>, Link>(manager, link)
{
}

TEMPLATE
Link CLASS::create(Link next, const Key& key, auto& write,
    size_t limit) NOEXCEPT
{
    const auto size = sizeof(Link) + key_size + limit;
    const auto memory = base::allocate(size);
    auto start = memory->data();
    system::write::bytes::copy writer({ start, std::next(start, size) });
    writer.write_little_endian<Link>(next);
    writer.write_bytes(key);
    write(writer);
    return base::link();
}

TEMPLATE
void CLASS::read(auto& read, size_t limit) const NOEXCEPT
{
    const auto memory = base::get(sizeof(Link) + key_size);
    const auto start = memory->data();
    system::read::bytes::copy reader({ start, std::next(start, limit) });
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
