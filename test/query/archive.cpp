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
#include "../test.hpp"
#include "../mocks/blocks.hpp"
#include "../mocks/chunk_store.hpp"

struct query_archive_setup_fixture
{
    DELETE_COPY_MOVE(query_archive_setup_fixture);
    BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

    query_archive_setup_fixture() NOEXCEPT
    {
        BOOST_REQUIRE(test::clear(test::directory));
    }

    ~query_archive_setup_fixture() NOEXCEPT
    {
        BOOST_REQUIRE(test::clear(test::directory));
    }

    BC_POP_WARNING()
};

BOOST_FIXTURE_TEST_SUITE(query_archive_tests, query_archive_setup_fixture)

// ensure context::flags is same size as chain_context::flags.
static_assert(is_same_type<database::context::flag_t::integer, decltype(system::chain::context{}.flags)>);

// nop event handler.
const auto events_handler = [](auto, auto) {};

// archive (natural-keyed)

// slow test (mmap)
BOOST_AUTO_TEST_CASE(query_archive__set_header__mmap_get_header__expected)
{
    constexpr auto milestone = false;
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

    settings settings{};
    settings.header_buckets = 16;
    settings.path = TEST_DIRECTORY;
    store<map> store{ settings };
    query<database::store<map>> query{ store };
    BOOST_REQUIRE(!store.create(events_handler));
    BOOST_REQUIRE(query.set(header, test::context, milestone));

    table::header::record element1{};
    BOOST_REQUIRE(store.header.get(query.to_header(block_hash), element1));

    const auto pointer = query.get_header(query.to_header(block_hash));
    BOOST_REQUIRE(pointer);
    BOOST_REQUIRE(*pointer == header);

    // must open/close mmap
    BOOST_REQUIRE(!store.close(events_handler));
    BOOST_REQUIRE_EQUAL(element1.ctx.height, system::mask_left(test::context.height, byte_bits));
    BOOST_REQUIRE_EQUAL(element1.ctx.flags, test::context.flags);
    BOOST_REQUIRE_EQUAL(element1.ctx.mtp, test::context.mtp);
    BOOST_REQUIRE_EQUAL(element1.milestone, milestone);
    BOOST_REQUIRE_EQUAL(element1.version, header.version());
    BOOST_REQUIRE_EQUAL(element1.parent_fk, schema::header::link::terminal);
    BOOST_REQUIRE_EQUAL(element1.merkle_root, header.merkle_root());
    BOOST_REQUIRE_EQUAL(element1.timestamp, header.timestamp());
    BOOST_REQUIRE_EQUAL(element1.bits, header.bits());
    BOOST_REQUIRE_EQUAL(element1.nonce, header.nonce());
}

BOOST_AUTO_TEST_CASE(query_archive__set_link_header__is_header__expected)
{
    constexpr auto milestone = true;
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

    // nosh
    const auto expected_header_head = system::base16_chunk(
        "010000ff" // record count
        "ffffffff" // bucket[0]...
        "ffffffff"
        "ffffffff"
        "ffffffff"
        "ffffffff"
        "000080"   // filter[8], pk->
        "de"       // filter[0-7]
        "ffffffff"
        "ffffffff"
        "ffffffff"
        "ffffffff"
        "ffffffff"
        "ffffffff"
        "ffffffff"
        "ffffffff"
        "ffffffff"
        "ffffffff");

    const auto expected_header_body = system::base16_chunk(
        "ffff7f"   // next->
        "85d0b02a16f6d645aa865fad4a8666f5e7bb2b0c4392a5d675496d6c3defa1f2" // sk (block.hash)
        "04030201" // flags
        "141312"   // height
        "24232221" // mtp
        "01"       // milestone
        "ffff7f"   // previous_block_hash (header_fk - not found)
        "34333231" // version
        "44434241" // timestamp
        "54535251" // bits
        "64636261" // nonce
        "119192939495969798999a9b9c9d9e9f229192939495969798999a9b9c9d9e9f"); //merkle_root

    header_link link{};
    settings settings{};
    settings.header_buckets = 16;
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(events_handler));

    // store open/close flushes record count to head.
    BOOST_REQUIRE(!query.is_header(header.hash()));
    BOOST_REQUIRE(!query.is_associated(0));
    BOOST_REQUIRE(!query.set_code(link, header, test::context, milestone));
    BOOST_REQUIRE(!link.is_terminal());
    BOOST_REQUIRE(query.is_header(header.hash()));
    BOOST_REQUIRE(!query.is_associated(0));
    table::header::record element1{};
    BOOST_REQUIRE(store.header.get(query.to_header(block_hash), element1));
    BOOST_REQUIRE(!store.close(events_handler));
    BOOST_REQUIRE_EQUAL(store.header_head(), expected_header_head);
    BOOST_REQUIRE_EQUAL(store.header_body(), expected_header_body);

    BOOST_REQUIRE_EQUAL(element1.ctx.height, system::mask_left(test::context.height, byte_bits));
    BOOST_REQUIRE_EQUAL(element1.ctx.flags, test::context.flags);
    BOOST_REQUIRE_EQUAL(element1.ctx.mtp, test::context.mtp);
    BOOST_REQUIRE_EQUAL(element1.milestone, milestone);
    BOOST_REQUIRE_EQUAL(element1.version, header.version());
    BOOST_REQUIRE_EQUAL(element1.parent_fk, schema::header::link::terminal);
    BOOST_REQUIRE_EQUAL(element1.merkle_root, header.merkle_root());
    BOOST_REQUIRE_EQUAL(element1.timestamp, header.timestamp());
    BOOST_REQUIRE_EQUAL(element1.bits, header.bits());
    BOOST_REQUIRE_EQUAL(element1.nonce, header.nonce());
}

BOOST_AUTO_TEST_CASE(query_archive__set_tx__empty__expected)
{
    const system::chain::transaction tx{};
    const auto expected_head5_array = system::base16_chunk("0000000000");
    const auto expected_head4_array = system::base16_chunk("00000000");
    const auto expected_head4_hash = system::base16_chunk(
        "01000000" // record count
        "ffffffff" // bucket[0]...
        "ffffffff"
        "ffffffff"
        "ffffffff"
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
    settings settings{};
    settings.tx_buckets = 8;
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(events_handler));

    // store open/close flushes record count to heads.
    BOOST_REQUIRE(!query.set(tx));
    BOOST_REQUIRE(!store.close(events_handler));
    BOOST_REQUIRE_EQUAL(store.tx_head(), expected_head4_hash);
    BOOST_REQUIRE_EQUAL(store.input_head(), expected_head5_array);
    BOOST_REQUIRE_EQUAL(store.output_head(), expected_head5_array);
    BOOST_REQUIRE_EQUAL(store.outs_head(), expected_head4_array);
    BOOST_REQUIRE_EQUAL(store.tx_body().size(), schema::transaction::minrow);
    BOOST_REQUIRE(store.point_body().empty());
    BOOST_REQUIRE(store.input_body().empty());
    BOOST_REQUIRE(store.output_body().empty());
    BOOST_REQUIRE(store.outs_body().empty());
}

