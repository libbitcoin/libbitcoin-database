/**
 * Copyright (c) 2011-2022 libbitcoin developers (see AUTHORS)
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
#ifndef LIBBITCOIN_DATABASE_TABLES_SCHEMA_HPP
#define LIBBITCOIN_DATABASE_TABLES_SCHEMA_HPP

#include <bitcoin/database/define.hpp>
#include <bitcoin/database/primitives/primitives.hpp>

namespace libbitcoin {
namespace database {
	
template <size_t Size>
using search = system::data_array<Size>;
using hash_digest = system::hash_digest;

namespace schema
{
	namespace c
	{
		constexpr size_t block = 3;
		constexpr size_t tx = 4;
		constexpr size_t puts = 4;
		constexpr size_t put = 5;

		constexpr size_t code = 1;
		constexpr size_t size = 3;
		constexpr size_t index = 3;
		constexpr size_t sigops = 3;
		constexpr size_t flags = 4;

		constexpr size_t identity = system::hash_size;
		constexpr size_t foreign_point = tx + index;
		constexpr size_t natural_point = identity + index;
	}

	namespace context
	{
		constexpr size_t height = c::block;
		constexpr size_t flags = c::flags;
		constexpr size_t mtp = sizeof(uint32_t);

		constexpr size_t bytes = height + flags + mtp;
		static_assert(bytes == 11u);
	}

	// slab_hashmap (natural point)
	namespace output
	{
		constexpr size_t sk = c::natural_point;
		constexpr size_t pk = c::put;

		// Also varint length prefixed script (minimum 1).
		constexpr size_t value = sizeof(uint64_t);
	}

	// slab_hashmap (foreign point)
	namespace input
	{
		constexpr size_t sk = c::foreign_point;
		constexpr size_t pk = c::put;

		// Also varint length/count prefixed script/witness (minimum 2).
		constexpr size_t output_nk = output::sk;
		constexpr size_t sequence = sizeof(uint32_t);
	}

	// array (no sk)
	namespace puts
	{
		constexpr size_t pk = c::puts;

		// Association from 4 byte puts to set of 5 byte put (not counted).
		constexpr size_t put_fk = c::put;

		constexpr size_t bytes = put_fk;
		static_assert(bytes == 5u);
	}

	// record_hashmap
	namespace transaction
	{
		constexpr size_t sk = c::identity;
		constexpr size_t pk = c::tx;

		constexpr size_t coinbase = c::code;
		constexpr size_t size = c::size;
		constexpr size_t weight = c::size;
		constexpr size_t locktime = sizeof(uint32_t);
		constexpr size_t version = sizeof(uint32_t);
		constexpr size_t ins_count = c::index;
		constexpr size_t ins_fk = puts::pk;
		constexpr size_t outs_count = c::index;
		constexpr size_t outs_fk = puts::pk;

		constexpr size_t bytes = coinbase + size + weight + locktime + version +
			ins_count + ins_fk + outs_count + outs_fk;
		static_assert(bytes == 29u);
	}

	// record_hashmap
	namespace header
	{
		constexpr size_t sk = c::identity;
		constexpr size_t pk = c::block;

		constexpr size_t parent_fk = header::pk;
		constexpr size_t version = sizeof(uint32_t);
		constexpr size_t time = sizeof(uint32_t);
		constexpr size_t bits = sizeof(uint32_t);
		constexpr size_t nonce = sizeof(uint32_t);
		constexpr size_t root = c::identity;

		constexpr size_t bytes = context::bytes + parent_fk + version + time +
			bits + nonce + root;
		static_assert(bytes == 62u);
	}

	// slab_hashmap
	namespace txs
	{
		constexpr size_t sk = header::pk;
		constexpr size_t pk = c::tx;

		// Association from header to varint count prefixed tx set (minimum 5).
		constexpr size_t transaction_fk = transaction::pk;
	}
}

} // namespace database
} // namespace libbitcoin

#endif
