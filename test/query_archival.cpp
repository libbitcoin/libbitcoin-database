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
#include "test.hpp"
#include "mocks/blocks.hpp"
#include "mocks/chunk_store.hpp"

struct query_archival_setup_fixture
{
    DELETE4(query_archival_setup_fixture);
    BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

    query_archival_setup_fixture() NOEXCEPT
    {
        BOOST_REQUIRE(test::clear(test::directory));
    }

    ~query_archival_setup_fixture() NOEXCEPT
    {
        BOOST_REQUIRE(test::clear(test::directory));
    }

    BC_POP_WARNING()
};

BOOST_FIXTURE_TEST_SUITE(query_archival_tests, query_archival_setup_fixture)

// archival (natural-keyed)

// slow test (mmap)
BOOST_AUTO_TEST_CASE(query_archival__set_header__mmap_get_header__expected)
{
    constexpr auto parent = system::null_hash;
    constexpr auto merkle_root = system::base16_array("119192939495969798999a9b9c9d9e9f229192939495969798999a9b9c9d9e9f");
    constexpr auto block_hash = system::base16_array("85d0b02a16f6d645aa865fad4a8666f5e7bb2b0c4392a5d675496d6c3defa1f2");
    const system::chain::header header
    {
        0x31323334, // version
        parent,     // previous_block_hash
        merkle_root,// merkle_root
        0x41424344, // timestamp
        0x51525354, // bits
        0x61626364  // nonce
    };

    settings settings1{};
    settings1.header_buckets = 10;
    settings1.dir = TEST_DIRECTORY;
    store<map> store1{ settings1 };
    query<store<map>> query1{ store1 };
    BOOST_REQUIRE_EQUAL(store1.create(), error::success);

    // must open/close mmap
    BOOST_REQUIRE_EQUAL(store1.open(), error::success);
    BOOST_REQUIRE(query1.set(header, test::context));

    table::header::record element1{};
    BOOST_REQUIRE(store1.header.get(query1.to_header(block_hash), element1));

    const auto pointer = query1.get_header(query1.to_header(block_hash));
    BOOST_REQUIRE(pointer);
    BOOST_REQUIRE(*pointer == header);

    // must open/close mmap
    BOOST_REQUIRE_EQUAL(store1.close(), error::success);
    BOOST_REQUIRE_EQUAL(element1.ctx.height, system::mask_left(test::context.height, byte_bits));
    BOOST_REQUIRE_EQUAL(element1.ctx.flags, test::context.flags);
    BOOST_REQUIRE_EQUAL(element1.ctx.mtp, test::context.mtp);
    BOOST_REQUIRE_EQUAL(element1.version, header.version());
    BOOST_REQUIRE_EQUAL(element1.parent_fk, linkage<schema::header::pk>::terminal);
    BOOST_REQUIRE_EQUAL(element1.merkle_root, header.merkle_root());
    BOOST_REQUIRE_EQUAL(element1.timestamp, header.timestamp());
    BOOST_REQUIRE_EQUAL(element1.bits, header.bits());
    BOOST_REQUIRE_EQUAL(element1.nonce, header.nonce());
}

