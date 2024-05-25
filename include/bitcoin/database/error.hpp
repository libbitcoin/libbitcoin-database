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
#ifndef LIBBITCOIN_DATABASE_ERROR_HPP
#define LIBBITCOIN_DATABASE_ERROR_HPP

#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

/// Alias system code.
/// std::error_code "database" category holds database::error::error_t.
typedef std::error_code code;

namespace error {

enum error_t : uint8_t
{
    /// general
    success,
    unknown_state,
    integrity,

    /// memory map
    open_open,
    size_failure,
    close_loaded,
    load_loaded,
    load_locked,
    load_failure,
    reload_unloaded,
    reload_locked,
    flush_unloaded,
    flush_failure,
    unload_locked,
    unload_failure,

    /// mmap
    disk_full,
    mmap_failure,
    mremap_failure,
    munmap_failure,
    madvise_failure,
    ftruncate_failure,
    fsync_failure,

    /// locks
    transactor_lock,
    process_lock,
    flush_lock,
    flush_unlock,
    process_unlock,

    /// filesystem
    missing_directory,
    clear_directory,
    rename_directory,
    copy_directory,

    /// store
    missing_snapshot,
    unloaded_file,

    /// tables
    create_table,
    close_table,
    backup_table,
    restore_table,
    verify_table,

    /// validation/confirmation
    tx_connected,
    tx_preconnected,
    tx_disconnected,
    block_confirmable,
    block_valid,
    block_unconfirmable,
    unassociated,
    unvalidated,

    /// confirmation (require not just context but prevouts and/or metadata).
    missing_previous_output,
    coinbase_maturity,
    unspent_coinbase_collision,
    relative_time_locked,
    unconfirmed_spend,
    confirmed_double_spend,

    /// tx archive
    tx_empty,
    tx_tx_allocate,
    tx_spend_allocate,
    tx_input_put,
    tx_point_put,
    tx_spend_set,
    tx_output_put,
    tx_tx_set,
    tx_puts_put,
    tx_spend_commit,
    tx_address_put,
    tx_tx_commit,

    /// txs archive
    txs_header,
    txs_txs_put
};

// No current need for error_code equivalence mapping.
DECLARE_ERROR_T_CODE_CATEGORY(error);

} // namespace error
} // namespace database
} // namespace libbitcoin

DECLARE_STD_ERROR_REGISTRATION(bc::database::error::error)

#endif
