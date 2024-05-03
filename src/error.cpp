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
#include <bitcoin/database/error.hpp>

#include <bitcoin/system.hpp>

namespace libbitcoin {
namespace database {
namespace error {

DEFINE_ERROR_T_MESSAGE_MAP(error)
{
    // general
    { success, "success" },
    { unknown_state, "unknown state" },
    { integrity, "integrity failure" },

    // memory map
    { open_open, "opening open file" },
    { open_failure, "file failed to open" },
    { size_failure, "filure obtaining file size" },
    { close_loaded, "closing loaded file" },
    { close_failure, "file failed to close" },
    { load_locked, "loading locked file" },
    { load_loaded, "loading loaded file" },
    { load_failure, "disk full" },
    { flush_unloaded, "flushing unloaded file" },
    { flush_failure, "file failed to flush" },
    { unload_locked, "unloading locked file" },
    { unload_failure, "file failed to unload" },

    // mmap
    { disk_full, "disk full" },
    { mmap_failure, "mmap failure" },
    { mremap_failure, "mremap failure" },
    { munmap_failure, "munmap failure" },
    { madvise_failure, "madvise failure" },
    { ftruncate_failure, "ftruncate failure" },
    { fsync_failure, "fsync failure" },

    // locks
    { transactor_lock, "transactor lock failure" },
    { process_lock, "process lock failure" },
    { flush_lock, "flush lock failure" },
    { flush_unlock, "flush unlock failure" },
    { process_unlock, "process unlock failure" },

    // filesystem
    { create_directory, "create directory failure" },
    { clear_directory, "clear directory failure" },
    { remove_directory, "remove directory failure" },
    { rename_directory, "rename directory failure" },
    { copy_directory, "copy directory failure" },
    { missing_snapshot, "missing snapshot" },
    { create_file, "file failed to create" },
    { unloaded_file, "file not loaded" },
    { dump_file, "file failed to dump" },

    // tables
    { create_table, "failed to create table" },
    { close_table, "failed to close table" },
    { backup_table, "failed to backup table" },
    { restore_table, "failed to restore table" },
    { verify_table, "failed to verify table" },

    // states
    { tx_connected, "transaction connected" },
    { tx_preconnected, "transaction preconnected" },
    { tx_disconnected, "transaction disconnected" },
    { block_confirmable, "block confirmable" },
    { block_preconfirmable, "block preconfirmable" },
    { block_unconfirmable, "block unconfirmable" },
    { unassociated, "unassociated" },
    { unvalidated, "unvalidated" },

    // states
    { missing_previous_output, "missing previous output" },
    { coinbase_maturity, "coinbase maturity" },
    { unspent_coinbase_collision, "unspent coinbase collision" },
    { relative_time_locked, "relative time locked" },
    { unconfirmed_spend, "unconfirmed spend" },
    { confirmed_double_spend, "confirmed double spend" }
};

DEFINE_ERROR_T_CATEGORY(error, "database", "database code")

} // namespace error
} // namespace database
} // namespace libbitcoin
