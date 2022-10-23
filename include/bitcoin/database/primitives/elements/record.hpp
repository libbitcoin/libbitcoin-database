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
#ifndef LIBBITCOIN_DATABASE_PRIMITIVES_RECORD_ELEMENTS_RECORD_HPP
#define LIBBITCOIN_DATABASE_PRIMITIVES_RECORD_ELEMENTS_RECORD_HPP

#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/primitives/element.hpp>
#include <bitcoin/database/primitives/manager.hpp>

namespace libbitcoin {
namespace database {

template <typename Link, size_t Size,
    if_link<Link> = true>
class record
  : public element<record_manager<Link, Size>, Link>
{
public:
    record(record_manager<Link, Size>& manager) NOEXCEPT;
    record(record_manager<Link, Size>& manager, Link link) NOEXCEPT;

    Link create(Link next, auto& write) NOEXCEPT;
    void read(auto& read) const NOEXCEPT;

private:
    using base = element<record_manager<Link, Size>, Link>;
};

} // namespace database
} // namespace libbitcoin

#define TEMPLATE template <typename Link, size_t Size, if_link<Link> If>
#define CLASS record<Link, Size, If>

#include <bitcoin/database/impl/primitives/elements/record.ipp>

#undef CLASS
#undef TEMPLATE

#endif