BOOST_AUTO_TEST_CASE(query_archive__set_link_tx__null_input__expected)
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
        "00000000"     // bucket[0]... pk->
        "ffffffff"
        "ffffffff"
        "ffffffff"
        "ffffffff"
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
        "0000000000"); // outs_fk->
    const auto expected_outs_head = system::base16_chunk("0900000000");
    const auto expected_outs_body = system::base16_chunk(
        "00000000"     // spend0_fk->
        "0000000000"); // output0_fk->
    const auto expected_output_head = system::base16_chunk("0600000000");
    const auto expected_output_body = system::base16_chunk(
        "00000000"     // parent_fk->
        "00"           // value
        "00");         // script
    const auto expected_point_body = system::base16_chunk("");
    const auto expected_spend_head = system::base16_chunk(
        "01000000"     // record count
        "00000000"     // pk->
        "ffffffff"
        "ffffffff"
        "ffffffff"
        "ffffffff");
    const auto expected_spend_body = system::base16_chunk(
        "ffffffff"     // terminal->
        "00000000"     // fp: point_stub
        "ffffff"       // fp: point_index (null)
        "ffffffff"     // point_fk->
        "00000000"     // parent_fk->
        "00000000"     // sequence
        "0000000000"); // input_fk->
    const auto expected_input_head = system::base16_chunk("0200000000");
    const auto expected_input_body = system::base16_chunk(
        "00"           // script
        "00");         // witness
    constexpr auto tx_hash = system::base16_array("601f0fa54d6de8362c17dc883cc047e1f3ae0523d732598a05e3010fac591f62");
    BOOST_REQUIRE_EQUAL(tx_hash, tx.hash(false));

    // data_chunk store.
    settings settings{};
    settings.tx_buckets = 8;
    settings.point_buckets = 8;
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(events_handler));
    BOOST_REQUIRE(!query.set_code(tx));
    BOOST_REQUIRE(!store.close(events_handler));

    BOOST_REQUIRE_EQUAL(store.tx_head(), expected_tx_head);
    BOOST_REQUIRE_EQUAL(store.input_head(), expected_input_head);
    BOOST_REQUIRE_EQUAL(store.output_head(), expected_output_head);
////    BOOST_REQUIRE_EQUAL(store.outs_head(), expected_outs_head);
////    BOOST_REQUIRE_EQUAL(store.spend_head(), expected_spend_head);

////    BOOST_REQUIRE_EQUAL(store.tx_body(), expected_tx_body);
////    BOOST_REQUIRE_EQUAL(store.point_body(), expected_point_body);
    BOOST_REQUIRE_EQUAL(store.input_body(), expected_input_body);
    BOOST_REQUIRE_EQUAL(store.output_body(), expected_output_body);
////    BOOST_REQUIRE_EQUAL(store.outs_body(), expected_outs_body);
////    BOOST_REQUIRE_EQUAL(store.spend_body(), expected_spend_body);
}

BOOST_AUTO_TEST_CASE(query_archive__set_tx__get_tx__expected)
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
        "01000000"     // record count
        "00000000"     // bucket[0]... pk->
        "ffffffff"
        "ffffffff"
        "ffffffff"
        "ffffffff"
        "ffffffff"
        "ffffffff"
        "ffffffff");
    const auto expected_tx_body = system::base16_chunk(
        "ffffffff"     // next->
        "d80f19b9c0f649081c0b279d9183b0fae35b41b72a34eb181001f82afe22043a" // sk (tx.hash(false))
        "00"           // coinbase
        "740000"       // witless
        "800000"       // witness
        "18000000"     // locktime
        "2a000000"     // version
        "020000"       // ins_count
        "020000"       // outs_count
        "0000000000"); // outs_fk->
    const auto expected_outs_head = system::base16_chunk("1200000000");
    const auto expected_outs_body = system::base16_chunk(
        "00000000"     // spend0_fk->
        "01000000"     // spend1_fk->
        "0000000000"   // output0_fk->
        "0700000000"); // output1_fk->
    const auto expected_output_head = system::base16_chunk("0e00000000");
    const auto expected_output_body = system::base16_chunk(
        "00000000"     // parent_fk->
        "18"           // value
        "0179"         // script
        "00000000"     // parent_fk->
        "2a"           // value
        "017a");       // script
    const auto expected_point_body = system::base16_chunk(
        "0100000000000000000000000000000000000000000000000000000000000000"
        "0100000000000000000000000000000000000000000000000000000000000000");
    const auto expected_spend_head = system::base16_chunk(
        "02000000"     // record count
        "00000000"     // spend0_fk->
        "ffffffff"
        "ffffffff"
        "01000000"     // spend1_fk->
        "ffffffff");
    const auto expected_spend_body = system::base16_chunk(
        "ffffffff"     // terminal->
        "01000000"     // fp: point_stub
        "180000"       // fp: point_index
        "00000000"     // point_fk->
        "00000000"     // parent_fk->
        "2a000000"     // sequence
        "0000000000"   // input_fk->

        "ffffffff"     // terminal->
        "01000000"     // fp: point_stub
        "2a0000"       // fp: point_index
        "01000000"     // point_fk->
        "00000000"     // parent_fk->
        "18000000"     // sequence
        "0800000000"); // input_fk->
    const auto expected_input_head = system::base16_chunk("1000000000");
    const auto expected_input_body = system::base16_chunk(
        "026a79"       // script
        "0103242424"   // witness
        "026a7a"       // script
        "0103424242"); // witness

    constexpr auto tx_hash = system::base16_array("d80f19b9c0f649081c0b279d9183b0fae35b41b72a34eb181001f82afe22043a");
    BOOST_REQUIRE_EQUAL(tx_hash, tx.hash(false));

    // data_chunk store.
    settings settings{};
    settings.tx_buckets = 8;
    settings.point_buckets = 8;
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(events_handler));
    BOOST_REQUIRE(!query.is_tx(tx.hash(false)));
    BOOST_REQUIRE(query.set(tx));
    BOOST_REQUIRE(query.is_tx(tx.hash(false)));

    const auto pointer1 = query.get_transaction(query.to_tx(tx_hash));
    BOOST_REQUIRE(pointer1);
////    BOOST_REQUIRE(*pointer1 == tx);
    BOOST_REQUIRE_EQUAL(pointer1->hash(false), tx_hash);
    BOOST_REQUIRE(!store.close(events_handler));

    BOOST_REQUIRE_EQUAL(store.tx_head(), expected_tx_head);
    BOOST_REQUIRE_EQUAL(store.input_head(), expected_input_head);
    BOOST_REQUIRE_EQUAL(store.output_head(), expected_output_head);
////    BOOST_REQUIRE_EQUAL(store.outs_head(), expected_outs_head);
////    BOOST_REQUIRE_EQUAL(store.spend_head(), expected_spend_head);

////    BOOST_REQUIRE_EQUAL(store.tx_body(), expected_tx_body);
////    BOOST_REQUIRE_EQUAL(store.point_body(), expected_point_body);
    BOOST_REQUIRE_EQUAL(store.input_body(), expected_input_body);
    BOOST_REQUIRE_EQUAL(store.output_body(), expected_output_body);
////    BOOST_REQUIRE_EQUAL(store.outs_body(), expected_outs_body);
////    BOOST_REQUIRE_EQUAL(store.spend_body(), expected_spend_body);
}

