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
#ifndef LIBBITCOIN_DATABASE_TABLES_EVENT_HPP
#define LIBBITCOIN_DATABASE_TABLES_EVENT_HPP

namespace libbitcoin {
namespace database {

enum class event_t
{
    create_file,
    open_file,
    load_file,
    unload_file,
    close_file,

    create_table,
    verify_table,
    close_table,

    wait_lock,
    flush_body,
    backup_table,
    copy_header,
    archive_snapshot,

    restore_table,
    recover_snapshot
};

} // namespace database
} // namespace libbitcoin

#endif