BOOST_AUTO_TEST_CASE(query_archival__set_header__is_header__expected)
{
    constexpr auto merkle_root = system::base16_array("119192939495969798999a9b9c9d9e9f229192939495969798999a9b9c9d9e9f");
    constexpr auto block_hash = system::base16_array("85d0b02a16f6d645aa865fad4a8666f5e7bb2b0c4392a5d675496d6c3defa1f2");
    const system::chain::header header
    {
        0x31323334, // version
        system::null_hash, // previous_block_hash
        merkle_root,// merkle_root
        0x41424344, // timestamp
        0x51525354, // bits
        0x61626364  // nonce
    };
    const auto expected_header_head = system::base16_chunk(
        "010000" // record count
        "ffffff" // bucket[0]...
        "000000" // pk->
        "ffffff"
        "ffffff"
        "ffffff"
        "ffffff"
        "ffffff"
        "ffffff"
        "ffffff"
        "ffffff");
    const auto expected_header_body = system::base16_chunk(
        "ffffff"   // next->
        "85d0b02a16f6d645aa865fad4a8666f5e7bb2b0c4392a5d675496d6c3defa1f2" // sk (block.hash)
        "04030201" // flags
        "141312"   // height
        "24232221" // mtp
        "ffffff"   // previous_block_hash (header_fk - not found)
        "34333231" // version
        "44434241" // timestamp
        "54535251" // bits
        "64636261" // nonce
        "119192939495969798999a9b9c9d9e9f229192939495969798999a9b9c9d9e9f"); //merkle_root

    settings settings1{};
    settings1.header_buckets = 10;
    settings1.dir = TEST_DIRECTORY;
    test::chunk_store store1{ settings1 };
    test::query_accessor query1{ store1 };
    BOOST_REQUIRE_EQUAL(store1.create(), error::success);

    // store open/close flushes record count to head.
    BOOST_REQUIRE_EQUAL(store1.open(), error::success);
    BOOST_REQUIRE(!query1.is_header(header.hash()));
    BOOST_REQUIRE(query1.set(header, test::context));
    BOOST_REQUIRE(query1.is_header(header.hash()));
    table::header::record element1{};
    BOOST_REQUIRE(store1.header.get(query1.to_header(block_hash), element1));
    BOOST_REQUIRE_EQUAL(store1.close(), error::success);
    BOOST_REQUIRE_EQUAL(store1.header_head(), expected_header_head);
    BOOST_REQUIRE_EQUAL(store1.header_body(), expected_header_body);

    BOOST_REQUIRE_EQUAL(element1.ctx.height, system::mask_left(test::context.height, byte_bits));
    BOOST_REQUIRE_EQUAL(element1.ctx.flags, test::context.flags);
    BOOST_REQUIRE_EQUAL(element1.ctx.mtp, test::context.mtp);
    BOOST_REQUIRE_EQUAL(element1.version, header.version());
    BOOST_REQUIRE_EQUAL(element1.parent_fk, linkage<schema::header::pk>::terminal);
    BOOST_REQUIRE_EQUAL(element1.merkle_root, header.merkle_root());
    BOOST_REQUIRE_EQUAL(element1.timestamp, header.timestamp());
    BOOST_REQUIRE_EQUAL(element1.bits, header.bits());
    BOOST_REQUIRE_EQUAL(element1.nonce, header.nonce());
}

BOOST_AUTO_TEST_CASE(query_archival__set_tx__empty__expected)
{
    const system::chain::transaction tx{};
    const auto expected_head4_array = system::base16_chunk("00000000");
    const auto expected_head5_array = system::base16_chunk("0000000000");
    const auto expected_head4_hash = system::base16_chunk(
        "00000000" // record count
        "ffffffff" // bucket[0]...
        "ffffffff"
        "ffffffff"
        "ffffffff"
        "ffffffff");
    const auto expected_head5_hash = system::base16_chunk(
        "0000000000" // record count
        "ffffffffff" // bucket[0]...
        "ffffffffff"
        "ffffffffff"
        "ffffffffff"
        "ffffffffff");

    // data_chunk store.
    settings settings1{};
    settings1.tx_buckets = 5;
    settings1.point_buckets = 5;
    settings1.input_buckets = 5;
    settings1.dir = TEST_DIRECTORY;
    test::chunk_store store1{ settings1 };
    test::query_accessor query1{ store1 };
    BOOST_REQUIRE_EQUAL(store1.create(), error::success);

    // store open/close flushes record count to heads.
    BOOST_REQUIRE_EQUAL(store1.open(), error::success);
    BOOST_REQUIRE(!query1.set(tx));
    BOOST_REQUIRE_EQUAL(store1.close(), error::success);
    BOOST_REQUIRE_EQUAL(store1.tx_head(), expected_head4_hash);
    BOOST_REQUIRE_EQUAL(store1.point_head(), expected_head4_hash);
    BOOST_REQUIRE_EQUAL(store1.input_head(), expected_head5_hash);
    BOOST_REQUIRE_EQUAL(store1.output_head(), expected_head5_array);
    BOOST_REQUIRE_EQUAL(store1.puts_head(), expected_head4_array);
    BOOST_REQUIRE(store1.tx_body().empty());
    BOOST_REQUIRE(store1.point_body().empty());
    BOOST_REQUIRE(store1.input_body().empty());
    BOOST_REQUIRE(store1.output_body().empty());
    BOOST_REQUIRE(store1.puts_body().empty());
}

