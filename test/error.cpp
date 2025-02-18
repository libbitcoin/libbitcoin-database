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
#include "test.hpp"

BOOST_AUTO_TEST_SUITE(error_tests)

// error_t
// These test std::error_code equality operator overrides.

BOOST_AUTO_TEST_CASE(error_t__code__success__false_exected_message)
{
    constexpr auto value = error::success;
    const auto ec = code(value);
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "success");
}

BOOST_AUTO_TEST_CASE(error_t__code__unknown_state__true_exected_message)
{
    constexpr auto value = error::unknown_state;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "unknown state");
}

BOOST_AUTO_TEST_CASE(error_t__code__integrity__true_exected_message)
{
    constexpr auto value = error::integrity;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "store corrupted");
}

BOOST_AUTO_TEST_CASE(error_t__code__open_open__true_exected_message)
{
    constexpr auto value = error::open_open;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "opening open file");
}

BOOST_AUTO_TEST_CASE(error_t__code__size_failure__true_exected_message)
{
    constexpr auto value = error::size_failure;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "filure obtaining file size");
}

BOOST_AUTO_TEST_CASE(error_t__code__close_loaded__true_exected_message)
{
    constexpr auto value = error::close_loaded;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "closing loaded file");
}

BOOST_AUTO_TEST_CASE(error_t__code__load_loaded__true_exected_message)
{
    constexpr auto value = error::load_loaded;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "loading loaded file");
}

BOOST_AUTO_TEST_CASE(error_t__code__load_locked__true_exected_message)
{
    constexpr auto value = error::load_locked;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "loading locked file");
}

BOOST_AUTO_TEST_CASE(error_t__code__load_failure__true_exected_message)
{
    constexpr auto value = error::load_failure;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "file failed to load, disk may be full");
}

BOOST_AUTO_TEST_CASE(error_t__code__reload_unloaded__true_exected_message)
{
    constexpr auto value = error::reload_unloaded;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "reloading unloaded file");
}

BOOST_AUTO_TEST_CASE(error_t__code__reload_locked__true_exected_message)
{
    constexpr auto value = error::reload_locked;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "reloading locked file");
}

BOOST_AUTO_TEST_CASE(error_t__code__flush_unloaded__true_exected_message)
{
    constexpr auto value = error::flush_unloaded;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "flushing unloaded file");
}

BOOST_AUTO_TEST_CASE(error_t__code__flush_failure__true_exected_message)
{
    constexpr auto value = error::flush_failure;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "file failed to flush");
}

BOOST_AUTO_TEST_CASE(error_t__code__unload_locked__true_exected_message)
{
    constexpr auto value = error::unload_locked;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "unloading locked file");
}

BOOST_AUTO_TEST_CASE(error_t__code__unload_failure__true_exected_message)
{
    constexpr auto value = error::unload_failure;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "file failed to unload");
}

BOOST_AUTO_TEST_CASE(error_t__code__disk_full__true_exected_message)
{
    constexpr auto value = error::disk_full;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "disk full");
}

BOOST_AUTO_TEST_CASE(error_t__code__mmap_failure__true_exected_message)
{
    constexpr auto value = error::mmap_failure;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "mmap failure");
}

BOOST_AUTO_TEST_CASE(error_t__code__mremap_failure__true_exected_message)
{
    constexpr auto value = error::mremap_failure;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "mremap failure");
}

BOOST_AUTO_TEST_CASE(error_t__code__munmap_failure__true_exected_message)
{
    constexpr auto value = error::munmap_failure;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "munmap failure");
}

BOOST_AUTO_TEST_CASE(error_t__code__madvise_failure__true_exected_message)
{
    constexpr auto value = error::madvise_failure;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "madvise failure");
}

BOOST_AUTO_TEST_CASE(error_t__code__ftruncate_failure__true_exected_message)
{
    constexpr auto value = error::ftruncate_failure;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "ftruncate failure");
}

BOOST_AUTO_TEST_CASE(error_t__code__fsync_failure__true_exected_message)
{
    constexpr auto value = error::fsync_failure;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "fsync failure");
}

BOOST_AUTO_TEST_CASE(error_t__code__transactor_lock__true_exected_message)
{
    constexpr auto value = error::transactor_lock;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "transactor lock failure");
}