BOOST_AUTO_TEST_CASE(query_archive__set_block__get_block__expected)
{
    constexpr auto milestone = true;
    const auto genesis_header_head = system::base16_chunk(
        "010000ff"     // record count
        "ffffffff"     // bucket[0]...
        "ffffffff"
        "ffffffff"
        "ffffffff"
        "ffffffff"
        "ffffffff"
        "ffffffff"
        "000080"       // filter[8], pk->
        "da");         // filter[0-7]
    const auto genesis_header_body = system::base16_chunk(
        "ffff7f"       // next->
        "6fe28c0ab6f1b372c1a6a246ae63f74f931e8365e15a089c68d6190000000000" // sk (block.hash)
        "04030201"     // flags
        "141312"       // height
        "24232221"     // mtp
        "01"           // milestone
        "ffff7f"       // previous_block_hash (header_fk - not found)
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
        "ffffffff"
        "ffffffff"
        "ffffffff"
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
        "0000000000");   // outs_fk->
    const auto genesis_outs_head = system::base16_chunk("0900000000");
    const auto genesis_outs_body = system::base16_chunk(
        "00000000"     // spend0_fk->
        "0000000000"); // output0_fk->

    const auto genesis_output_head = system::base16_chunk("5100000000");
    const auto genesis_output_body = system::base16_chunk(
        "00000000"     // parent_fk->
        "ff00f2052a01000000" // value
        "434104678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38c4f35504e51ec112de5c384df7ba0b8d578a4c702b6bf11d5fac"); // script
    const auto genesis_point_body = system::base16_chunk("");
    const auto genesis_spend_head = system::base16_chunk(
        "01000000"     // record count
        "00000000"     // spend0_fk->
        "ffffffff"
        "ffffffff"
        "ffffffff"
        "ffffffff");
    const auto genesis_spend_body = system::base16_chunk(
        "ffffffff"     // terminal->
        "00000000"     // fp: point_stub
        "ffffff"       // fp: point_index (null)
        "ffffffff"     // point_fk->
        "00000000"     // parent_fk->
        "ffffffff"     // sequence
        "0000000000"); // input_fk-> (coinbase)
    const auto genesis_input_head = system::base16_chunk("4f00000000");
    const auto genesis_input_body = system::base16_chunk(
        "4d04ffff001d0104455468652054696d65732030332f4a616e2f32303039204368616e63656c6c6f72206f6e206272696e6b206f66207365636f6e64206261696c6f757420666f722062616e6b73" // script
        "00");         // witness
    const auto genesis_txs_head = system::base16_chunk(
        "1200000000"   // slabs size
        "0000000000"   // pk->
        "ffffffffff"
        "ffffffffff"
        "ffffffffff"
        "ffffffffff"
        "ffffffffff"
        "ffffffffff"
        "ffffffffff"
        "ffffffffff"
        "ffffffffff"
        "ffffffffff"
        "ffffffffff"
        "ffffffffff"
        "ffffffffff"
        "ffffffffff"
        "ffffffffff");
    const auto genesis_txs_body = system::base16_chunk(
        "ffffffffff"   // next->
        "000000"       // header_fk
        "010000"       // txs count (1)
        "1d0100"       // txs wire (285)
        "00000000");   // transaction[0]

    settings settings{};
    settings.header_buckets = 8;
    settings.tx_buckets = 8;
    settings.point_buckets = 8;
    settings.txs_buckets = 16;
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(events_handler));

    // Set block (header/txs).
    BOOST_REQUIRE(!query.is_block(test::genesis.hash()));
    BOOST_REQUIRE(query.set(test::genesis, test::context, milestone, false));
    BOOST_REQUIRE(query.is_block(test::genesis.hash()));

    // Verify idempotentcy (these do not change store state).
    ////BOOST_REQUIRE(query.set(test::genesis.header(), test::context, milestone, false));
    ////BOOST_REQUIRE(query.set(test::genesis.header(), test::context, milestone, false));
    ////BOOST_REQUIRE(query.set(test::genesis, test::context, milestone, false, false));
    ////BOOST_REQUIRE(query.set(test::genesis, test::context, milestone, false, false));

    table::header::record element1{};
    BOOST_REQUIRE(store.header.get(query.to_header(test::genesis.hash()), element1));
    BOOST_REQUIRE(!store.close(events_handler));

    BOOST_REQUIRE_EQUAL(store.header_head(), genesis_header_head);
    BOOST_REQUIRE_EQUAL(store.tx_head(), genesis_tx_head);
    BOOST_REQUIRE_EQUAL(store.input_head(), genesis_input_head);
    BOOST_REQUIRE_EQUAL(store.output_head(), genesis_output_head);
////    BOOST_REQUIRE_EQUAL(store.outs_head(), genesis_outs_head);
////    BOOST_REQUIRE_EQUAL(store.spend_head(), genesis_spend_head);
    BOOST_REQUIRE_EQUAL(store.txs_head(), genesis_txs_head);

    BOOST_REQUIRE_EQUAL(store.header_body(), genesis_header_body);
////    BOOST_REQUIRE_EQUAL(store.tx_body(), genesis_tx_body);
////    BOOST_REQUIRE_EQUAL(store.point_body(), genesis_point_body);
    BOOST_REQUIRE_EQUAL(store.input_body(), genesis_input_body);
    BOOST_REQUIRE_EQUAL(store.output_body(), genesis_output_body);
////    BOOST_REQUIRE_EQUAL(store.spend_body(), genesis_spend_body);
    BOOST_REQUIRE_EQUAL(store.txs_body(), genesis_txs_body);

    const auto pointer1 = query.get_block(query.to_header(test::genesis.hash()));
    BOOST_REQUIRE(pointer1);
////    BOOST_REQUIRE(*pointer1 == test::genesis);

    const auto hashes = query.get_tx_keys(query.to_header(test::genesis.hash()));
    BOOST_REQUIRE_EQUAL(hashes.size(), 1u);
    BOOST_REQUIRE_EQUAL(hashes, test::genesis.transaction_hashes(false));
}