BOOST_AUTO_TEST_CASE(query_archival__set_tx__null_input__expected)
{
    using namespace system::chain;
    const transaction tx
    {
        0x01020304,    // version
        inputs
        {
            input{ point{}, script{}, witness{}, 0 }
        },
        outputs
        {
            output{ 0, script{} }
        },
        0x11121314     // locktime
    };
    const auto expected_tx_head = system::base16_chunk(
        "01000000"     // record count
        "ffffffff"     // bucket[0]...
        "00000000"     // pk->
        "ffffffff"
        "ffffffff"
        "ffffffff");
    const auto expected_tx_body = system::base16_chunk(
        "ffffffff"     // next->
        "601f0fa54d6de8362c17dc883cc047e1f3ae0523d732598a05e3010fac591f62" // sk (tx.hash(false))
        "01"           // coinbase
        "3c0000"       // witless
        "3c0000"       // witness
        "14131211"     // locktime
        "04030201"     // version
        "010000"       // ins_count
        "010000"       // outs_count
        "00000000");   // puts_fk->
    const auto expected_puts_head = system::base16_chunk("02000000");
    const auto expected_puts_body = system::base16_chunk(
        "0000000000"   // input0_fk->
        "0000000000"); // output0_fk->
    const auto expected_output_head = system::base16_chunk("0700000000");
    const auto expected_output_body = system::base16_chunk(
        "00000000"     // parent_fk->
        "00"           // index
        "00"           // value
        "00");         // script
    const auto expected_point_head = system::base16_chunk(
        "00000000"     // record count (null point, empty)
        "ffffffff"
        "ffffffff"
        "ffffffff"
        "ffffffff"
        "ffffffff");
    const auto expected_point_body = system::base16_chunk("");
    const auto expected_input_head = system::base16_chunk(
        "1700000000"   // slabs size
        "ffffffffff"   // bucket[0]...
        "ffffffffff"
        "0000000000"   // pk->
        "ffffffffff"
        "ffffffffff");
    const auto expected_input_body = system::base16_chunk(
        "ffffffffff"   // next->
        "ffffffff"     // sk (point_fk)
        "ffffff"       // sk (point.index)
        "00000000"     // parent_fk->
        "00"           // index
        "00000000"     // sequence
        "00"           // script
        "00");         // witness

    constexpr auto tx_hash = system::base16_array("601f0fa54d6de8362c17dc883cc047e1f3ae0523d732598a05e3010fac591f62");
    BOOST_REQUIRE_EQUAL(tx_hash, tx.hash(false));

    // data_chunk store.
    settings settings1{};
    settings1.tx_buckets = 5;
    settings1.point_buckets = 5;
    settings1.input_buckets = 5;
    settings1.dir = TEST_DIRECTORY;
    test::chunk_store store1{ settings1 };
    test::query_accessor query1{ store1 };
    BOOST_REQUIRE_EQUAL(store1.create(), error::success);
    BOOST_REQUIRE_EQUAL(store1.open(), error::success);
    BOOST_REQUIRE(query1.set(tx));
    BOOST_REQUIRE_EQUAL(store1.close(), error::success);
    BOOST_REQUIRE_EQUAL(store1.tx_head(), expected_tx_head);
    BOOST_REQUIRE_EQUAL(store1.point_head(), expected_point_head);
    BOOST_REQUIRE_EQUAL(store1.input_head(), expected_input_head);
    BOOST_REQUIRE_EQUAL(store1.output_head(), expected_output_head);
    BOOST_REQUIRE_EQUAL(store1.puts_head(), expected_puts_head);
    BOOST_REQUIRE_EQUAL(store1.tx_body(), expected_tx_body);
    BOOST_REQUIRE_EQUAL(store1.point_body(), expected_point_body);
    BOOST_REQUIRE_EQUAL(store1.input_body(), expected_input_body);
    BOOST_REQUIRE_EQUAL(store1.output_body(), expected_output_body);
    BOOST_REQUIRE_EQUAL(store1.puts_body(), expected_puts_body);
}

