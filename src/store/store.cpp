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
#include <bitcoin/database/store/store.hpp>

#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

store::store(const settings& configuration) NOEXCEPT
  : configuration_(configuration),
	flush_lock_(configuration.directory),
	process_lock_(configuration.directory),
	header_head_(configuration.directory),
	header_body_(configuration.directory),
	header(header_head_, header_body_, 42)
{
}

bool store::open() NOEXCEPT
{
	// TODO: log/report transactor/process/flush/open failure code.
	if (transactor_mutex_.try_lock())
	{
		const auto result = process_lock_.try_lock() && flush_lock_.try_lock();
		// TODO: if (result) open tables.
		transactor_mutex_.unlock();
		return result;
	}

	return false;
}

bool store::snapshot() NOEXCEPT
{
	while (!transactor_mutex_.try_lock_for(boost::chrono::seconds(1)))
	{
		// TODO: log deadlock_hint
	}

	transactor_mutex_.unlock();
	return true;
}

bool store::close() NOEXCEPT
{
	// TODO: log/report transactor/close/process/flush failure code.
	if (transactor_mutex_.try_lock())
	{
		// TODO: close tables.
		const auto result = process_lock_.try_unlock() && flush_lock_.try_unlock();
		transactor_mutex_.unlock();
		return result;
	}

	return false;
}

} // namespace database
} // namespace libbitcoin
