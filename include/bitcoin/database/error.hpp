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
    open_failure,
    size_failure,
    close_loaded,
    close_failure,
    load_locked,
    load_loaded,
    load_failure,
    flush_unloaded,
    flush_failure,
    unload_locked,
    unload_failure,

    // mmap
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
    create_directory,
    clear_directory,
    remove_directory,
    rename_directory,
    copy_directory,
    missing_snapshot,
    create_file,
    unloaded_file,
    dump_file,

    // tables
    create_table,
    close_table,
    backup_table,
    restore_table,
    verify_table,

    /// states

    /// tx fully valid in the given header context.
    tx_connected,

    /// tx conditionally valid in the given header context.
    tx_preconnected,

    /// tx not valid in the given header context.
    tx_disconnected,

    /// block is valid and confirmable.
    block_confirmable,

    /// block is valid but has not been evaluated for confirmability.
    block_preconfirmable,

    /// block is unconfirmable (invalid or spends not confirmable).
    block_unconfirmable,

    /// header has not been invalidated and its txs are not associated.
    unassociated,

    /// header|block has not been validated.
    /// tx has not been validated in the given block context.
    unvalidated,

    /// confirmation (require not just context but prevouts and/or metadata).
    missing_previous_output,
    coinbase_maturity,
    unspent_coinbase_collision,
    relative_time_locked,
    unconfirmed_spend,
    confirmed_double_spend
};

// No current need for error_code equivalence mapping.
DECLARE_ERROR_T_CODE_CATEGORY(error);

} // namespace error
} // namespace database
} // namespace libbitcoin

DECLARE_STD_ERROR_REGISTRATION(bc::database::error::error)

#endif