BOOST_AUTO_TEST_CASE(query_archive__set_block_txs__get_block__expected)
{
    constexpr auto milestone = true;
    const auto genesis_header_head = system::base16_chunk(
        "010000ff"     // record count
        "ffffffff"     // bucket[0]...
        "ffffffff"
        "ffffffff"
        "ffffffff"
        "ffffffff"
        "ffffffff"
        "ffffffff"
        "000080"       // filter[8], pk->
        "da");         // filter[0-7]
    const auto genesis_header_body = system::base16_chunk(
        "ffff7f"       // next->
        "6fe28c0ab6f1b372c1a6a246ae63f74f931e8365e15a089c68d6190000000000" // sk (block.hash)
        "04030201"     // flags
        "141312"       // height
        "24232221"     // mtp
        "01"           // milestone
        "ffff7f"       // previous_block_hash (header_fk - not found)
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
        "ffffffff"
        "ffffffff"
        "ffffffff"
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
        "0000000000"); // outs_fk->
    const auto genesis_outs_head = system::base16_chunk("0900000000");
    const auto genesis_outs_body = system::base16_chunk(
        "00000000"     // spend0_fk->
        "0000000000"); // output0_fk->

    const auto genesis_output_head = system::base16_chunk("5100000000");
    const auto genesis_output_body = system::base16_chunk(
        "00000000"     // parent_fk->
        "ff00f2052a01000000" // value
        "434104678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38c4f35504e51ec112de5c384df7ba0b8d578a4c702b6bf11d5fac"); // script
    const auto genesis_point_body = system::base16_chunk("");
    const auto genesis_spend_head = system::base16_chunk(
        "01000000"     // record count
        "00000000"     // spend0_fk->
        "ffffffff"
        "ffffffff"
        "ffffffff"
        "ffffffff");
    const auto genesis_spend_body = system::base16_chunk(
        "ffffffff"     // terminal->
        "00000000"     // fp: point_hash
        "ffffff"       // fp: point_index (null)
        "ffffffff"     // point_fk->
        "00000000"     // parent_fk->
        "ffffffff"     // sequence
        "0000000000"); // input_fk-> (coinbase)
    const auto genesis_input_head = system::base16_chunk("4f00000000");
    const auto genesis_input_body = system::base16_chunk(
        "4d04ffff001d0104455468652054696d65732030332f4a616e2f32303039204368616e63656c6c6f72206f6e206272696e6b206f66207365636f6e64206261696c6f757420666f722062616e6b73" // script
        "00");         // witness
    const auto genesis_txs_head = system::base16_chunk(
        "1200000000"   // slabs size
        "0000000000"   // pk->
        "ffffffffff"
        "ffffffffff"
        "ffffffffff"
        "ffffffffff"
        "ffffffffff"
        "ffffffffff"
        "ffffffffff"
        "ffffffffff"
        "ffffffffff"
        "ffffffffff"
        "ffffffffff"
        "ffffffffff"
        "ffffffffff"
        "ffffffffff"
        "ffffffffff");
    const auto genesis_txs_body = system::base16_chunk(
        "ffffffffff"   // next->
        "000000"       // header_fk
        "010000"       // txs count (1)
        "1d0100"       // txs wire (285)
        "00000000");   // transaction[0]

    settings settings{};
    settings.header_buckets = 8;
    settings.tx_buckets = 8;
    settings.point_buckets = 8;
    settings.txs_buckets = 16;
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(events_handler));

    // Set header and then txs.
    BOOST_REQUIRE(!query.is_block(test::genesis.hash()));
    BOOST_REQUIRE(query.set(test::genesis.header(), test::context, milestone));
    BOOST_REQUIRE(!query.is_associated(0));
    BOOST_REQUIRE(query.set(test::genesis, false));
    BOOST_REQUIRE(query.is_block(test::genesis.hash()));
    BOOST_REQUIRE(query.is_associated(0));

    // Verify idempotentcy (these do not change store state).
    ////BOOST_REQUIRE(query.set(test::genesis.header(), test::context, milestone));
    ////BOOST_REQUIRE(query.set(test::genesis.header(), test::context, milestone));
    ////BOOST_REQUIRE(query.set(test::genesis, test::context, milestone));
    ////BOOST_REQUIRE(query.set(test::genesis, test::context, milestone));

    table::header::record element1{};
    BOOST_REQUIRE(store.header.get(query.to_header(test::genesis.hash()), element1));
    BOOST_REQUIRE(!store.close(events_handler));

    BOOST_REQUIRE_EQUAL(store.header_head(), genesis_header_head);
    BOOST_REQUIRE_EQUAL(store.tx_head(), genesis_tx_head);
    BOOST_REQUIRE_EQUAL(store.input_head(), genesis_input_head);
    BOOST_REQUIRE_EQUAL(store.output_head(), genesis_output_head);
////    BOOST_REQUIRE_EQUAL(store.outs_head(), genesis_outs_head);
////    BOOST_REQUIRE_EQUAL(store.spend_head(), genesis_spend_head);
    BOOST_REQUIRE_EQUAL(store.txs_head(), genesis_txs_head);

    BOOST_REQUIRE_EQUAL(store.header_body(), genesis_header_body);
////    BOOST_REQUIRE_EQUAL(store.tx_body(), genesis_tx_body);
////    BOOST_REQUIRE_EQUAL(store.point_body(), genesis_point_body);
    BOOST_REQUIRE_EQUAL(store.input_body(), genesis_input_body);
    BOOST_REQUIRE_EQUAL(store.output_body(), genesis_output_body);
////    BOOST_REQUIRE_EQUAL(store.spend_body(), genesis_spend_body);
    BOOST_REQUIRE_EQUAL(store.txs_body(), genesis_txs_body);

    const auto pointer1 = query.get_block(query.to_header(test::genesis.hash()));
    BOOST_REQUIRE(pointer1);
////    BOOST_REQUIRE(*pointer1 == test::genesis);

    const auto hashes = query.get_tx_keys(query.to_header(test::genesis.hash()));
    BOOST_REQUIRE_EQUAL(hashes.size(), 1u);
    BOOST_REQUIRE_EQUAL(hashes, test::genesis.transaction_hashes(false));
}

// populate_with_metadata
// ----------------------------------------------------------------------------

// METADATA IS SETTABLE ON CONST TEST OBJECTS.
// COPY CONSTRUCTION CREATES SAHRED POINTER REFERENCES.
// SO POPULATE ON CONST OBJECTS HAS SIDE EFFECTS IF THE OBJECTS SPAN TESTS.
const auto& clean_(const auto& block_or_tx) NOEXCEPT
{
    const auto inputs = block_or_tx.inputs_ptr();
    for (const auto& input: *inputs)
    {
        input->prevout.reset();
        input->metadata = system::chain::prevout{};
    }

    return block_or_tx;
}

// First four blocks have only coinbase txs.
BOOST_AUTO_TEST_CASE(query_archive__populate_with_metadata__null_prevouts__true)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, test::context, false, false));
    BOOST_REQUIRE(query.set(test::block2, test::context, false, false));
    BOOST_REQUIRE(query.set(test::block3, test::context, false, false));

    const auto& copy = clean_(test::genesis);
    const auto& copy1 = clean_(test::block1);
    const auto& copy2 = clean_(test::block2);
    const auto& copy3 = clean_(test::block3);

    BOOST_REQUIRE(query.populate_with_metadata(copy));
    BOOST_REQUIRE(query.populate_with_metadata(copy1));
    BOOST_REQUIRE(query.populate_with_metadata(copy2));
    BOOST_REQUIRE(query.populate_with_metadata(copy3));
}

BOOST_AUTO_TEST_CASE(query_archive__populate_with_metadata__partial_prevouts__false)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1a, test::context, false, false));
    BOOST_REQUIRE(query.set(test::block2a, test::context, false, false));
    BOOST_REQUIRE(query.set(test::tx4));

    const auto& block1a = clean_(test::block1a);

    // Block populate treates first tx as null point.
    BOOST_REQUIRE( query.populate_with_metadata(block1a));
    BOOST_REQUIRE(!query.populate_with_metadata(*block1a.transactions_ptr()->at(0)));
    BOOST_REQUIRE(!query.populate_with_metadata(*block1a.inputs_ptr()->at(0)));
    BOOST_REQUIRE(!query.populate_with_metadata(*block1a.inputs_ptr()->at(2)));

    // Block populate treates first tx as null point and other has missing prevouts.
    const auto& block2a = clean_(test::block2a);
    BOOST_REQUIRE(!query.populate_with_metadata(block2a));
    BOOST_REQUIRE( query.populate_with_metadata(*block2a.transactions_ptr()->at(0)));
    BOOST_REQUIRE(!query.populate_with_metadata(*block2a.transactions_ptr()->at(1)));
    BOOST_REQUIRE( query.populate_with_metadata(*block2a.inputs_ptr()->at(0)));
    BOOST_REQUIRE(!query.populate_with_metadata(*block2a.inputs_ptr()->at(3)));

    const auto& tx4 = clean_(test::tx4);
    BOOST_REQUIRE(query.populate_with_metadata(tx4));
    BOOST_REQUIRE(query.populate_with_metadata(*tx4.inputs_ptr()->at(0)));
    BOOST_REQUIRE(query.populate_with_metadata(*tx4.inputs_ptr()->at(1)));
}

