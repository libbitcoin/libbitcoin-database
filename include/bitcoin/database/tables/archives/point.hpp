/**
 * Copyright (c) 2011-2025 libbitcoin developers (see AUTHORS)
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
#ifndef LIBBITCOIN_DATABASE_TABLES_ARCHIVES_POINT_HPP
#define LIBBITCOIN_DATABASE_TABLES_ARCHIVES_POINT_HPP

#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/memory.hpp>
#include <bitcoin/database/primitives/primitives.hpp>
#include <bitcoin/database/tables/schema.hpp>

namespace libbitcoin {
namespace database {
namespace table {

/// Point records are empty, providing only a sk<->fk compression mapping.
/// Each record is 32+4=36 bytes, enabling 4 byte point.hash storage.
/// This reduces point hash storage to tx from input scale (tx/in=38%).
/// This benefit doubles due to fp indexation by the spend table.
////struct point
////  : public hash_map<schema::point>
////{
////    using search_key = search<schema::hash>;
////    using hash_map<schema::point>::hashmap;
////
////    struct record
////      : public schema::point
////    {
////        inline bool from_data(const reader& source) NOEXCEPT
////        {
////            return source;
////        }
////
////        inline bool to_data(const finalizer& sink) const NOEXCEPT
////        {
////            return sink;
////        }
////
////        inline bool operator==(const record&) const NOEXCEPT
////        {
////            return true;
////        }
////    };
////};

} // namespace table
} // namespace database
} // namespace libbitcoin

#endif
