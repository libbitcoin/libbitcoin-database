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
#include <bitcoin/database/error.hpp>

#include <bitcoin/system.hpp>

namespace libbitcoin {
namespace database {
namespace error {

DEFINE_ERROR_T_MESSAGE_MAP(error)
{
    // memory map

    { success, "success" },
    { unknown, "unknown error" },

    { open_open, "opening open file" },
    { open_failure, "file failed to open" },

    { close_loaded, "closing loaded file" },
    { close_failure, "file failed to close" },

    { load_locked, "loading locked file" },
    { load_loaded, "loading loaded file" },
    { load_failure, "file failed to load" },

    { flush_unloaded, "flushing unloaded file" },
    { flush_failure, "file failed to flush" },

    { unload_locked, "unloading unloaded file" },
    { unload_failure, "file failed to unload" },

    // store

    { transactor_lock, "transactor lock failure" },
    { process_lock, "process lock failure" },
    { flush_lock, "flush lock failure" },
    { flush_unlock, "flush unlock failure" },
    { process_unlock, "process unlock failure" },

    { create_directory, "create directory failure" },
    { clear_directory, "clear directory failure" },
    { remove_directory, "remove directory failure" },
    { rename_directory, "rename directory failure" },
    { missing_backup, "missing backup" },

    { create_file, "file failed to create" },
    { unloaded_file, "file not loaded" },
    { dump_file, "file failed to dump" },
    { create_map, "failed to create map" }
};

DEFINE_ERROR_T_CATEGORY(error, "database", "database code")

} // namespace error
} // namespace database
} // namespace libbitcoin