BOOST_AUTO_TEST_CASE(query_archival__set_tx__get_tx__expected)
{
    using namespace system::chain;
    const transaction tx
    {
        0x2a,          // version
        inputs
        {
            input
            {
                point{ system::one_hash, 0x18 },
                script{ { { opcode::op_return }, { opcode::pick } } },
                witness{ "[242424]" },
                0x2a     // sequence
            },
            input
            {
                point{ system::one_hash, 0x2a },
                script{ { { opcode::op_return }, { opcode::roll } } },
                witness{ "[424242]" },
                0x18     // sequence
            }
        },
        outputs
        {
            output
            {
                0x18,    // value
                script{ { { opcode::pick } } }
            },
            output
            {
                0x2a,    // value
                script{ { { opcode::roll } } }
            }
        },
        0x18             // locktime
    };
    const auto expected_tx_head = system::base16_chunk(
        "01000000"       // record count
        "ffffffff"       // bucket[0]...
        "ffffffff"
        "00000000"       // pk->
        "ffffffff"
        "ffffffff");
    const auto expected_tx_body = system::base16_chunk(
        "ffffffff"       // next->
        "d80f19b9c0f649081c0b279d9183b0fae35b41b72a34eb181001f82afe22043a" // sk (tx.hash(false))
        "00"             // coinbase
        "740000"         // witless
        "800000"         // witness
        "18000000"       // locktime
        "2a000000"       // version
        "020000"         // ins_count
        "020000"         // outs_count
        "00000000");     // puts_fk->
    const auto expected_puts_head = system::base16_chunk("04000000");
    const auto expected_puts_body = system::base16_chunk(
        "0000000000"     // input0_fk->
        "1d00000000"     // input1_fk->
        "0000000000"     // output0_fk->
        "0800000000");   // output1_fk->
    const auto expected_output_head = system::base16_chunk("1000000000");
    const auto expected_output_body = system::base16_chunk(
        "00000000"       // parent_fk->
        "00"             // index
        "18"             // value
        "0179"           // script
        "00000000"       // parent_fk->
        "01"             // index
        "2a"             // value
        "017a");         // script
    const auto expected_point_head = system::base16_chunk(
        "01000000"       // record count
        "ffffffff"       // bucket[0]...
        "ffffffff"
        "ffffffff"
        "ffffffff"
        "00000000");     // pk->
    const auto expected_point_body = system::base16_chunk(
        "ffffffff"       // next->
        "0100000000000000000000000000000000000000000000000000000000000000"); // sk (prevout.hash)
    const auto expected_input_head = system::base16_chunk(
        "3a00000000"     // slabs size
        "1d00000000"     // pk->[1d]
        "ffffffffff"
        "ffffffffff"
        "0000000000"     // pk->[0]
        "ffffffffff");
    const auto expected_input_body = system::base16_chunk(
        "ffffffffff"     // [0] next->
        "00000000"       // sk (point_fk)
        "180000"         // sk (point.index)
        "00000000"       // parent_fk->
        "00"             // index
        "2a000000"       // sequence
        "026a79"         // script
        "0103242424"     // witness
        "ffffffffff"     // [1d] next->
        "00000000"       // sk (point_fk)
        "2a0000"         // sk (point.index)
        "00000000"       // parent_fk->
        "01"             // index
        "18000000"       // sequence
        "026a7a"         // script
        "0103424242");   // witness

    constexpr auto tx_hash = system::base16_array("d80f19b9c0f649081c0b279d9183b0fae35b41b72a34eb181001f82afe22043a");
    BOOST_REQUIRE_EQUAL(tx_hash, tx.hash(false));

    // data_chunk store.
    settings settings1{};
    settings1.tx_buckets = 5;
    settings1.point_buckets = 5;
    settings1.input_buckets = 5;
    settings1.dir = TEST_DIRECTORY;
    test::chunk_store store1{ settings1 };
    test::query_accessor query1{ store1 };
    BOOST_REQUIRE_EQUAL(store1.create(), error::success);
    BOOST_REQUIRE_EQUAL(store1.open(), error::success);
    BOOST_REQUIRE(!query1.is_tx(tx.hash(false)));
    BOOST_REQUIRE(query1.set(tx));
    BOOST_REQUIRE(query1.is_tx(tx.hash(false)));

    const auto pointer1 = query1.get_tx(query1.to_tx(tx_hash));
    BOOST_REQUIRE(pointer1);
    BOOST_REQUIRE(*pointer1 == tx);

    BOOST_REQUIRE_EQUAL(store1.close(), error::success);

    BOOST_REQUIRE_EQUAL(store1.tx_head(), expected_tx_head);
    BOOST_REQUIRE_EQUAL(store1.point_head(), expected_point_head);
    BOOST_REQUIRE_EQUAL(store1.input_head(), expected_input_head);
    BOOST_REQUIRE_EQUAL(store1.output_head(), expected_output_head);
    BOOST_REQUIRE_EQUAL(store1.puts_head(), expected_puts_head);
    BOOST_REQUIRE_EQUAL(store1.tx_body(), expected_tx_body);
    BOOST_REQUIRE_EQUAL(store1.point_body(), expected_point_body);
    BOOST_REQUIRE_EQUAL(store1.input_body(), expected_input_body);
    BOOST_REQUIRE_EQUAL(store1.output_body(), expected_output_body);
    BOOST_REQUIRE_EQUAL(store1.puts_body(), expected_puts_body);
}