BOOST_AUTO_TEST_CASE(query_archive__populate_with_metadata__metadata__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1a, test::context, false, false));
    BOOST_REQUIRE(query.set(test::block2a, test::context, false, false));
    BOOST_REQUIRE(query.set(test::tx4));
    BOOST_REQUIRE(!query.is_coinbase(1));

    // Genesis only has coinbase, which does not spend.
    const auto& genesis = clean_(test::genesis);

    BOOST_REQUIRE(!genesis.inputs_ptr()->at(0)->prevout);
    BOOST_REQUIRE( genesis.inputs_ptr()->at(0)->metadata.inside);
    BOOST_REQUIRE( genesis.inputs_ptr()->at(0)->metadata.coinbase);
    BOOST_REQUIRE_EQUAL(genesis.inputs_ptr()->at(0)->metadata.parent, 0u);

    BOOST_REQUIRE(query.populate_with_metadata(genesis));

    BOOST_REQUIRE(!genesis.inputs_ptr()->at(0)->prevout);
    BOOST_REQUIRE( genesis.inputs_ptr()->at(0)->metadata.inside);
    BOOST_REQUIRE( genesis.inputs_ptr()->at(0)->metadata.coinbase);
    BOOST_REQUIRE_EQUAL(genesis.inputs_ptr()->at(0)->metadata.parent, 0u);

    // Transaction population.
    const auto& tx4 = clean_(test::tx4);

    BOOST_REQUIRE(!tx4.inputs_ptr()->at(0)->prevout);
    BOOST_REQUIRE( tx4.inputs_ptr()->at(0)->metadata.inside);
    BOOST_REQUIRE( tx4.inputs_ptr()->at(0)->metadata.coinbase);
    BOOST_REQUIRE_EQUAL(tx4.inputs_ptr()->at(0)->metadata.parent, 0u);
    BOOST_REQUIRE(!tx4.inputs_ptr()->at(1)->prevout);
    BOOST_REQUIRE( tx4.inputs_ptr()->at(1)->metadata.inside);
    BOOST_REQUIRE( tx4.inputs_ptr()->at(1)->metadata.coinbase);
    BOOST_REQUIRE_EQUAL(tx4.inputs_ptr()->at(1)->metadata.parent, 0u);

    BOOST_REQUIRE(query.populate_with_metadata(tx4));

    // TODO: test non-coinbase and other parent.
    // spent/mtp are defaults, coinbase/parent are set (to non-default values).
    BOOST_REQUIRE( tx4.inputs_ptr()->at(0)->prevout);
    BOOST_REQUIRE(!tx4.inputs_ptr()->at(0)->metadata.inside);
    BOOST_REQUIRE(!tx4.inputs_ptr()->at(0)->metadata.coinbase);
    BOOST_REQUIRE_EQUAL(tx4.inputs_ptr()->at(0)->metadata.parent, 1u);
    BOOST_REQUIRE( tx4.inputs_ptr()->at(1)->prevout);
    BOOST_REQUIRE(!tx4.inputs_ptr()->at(1)->metadata.inside);
    BOOST_REQUIRE(!tx4.inputs_ptr()->at(1)->metadata.coinbase);
    BOOST_REQUIRE_EQUAL(tx4.inputs_ptr()->at(1)->metadata.parent, 1u);
}

// populate_without_metadata
// ----------------------------------------------------------------------------

// First four blocks have only coinbase txs.
BOOST_AUTO_TEST_CASE(query_archive__populate_without_metadata__null_prevouts__true)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, test::context, false, false));
    BOOST_REQUIRE(query.set(test::block2, test::context, false, false));
    BOOST_REQUIRE(query.set(test::block3, test::context, false, false));
    BOOST_REQUIRE(query.populate_without_metadata(test::genesis));
    BOOST_REQUIRE(query.populate_without_metadata(test::block1));
    BOOST_REQUIRE(query.populate_without_metadata(test::block2));
    BOOST_REQUIRE(query.populate_without_metadata(test::block3));
}

BOOST_AUTO_TEST_CASE(query_archive__populate_without_metadata__partial_prevouts__false)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1a, test::context, false, false));
    BOOST_REQUIRE(query.set(test::block2a, test::context, false, false));
    BOOST_REQUIRE(query.set(test::tx4));

    // Block populate treates first tx as null point.
    BOOST_REQUIRE( query.populate_without_metadata(test::block1a));
    BOOST_REQUIRE(!query.populate_without_metadata(*test::block1a.transactions_ptr()->at(0)));
    BOOST_REQUIRE(!query.populate_without_metadata(*test::block1a.inputs_ptr()->at(0)));
    BOOST_REQUIRE(!query.populate_without_metadata(*test::block1a.inputs_ptr()->at(2)));

    // Block populate treates first tx as null point and other has missing prevouts.
    BOOST_REQUIRE(!query.populate_without_metadata(test::block2a));
    BOOST_REQUIRE( query.populate_without_metadata(*test::block2a.transactions_ptr()->at(0)));
    BOOST_REQUIRE(!query.populate_without_metadata(*test::block2a.transactions_ptr()->at(1)));
    BOOST_REQUIRE( query.populate_without_metadata(*test::block2a.inputs_ptr()->at(0)));
    BOOST_REQUIRE(!query.populate_without_metadata(*test::block2a.inputs_ptr()->at(3)));

    BOOST_REQUIRE(query.populate_without_metadata(test::tx4));
    BOOST_REQUIRE(query.populate_without_metadata(*test::tx4.inputs_ptr()->at(0)));
    BOOST_REQUIRE(query.populate_without_metadata(*test::tx4.inputs_ptr()->at(1)));
}

// ----------------------------------------------------------------------------

// archive (foreign-keyed)

BOOST_AUTO_TEST_CASE(query_archive__is_coinbase__coinbase__true)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, context{}, false, false));
    BOOST_REQUIRE(query.set(test::block2, context{}, false, false));
    BOOST_REQUIRE(query.set(test::block3, context{}, false, false));
    BOOST_REQUIRE(query.is_coinbase(0));
    BOOST_REQUIRE(query.is_coinbase(1));
    BOOST_REQUIRE(query.is_coinbase(2));
    BOOST_REQUIRE(query.is_coinbase(3));
}

BOOST_AUTO_TEST_CASE(query_archive__is_coinbase__non_coinbase__false)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1a, context{}, false, false));
    BOOST_REQUIRE(query.set(test::block2a, context{}, false, false));
    BOOST_REQUIRE(!query.is_coinbase(1));
    BOOST_REQUIRE(!query.is_coinbase(2));
    BOOST_REQUIRE(!query.is_coinbase(3));
    BOOST_REQUIRE(!query.is_coinbase(4));
    BOOST_REQUIRE(!query.is_coinbase(5));
    BOOST_REQUIRE(!query.is_coinbase(42));
}

BOOST_AUTO_TEST_CASE(query_archive__is_milestone__genesis__false)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(events_handler), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(!query.is_milestone(0));
    BOOST_REQUIRE(!query.is_milestone(1));
}

BOOST_AUTO_TEST_CASE(query_archive__is_milestone__set__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(events_handler), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, context{}, true, false));
    BOOST_REQUIRE(query.set(test::block2, context{}, false, false));;
    BOOST_REQUIRE(!query.is_milestone(0));
    BOOST_REQUIRE(query.is_milestone(1));
    BOOST_REQUIRE(!query.is_milestone(2));
}

BOOST_AUTO_TEST_CASE(query_archive__get_header__invalid_parent__expected)
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
        "424242"   // previous_block_hash (header_fk - invalid)
        "34333231" // version
        "44434241" // timestamp
        "54535251" // bits
        "64636261" // nonce
        "119192939495969798999a9b9c9d9e9f229192939495969798999a9b9c9d9e9f"); // merkle_root

    settings settings{};
    settings.header_buckets = 16;
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(events_handler));

    store.header_head() = expected_header_head;
    store.header_body() = expected_header_body;
    BOOST_REQUIRE(!query.get_header(query.to_header(block_hash)));
    BOOST_REQUIRE(!query.get_header(header_link::terminal));
}

