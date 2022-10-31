/////**
//// * Copyright (c) 2011-2019 libbitcoin developers (see AUTHORS)
//// *
//// * This file is part of libbitcoin.
//// *
//// * This program is free software: you can redistribute it and/or modify
//// * it under the terms of the GNU Affero General Public License as published by
//// * the Free Software Foundation, either version 3 of the License, or
//// * (at your option) any later version.
//// *
//// * This program is distributed in the hope that it will be useful,
//// * but WITHOUT ANY WARRANTY; without even the implied warranty of
//// * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//// * GNU Affero General Public License for more details.
//// *
//// * You should have received a copy of the GNU Affero General Public License
//// * along with this program.  If not, see <http://www.gnu.org/licenses/>.
//// */
////#ifndef LIBBITCOIN_DATABASE_ELEMENTS_KEYED_SLAB_HPP
////#define LIBBITCOIN_DATABASE_ELEMENTS_KEYED_SLAB_HPP
////
////#include <bitcoin/system.hpp>
////#include <bitcoin/database/define.hpp>
////#include <bitcoin/database/primitives/element.hpp>
////#include <bitcoin/database/primitives/manager.hpp>
////
////namespace libbitcoin {
////namespace database {
////
////template <typename Link, typename Key,
////    if_link<Link> = true, if_key<Key> = true>
////class keyed_slab
////  : public element<slab_manager<Link>>
////{
////public:
////    using key = Key;
////    using link = Link;
////    using manager = slab_manager<Link>;
////
////    keyed_slab(slab_manager<Link>& manager) NOEXCEPT;
////    keyed_slab(slab_manager<Link>& manager, Link link) NOEXCEPT;
////
////    Link create(Link next, const Key& key, auto& write, size_t limit) NOEXCEPT;
////    void read(auto& read, size_t limit) const NOEXCEPT;
////
////    bool match(const Key& key) const NOEXCEPT;
////    Key key() const NOEXCEPT;
////
////private:
////    using base = element<slab_manager<Link>>;
////    static constexpr auto key_size = array_count<Key>;
////};
////
////} // namespace database
////} // namespace libbitcoin
////
////#define TEMPLATE \
////template <typename Link, typename Key, if_link<Link> If1, if_key<Key> If2>
////#define CLASS keyed_slab<Link, Key, If1, If2>
////
////#include <bitcoin/database/impl/elements/keyed_slab.ipp>
////
////#undef CLASS
////#undef TEMPLATE
////
////#endif
