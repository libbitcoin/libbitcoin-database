/**
 * Copyright (c) 2011-2026 libbitcoin developers
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
#include "test.hpp"

BC_AUTO_TEST_SUITE(error_tests)

// error_t
// These test std::error_code equality operator overrides.

BC_AUTO_TEST_CASE(error_t__code__success__false_expected_message)
{
    constexpr auto value = error::success;
    const auto ec = code(value);
    BC_REQUIRE(!ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "success");
}

// integrity

BC_AUTO_TEST_CASE(error_t__code__integrity__true_expected_message)
{
    constexpr auto value = error::integrity;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "store corrupted");
}

BC_AUTO_TEST_CASE(error_t__code__integrity_spendable__true_expected_message)
{
    constexpr auto value = error::integrity_spendable;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "store corrupted, unspendable");
}

BC_AUTO_TEST_CASE(error_t__code__integrity_get_prevouts__true_expected_message)
{
    constexpr auto value = error::integrity_get_prevouts;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "store corrupted, get_prevouts");
}

BC_AUTO_TEST_CASE(error_t__code__integrity_block_confirmable1__true_expected_message)
{
    constexpr auto value = error::integrity_block_confirmable1;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "store corrupted, block_confirmable1");
}

BC_AUTO_TEST_CASE(error_t__code__integrity_block_confirmable2__true_expected_message)
{
    constexpr auto value = error::integrity_block_confirmable2;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "store corrupted, block_confirmable2");
}

// memory map

BC_AUTO_TEST_CASE(error_t__code__create_open__true_expected_message)
{
    constexpr auto value = error::create_open;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "creating open file");
}

BC_AUTO_TEST_CASE(error_t__code__open_open__true_expected_message)
{
    constexpr auto value = error::open_open;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "opening open file");
}

BC_AUTO_TEST_CASE(error_t__code__size_failure__true_expected_message)
{
    constexpr auto value = error::size_failure;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "filure obtaining file size");
}

BC_AUTO_TEST_CASE(error_t__code__close_loaded__true_expected_message)
{
    constexpr auto value = error::close_loaded;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "closing loaded file");
}

BC_AUTO_TEST_CASE(error_t__code__load_loaded__true_expected_message)
{
    constexpr auto value = error::load_loaded;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "loading loaded file");
}

BC_AUTO_TEST_CASE(error_t__code__load_locked__true_expected_message)
{
    constexpr auto value = error::load_locked;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "loading locked file");
}

BC_AUTO_TEST_CASE(error_t__code__load_failure__true_expected_message)
{
    constexpr auto value = error::load_failure;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "file failed to load, disk may be full");
}

BC_AUTO_TEST_CASE(error_t__code__reload_unloaded__true_expected_message)
{
    constexpr auto value = error::reload_unloaded;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "reloading unloaded file");
}

BC_AUTO_TEST_CASE(error_t__code__reload_locked__true_expected_message)
{
    constexpr auto value = error::reload_locked;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "reloading locked file");
}

BC_AUTO_TEST_CASE(error_t__code__flush_unloaded__true_expected_message)
{
    constexpr auto value = error::flush_unloaded;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "flushing unloaded file");
}

BC_AUTO_TEST_CASE(error_t__code__flush_failure__true_expected_message)
{
    constexpr auto value = error::flush_failure;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "file failed to flush");
}

BC_AUTO_TEST_CASE(error_t__code__unload_locked__true_expected_message)
{
    constexpr auto value = error::unload_locked;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "unloading locked file");
}

BC_AUTO_TEST_CASE(error_t__code__unload_failure__true_expected_message)
{
    constexpr auto value = error::unload_failure;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "file failed to unload");
}

BC_AUTO_TEST_CASE(error_t__code__shrink_unloaded__true_expected_message)
{
    constexpr auto value = error::shrink_unloaded;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "shrinking unloaded file");
}

BC_AUTO_TEST_CASE(error_t__code__shrink_unload_failure__true_expected_message)
{
    constexpr auto value = error::shrink_unload_failure;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "shrink failed to unload file");
}

BC_AUTO_TEST_CASE(error_t__code__shrink_load_failure__true_expected_message)
{
    constexpr auto value = error::shrink_load_failure;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "shrink failed to load file");
}

BC_AUTO_TEST_CASE(error_t__code__shrink_locked__true_expected_message)
{
    constexpr auto value = error::shrink_locked;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "shrinking locked file");
}

BC_AUTO_TEST_CASE(error_t__code__disk_full__true_expected_message)
{
    constexpr auto value = error::disk_full;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "disk full");
}

BC_AUTO_TEST_CASE(error_t__code__mmap_failure__true_expected_message)
{
    constexpr auto value = error::mmap_failure;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "mmap failure");
}

BC_AUTO_TEST_CASE(error_t__code__mremap_failure__true_expected_message)
{
    constexpr auto value = error::mremap_failure;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "mremap failure");
}

BC_AUTO_TEST_CASE(error_t__code__munmap_failure__true_expected_message)
{
    constexpr auto value = error::munmap_failure;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "munmap failure");
}

BC_AUTO_TEST_CASE(error_t__code__madvise_failure__true_expected_message)
{
    constexpr auto value = error::madvise_failure;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "madvise failure");
}

BC_AUTO_TEST_CASE(error_t__code__sysconf_failure__true_expected_message)
{
    constexpr auto value = error::sysconf_failure;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "sysconf failure");
}

BC_AUTO_TEST_CASE(error_t__code__ftruncate_failure__true_expected_message)
{
    constexpr auto value = error::ftruncate_failure;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "ftruncate failure");
}

BC_AUTO_TEST_CASE(error_t__code__fsync_failure__true_expected_message)
{
    constexpr auto value = error::fsync_failure;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "fsync failure");
}

BC_AUTO_TEST_CASE(error_t__code__transactor_lock__true_expected_message)
{
    constexpr auto value = error::transactor_lock;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "transactor lock failure");
}

BC_AUTO_TEST_CASE(error_t__code__process_lock__true_expected_message)
{
    constexpr auto value = error::process_lock;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "process lock failure");
}

BC_AUTO_TEST_CASE(error_t__code__flush_lock__true_expected_message)
{
    constexpr auto value = error::flush_lock;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "flush lock failure");
}

BC_AUTO_TEST_CASE(error_t__code__flush_unlock__true_expected_message)
{
    constexpr auto value = error::flush_unlock;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "flush unlock failure");
}

BC_AUTO_TEST_CASE(error_t__code__process_unlock__true_expected_message)
{
    constexpr auto value = error::process_unlock;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "process unlock failure");
}

BC_AUTO_TEST_CASE(error_t__code__missing_directory__true_expected_message)
{
    constexpr auto value = error::missing_directory;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "missing directory failure");
}

BC_AUTO_TEST_CASE(error_t__code__clear_directory__true_expected_message)
{
    constexpr auto value = error::clear_directory;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "clear directory failure");
}

BC_AUTO_TEST_CASE(error_t__code__rename_directory__true_expected_message)
{
    constexpr auto value = error::rename_directory;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "rename directory failure");
}

BC_AUTO_TEST_CASE(error_t__code__copy_directory__true_expected_message)
{
    constexpr auto value = error::copy_directory;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "copy directory failure");
}

BC_AUTO_TEST_CASE(error_t__code__not_coalesced__true_expected_message)
{
    constexpr auto value = error::not_coalesced;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "not coalesced");
}

BC_AUTO_TEST_CASE(error_t__code__missing_snapshot__true_expected_message)
{
    constexpr auto value = error::missing_snapshot;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "missing snapshot");
}

BC_AUTO_TEST_CASE(error_t__code__unloaded_file__true_expected_message)
{
    constexpr auto value = error::unloaded_file;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "file not loaded");
}

BC_AUTO_TEST_CASE(error_t__code__create_table__true_expected_message)
{
    constexpr auto value = error::create_table;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "failed to create table");
}

BC_AUTO_TEST_CASE(error_t__code__close_table__true_expected_message)
{
    constexpr auto value = error::close_table;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "failed to close table");
}

BC_AUTO_TEST_CASE(error_t__code__prune_table__true_expected_message)
{
    constexpr auto value = error::prune_table;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "failed to prune table");
}

BC_AUTO_TEST_CASE(error_t__code__backup_table__true_expected_message)
{
    constexpr auto value = error::backup_table;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "failed to backup table");
}

BC_AUTO_TEST_CASE(error_t__code__restore_table__true_expected_message)
{
    constexpr auto value = error::restore_table;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "failed to restore table");
}

BC_AUTO_TEST_CASE(error_t__code__verify_table__true_expected_message)
{
    constexpr auto value = error::verify_table;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "failed to verify table");
}

BC_AUTO_TEST_CASE(error_t__code__block_unconfirmable__true_expected_message)
{
    constexpr auto value = error::block_unconfirmable;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "block unconfirmable");
}

BC_AUTO_TEST_CASE(error_t__code__unvalidated__true_expected_message)
{
    constexpr auto value = error::unvalidated;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "unvalidated");
}

// tx archive

BC_AUTO_TEST_CASE(error_t__code__tx_empty__true_expected_message)
{
    constexpr auto value = error::tx_empty;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "tx_empty");
}

BC_AUTO_TEST_CASE(error_t__code__tx_tx_allocate__true_expected_message)
{
    constexpr auto value = error::tx_tx_allocate;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "tx_tx_allocate");
}

BC_AUTO_TEST_CASE(error_t__code__tx_input_put__true_expected_message)
{
    constexpr auto value = error::tx_input_put;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "tx_input_put");
}

BC_AUTO_TEST_CASE(error_t__code__tx_ins_put__true_expected_message)
{
    constexpr auto value = error::tx_ins_put;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "tx_ins_put");
}

BC_AUTO_TEST_CASE(error_t__code__tx_output_put__true_expected_message)
{
    constexpr auto value = error::tx_output_put;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "tx_output_put");
}

BC_AUTO_TEST_CASE(error_t__code__tx_outs_put__true_expected_message)
{
    constexpr auto value = error::tx_outs_put;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "tx_outs_put");
}

BC_AUTO_TEST_CASE(error_t__code__tx_point_allocate__true_expected_message)
{
    constexpr auto value = error::tx_point_allocate;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "tx_point_allocate");
}

BC_AUTO_TEST_CASE(error_t__code__tx_point_put__true_expected_message)
{
    constexpr auto value = error::tx_point_put;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "tx_point_put");
}

BC_AUTO_TEST_CASE(error_t__code__tx_null_point_put__true_expected_message)
{
    constexpr auto value = error::tx_null_point_put;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "tx_null_point_put");
}

BC_AUTO_TEST_CASE(error_t__code__tx_duplicate_put__true_expected_message)
{
    constexpr auto value = error::tx_duplicate_put;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "tx_duplicate_put");
}

BC_AUTO_TEST_CASE(error_t__code__tx_tx_set__true_expected_message)
{
    constexpr auto value = error::tx_tx_set;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "tx_tx_set");
}

BC_AUTO_TEST_CASE(error_t__code__tx_address_allocate__true_expected_message)
{
    constexpr auto value = error::tx_address_allocate;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "tx_address_allocate");
}

BC_AUTO_TEST_CASE(error_t__code__tx_address_put__true_expected_message)
{
    constexpr auto value = error::tx_address_put;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "tx_address_put");
}

BC_AUTO_TEST_CASE(error_t__code__tx_tx_commit__true_expected_message)
{
    constexpr auto value = error::tx_tx_commit;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "tx_tx_commit");
}

// header archive

BC_AUTO_TEST_CASE(error_t__code__header_put__true_expected_message)
{
    constexpr auto value = error::header_put;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "header_put");
}

BC_AUTO_TEST_CASE(error_t__code__header_state_put__true_expected_message)
{
    constexpr auto value = error::header_state_put;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "header_state_put");
}

// txs archive

BC_AUTO_TEST_CASE(error_t__code__txs_header__true_expected_message)
{
    constexpr auto value = error::txs_header;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "txs_header");
}

BC_AUTO_TEST_CASE(error_t__code__txs_empty__true_expected_message)
{
    constexpr auto value = error::txs_empty;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "txs_empty");
}

BC_AUTO_TEST_CASE(error_t__code__txs_height__true_expected_message)
{
    constexpr auto value = error::txs_height;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "txs_height");
}

BC_AUTO_TEST_CASE(error_t__code__txs_confirm__true_expected_message)
{
    constexpr auto value = error::txs_confirm;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "txs_confirm");
}

BC_AUTO_TEST_CASE(error_t__code__txs_txs_put__true_expected_message)
{
    constexpr auto value = error::txs_txs_put;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "txs_txs_put");
}

BC_AUTO_TEST_CASE(error_t__code__txs_state_put__true_expected_message)
{
    constexpr auto value = error::txs_state_put;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "txs_state_put");
}

// services

BC_AUTO_TEST_CASE(error_t__code__not_found__true_expected_message)
{
    constexpr auto value = error::not_found;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "not_found");
}

BC_AUTO_TEST_CASE(error_t__code__empty_block__true_expected_message)
{
    constexpr auto value = error::empty_block;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "empty_block");
}

BC_AUTO_TEST_CASE(error_t__code__depth_limited__true_expected_message)
{
    constexpr auto value = error::depth_limited;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "depth_limited");
}

BC_AUTO_TEST_CASE(error_t__code__invalid_cursor__true_expected_message)
{
    constexpr auto value = error::invalid_cursor;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "invalid_cursor");
}

BC_AUTO_TEST_CASE(error_t__code__query_canceled__true_expected_message)
{
    constexpr auto value = error::query_canceled;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "query_canceled");
}

BC_AUTO_TEST_CASE(error_t__code__invalid_argument__true_expected_message)
{
    constexpr auto value = error::invalid_argument;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "invalid_argument");
}

BC_AUTO_TEST_CASE(error_t__code__missing_prevouts__true_expected_message)
{
    constexpr auto value = error::missing_prevouts;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "missing_prevouts");
}

BC_AUTO_TEST_CASE(error_t__code__merkle_proof__true_expected_message)
{
    constexpr auto value = error::merkle_proof;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "merkle_proof");
}

BC_AUTO_TEST_CASE(error_t__code__merkle_interval__true_expected_message)
{
    constexpr auto value = error::merkle_interval;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "merkle_interval");
}

BC_AUTO_TEST_CASE(error_t__code__merkle_hashes__true_expected_message)
{
    constexpr auto value = error::merkle_hashes;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "merkle_hashes");
}

BC_AUTO_TEST_SUITE_END()