BOOST_AUTO_TEST_CASE(query_archival__set_block__get_block__expected)
{
    const auto genesis_header_head = system::base16_chunk(
        "010000"       // record count
        "ffffff"       // bucket[0]...
        "000000"       // pk->
        "ffffff"
        "ffffff"
        "ffffff");
    const auto genesis_header_body = system::base16_chunk(
        "ffffff"       // next->
        "6fe28c0ab6f1b372c1a6a246ae63f74f931e8365e15a089c68d6190000000000" // sk (block.hash)
        "04030201"     // flags
        "141312"       // height
        "24232221"     // mtp
        "ffffff"       // previous_block_hash (header_fk - not found)
        "01000000"     // version
        "29ab5f49"     // timestamp
        "ffff001d"     // bits
        "1dac2b7c"     // nonce
        "3ba3edfd7a7b12b27ac72c3e67768f617fc81bc3888a51323a9fb8aa4b1e5e4a"); // merkle_root
    const auto genesis_tx_head = system::base16_chunk(
        "01000000"     // record count
        "ffffffff"     // bucket[0]...
        "ffffffff"
        "ffffffff"
        "00000000"     // pk->
        "ffffffff");
    const auto genesis_tx_body = system::base16_chunk(
        "ffffffff"     // next->
        "3ba3edfd7a7b12b27ac72c3e67768f617fc81bc3888a51323a9fb8aa4b1e5e4a" // sk (tx.hash(false))
        "01"           // coinbase
        "cc0000"       // witless
        "cc0000"       // witness
        "00000000"     // locktime
        "01000000"     // version
        "010000"       // ins_count
        "010000"       // outs_count
        "00000000");   // puts_fk->
    const auto genesis_puts_head = system::base16_chunk("02000000");
    const auto genesis_puts_body = system::base16_chunk(
        "0000000000"   // input0_fk->
        "0000000000"); // output0_fk->
    const auto genesis_output_head = system::base16_chunk("5200000000");
    const auto genesis_output_body = system::base16_chunk(
        "00000000"     // parent_fk->
        "00"           // index
        "ff00f2052a01000000" // value
        "434104678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38c4f35504e51ec112de5c384df7ba0b8d578a4c702b6bf11d5fac"); // script
    const auto genesis_point_head = system::base16_chunk(
        "00000000"     // record count (null point, empty)
        "ffffffff"
        "ffffffff"
        "ffffffff"
        "ffffffff"
        "ffffffff");
    const auto genesis_point_body = system::base16_chunk("");
    const auto genesis_input_head = system::base16_chunk(
        "6400000000"   // slabs size
        "ffffffffff"
        "ffffffffff"
        "0000000000"   // pk->[0]
        "ffffffffff"
        "ffffffffff");
    const auto genesis_input_body = system::base16_chunk(
        "ffffffffff"   // next->
        "ffffffff"     // sk (point_fk)
        "ffffff"       // sk (point.index) [this tests 4 bytes null_index recovery]
        "00000000"     // parent_fk->
        "00"           // index
        "ffffffff"     // sequence
        "4d04ffff001d0104455468652054696d65732030332f4a616e2f32303039204368616e63656c6c6f72206f6e206272696e6b206f66207365636f6e64206261696c6f757420666f722062616e6b73" // script
        "00");         // witness
    const auto genesis_txs_head = system::base16_chunk(
        "0f000000"     // slabs size
        "ffffffff"
        "ffffffff"
        "ffffffff"
        "ffffffff"
        "ffffffff"
        "ffffffff"
        "ffffffff"
        "00000000"      // pk->
        "ffffffff"
        "ffffffff");
    const auto genesis_txs_body = system::base16_chunk(
        "ffffffff"      // next->
        "000000"        // header_fk
        "01000000"      // txs count (1)
        "00000000");    // transaction[0]

    settings settings1{};
    settings1.header_buckets = 5;
    settings1.tx_buckets = 5;
    settings1.point_buckets = 5;
    settings1.input_buckets = 5;
    settings1.txs_buckets = 10;
    settings1.dir = TEST_DIRECTORY;
    test::chunk_store store1{ settings1 };
    test::query_accessor query1{ store1 };
    BOOST_REQUIRE_EQUAL(store1.create(), error::success);
    BOOST_REQUIRE_EQUAL(store1.open(), error::success);

    // Set header/tx/association.
    BOOST_REQUIRE(!query1.is_block(test::genesis.hash()));
    BOOST_REQUIRE(query1.set(test::genesis, test::context));
    BOOST_REQUIRE(query1.is_block(test::genesis.hash()));

    // Verify idempotentcy (these do not change store state).
    BOOST_REQUIRE(query1.set(test::genesis.header(), test::context));
    BOOST_REQUIRE(query1.set(test::genesis.header(), test::context));
    BOOST_REQUIRE(query1.set(test::genesis, test::context));
    BOOST_REQUIRE(query1.set(test::genesis, test::context));
    BOOST_REQUIRE(query1.set(query1.to_header(test::genesis.hash()), test::genesis.transaction_hashes(false)));
    BOOST_REQUIRE(query1.set(query1.to_header(test::genesis.hash()), test::genesis.transaction_hashes(true)));

    table::header::record element1{};
    BOOST_REQUIRE(store1.header.get(query1.to_header(test::genesis.hash()), element1));
    BOOST_REQUIRE_EQUAL(store1.close(), error::success);

    BOOST_REQUIRE_EQUAL(store1.header_head(), genesis_header_head);
    BOOST_REQUIRE_EQUAL(store1.tx_head(), genesis_tx_head);
    BOOST_REQUIRE_EQUAL(store1.point_head(), genesis_point_head);
    BOOST_REQUIRE_EQUAL(store1.input_head(), genesis_input_head);
    BOOST_REQUIRE_EQUAL(store1.output_head(), genesis_output_head);
    BOOST_REQUIRE_EQUAL(store1.puts_head(), genesis_puts_head);
    BOOST_REQUIRE_EQUAL(store1.txs_head(), genesis_txs_head);

    BOOST_REQUIRE_EQUAL(store1.header_body(), genesis_header_body);
    BOOST_REQUIRE_EQUAL(store1.tx_body(), genesis_tx_body);
    BOOST_REQUIRE_EQUAL(store1.point_body(), genesis_point_body);
    BOOST_REQUIRE_EQUAL(store1.input_body(), genesis_input_body);
    BOOST_REQUIRE_EQUAL(store1.output_body(), genesis_output_body);
    BOOST_REQUIRE_EQUAL(store1.puts_body(), genesis_puts_body);
    BOOST_REQUIRE_EQUAL(store1.txs_body(), genesis_txs_body);

    const auto pointer1 = query1.get_block(query1.to_header(test::genesis.hash()));
    BOOST_REQUIRE(pointer1);
    BOOST_REQUIRE(*pointer1 == test::genesis);

    const auto hashes = query1.get_txs(query1.to_header(test::genesis.hash()));
    BOOST_REQUIRE_EQUAL(hashes.size(), 1u);
    BOOST_REQUIRE_EQUAL(hashes, test::genesis.transaction_hashes(false));
}

