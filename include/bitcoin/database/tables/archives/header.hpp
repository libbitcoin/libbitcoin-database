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
#ifndef LIBBITCOIN_DATABASE_TABLES_ARCHIVES_HEADER_HPP
#define LIBBITCOIN_DATABASE_TABLES_ARCHIVES_HEADER_HPP

#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/tables/schema.hpp>
#include <bitcoin/database/memory/memory.hpp>

namespace libbitcoin {
namespace database {

namespace schema
{
	namespace header
	{
		class record
		{
			////bool from_data(reader& source) NOEXCEPT
			////{
			////}

			////bool to_data(writer& sink) NOEXCEPT
			////{
			////}

			const keying<pk> parent;
			const uint32_t version;
			const uint32_t time;
			const uint32_t bits;
			const uint32_t nonce;
			const hash_digest root;
		};

		using element = iterator<linkage<pk>, keying<sk>, bytes>;
	}
}

class BCD_API header
  : public hashmap<schema::header::element>
{
public:
	DEFAULT5(header);

	using hashmap = hashmap;



private:
};

} // namespace database
} // namespace libbitcoin

#endif