BOOST_AUTO_TEST_CASE(error_t__code__process_lock__true_exected_message)
{
    constexpr auto value = error::process_lock;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "process lock failure");
}

BOOST_AUTO_TEST_CASE(error_t__code__flush_lock__true_exected_message)
{
    constexpr auto value = error::flush_lock;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "flush lock failure");
}

BOOST_AUTO_TEST_CASE(error_t__code__flush_unlock__true_exected_message)
{
    constexpr auto value = error::flush_unlock;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "flush unlock failure");
}

BOOST_AUTO_TEST_CASE(error_t__code__process_unlock__true_exected_message)
{
    constexpr auto value = error::process_unlock;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "process unlock failure");
}

BOOST_AUTO_TEST_CASE(error_t__code__missing_directory__true_exected_message)
{
    constexpr auto value = error::missing_directory;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "missing directory failure");
}

BOOST_AUTO_TEST_CASE(error_t__code__clear_directory__true_exected_message)
{
    constexpr auto value = error::clear_directory;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "clear directory failure");
}

BOOST_AUTO_TEST_CASE(error_t__code__rename_directory__true_exected_message)
{
    constexpr auto value = error::rename_directory;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "rename directory failure");
}

BOOST_AUTO_TEST_CASE(error_t__code__missing_snapshot__true_exected_message)
{
    constexpr auto value = error::missing_snapshot;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "missing snapshot");
}

BOOST_AUTO_TEST_CASE(error_t__code__unloaded_file__true_exected_message)
{
    constexpr auto value = error::unloaded_file;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "file not loaded");
}

BOOST_AUTO_TEST_CASE(error_t__code__create_table__true_exected_message)
{
    constexpr auto value = error::create_table;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "failed to create table");
}

BOOST_AUTO_TEST_CASE(error_t__code__close_table__true_exected_message)
{
    constexpr auto value = error::close_table;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "failed to close table");
}

BOOST_AUTO_TEST_CASE(error_t__code__backup_table__true_exected_message)
{
    constexpr auto value = error::backup_table;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "failed to backup table");
}

BOOST_AUTO_TEST_CASE(error_t__code__restore_table__true_exected_message)
{
    constexpr auto value = error::restore_table;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "failed to restore table");
}

BOOST_AUTO_TEST_CASE(error_t__code__verify_table__true_exected_message)
{
    constexpr auto value = error::verify_table;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "failed to verify table");
}

BOOST_AUTO_TEST_CASE(error_t__code__tx_connected__true_exected_message)
{
    constexpr auto value = error::tx_connected;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "transaction connected");
}

BOOST_AUTO_TEST_CASE(error_t__code__tx_disconnected__true_exected_message)
{
    constexpr auto value = error::tx_disconnected;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "transaction disconnected");
}

BOOST_AUTO_TEST_CASE(error_t__code__block_valid__true_exected_message)
{
    constexpr auto value = error::block_valid;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "block valid");
}

BOOST_AUTO_TEST_CASE(error_t__code__block_confirmable__true_exected_message)
{
    constexpr auto value = error::block_confirmable;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "block confirmable");
}

BOOST_AUTO_TEST_CASE(error_t__code__block_unconfirmable__true_exected_message)
{
    constexpr auto value = error::block_unconfirmable;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "block unconfirmable");
}

BOOST_AUTO_TEST_CASE(error_t__code__unassociated__true_exected_message)
{
    constexpr auto value = error::unassociated;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "unassociated");
}

BOOST_AUTO_TEST_CASE(error_t__code__unvalidated__true_exected_message)
{
    constexpr auto value = error::unvalidated;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "unvalidated");
}

BOOST_AUTO_TEST_CASE(error_t__code__missing_previous_output__true_exected_message)
{
    constexpr auto value = error::missing_previous_output;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "missing previous output");
}

BOOST_AUTO_TEST_CASE(error_t__code__coinbase_maturity__true_exected_message)
{
    constexpr auto value = error::coinbase_maturity;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "coinbase maturity");
}

BOOST_AUTO_TEST_CASE(error_t__code__unspent_coinbase_collision__true_exected_message)
{
    constexpr auto value = error::unspent_coinbase_collision;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "unspent coinbase collision");
}