BOOST_AUTO_TEST_CASE(query_archive__get_header__default__expected)
{
    constexpr auto root = system::base16_array("119192939495969798999a9b9c9d9e9f229192939495969798999a9b9c9d9e9f");
    constexpr auto block_hash = system::base16_array("85d0b02a16f6d645aa865fad4a8666f5e7bb2b0c4392a5d675496d6c3defa1f2");
    static_assert(0x45d6f6162ab0d085_u64 % 10u == 5u);
    const system::chain::header header
    {
        0x31323334, // version
        system::null_hash, // previous_block_hash
        root,       // merkle_root
        0x41424344, // timestamp
        0x51525354, // bits
        0x61626364  // nonce
    };

    // TODO: heads may be undersized due to the change to base2 sizing.
    const auto expected_header_head = system::base16_chunk(
        "000000ff" // record count
        "ffffffff" // bucket[0]...
        "ffffffff"
        "ffffffff"
        "ffffffff"
        "ffffffff"
        "000080"   // filter[8], pk->
        "de"       // filter[0-7]
        "ffffffff"
        "ffffffff"
        "ffffffff"
        "ffffffff"
        "ffffffff"
        "ffffffff"
        "ffffffff"
        "ffffffff"
        "ffffffff"
        "ffffffff");

    const auto expected_header_body = system::base16_chunk(
        "ffff7f"   // next->
        "85d0b02a16f6d645aa865fad4a8666f5e7bb2b0c4392a5d675496d6c3defa1f2" // sk (block.hash)
        "14131211" // flags
        "040302"   // height
        "24232221" // mtp
        "01"       // milestone
        "ffff7f"   // previous_block_hash (header_fk - terminal)
        "34333231" // version
        "44434241" // timestamp
        "54535251" // bits
        "64636261" // nonce
        "119192939495969798999a9b9c9d9e9f229192939495969798999a9b9c9d9e9f"); // merkle_root

    settings settings{};
    settings.header_buckets = 16;
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(events_handler));

    ////store.header_head() = expected_header_head;
    ////store.header_body() = expected_header_body;
    BOOST_REQUIRE(query.set(header, context{ 0x11121314, 0x01020304, 0x21222324 }, true));
    BOOST_REQUIRE_EQUAL(store.header_head(), expected_header_head);
    BOOST_REQUIRE_EQUAL(store.header_body(), expected_header_body);

    const auto foo = query.to_header(block_hash);
    const auto pointer1 = query.get_header(foo);
    BOOST_REQUIRE(pointer1);
    BOOST_REQUIRE(*pointer1 == header);

    // Verify hash caching.
    BOOST_REQUIRE_EQUAL(pointer1->hash(), block_hash);
}

BOOST_AUTO_TEST_CASE(query_archive__get_tx_keys__not_found__empty)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(events_handler));
    BOOST_REQUIRE(query.get_tx_keys(query.to_header(system::null_hash)).empty());
    BOOST_REQUIRE(!store.close(events_handler));
}

BOOST_AUTO_TEST_CASE(query_archive__get_header_key__always__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE_EQUAL(query.get_header_key(0), test::genesis.hash());
    BOOST_REQUIRE_EQUAL(query.get_header_key(1), system::null_hash);
}

BOOST_AUTO_TEST_CASE(query_archive__get_point_key__always__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE_EQUAL(query.get_point_hash(0), system::null_hash);

    // tx4/5 prevouts are all block1a.tx1.
    BOOST_REQUIRE(query.set(test::tx4));
    BOOST_REQUIRE(query.set(test::tx5));
////    BOOST_REQUIRE_EQUAL(query.get_point_hash(0), test::block1a.transactions_ptr()->front()->hash(false));
    BOOST_REQUIRE_EQUAL(query.get_point_hash(1), test::block1a.transactions_ptr()->front()->hash(false));
    BOOST_REQUIRE_EQUAL(query.get_point_hash(2), test::block1a.transactions_ptr()->front()->hash(false));
////    BOOST_REQUIRE_EQUAL(query.get_point_hash(3), system::null_hash);

    // block1a adds three prevouts of two txs.
    BOOST_REQUIRE(query.set(test::block1a, context{}, false, false));
////    BOOST_REQUIRE_EQUAL(query.get_point_hash(3), system::one_hash);
    BOOST_REQUIRE_EQUAL(query.get_point_hash(4), system::one_hash);
////    BOOST_REQUIRE_EQUAL(query.get_point_hash(5), test::two_hash);
////    BOOST_REQUIRE_EQUAL(query.get_point_hash(6), system::null_hash);
}

BOOST_AUTO_TEST_CASE(query_archive__get_tx_key__always__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE_EQUAL(query.get_tx_key(0), test::genesis.transactions_ptr()->front()->hash(false));
    BOOST_REQUIRE_EQUAL(query.get_tx_key(1), system::null_hash);
    BOOST_REQUIRE(query.set(test::tx4));
    BOOST_REQUIRE(query.set(test::tx5));
    BOOST_REQUIRE_EQUAL(query.get_tx_key(1), test::tx4.hash(false));
    BOOST_REQUIRE_EQUAL(query.get_tx_key(2), test::tx5.hash(false));
    BOOST_REQUIRE_EQUAL(query.get_tx_key(3), system::null_hash);
}

BOOST_AUTO_TEST_CASE(query_archive__get_height1__always__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, context{ 0, 1, 0 }, false, false));
    BOOST_REQUIRE(query.set(test::block2, context{ 0, 2, 0 }, false, false));
    BOOST_REQUIRE(query.set(test::block3, context{ 0, 3, 0 }, false, false));
    BOOST_REQUIRE(query.set(test::block1a, context{ 0, 1, 0 }, false, false));
    BOOST_REQUIRE(query.set(test::block2a, context{ 0, 2, 0 }, false, false));

    size_t out{};
    BOOST_REQUIRE(query.get_height(out, 0));
    BOOST_REQUIRE_EQUAL(out, 0u);
    BOOST_REQUIRE(query.get_height(out, 1));
    BOOST_REQUIRE_EQUAL(out, 1u);
    BOOST_REQUIRE(query.get_height(out, 2));
    BOOST_REQUIRE_EQUAL(out, 2u);
    BOOST_REQUIRE(query.get_height(out, 3));
    BOOST_REQUIRE_EQUAL(out, 3u);
    BOOST_REQUIRE(query.get_height(out, 4));
    BOOST_REQUIRE_EQUAL(out, 1u);
    BOOST_REQUIRE(query.get_height(out, 5));
    BOOST_REQUIRE_EQUAL(out, 2u);
    BOOST_REQUIRE(!query.get_height(out, 6));
}

BOOST_AUTO_TEST_CASE(query_archive__get_height2__always__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, context{ 0, 1, 0 }, false, false));
    BOOST_REQUIRE(query.set(test::block2, context{ 0, 2, 0 }, false, false));
    BOOST_REQUIRE(query.set(test::block3, context{ 0, 3, 0 }, false, false));
    BOOST_REQUIRE(query.set(test::block1a, context{ 0, 1, 0 }, false, false));
    BOOST_REQUIRE(query.set(test::block2a, context{ 0, 2, 0 }, false, false));

    size_t out{};
    BOOST_REQUIRE(query.get_height(out, test::genesis.hash()));
    BOOST_REQUIRE_EQUAL(out, 0u);
    BOOST_REQUIRE(query.get_height(out, test::block1.hash()));
    BOOST_REQUIRE_EQUAL(out, 1u);
    BOOST_REQUIRE(query.get_height(out, test::block2.hash()));
    BOOST_REQUIRE_EQUAL(out, 2u);
    BOOST_REQUIRE(query.get_height(out, test::block3.hash()));
    BOOST_REQUIRE_EQUAL(out, 3u);
    BOOST_REQUIRE(query.get_height(out, test::block1a.hash()));
    BOOST_REQUIRE_EQUAL(out, 1u);
    BOOST_REQUIRE(query.get_height(out, test::block2a.hash()));
    BOOST_REQUIRE_EQUAL(out, 2u);
    BOOST_REQUIRE(!query.get_height(out, system::one_hash));
}

