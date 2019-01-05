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
#ifndef LIBBITCOIN_DATABASE_DEFINE_HPP
#define LIBBITCOIN_DATABASE_DEFINE_HPP

#include <array>
#include <cstdint>
#include <tuple>
#include <vector>
#include <bitcoin/system.hpp>

// Now we use the generic helper definitions in libbitcoin to
// define BCD_API and BCD_INTERNAL.
// BCD_API is used for the public API symbols. It either DLL imports or
// DLL exports (or does nothing for static build)
// BCD_INTERNAL is used for non-api symbols.

#if defined BCD_STATIC
    #define BCD_API
    #define BCD_INTERNAL
#elif defined BCD_DLL
    #define BCD_API      BC_HELPER_DLL_EXPORT
    #define BCD_INTERNAL BC_HELPER_DLL_LOCAL
#else
    #define BCD_API      BC_HELPER_DLL_IMPORT
    #define BCD_INTERNAL BC_HELPER_DLL_LOCAL
#endif

// Log name.
#define LOG_DATABASE "database"

namespace libbitcoin {
namespace database {

typedef uint32_t array_index;
typedef uint64_t file_offset;
typedef std::vector<file_offset> link_list;
typedef bc::system::serializer<uint8_t*> byte_serializer;
typedef bc::system::deserializer<uint8_t*, false> byte_deserializer;
typedef std::array<uint8_t, 0> empty_key;
static_assert(std::tuple_size<empty_key>::value == 0, "non-empty empty key");

} // namespace database
} // namespace libbitcoin

#endif
