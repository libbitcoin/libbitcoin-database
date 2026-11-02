/**
 * Copyright (c) 2011-2026 libbitcoin developers
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

#include <bitcoin/database/error.hpp>

// map is able to support 32 bit, but because the database
// requires a larger file this is neither validated nor supported.
////static_assert(sizeof(void*) == sizeof(uint64_t), "Not a 64 bit system!");

namespace libbitcoin {
namespace database {

using hash_digest = system::hash_digest;
using hashes = system::hashes;
using code = system::code;

} // namespace database
} // namespace libbitcoin

#endif