BOOST_AUTO_TEST_CASE(query_archive__get_tx_height__not_strong__false)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::tx4));

    size_t out{};
    BOOST_REQUIRE(!query.get_tx_height(out, 1));
}

BOOST_AUTO_TEST_CASE(query_archive__get_tx_position__confirmed__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1a, context{ 0, 1, 0 }, false, false));
    BOOST_REQUIRE(query.set(test::block2a, context{ 0, 2, 0 }, false, false));
    BOOST_REQUIRE(query.set(test::block3a, context{ 0, 3, 0 }, false, false));
    BOOST_REQUIRE(query.set_strong(1));
    BOOST_REQUIRE(query.set_strong(2));
    BOOST_REQUIRE(query.set_strong(3));

    size_t out{};
    const auto foo = query.get_tx_position(out, 0);
    BOOST_REQUIRE(foo);
    BOOST_REQUIRE_EQUAL(out, 0u);

    BOOST_REQUIRE(!query.get_tx_position(out, 1));
    BOOST_REQUIRE(!query.get_tx_position(out, 2));
    BOOST_REQUIRE(!query.get_tx_position(out, 3));
    BOOST_REQUIRE(!query.get_tx_position(out, 4));
    BOOST_REQUIRE(query.push_confirmed(1, false));
    BOOST_REQUIRE(query.push_confirmed(2, false));
    BOOST_REQUIRE(query.push_confirmed(3, false));

    BOOST_REQUIRE_EQUAL(out, 0u);
    BOOST_REQUIRE(query.get_tx_position(out, 0));
    BOOST_REQUIRE_EQUAL(out, 0u);
    BOOST_REQUIRE(query.get_tx_position(out, 1));
    BOOST_REQUIRE_EQUAL(out, 0u);
    BOOST_REQUIRE(query.get_tx_position(out, 2));
    BOOST_REQUIRE_EQUAL(out, 0u);
    BOOST_REQUIRE(query.get_tx_position(out, 3));
    BOOST_REQUIRE_EQUAL(out, 1u);
    BOOST_REQUIRE(query.get_tx_position(out, 4));
    BOOST_REQUIRE_EQUAL(out, 0u);
    BOOST_REQUIRE(!query.get_tx_position(out, 5));
}

BOOST_AUTO_TEST_CASE(query_archive__get_tx_position__always__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1a, context{ 0, 1, 0 }, false, false));
    BOOST_REQUIRE(query.set(test::block2a, context{ 0, 2, 0 }, false, false));
    BOOST_REQUIRE(query.set(test::block3a, context{ 0, 3, 0 }, false, false));
    BOOST_REQUIRE(query.set(test::tx4));
    BOOST_REQUIRE(query.set_strong(1));
    BOOST_REQUIRE(query.set_strong(2));
    BOOST_REQUIRE(query.set_strong(3));

    size_t out{};
    BOOST_REQUIRE(query.get_tx_position(out, 0));
    BOOST_REQUIRE_EQUAL(out, 0u);

    BOOST_REQUIRE(!query.get_tx_position(out, 1));
    BOOST_REQUIRE(!query.get_tx_position(out, 2));
    BOOST_REQUIRE(!query.get_tx_position(out, 3));
    BOOST_REQUIRE(!query.get_tx_position(out, 4));
    BOOST_REQUIRE(query.push_confirmed(1, false));
    BOOST_REQUIRE(query.push_confirmed(2, false));
    BOOST_REQUIRE(query.push_confirmed(3, false));

    BOOST_REQUIRE_EQUAL(out, 0u);
    BOOST_REQUIRE(query.get_tx_position(out, 0));
    BOOST_REQUIRE_EQUAL(out, 0u);
    BOOST_REQUIRE(query.get_tx_position(out, 1));
    BOOST_REQUIRE_EQUAL(out, 0u);
    BOOST_REQUIRE(query.get_tx_position(out, 2));
    BOOST_REQUIRE_EQUAL(out, 0u);
    BOOST_REQUIRE(query.get_tx_position(out, 3));
    BOOST_REQUIRE_EQUAL(out, 1u);
    BOOST_REQUIRE(query.get_tx_position(out, 4));
    BOOST_REQUIRE_EQUAL(out, 0u);

    // tx4 is unconfirmed.
    BOOST_REQUIRE(!query.get_tx_position(out, 5));
}

BOOST_AUTO_TEST_CASE(query_archive__get_tx_sizes__coinbase__204)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));

    size_t light{};
    size_t heavy{};
    BOOST_REQUIRE(query.get_tx_sizes(light, heavy, 0));
    BOOST_REQUIRE_EQUAL(light, 204u);
    BOOST_REQUIRE_EQUAL(heavy, 204u);
}

BOOST_AUTO_TEST_CASE(query_archive__get_tx_count__coinbase__1)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE_EQUAL(query.get_tx_count(0), 1u);
}

BOOST_AUTO_TEST_CASE(query_archive__get_input__not_found__nullptr)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(events_handler));
    BOOST_REQUIRE(!query.get_input(query.to_tx(system::null_hash), 0u));
    BOOST_REQUIRE(!store.close(events_handler));
}

BOOST_AUTO_TEST_CASE(query_archive__get_input__genesis__expected)
{
    settings settings{};
    settings.header_buckets = 8;
    settings.tx_buckets = 8;
    settings.txs_buckets = 16;
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(events_handler));
    BOOST_REQUIRE(query.set(test::genesis, test::context, false, false));

    const auto tx = test::genesis.transactions_ptr()->front();
    const auto input = tx->inputs_ptr()->front();
////    BOOST_REQUIRE(*input == *query.get_input(query.to_tx(tx->hash(false)), 0u));
////    BOOST_REQUIRE(*input == *query.get_input(0));
}

BOOST_AUTO_TEST_CASE(query_archive__get_inputs__tx_not_found__nullptr)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(!query.get_inputs(1));
}

BOOST_AUTO_TEST_CASE(query_archive__get_inputs__found__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::tx4));
    BOOST_REQUIRE_EQUAL(query.get_inputs(1)->size(), 2u);
}

BOOST_AUTO_TEST_CASE(query_archive__get_output__not_found__nullptr)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(events_handler));
    BOOST_REQUIRE(!query.get_output(query.to_tx(system::null_hash), 0u));
    BOOST_REQUIRE(!query.get_output(query.to_output(system::chain::point{ system::null_hash, 0u })));
    BOOST_REQUIRE(!query.get_output(0));
    BOOST_REQUIRE(!store.close(events_handler));
}

BOOST_AUTO_TEST_CASE(query_archive__get_output__genesis__expected)
{
    settings settings{};
    settings.header_buckets = 8;
    settings.tx_buckets = 8;
    settings.txs_buckets = 16;
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(events_handler));
    BOOST_REQUIRE(query.set(test::genesis, test::context, false, false));

    const auto tx = test::genesis.transactions_ptr()->front();
    const auto output1 = tx->outputs_ptr()->front();
    BOOST_REQUIRE(*output1 == *query.get_output(query.to_tx(tx->hash(false)), 0u));
    BOOST_REQUIRE(*output1 == *query.get_output(query.to_output(tx->hash(false), 0u)));
    BOOST_REQUIRE(*output1 == *query.get_output(0));
}

