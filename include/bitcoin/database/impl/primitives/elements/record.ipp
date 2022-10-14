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
#ifndef LIBBITCOIN_DATABASE_PRIMITIVES_ELEMENTS_RECORD_IPP
#define LIBBITCOIN_DATABASE_PRIMITIVES_ELEMENTS_RECORD_IPP

#include <iterator>
#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {
namespace primitives {

TEMPLATE
CLASS::record(record_manager<Link>& manager) NOEXCEPT
  : element<record_manager<Link>, Link>(manager,
      element<record_manager<Link>, Link>::eof)
{
}

TEMPLATE
CLASS::record(record_manager<Link>& manager, Link link) NOEXCEPT
  : element<record_manager<Link>, Link>(manager, link)
{
}

TEMPLATE
Link CLASS::create(Link next, auto& write) NOEXCEPT
{
    constexpr auto size = sizeof(Link) + Size;
    link_ = manager_.allocate(one);
    const auto memory = get();
    auto start = memory->buffer();
    system::write::bytes::copy writer({ start, std::next(start, size) });
    writer.write_little_endian<Link>(next);
    write(writer);
    return link_;
}


TEMPLATE
void CLASS::read(auto& read) const NOEXCEPT
{
    const auto memory = get(sizeof(Link));
    const auto start = memory->buffer();
    system::read::bytes::copy reader({ start, std::next(start, Size) });
    read(reader);
}

} // namespace primitives
} // namespace database
} // namespace libbitcoin

#endif
