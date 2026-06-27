/**
 * Copyright (c) 2011-2026 libbitcoin developers (see AUTHORS)
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
#ifndef LIBBITCOIN_DATABASE_STORE_EVENTS_IPP
#define LIBBITCOIN_DATABASE_STORE_EVENTS_IPP

#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

TEMPLATE
const CLASS::event_map CLASS::events
{
    { event_t::create_file, "create_file" },
    { event_t::open_file,   "open_file" },
    { event_t::load_file, "load_file" },
    { event_t::unload_file, "unload_file" },
    { event_t::close_file, "close_file" },
    { event_t::create_table, "create_table" },
    { event_t::verify_table, "verify_table" },
    { event_t::close_table, "close_table" },

    { event_t::wait_lock, "wait_lock" },
    { event_t::flush_body, "flush_body" },
    { event_t::prune_table, "prune_table" },
    { event_t::backup_table, "backup_table" },
    { event_t::copy_header, "copy_header" },
    { event_t::archive_snapshot, "archive_snapshot" },

    { event_t::restore_table, "restore_table" },
    { event_t::recover_snapshot, "recover_snapshot" }
};

} // namespace database
} // namespace libbitcoin

#endif