BOOST_AUTO_TEST_CASE(query_archive__get_outputs__tx_not_found__nullptr)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(!query.get_outputs(1));
}

BOOST_AUTO_TEST_CASE(query_archive__get_outputs__found__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::tx4));
    BOOST_REQUIRE_EQUAL(query.get_outputs(1)->size(), 1u);
}

BOOST_AUTO_TEST_CASE(query_archive__get_transactions__tx_not_found__nullptr)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::tx4));
    BOOST_REQUIRE(!query.get_transactions(3));
}

BOOST_AUTO_TEST_CASE(query_archive__get_transactions__found__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1a, test::context, false, false));
    BOOST_REQUIRE(query.set(test::block2a, test::context, false, false));
    BOOST_REQUIRE(query.set(test::tx4));
    BOOST_REQUIRE_EQUAL(query.get_transactions(0)->size(), 1u);
    BOOST_REQUIRE_EQUAL(query.get_transactions(1)->size(), 1u);
    BOOST_REQUIRE_EQUAL(query.get_transactions(2)->size(), 2u);
}

BOOST_AUTO_TEST_CASE(query_archive__get_spenders__unspent_or_not_found__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, test::context, false, false));
    BOOST_REQUIRE(query.set(test::block2, test::context, false, false));
    BOOST_REQUIRE(query.set(test::block3, test::context, false, false));

    // Caller should always test for nullptr.
    BOOST_REQUIRE(query.get_spenders(output_link::terminal)->empty());
    BOOST_REQUIRE(query.get_spenders(tx_link::terminal, 0)->empty());
    BOOST_REQUIRE(query.get_spenders(tx_link::terminal, 1)->empty());

    BOOST_REQUIRE(query.get_spenders(query.to_output(0, 0))->empty());
    BOOST_REQUIRE(query.get_spenders(query.to_output(0, 1))->empty());
    BOOST_REQUIRE(query.get_spenders(0, 0)->empty());
    BOOST_REQUIRE(query.get_spenders(0, 1)->empty());

    BOOST_REQUIRE(query.get_spenders(query.to_output(1, 0))->empty());
    BOOST_REQUIRE(query.get_spenders(query.to_output(1, 1))->empty());
    BOOST_REQUIRE(query.get_spenders(1, 0)->empty());
    BOOST_REQUIRE(query.get_spenders(1, 1)->empty());

    BOOST_REQUIRE(query.get_spenders(query.to_output(2, 0))->empty());
    BOOST_REQUIRE(query.get_spenders(query.to_output(2, 1))->empty());
    BOOST_REQUIRE(query.get_spenders(2, 0)->empty());
    BOOST_REQUIRE(query.get_spenders(2, 1)->empty());

    BOOST_REQUIRE(query.get_spenders(query.to_output(3, 0))->empty());
    BOOST_REQUIRE(query.get_spenders(query.to_output(3, 1))->empty());
    BOOST_REQUIRE(query.get_spenders(3, 0)->empty());
    BOOST_REQUIRE(query.get_spenders(3, 1)->empty());
}

////BOOST_AUTO_TEST_CASE(query_archive__get_spenders__found_and_spent__expected)
////{
////    settings settings{};
////    settings.path = TEST_DIRECTORY;
////    test::chunk_store store{ settings };
////    test::query_accessor query{ store };
////    BOOST_REQUIRE(!store.create(events_handler));
////    BOOST_REQUIRE(query.initialize(test::genesis));
////
////    // Neither of the two block1a outputs spent yet.
////    BOOST_REQUIRE(query.set(test::block1a, test::context));
////    BOOST_REQUIRE(query.get_spenders(query.to_output(1, 0))->empty());
////    BOOST_REQUIRE(query.get_spenders(query.to_output(1, 1))->empty());
////    BOOST_REQUIRE(query.get_spenders(query.to_output(1, 2))->empty());
////    BOOST_REQUIRE(query.get_spenders(1, 0)->empty());
////    BOOST_REQUIRE(query.get_spenders(1, 1)->empty());
////    BOOST_REQUIRE(query.get_spenders(1, 2)->empty());
////
////    // Each of the two outputs of block1a spent once.
////    BOOST_REQUIRE(query.set(test::block2a, test::context));
////
////    BOOST_REQUIRE_EQUAL(query.get_spenders(query.to_output(1, 0))->size(), 1u);
////    BOOST_REQUIRE_EQUAL(query.get_spenders(query.to_output(1, 1))->size(), 1u);
////    BOOST_REQUIRE(query.get_spenders(query.to_output(1, 2))->empty());
////    BOOST_REQUIRE_EQUAL(query.get_spenders(1, 0)->size(), 1u);
////    BOOST_REQUIRE_EQUAL(query.get_spenders(1, 1)->size(), 1u);
////    BOOST_REQUIRE_EQUAL(query.get_spenders(1, 2)->size(), 0u);
////
////    // Match the two spenders.
////    const auto block_inputs = test::block2a.transactions_ptr()->front()->inputs_ptr();
////    BOOST_REQUIRE(*query.get_spenders(query.to_output(1, 0))->front() == *(*block_inputs).front());
////    BOOST_REQUIRE(*query.get_spenders(query.to_output(1, 1))->front() == *(*block_inputs).back());
////    BOOST_REQUIRE(*query.get_spenders(1, 0)->front() == *(*block_inputs).front());
////    BOOST_REQUIRE(*query.get_spenders(1, 1)->front() == *(*block_inputs).back());
////
////    // Each of the two outputs of block1a spent twice (two unconfirmed double spends).
////    BOOST_REQUIRE(query.set(test::tx4));
////    BOOST_REQUIRE_EQUAL(query.get_spenders(query.to_output(1, 0))->size(), 2u);
////    BOOST_REQUIRE_EQUAL(query.get_spenders(query.to_output(1, 1))->size(), 2u);
////    BOOST_REQUIRE(query.get_spenders(query.to_output(1, 2))->empty());
////    BOOST_REQUIRE_EQUAL(query.get_spenders(1, 0)->size(), 2u);
////    BOOST_REQUIRE_EQUAL(query.get_spenders(1, 1)->size(), 2u);
////    BOOST_REQUIRE_EQUAL(query.get_spenders(1, 2)->size(), 0u);
////
////    // Match the four spenders.
////    const auto tx_inputs = test::tx4.inputs_ptr();
////    BOOST_REQUIRE(*query.get_spenders(query.to_output(1, 0))->front() == *(*tx_inputs).front());
////    BOOST_REQUIRE(*query.get_spenders(query.to_output(1, 1))->front() == *(*tx_inputs).back());
////    BOOST_REQUIRE(*query.get_spenders(query.to_output(1, 0))->back() == *(*block_inputs).front());
////    BOOST_REQUIRE(*query.get_spenders(query.to_output(1, 1))->back() == *(*block_inputs).back());
////    BOOST_REQUIRE(*query.get_spenders(1, 0)->front() == *(*tx_inputs).front());
////    BOOST_REQUIRE(*query.get_spenders(1, 1)->front() == *(*tx_inputs).back());
////    BOOST_REQUIRE(*query.get_spenders(1, 0)->back() == *(*block_inputs).front());
////    BOOST_REQUIRE(*query.get_spenders(1, 1)->back() == *(*block_inputs).back());
////}

BOOST_AUTO_TEST_CASE(query_archive__get_value__genesis__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));

    uint64_t value{};
    BOOST_REQUIRE(query.get_value(value, query.to_output(0, 0)));
    BOOST_REQUIRE_EQUAL(value, 5000000000u);
}

BOOST_AUTO_TEST_SUITE_END()
