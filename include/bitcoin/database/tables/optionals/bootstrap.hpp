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
////#ifndef LIBBITCOIN_DATABASE_TABLES_OPTIONALS_BOOTSTRAP_HPP
////#define LIBBITCOIN_DATABASE_TABLES_OPTIONALS_BOOTSTRAP_HPP
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
/////// bootstrap is an array of header hashes (initial blockchain).
////struct bootstrap
////  : public no_map<schema::bootstrap>
////{
////    using no_map<schema::bootstrap>::nomap;
////
////    struct record
////      : public schema::bootstrap
////    {
////        link count() const NOEXCEPT
////        {
////            using namespace system;
////            return possible_narrow_cast<link::integer>(block_hashes.size());
////        }
////
////        inline bool from_data(reader& source) NOEXCEPT
////        {
////            // Clear the single record limit (file limit remains).
////            source.set_limit();
////
////            // TODO: stream-to-stream.
////            std::for_each(block_hashes.begin(), block_hashes.end(),
////                [&](auto& hash) NOEXCEPT
////                {
////                    hash = source.read_hash();
////                });
////
////            BC_ASSERT(source.get_read_position() == count() * schema::hash);
////            return source;
////        }
////
////        inline bool to_data(flipper& sink) const NOEXCEPT
////        {
////            // Clear the single record limit (file limit remains).
////            sink.set_limit();
////
////            // TODO: stream-to-stream.
////            std::for_each(block_hashes.begin(), block_hashes.end(),
////                [&](const auto& hash) NOEXCEPT
////                {
////                    sink.write_bytes(hash);
////                });
////
////            BC_ASSERT(sink.get_write_position() == count() * schema::hash);
////            return sink;
////        }
////
////        inline bool operator==(const record& other) const NOEXCEPT
////        {
////            return block_hashes == other.block_hashes;
////        }
////
////        hashes block_hashes{};
////    };
////};
////
////} // namespace table
////} // namespace database
////} // namespace libbitcoin
////
////#endif