BOOST_AUTO_TEST_CASE(error_t__code__relative_time_locked__true_exected_message)
{
    constexpr auto value = error::relative_time_locked;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "relative time locked");
}

BOOST_AUTO_TEST_CASE(error_t__code__unconfirmed_spend__true_exected_message)
{
    constexpr auto value = error::unconfirmed_spend;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "unconfirmed spend");
}

BOOST_AUTO_TEST_CASE(error_t__code__confirmed_double_spend__true_exected_message)
{
    constexpr auto value = error::confirmed_double_spend;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "confirmed double spend");
}

// tx archive

BOOST_AUTO_TEST_CASE(error_t__code__tx_empty__true_exected_message)
{
    constexpr auto value = error::tx_empty;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "tx_empty");
}

BOOST_AUTO_TEST_CASE(error_t__code__tx_tx_allocate__true_exected_message)
{
    constexpr auto value = error::tx_tx_allocate;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "tx_tx_allocate");
}

BOOST_AUTO_TEST_CASE(error_t__code__tx_spend_allocate__true_exected_message)
{
    constexpr auto value = error::tx_spend_allocate;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "tx_spend_allocate");
}

BOOST_AUTO_TEST_CASE(error_t__code__tx_input_put__true_exected_message)
{
    constexpr auto value = error::tx_input_put;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "tx_input_put");
}

BOOST_AUTO_TEST_CASE(error_t__code__tx_point_allocate__true_exected_message)
{
    constexpr auto value = error::tx_point_allocate;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "tx_point_allocate");
}

BOOST_AUTO_TEST_CASE(error_t__code__tx_point_put__true_exected_message)
{
    constexpr auto value = error::tx_point_put;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "tx_point_put");
}

BOOST_AUTO_TEST_CASE(error_t__code__tx_ins_allocate__true_exected_message)
{
    constexpr auto value = error::tx_ins_allocate;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "tx_ins_allocate");
}

BOOST_AUTO_TEST_CASE(error_t__code__tx_ins_put__true_exected_message)
{
    constexpr auto value = error::tx_ins_put;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "tx_ins_put");
}

BOOST_AUTO_TEST_CASE(error_t__code__tx_output_put__true_exected_message)
{
    constexpr auto value = error::tx_output_put;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "tx_output_put");
}

BOOST_AUTO_TEST_CASE(error_t__code__tx_puts_put__true_exected_message)
{
    constexpr auto value = error::tx_puts_put;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "tx_puts_put");
}

BOOST_AUTO_TEST_CASE(error_t__code__tx_tx_set__true_exected_message)
{
    constexpr auto value = error::tx_tx_set;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "tx_tx_set");
}

BOOST_AUTO_TEST_CASE(error_t__code__tx_spend_put__true_exected_message)
{
    constexpr auto value = error::tx_spend_put;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "tx_spend_put");
}

BOOST_AUTO_TEST_CASE(error_t__code__tx_address_allocate__true_exected_message)
{
    constexpr auto value = error::tx_address_allocate;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "tx_address_allocate");
}

BOOST_AUTO_TEST_CASE(error_t__code__tx_address_put__true_exected_message)
{
    constexpr auto value = error::tx_address_put;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "tx_address_put");
}

BOOST_AUTO_TEST_CASE(error_t__code__tx_tx_commit__true_exected_message)
{
    constexpr auto value = error::tx_tx_commit;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "tx_tx_commit");
}

// header archive

BOOST_AUTO_TEST_CASE(error_t__code__header_put__true_exected_message)
{
    constexpr auto value = error::header_put;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "header_put");
}

// txs archive

BOOST_AUTO_TEST_CASE(error_t__code__txs_header__true_exected_message)
{
    constexpr auto value = error::txs_header;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "txs_header");
}

BOOST_AUTO_TEST_CASE(error_t__code__txs_empty__true_exected_message)
{
    constexpr auto value = error::txs_empty;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "txs_empty");
}

BOOST_AUTO_TEST_CASE(error_t__code__txs_confirm__true_exected_message)
{
    constexpr auto value = error::txs_confirm;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "txs_confirm");
}

BOOST_AUTO_TEST_CASE(error_t__code__txs_txs_put__true_exected_message)
{
    constexpr auto value = error::txs_txs_put;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "txs_txs_put");
}

BOOST_AUTO_TEST_SUITE_END()
