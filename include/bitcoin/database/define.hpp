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
#include <functional>
#include <shared_mutex>
#include <tuple>
#include <vector>
#include <bitcoin/system.hpp>

// map is able to support 32 bit, but because the database
// requires a larger file this is neither validated nor supported.
static_assert(sizeof(void*) == sizeof(uint64_t), "Not a 64 bit system!");

/// Attributes.
/// ---------------------------------------------------------------------------

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

/// Class helpers.
/// ---------------------------------------------------------------------------

/// Used when defining only the destructor.
#define DELETE4(class_name) \
    class_name(class_name&&) = delete; \
    class_name(const class_name&) = delete; \
    class_name& operator=(class_name&&) = delete; \
    class_name& operator=(const class_name&) = delete

/// Logging.
/// ---------------------------------------------------------------------------
#define LOG_DATABASE "database"

#define LOG_INFO(name) std::cout << name << " : "
#define LOG_DEBUG(name) std::cout << name << " : "
#define LOG_VERBOSE(name) std::cout << name << " : "
#define LOG_ERROR(name) std::cerr << name << " : "
#define LOG_WARNING(name) std::cerr << name << " : "
#define LOG_FATAL(name) std::cerr << name << " : "

/// Types.
/// ---------------------------------------------------------------------------

namespace libbitcoin {
namespace database {

using shared_mutex = std::shared_mutex;

typedef uint32_t array_index;
typedef uint64_t file_offset;
typedef std::vector<file_offset> link_list;
typedef std::array<uint8_t,zero> empty_key;
////typedef std::function<void(system::reader)> read_bytes;
////typedef std::function<void(system::writer)> write_bytes;
static_assert(is_zero(std::tuple_size<empty_key>::value));

template <typename Type>
using if_key = if_integral_array<Type>;
template <typename Type>
using if_link = if_unsigned_integral_integer<Type>;

} // namespace database
} // namespace libbitcoin

#endif

// hash_table_header
// list_element
// hash_table          -> hash_table_header, list_element
// hash_table_multimap -> hash_table, list_element, record_manager
// list                -> list_element, list_iterator
// list_iterator       -> list_element
// record_manager
// slab_manager