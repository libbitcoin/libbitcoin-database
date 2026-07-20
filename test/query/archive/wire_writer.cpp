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
#include "../../test.hpp"
#include "../../mocks/blocks.hpp"
#include "../../mocks/chunk_store.hpp"

BOOST_FIXTURE_TEST_SUITE(query_wire_writer_writer_tests, test::directory_setup_fixture)

// The view (wire) block writer must produce a store byte-identical to the
// chain block writer (see analogous chain_writer expectations).
BOOST_AUTO_TEST_CASE(query_wire_writer__set_block_view__genesis__expected)
{
    constexpr auto milestone = true;
    const auto genesis_header_body = system::base16_chunk(
        "ffff7f"       // next->
        "6fe28c0ab6f1b372c1a6a246ae63f74f931e8365e15a089c68d6190000000000" // sk (block.hash)
        "04030201"     // flags
        "141312"       // height
        "24232221"     // mtp
        "ffffff"       // previous_block_hash (header_fk - not found) (milestone true)
        "01000000"     // version
        "29ab5f49"     // timestamp
        "ffff001d"     // bits
        "1dac2b7c"     // nonce
        "3ba3edfd7a7b12b27ac72c3e67768f617fc81bc3888a51323a9fb8aa4b1e5e4a"); // merkle_root
    const auto genesis_tx_body = system::base16_chunk(
        "ffffff7f"     // next->
        "3ba3edfd7a7b12b27ac72c3e67768f617fc81bc3888a51323a9fb8aa4b1e5e4a" // sk (tx.hash(false))
        "cc0080"       // light (coinbase merged)
        "cc0000"       // heavy
        "00000000"     // locktime
        "01000000"     // version
        "010000"       // ins_count
        "010000"       // outs_count
        "00000000"     // point_fk-> (ins_fk)
        "00000000");   // outs_fk->
    const auto genesis_outs_body = system::base16_chunk(
        "0000000000"); // output0_fk->
    const auto genesis_output_body = system::base16_chunk(
        "00000000"     // parent_fk->
        "ff00f2052a01000000" // value
        "434104678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38c4f35504e51ec112de5c384df7ba0b8d578a4c702b6bf11d5fac"); // script
    const auto genesis_ins_body = system::base16_chunk(
        "ffffffff"     // next->
        "0000000000000000000000000000000000000000000000000000000000000000"
        "ffffff");     // index
    const auto genesis_sequence_body = system::base16_chunk(
        "ffffffff"     // sequence
        "0000000000"   // input_fk-> (coinbase)
        "00000000");   // parent_fk->
    const auto genesis_input_body = system::base16_chunk(
        "4d04ffff001d0104455468652054696d65732030332f4a616e2f32303039204368616e63656c6c6f72206f6e206272696e6b206f66207365636f6e64206261696c6f757420666f722062616e6b73" // script
        "00");         // witness
    const auto genesis_txs_body = system::base16_chunk(
        "1d0100"       // size light (285)
        "1d0100"       // size heavy (285)
        "0100"         // txs count (1)
        "00000000"     // transaction[0]
        "ff"           // depth (255) - genesis only
        "00000000");   // forks (0) - genesis only

    settings settings{};
    settings.header_buckets = 8;
    settings.tx_buckets = 8;
    settings.ins_buckets = 8;
    settings.txs_buckets = 16;
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_CHECK(!store.create(test::events_handler));

    // Set header, then txs from the parsed view of the serialized block.
    BOOST_CHECK(query.set(test::genesis.header(), test::context, milestone));
    system::chain::block_view view{ test::genesis.to_data(true), true };
    BOOST_CHECK(view.is_valid());
    BOOST_CHECK_EQUAL(query.set_code(view, false, true), error::success);
    BOOST_CHECK(query.is_block(test::genesis.hash()));
    BOOST_CHECK(!store.close(test::events_handler));

    BOOST_CHECK_EQUAL(store.header_body(), genesis_header_body);
    BOOST_CHECK_EQUAL(store.tx_body(), genesis_tx_body);
    BOOST_CHECK_EQUAL(store.ins_body(), genesis_ins_body);
    BOOST_CHECK_EQUAL(store.ins_sequence_body(), genesis_sequence_body);
    BOOST_CHECK_EQUAL(store.input_body(), genesis_input_body);
    BOOST_CHECK_EQUAL(store.output_body(), genesis_output_body);
    BOOST_CHECK_EQUAL(store.outs_body(), genesis_outs_body);
    BOOST_CHECK_EQUAL(store.txs_body(), genesis_txs_body);

    const auto pointer = query.get_block(query.to_header(test::genesis.hash()), false);
    BOOST_CHECK(pointer);
    BOOST_CHECK(*pointer == test::genesis);
}

BOOST_AUTO_TEST_SUITE_END()
