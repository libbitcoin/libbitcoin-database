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
#ifndef LIBBITCOIN_DATABASE_QUERY_IMPL_HPP
#define LIBBITCOIN_DATABASE_QUERY_IMPL_HPP

#include <bitcoin/database/define.hpp>
#include <bitcoin/database/store/query.hpp>
#include <bitcoin/database/store/store.hpp>

namespace libbitcoin {
namespace database {

class BCD_API query_implementation
  : public query
{
public:
	DELETE4(query_implementation);

	query_implementation(store& store) NOEXCEPT;
	virtual ~query_implementation() NOEXCEPT;

	foo get_foo() const NOEXCEPT override;

private:
	store& store_;
};

} // namespace database
} // namespace libbitcoin

#endif
