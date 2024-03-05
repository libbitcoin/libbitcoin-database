/**
 * Copyright (c) 2011-2023 libbitcoin developers (see AUTHORS)
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

namespace libbitcoin {
namespace database {

using code = system::code;

} // namespace database
} // namespace libbitcoin

#endif
