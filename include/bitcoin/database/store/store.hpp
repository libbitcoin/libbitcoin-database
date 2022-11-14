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
#ifndef LIBBITCOIN_DATABASE_STORE_HPP
#define LIBBITCOIN_DATABASE_STORE_HPP

#include <shared_mutex>
#include <bitcoin/database/boost.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/settings.hpp>
#include <bitcoin/database/locks/locks.hpp>
#include <bitcoin/database/memory/memory.hpp>
#include <bitcoin/database/tables/tables.hpp>

namespace libbitcoin {
namespace database {

class BCD_API store
{
public:
	DELETE5(store);

	/// Construct a store from settings.
	store(const settings& configuration) NOEXCEPT;

	/// Create or open the set of tables, set locks.
	bool open() NOEXCEPT;

	/// Snapshot the set of tables.
	/// Pause writes, set body sizes, flush files, copy headers, swap backups.
	bool snapshot() NOEXCEPT;

	/// Flush and close the set of tables, clear locks.
	bool close() NOEXCEPT;

protected:
	header::table header;
	////point::table point;
	////input::table input;
	////output::table output;
	////puts::table puts;
	////transaction::table transaction;
	////txs::table txs;

private:
	// This is thread safe.
	const settings& configuration_;

	// These are protected by mutex.
	flush_lock flush_lock_;
	interprocess_lock process_lock_;
	boost::upgrade_mutex transactor_mutex_;

	// record hashmap
	map header_head_;
	map header_body_;

	////// record hashmap
	////map point_head_;
	////map point_body_;

	////// slab hashmap
	////map input_head_;
	////map input_body_;

	////// blob
	////map output_body_;

	////// array
	////map puts_body_;

	////// record hashmap
	////map transaction_head_;
	////map transaction_body_;

	////// slab hashmap
	////map txs_head_;
	////map txs_body_;
};

} // namespace database
} // namespace libbitcoin

#endif