BOOST_AUTO_TEST_CASE(query_archival__set_txs__get_block__expected)
{
    settings settings1{};
    settings1.header_buckets = 5;
    settings1.tx_buckets = 5;
    settings1.point_buckets = 5;
    settings1.input_buckets = 5;
    settings1.txs_buckets = 10;
    settings1.dir = TEST_DIRECTORY;
    test::chunk_store store1{ settings1 };
    test::query_accessor query1{ store1 };
    BOOST_REQUIRE_EQUAL(store1.create(), error::success);
    BOOST_REQUIRE_EQUAL(store1.open(), error::success);

    // Assemble block.
    BOOST_REQUIRE(query1.set(test::genesis.header(), test::context));
    BOOST_REQUIRE(query1.set(*test::genesis.transactions_ptr()->front()));
    BOOST_REQUIRE(query1.set(query1.to_header(test::genesis.hash()), test::genesis.transaction_hashes(false)));

    const auto pointer1 = query1.get_block(query1.to_header(test::genesis.hash()));
    BOOST_REQUIRE(pointer1);
    BOOST_REQUIRE(*pointer1 == test::genesis);
}

BOOST_AUTO_TEST_CASE(query_archival__populate__null_prevout__false)
{
    settings settings{};
    settings.dir = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));

    system::chain::block copy{ test::genesis };
    BOOST_REQUIRE(!query.populate(copy));
    BOOST_REQUIRE(!query.populate(*test::genesis.transactions_ptr()->front()));
    BOOST_REQUIRE(!query.populate(*test::genesis.inputs_ptr()->front()));
}

