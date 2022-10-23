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
#ifndef LIBBITCOIN_DATABASE_ELEMENTS_KEYED_RECORD_HPP
#define LIBBITCOIN_DATABASE_ELEMENTS_KEYED_RECORD_HPP

#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/primitives/element.hpp>
#include <bitcoin/database/primitives/manager.hpp>

namespace libbitcoin {
namespace database {

/// Size includes Key but not Link (Key is treated as part of element value).
template <typename Link, typename Key, size_t Size,
    if_link<Link> = true, if_key<Key> = true,
    if_greater<Size, array_count<Key>> = true>
class keyed_record
  : public element<record_manager<Link, Size>>
{
public:
    keyed_record(record_manager<Link, Size>& manager) NOEXCEPT;
    keyed_record(record_manager<Link, Size>& manager, Link link) NOEXCEPT;

    Link create(Link next, const Key& key, auto& write) NOEXCEPT;
    void read(auto& read) const NOEXCEPT;

    bool match(const Key& key) const NOEXCEPT;
    Key key() const NOEXCEPT;

private:
    using base = element<record_manager<Link, Size>>;
    static constexpr auto key_size = array_count<Key>;
};

} // namespace database
} // namespace libbitcoin

#define TEMPLATE \
template <typename Link, typename Key, size_t Size,\
if_link<Link> If1, if_key<Key> If2, if_greater<Size, array_count<Key>> If3>
#define CLASS keyed_record<Link, Key, Size, If1, If2, If3>

#include <bitcoin/database/impl/elements/keyed_record.ipp>

#undef CLASS
#undef TEMPLATE

#endif
