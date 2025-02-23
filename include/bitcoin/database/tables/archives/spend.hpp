/////**
//// * Copyright (c) 2011-2025 libbitcoin developers (see AUTHORS)
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
////#ifndef LIBBITCOIN_DATABASE_TABLES_ARCHIVES_SPEND_HPP
////#define LIBBITCOIN_DATABASE_TABLES_ARCHIVES_SPEND_HPP
////
////#include <bitcoin/system.hpp>
////#include <bitcoin/database/define.hpp>
////#include <bitcoin/database/primitives/primitives.hpp>
////#include <bitcoin/database/tables/schema.hpp>
////
////namespace libbitcoin {
////namespace database {
////namespace table {
////
/////// Spend is a record multimap of (stub) transactions by output(s) spent.
////struct spend
////  : public hash_map<schema::spend>
////{
////    using tx = linkage<schema::tx>;
////    using ix = linkage<schema::index>;
////    using pt = linkage<schema::point::pk>;
////    using hash_map<schema::spend>::hashmap;
////    using search_key = search<schema::spend::sk>;
////
////    // Composers/decomposers do not adjust to type changes.
////    static_assert(pt::size == 4 && ix::size == 3);
////
////    static constexpr search_key compose(tx::integer point_stub,
////        ix::integer point_index) NOEXCEPT
////    {
////        return
////        {
////            system::byte<0>(point_stub),
////            system::byte<1>(point_stub),
////            system::byte<2>(point_stub),
////            system::byte<3>(point_stub),
////            system::byte<0>(point_index),
////            system::byte<1>(point_index),
////            system::byte<2>(point_index)
////        };
////    }
////
////    static constexpr search_key compose(const hash_digest& point_hash,
////        ix::integer point_index) NOEXCEPT
////    {
////        return
////        {
////            point_hash.at(0),
////            point_hash.at(1),
////            point_hash.at(2),
////            point_hash.at(3),
////            system::byte<0>(point_index),
////            system::byte<1>(point_index),
////            system::byte<2>(point_index)
////        };
////    }
////
////    static search_key compose(const system::chain::point& point) NOEXCEPT
////    {
////        return compose(point.hash(), point.index());
////    }
////
////    struct record
////      : public schema::spend
////    {
////        inline bool from_data(reader& source) NOEXCEPT
////        {
////            point_fk = source.read_little_endian<pt::integer, pt::size>();
////            BC_ASSERT(!source || source.get_read_position() == minrow);
////            return source;
////        }
////
////        inline bool to_data(finalizer& sink) const NOEXCEPT
////        {
////            sink.write_little_endian<pt::integer, pt::size>(point_fk);
////            BC_ASSERT(!sink || sink.get_write_position() == minrow);
////            return sink;
////        }
////
////        inline bool operator==(const record& other) const NOEXCEPT
////        {
////            return point_fk == other.point_fk;
////        }
////
////        pt::integer point_fk{};
////    };
////
////    struct get_key
////      : public schema::spend
////    {
////        inline bool from_data(reader& source) NOEXCEPT
////        {
////            source.rewind_bytes(sk);
////            key = source.read_forward<sk>();
////            return source;
////        }
////
////        search_key key{};
////    };
////};
////
////} // namespace table
////} // namespace database
////} // namespace libbitcoin
////
////#endif