// archival (foreign-keyed)

BOOST_AUTO_TEST_CASE(query_archival__get_header__default__expected)
{
    constexpr auto root = system::base16_array("119192939495969798999a9b9c9d9e9f229192939495969798999a9b9c9d9e9f");
    constexpr auto block_hash = system::base16_array("85d0b02a16f6d645aa865fad4a8666f5e7bb2b0c4392a5d675496d6c3defa1f2");
    const system::chain::header header
    {
        0x31323334, // version
        system::null_hash, // previous_block_hash
        root,       // merkle_root
        0x41424344, // timestamp
        0x51525354, // bits
        0x61626364  // nonce
    };
    const auto expected_header_head = system::base16_chunk(
        "010000" // record count
        "ffffff" // bucket[0]...
        "000000" // pk->
        "ffffff"
        "ffffff"
        "ffffff"
        "ffffff"
        "ffffff"
        "ffffff"
        "ffffff"
        "ffffff");
    const auto expected_header_body = system::base16_chunk(
        "ffffff"   // next->
        "85d0b02a16f6d645aa865fad4a8666f5e7bb2b0c4392a5d675496d6c3defa1f2" // sk (block.hash)
        "14131211" // flags
        "040302"   // height
        "24232221" // mtp
        "ffffff"   // previous_block_hash (header_fk - not found)
        "34333231" // version
        "44434241" // timestamp
        "54535251" // bits
        "64636261" // nonce
        "119192939495969798999a9b9c9d9e9f229192939495969798999a9b9c9d9e9f"); // merkle_root

    settings settings1{};
    settings1.header_buckets = 10;
    settings1.dir = TEST_DIRECTORY;
    test::chunk_store store1{ settings1 };
    test::query_accessor query1{ store1 };
    BOOST_REQUIRE_EQUAL(store1.create(), error::success);
    BOOST_REQUIRE_EQUAL(store1.open(), error::success);

    store1.header_head() = expected_header_head;
    store1.header_body() = expected_header_body;
    const auto pointer1 = query1.get_header(query1.to_header(block_hash));
    BOOST_REQUIRE(pointer1);
    BOOST_REQUIRE(*pointer1 == header);
}

