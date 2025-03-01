/**
 * Copyright (c) 2011-2025 libbitcoin developers (see AUTHORS)
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
    { integrity, "store corrupted" },
    { integrity1, "store corrupted1" },
    { integrity2, "store corrupted2" },
    { integrity3, "store corrupted3" },
    { integrity4, "store corrupted4" },
    { integrity5, "store corrupted5" },
    { integrity6, "store corrupted6" },
    { integrity7, "store corrupted7" },
    { integrity8, "store corrupted8" },
    { integrity9, "store corrupted9" },
    { integrity10, "store corrupted10" },
    { integrity11, "store corrupted11" },

    // memory map
    { open_open, "opening open file" },
    { size_failure, "filure obtaining file size" },
    { close_loaded, "closing loaded file" },
    { load_loaded, "loading loaded file" },
    { load_locked, "loading locked file" },
    { load_failure, "file failed to load, disk may be full" },
    { reload_unloaded, "reloading unloaded file" },
    { reload_locked, "reloading locked file" },
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
    { missing_directory, "missing directory failure" },
    { clear_directory, "clear directory failure" },
    { rename_directory, "rename directory failure" },

    // store
    { missing_snapshot, "missing snapshot" },
    { unloaded_file, "file not loaded" },

    // tables
    { create_table, "failed to create table" },
    { close_table, "failed to close table" },
    { backup_table, "failed to backup table" },
    { restore_table, "failed to restore table" },
    { verify_table, "failed to verify table" },

    // states
    { tx_connected, "transaction connected" },
    { tx_disconnected, "transaction disconnected" },
    { block_valid, "block valid" },
    { block_confirmable, "block confirmable" },
    { block_unconfirmable, "block unconfirmable" },
    { unassociated, "unassociated" },
    { unvalidated, "unvalidated" },

    // states
    { missing_previous_output, "missing previous output" },
    { coinbase_maturity, "coinbase maturity" },
    { unspent_coinbase_collision, "unspent coinbase collision" },
    { relative_time_locked, "relative time locked" },
    { unconfirmed_spend, "unconfirmed spend" },
    { confirmed_double_spend, "confirmed double spend" },

    // tx archive
    { tx_empty, "tx_empty" },
    { tx_tx_allocate, "tx_tx_allocate" },
    { tx_input_put, "tx_input_put" },
    { tx_ins_put, "tx_ins_put" },
    { tx_output_put, "tx_output_put" },
    { tx_outs_put, "tx_outs_put" },
    { tx_point_allocate, "tx_point_allocate" },
    { tx_point_put, "tx_point_put" },
    { tx_tx_set, "tx_tx_set" },
    { tx_address_allocate, "tx_address_allocate" },
    { tx_address_put, "tx_address_put" },
    { tx_tx_commit, "tx_tx_commit" },

    // header archive
    { header_put, "header_put" },

    // txs archive
    { txs_header, "txs_header" },
    { txs_empty, "txs_empty" },
    { txs_confirm, "txs_confirm" },
    { txs_txs_put, "txs_txs_put" }
};

DEFINE_ERROR_T_CATEGORY(error, "database", "database code")

} // namespace error
} // namespace database
} // namespace libbitcoin