BOOST_AUTO_TEST_CASE(query_archival__get_txs__not_found__empty)
{
    settings settings1{};
    settings1.dir = TEST_DIRECTORY;
    test::chunk_store store1{ settings1 };
    test::query_accessor query1{ store1 };
    BOOST_REQUIRE_EQUAL(store1.create(), error::success);
    BOOST_REQUIRE_EQUAL(store1.open(), error::success);
    BOOST_REQUIRE(query1.get_txs(query1.to_header(system::null_hash)).empty());
    BOOST_REQUIRE_EQUAL(store1.close(), error::success);
}

BOOST_AUTO_TEST_CASE(query_archival__get_input__not_found__nullptr)
{
    settings settings1{};
    settings1.dir = TEST_DIRECTORY;
    test::chunk_store store1{ settings1 };
    test::query_accessor query1{ store1 };
    BOOST_REQUIRE_EQUAL(store1.create(), error::success);
    BOOST_REQUIRE_EQUAL(store1.open(), error::success);
    BOOST_REQUIRE(!query1.get_input(query1.to_tx(system::null_hash), 0u));
    BOOST_REQUIRE_EQUAL(store1.close(), error::success);
}

BOOST_AUTO_TEST_CASE(query_archival__get_input__genesis__expected)
{
    settings settings1{};
    settings1.header_buckets = 5;
    settings1.tx_buckets = 5;
    settings1.point_buckets = 5;
    settings1.input_buckets = 5;
    settings1.txs_buckets = 10;
    settings1.dir = TEST_DIRECTORY;
    test::chunk_store store1{ settings1 };
    test::query_accessor query1{ store1 };
    BOOST_REQUIRE_EQUAL(store1.create(), error::success);
    BOOST_REQUIRE_EQUAL(store1.open(), error::success);
    BOOST_REQUIRE(query1.set(test::genesis, test::context));

    const auto tx = test::genesis.transactions_ptr()->front();
    const auto input = query1.get_input(query1.to_tx(tx->hash(false)), 0u);
    BOOST_REQUIRE(input);
    BOOST_REQUIRE(*input == *tx->inputs_ptr()->front());
}

BOOST_AUTO_TEST_CASE(query_archival__get_output__not_found__nullptr)
{
    settings settings1{};
    settings1.dir = TEST_DIRECTORY;
    test::chunk_store store1{ settings1 };
    test::query_accessor query1{ store1 };
    BOOST_REQUIRE_EQUAL(store1.create(), error::success);
    BOOST_REQUIRE_EQUAL(store1.open(), error::success);
    BOOST_REQUIRE(!query1.get_output(query1.to_tx(system::null_hash), 0u));
    BOOST_REQUIRE_EQUAL(store1.close(), error::success);
}

BOOST_AUTO_TEST_CASE(query_archival__get_output__genesis__expected)
{
    settings settings1{};
    settings1.header_buckets = 5;
    settings1.tx_buckets = 5;
    settings1.point_buckets = 5;
    settings1.input_buckets = 5;
    settings1.txs_buckets = 10;
    settings1.dir = TEST_DIRECTORY;
    test::chunk_store store1{ settings1 };
    test::query_accessor query1{ store1 };
    BOOST_REQUIRE_EQUAL(store1.create(), error::success);
    BOOST_REQUIRE_EQUAL(store1.open(), error::success);
    BOOST_REQUIRE(query1.set(test::genesis, test::context));

    // get_output1
    const auto tx = test::genesis.transactions_ptr()->front();
    auto output1 = query1.get_output(query1.to_tx(tx->hash(false)), 0u);
    BOOST_REQUIRE(output1);
    BOOST_REQUIRE(*output1 == *tx->outputs_ptr()->front());
}

BOOST_AUTO_TEST_CASE(query_archival__get_spenders__not_found__empty)
{
    settings settings1{};
    settings1.dir = TEST_DIRECTORY;
    test::chunk_store store1{ settings1 };
    test::query_accessor query1{ store1 };
    BOOST_REQUIRE_EQUAL(store1.create(), error::success);
    BOOST_REQUIRE_EQUAL(store1.open(), error::success);
    BOOST_REQUIRE(query1.get_spenders(tx_link::terminal, 0u)->empty());
    BOOST_REQUIRE_EQUAL(store1.close(), error::success);
}

BOOST_AUTO_TEST_SUITE_END()
