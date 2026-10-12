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
#include "../../test.hpp"
#include "../../mocks/blocks.hpp"
#include "../../mocks/chunk_store.hpp"

// ensure context::flags is same size as chain_context::flags.
static_assert(is_same_type<database::context::flag_t::integer,
    decltype(system::chain::context{}.flags)>);

BOOST_FIXTURE_TEST_SUITE(query_chain_writer_tests, test::directory_setup_fixture)

// slow test (mmap)
BOOST_AUTO_TEST_CASE(query_chain_writer__set_header__mmap_get_header__expected)
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
    settings.header.buckets = 16;
    settings.path = TEST_DIRECTORY;
    store<database::mmap> store{ settings };
    query<database::store<database::mmap>> query{ store };
    BOOST_CHECK(!store.create(test::events_handler));
    BOOST_CHECK(query.set(header, test::context, {}, milestone));

    table::header::record element1{};
    BOOST_CHECK(store.header.get(query.to_header(block_hash), element1));

    const auto pointer = query.get_header(query.to_header(block_hash));
    BOOST_CHECK(pointer);
    BOOST_CHECK(*pointer == header);

    // must open/close mmap
    BOOST_CHECK(!store.close(test::events_handler));
    BOOST_CHECK_EQUAL(element1.ctx.height, system::mask_left(test::context.height, byte_bits));
    BOOST_CHECK_EQUAL(element1.ctx.flags, test::context.flags);
    BOOST_CHECK_EQUAL(element1.ctx.mtp, test::context.mtp);
    BOOST_CHECK_EQUAL(element1.milestone, milestone);
    BOOST_CHECK_EQUAL(element1.version, header.version());
    BOOST_CHECK_EQUAL(element1.parent_fk, schema::header::link::terminal);
    BOOST_CHECK_EQUAL(element1.merkle_root, header.merkle_root());
    BOOST_CHECK_EQUAL(element1.timestamp, header.timestamp());
    BOOST_CHECK_EQUAL(element1.bits, header.bits());
    BOOST_CHECK_EQUAL(element1.nonce, header.nonce());
}

BOOST_AUTO_TEST_CASE(query_chain_writer__set_link_header__is_header__expected)
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
        "ffff7f"   // previous_block_hash (header_fk - not found)
        "34333231" // version
        "44434241" // timestamp
        "54535251" // bits
        "64636261" // nonce
        "119192939495969798999a9b9c9d9e9f229192939495969798999a9b9c9d9e9f" // merkle_root
        "00000000000000000000000000000000" // work
        "01"); // header flags (milestone)

    header_link link{};
    settings settings{};
    settings.header.buckets = 16;
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_CHECK(!store.create(test::events_handler));

    // store open/close flushes record count to head.
    BOOST_CHECK(!query.is_header(header.hash()));
    BOOST_CHECK(!query.is_associated(0));
    BOOST_CHECK(!query.set_code(link, header, test::context, {}, milestone));
    BOOST_CHECK(!link.is_terminal());
    BOOST_CHECK(query.is_header(header.hash()));
    BOOST_CHECK(!query.is_associated(0));
    table::header::record element1{};
    BOOST_CHECK(store.header.get(query.to_header(block_hash), element1));
    BOOST_CHECK(!store.close(test::events_handler));
    BOOST_CHECK_EQUAL(store.header_head(), expected_header_head);
    BOOST_CHECK_EQUAL(store.header_body(), expected_header_body);

    BOOST_CHECK_EQUAL(element1.ctx.height, system::mask_left(test::context.height, byte_bits));
    BOOST_CHECK_EQUAL(element1.ctx.flags, test::context.flags);
    BOOST_CHECK_EQUAL(element1.ctx.mtp, test::context.mtp);
    BOOST_CHECK_EQUAL(element1.milestone, milestone);
    BOOST_CHECK_EQUAL(element1.version, header.version());
    BOOST_CHECK_EQUAL(element1.parent_fk, schema::header::link::terminal);
    BOOST_CHECK_EQUAL(element1.merkle_root, header.merkle_root());
    BOOST_CHECK_EQUAL(element1.timestamp, header.timestamp());
    BOOST_CHECK_EQUAL(element1.bits, header.bits());
    BOOST_CHECK_EQUAL(element1.nonce, header.nonce());
}

BOOST_AUTO_TEST_CASE(query_chain_writer__set_tx__empty__expected)
{
    const system::chain::transaction tx{};
    const auto expected_head5_array = system::base16_chunk("0000000000");
    const auto expected_outs_head = system::base16_chunk(
        "00000000"   // record count
        "ffffffff"   // bucket[0]
        "ffffffff");// bucket[1]
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
    settings.tx.buckets = 8;
    settings.outs.buckets = 2;
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_CHECK(!store.create(test::events_handler));

    // store open/close flushes record count to heads.
    BOOST_CHECK(!query.set(tx));
    BOOST_CHECK(!store.close(test::events_handler));
    BOOST_CHECK_EQUAL(store.tx_head(), expected_head4_hash);
    BOOST_CHECK_EQUAL(store.input_head(), expected_head5_array);
    BOOST_CHECK_EQUAL(store.output_head(), expected_head5_array);
    BOOST_CHECK_EQUAL(store.outs_head(), expected_outs_head);
    BOOST_CHECK_EQUAL(store.tx_body().size(), schema::transaction::minrow);
    BOOST_CHECK(store.ins_body().empty());
    BOOST_CHECK(store.input_body().empty());
    BOOST_CHECK(store.output_body().empty());
    BOOST_CHECK(store.outs_body().empty());
}

BOOST_AUTO_TEST_CASE(query_chain_writer__set_link_tx__null_input__expected)
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
        "ffffff7f"     // next->
        "601f0fa54d6de8362c17dc883cc047e1f3ae0523d732598a05e3010fac591f62" // sk (tx.hash(false))
        "3c0080"       // light (coinbase merged)
        "3c0000"       // heavy
        "14131211"     // locktime
        "04030201"     // version
        "010000"       // ins_count
        "010000"       // outs_count
        "00000000"     // point_fk-> (ins_fk)
        "00000000");   // outs_fk->
    const auto expected_outs_head = system::base16_chunk(
        "01000000"   // record count
        "00000000"   // bucket[0] (link->0)
        "ffffffff");// bucket[1]
    const auto expected_address_body = system::base16_chunk(
        "ffffffff");  // next->end
    const auto expected_outs_body = system::base16_chunk(
        "0000000000"); // output0_fk->
    const auto expected_output_head = system::base16_chunk("0600000000");
    const auto expected_output_body = system::base16_chunk(
        "00000000"     // parent_fk->
        "00"           // value
        "00");         // script
    const auto expected_ins_body = system::base16_chunk(
        "ffffffff"     // next->
        "0000000000000000000000000000000000000000000000000000000000000000"
        "ffffff");     // index
    ////const auto expected_spend_head = system::base16_chunk(
    ////    "01000000"     // record count
    ////    "00000000"     // pk->
    ////    "ffffffff"
    ////    "ffffffff"
    ////    "ffffffff"
    ////    "ffffffff");
    ////const auto expected_spend_body = system::base16_chunk(
    ////    "ffffffff"     // terminal->
    ////    "00000000"     // fp: point_stub
    ////    "ffffff"       // fp: point_index (null)
    ////    "ffffffff"     // point_fk->
    ////    "00000000"     // parent_fk->
    ////    "00000000"     // sequence
    ////    "0000000000"); // input_fk->
    const auto expected_input_head = system::base16_chunk("0200000000");
    const auto expected_input_body = system::base16_chunk(
        "00"           // script
        "00");         // witness
    constexpr auto tx_hash = system::base16_array("601f0fa54d6de8362c17dc883cc047e1f3ae0523d732598a05e3010fac591f62");
    BOOST_CHECK_EQUAL(tx_hash, tx.hash(false));

    // data_chunk store.
    settings settings{};
    settings.tx.buckets = 8;
    settings.ins.buckets = 8;
    settings.outs.buckets = 2;
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_CHECK(!store.create(test::events_handler));
    BOOST_CHECK(!query.set_code(tx));
    BOOST_CHECK(!store.close(test::events_handler));

    BOOST_CHECK_EQUAL(store.tx_head(), expected_tx_head);
    BOOST_CHECK_EQUAL(store.input_head(), expected_input_head);
    BOOST_CHECK_EQUAL(store.output_head(), expected_output_head);
    BOOST_CHECK_EQUAL(store.outs_head(), expected_outs_head);
    ////BOOST_CHECK_EQUAL(store.spend_head(), expected_spend_head);

    BOOST_CHECK_EQUAL(store.tx_body(), expected_tx_body);
    BOOST_CHECK_EQUAL(store.ins_body(), expected_ins_body);
    BOOST_CHECK_EQUAL(store.input_body(), expected_input_body);
    BOOST_CHECK_EQUAL(store.output_body(), expected_output_body);
    BOOST_CHECK_EQUAL(store.outs_body(), expected_outs_body);
    BOOST_CHECK_EQUAL(store.address_body(), expected_address_body);
    ////BOOST_CHECK_EQUAL(store.spend_body(), expected_spend_body);
}

BOOST_AUTO_TEST_CASE(query_chain_writer__set_tx__get_tx__expected)
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
        "ffffff7f"     // next->
        "d80f19b9c0f649081c0b279d9183b0fae35b41b72a34eb181001f82afe22043a" // sk (tx.hash(false))
        "740000"       // light (not coinbase)
        "800000"       // heavy
        "18000000"     // locktime
        "2a000000"     // version
        "020000"       // ins_count
        "020000"       // outs_count
        "00000000"     // point_fk-> (ins_fk)
        "00000000");   // outs_fk->
    const auto expected_outs_head = system::base16_chunk(
        "02000000"   // record count
        "ffffffff"   // bucket[0]
        "01000000");// bucket[1] (link->1)
    const auto expected_address_body = system::base16_chunk(
        "ffffffff"    // next->end
        "00000000");  // next->0
    const auto expected_outs_body = system::base16_chunk(
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
    const auto expected_ins_body = system::base16_chunk(
        "ffffffff"     // next->
        "0100000000000000000000000000000000000000000000000000000000000000"
        "180000"       // index
        "ffffffff"     // next->
        "0100000000000000000000000000000000000000000000000000000000000000"
        "2a0000");     // index
    ////const auto expected_spend_head = system::base16_chunk(
    ////    "02000000"     // record count
    ////    "00000000"     // spend0_fk->
    ////    "ffffffff"
    ////    "ffffffff"
    ////    "01000000"     // spend1_fk->
    ////    "ffffffff");
    ////const auto expected_spend_body = system::base16_chunk(
    ////    "ffffffff"     // terminal->
    ////    "01000000"     // fp: point_stub
    ////    "180000"       // fp: point_index
    ////    "00000000"     // point_fk->
    ////    "00000000"     // parent_fk->
    ////    "2a000000"     // sequence
    ////    "0000000000"   // input_fk->
    ////
    ////    "ffffffff"     // terminal->
    ////    "01000000"     // fp: point_stub
    ////    "2a0000"       // fp: point_index
    ////    "01000000"     // point_fk->
    ////    "00000000"     // parent_fk->
    ////    "18000000"     // sequence
    ////    "0800000000"); // input_fk->
    const auto expected_input_head = system::base16_chunk("1000000000");
    const auto expected_input_body = system::base16_chunk(
        "026a79"       // script
        "0103242424"   // witness
        "026a7a"       // script
        "0103424242"); // witness

    constexpr auto tx_hash = system::base16_array("d80f19b9c0f649081c0b279d9183b0fae35b41b72a34eb181001f82afe22043a");
    BOOST_CHECK_EQUAL(tx_hash, tx.hash(false));

    // data_chunk store.
    settings settings{};
    settings.tx.buckets = 8;
    settings.ins.buckets = 8;
    settings.outs.buckets = 2;
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_CHECK(!store.create(test::events_handler));
    BOOST_CHECK(!query.is_tx(tx.hash(false)));
    BOOST_CHECK(query.set(tx));
    BOOST_CHECK(query.is_tx(tx.hash(false)));

    const auto pointer = query.get_transaction(query.to_tx(tx_hash), true);
    BOOST_CHECK(pointer);
    BOOST_CHECK(*pointer == tx);
    BOOST_CHECK_EQUAL(pointer->hash(false), tx_hash);
    BOOST_CHECK(!store.close(test::events_handler));

    BOOST_CHECK_EQUAL(store.tx_head(), expected_tx_head);
    BOOST_CHECK_EQUAL(store.input_head(), expected_input_head);
    BOOST_CHECK_EQUAL(store.output_head(), expected_output_head);
    BOOST_CHECK_EQUAL(store.outs_head(), expected_outs_head);
    ////BOOST_CHECK_EQUAL(store.spend_head(), expected_spend_head);

    BOOST_CHECK_EQUAL(store.tx_body(), expected_tx_body);
    BOOST_CHECK_EQUAL(store.ins_body(), expected_ins_body);
    BOOST_CHECK_EQUAL(store.input_body(), expected_input_body);
    BOOST_CHECK_EQUAL(store.output_body(), expected_output_body);
    BOOST_CHECK_EQUAL(store.outs_body(), expected_outs_body);
    BOOST_CHECK_EQUAL(store.address_body(), expected_address_body);
    ////BOOST_CHECK_EQUAL(store.spend_body(), expected_spend_body);
}

BOOST_AUTO_TEST_CASE(query_chain_writer__set_block__get_block__expected)
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
        "ffff7f"       // previous_block_hash (header_fk - not found)
        "01000000"     // version
        "29ab5f49"     // timestamp
        "ffff001d"     // bits
        "1dac2b7c"     // nonce
        "3ba3edfd7a7b12b27ac72c3e67768f617fc81bc3888a51323a9fb8aa4b1e5e4a" // merkle_root
        "00000000000000000000000000000000" // work
        "01"); // header flags (milestone)
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
    const auto genesis_outs_head = system::base16_chunk(
        "01000000"   // record count
        "ffffffff"   // bucket[0]
        "00000000");// bucket[1] (link->0)
    const auto genesis_address_body = system::base16_chunk(
        "ffffffff");  // next->end
    const auto genesis_outs_body = system::base16_chunk(
        "0000000000"); // output0_fk->
    const auto genesis_output_head = system::base16_chunk("5100000000");
    const auto genesis_output_body = system::base16_chunk(
        "00000000"     // parent_fk->
        "ff00f2052a01000000" // value
        "434104678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38c4f35504e51ec112de5c384df7ba0b8d578a4c702b6bf11d5fac"); // script
    const auto genesis_ins_body = system::base16_chunk(
        "ffffffff"     // next->
        "0000000000000000000000000000000000000000000000000000000000000000"
        "ffffff");     // index
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
        "0c00000000"   // slab size
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
    const auto genesis_txs_body = system::build_chunk(
    {
        system::base16_chunk(
            "1d0100"       // size light (285)
            "1d0100"       // size heavy (285)
            "0100"         // txs count (1)
            "00000000")    // transaction[0]
    });

    settings settings{};
    settings.header.buckets = 8;
    settings.tx.buckets = 8;
    settings.ins.buckets = 8;
    settings.txs.buckets = 16;
    settings.outs.buckets = 2;
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_CHECK(!store.create(test::events_handler));

    // Set block (header/txs).
    BOOST_CHECK(!query.is_block(test::genesis.hash()));
    BOOST_CHECK(query.set(test::genesis, test::context, {}, milestone, false));
    BOOST_CHECK(query.is_block(test::genesis.hash()));

    table::header::record element1{};
    BOOST_CHECK(store.header.get(query.to_header(test::genesis.hash()), element1));
    BOOST_CHECK(!store.close(test::events_handler));

    BOOST_CHECK_EQUAL(store.header_head(), genesis_header_head);
    BOOST_CHECK_EQUAL(store.tx_head(), genesis_tx_head);
    BOOST_CHECK_EQUAL(store.input_head(), genesis_input_head);
    BOOST_CHECK_EQUAL(store.output_head(), genesis_output_head);
    BOOST_CHECK_EQUAL(store.outs_head(), genesis_outs_head);
    ////BOOST_CHECK_EQUAL(store.spend_head(), genesis_spend_head);
    BOOST_CHECK_EQUAL(store.txs_head(), genesis_txs_head);

    BOOST_CHECK_EQUAL(store.header_body(), genesis_header_body);
    BOOST_CHECK_EQUAL(store.tx_body(), genesis_tx_body);
    BOOST_CHECK_EQUAL(store.ins_body(), genesis_ins_body);
    BOOST_CHECK_EQUAL(store.input_body(), genesis_input_body);
    BOOST_CHECK_EQUAL(store.output_body(), genesis_output_body);
    ////BOOST_CHECK_EQUAL(store.spend_body(), genesis_spend_body);
    BOOST_CHECK_EQUAL(store.txs_body(), genesis_txs_body);

    const auto pointer = query.get_block(query.to_header(test::genesis.hash()), false);
    BOOST_CHECK(pointer);
    BOOST_CHECK(*pointer == test::genesis);

    const auto hashes = query.get_tx_keys(query.to_header(test::genesis.hash()));
    BOOST_CHECK_EQUAL(hashes.size(), 1u);
    BOOST_CHECK_EQUAL(hashes, test::genesis.transaction_hashes(false));
}

BOOST_AUTO_TEST_CASE(query_chain_writer__set_block_txs__get_block__expected)
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
        "ffff7f"       // previous_block_hash (header_fk - not found)
        "01000000"     // version
        "29ab5f49"     // timestamp
        "ffff001d"     // bits
        "1dac2b7c"     // nonce
        "3ba3edfd7a7b12b27ac72c3e67768f617fc81bc3888a51323a9fb8aa4b1e5e4a" // merkle_root
        "00000000000000000000000000000000" // work
        "01"); // header flags (milestone)
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
    const auto genesis_outs_head = system::base16_chunk(
        "01000000"   // record count
        "ffffffff"   // bucket[0]
        "00000000");// bucket[1] (link->0)
    const auto genesis_outs_body = system::base16_chunk(
        "00000000"     // spend0_fk->
        "0000000000"); // output0_fk->
    const auto genesis_output_head = system::base16_chunk("5100000000");
    const auto genesis_output_body = system::base16_chunk(
        "00000000"     // parent_fk->
        "ff00f2052a01000000" // value
        "434104678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38c4f35504e51ec112de5c384df7ba0b8d578a4c702b6bf11d5fac"); // script
    const auto genesis_ins_body = system::base16_chunk(
        "ffffffff"     // next->
        "0000000000000000000000000000000000000000000000000000000000000000"
        "ffffff");     // index
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
        "0c00000000"   // slab size
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
    const auto genesis_txs_body = system::build_chunk(
    {
        system::base16_chunk(
            "1d0100"       // size light (285)
            "1d0100"       // size heavy (285)
            "0100"         // txs count (1)
            "00000000")    // transaction[0]
    });

    settings settings{};
    settings.header.buckets = 8;
    settings.tx.buckets = 8;
    settings.ins.buckets = 8;
    settings.txs.buckets = 16;
    settings.outs.buckets = 2;
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_CHECK(!store.create(test::events_handler));

    // Set header and then txs.
    BOOST_CHECK(!query.is_block(test::genesis.hash()));
    BOOST_CHECK(query.set(test::genesis.header(), test::context, {}, milestone));
    BOOST_CHECK(!query.is_associated(0));
    BOOST_CHECK(query.set(test::genesis, false, false));
    BOOST_CHECK(query.is_block(test::genesis.hash()));
    BOOST_CHECK(query.is_associated(0));

    table::header::record element1{};
    BOOST_CHECK(store.header.get(query.to_header(test::genesis.hash()), element1));
    BOOST_CHECK(!store.close(test::events_handler));

    BOOST_CHECK_EQUAL(store.header_head(), genesis_header_head);
    BOOST_CHECK_EQUAL(store.tx_head(), genesis_tx_head);
    BOOST_CHECK_EQUAL(store.input_head(), genesis_input_head);
    BOOST_CHECK_EQUAL(store.output_head(), genesis_output_head);
    BOOST_CHECK_EQUAL(store.outs_head(), genesis_outs_head);
    ////BOOST_CHECK_EQUAL(store.spend_head(), genesis_spend_head);
    BOOST_CHECK_EQUAL(store.txs_head(), genesis_txs_head);

    BOOST_CHECK_EQUAL(store.header_body(), genesis_header_body);
    BOOST_CHECK_EQUAL(store.tx_body(), genesis_tx_body);
    BOOST_CHECK_EQUAL(store.ins_body(), genesis_ins_body);
    BOOST_CHECK_EQUAL(store.input_body(), genesis_input_body);
    BOOST_CHECK_EQUAL(store.output_body(), genesis_output_body);
    ////BOOST_CHECK_EQUAL(store.spend_body(), genesis_spend_body);
    BOOST_CHECK_EQUAL(store.txs_body(), genesis_txs_body);

    const auto pointer = query.get_block(query.to_header(test::genesis.hash()), false);
    BOOST_CHECK(pointer);
    BOOST_CHECK(*pointer == test::genesis);

    const auto hashes = query.get_tx_keys(query.to_header(test::genesis.hash()));
    BOOST_CHECK_EQUAL(hashes.size(), 1u);
    BOOST_CHECK_EQUAL(hashes, test::genesis.transaction_hashes(false));
}

// pooling
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(query_chain_writer__set_tx__not_pooling_duplicate__written)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(!store.is_pooling());

    tx_link first{};
    tx_link second{};
    BOOST_REQUIRE_EQUAL(query.set_code(first, test::tx4), error::success);
    BOOST_REQUIRE_EQUAL(query.set_code(second, test::tx4), error::success);
    BOOST_REQUIRE(first != second);
    BOOST_REQUIRE_EQUAL(query.tx_records(), 2u);
    BOOST_REQUIRE_EQUAL(query.duplicate_records(), 2u);
}

BOOST_AUTO_TEST_CASE(query_chain_writer__set_tx__pooling_duplicate__substituted)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    store.set_pooling();

    tx_link first{};
    tx_link second{};
    BOOST_REQUIRE_EQUAL(query.set_code(first, test::tx4), error::success);
    BOOST_REQUIRE_EQUAL(query.set_code(second, test::tx4), error::success);
    BOOST_REQUIRE(first == second);
    BOOST_REQUIRE_EQUAL(query.tx_records(), 1u);
    BOOST_REQUIRE_EQUAL(query.duplicate_records(), 0u);
}

BOOST_AUTO_TEST_CASE(query_chain_writer__set_tx__pooling_duplicate__pooled)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    store.set_pooling();

    tx_link first{};
    tx_link second{};
    bool pooled{ true };
    BOOST_REQUIRE_EQUAL(query.set_code(first, pooled, test::tx4), error::success);
    BOOST_REQUIRE(!pooled);
    BOOST_REQUIRE_EQUAL(query.set_code(second, pooled, test::tx4), error::success);
    BOOST_REQUIRE(pooled);
    BOOST_REQUIRE(first == second);
}

BOOST_AUTO_TEST_CASE(query_chain_writer__set_tx__pooling_conflict__written)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    store.set_pooling();

    tx_link first{};
    tx_link second{};
    BOOST_REQUIRE_EQUAL(query.set_code(first, test::tx4), error::success);
    BOOST_REQUIRE_EQUAL(query.set_code(second, test::tx5), error::success);
    BOOST_REQUIRE(first != second);
    BOOST_REQUIRE_EQUAL(query.tx_records(), 2u);
    BOOST_REQUIRE_EQUAL(query.duplicate_records(), 1u);
}

BOOST_AUTO_TEST_CASE(query_chain_writer__set_block__not_pooling_pooled_tx__written)
{
    using namespace system::chain;
    const block block{ header{ 0x31323334, test::block0_hash, system::hash_digest{ 0xf4 }, 0x41424344, 0x51525354, 0x61626364 }, transactions{ transaction{ 0xb1, inputs{ input{ point{}, script{ { { opcode::size } } }, witness{}, 0xb1 } }, outputs{ output{ 0x42, script{ { { opcode::pick } } } } }, 0xc1 }, test::tx4 } };

    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));

    tx_link pooled{};
    BOOST_REQUIRE_EQUAL(query.set_code(pooled, test::tx4), error::success);
    BOOST_REQUIRE_EQUAL(query.tx_records(), 2u);
    BOOST_REQUIRE(query.set(block, database::context{ 0, 1, 0 }, {}, false, false));
    BOOST_REQUIRE_EQUAL(query.tx_records(), 4u);
    BOOST_REQUIRE_EQUAL(query.duplicate_records(), 2u);
    BOOST_REQUIRE(query.to_tx(test::tx4.hash(false)) != pooled);
}

BOOST_AUTO_TEST_CASE(query_chain_writer__set_block__pooling_pooled_tx__substituted)
{
    using namespace system::chain;
    const block block{ header{ 0x31323334, test::block0_hash, system::hash_digest{ 0xf4 }, 0x41424344, 0x51525354, 0x61626364 }, transactions{ transaction{ 0xb1, inputs{ input{ point{}, script{ { { opcode::size } } }, witness{}, 0xb1 } }, outputs{ output{ 0x42, script{ { { opcode::pick } } } } }, 0xc1 }, test::tx4 } };

    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    store.set_pooling();

    tx_link pooled{};
    BOOST_REQUIRE_EQUAL(query.set_code(pooled, test::tx4), error::success);
    BOOST_REQUIRE_EQUAL(query.tx_records(), 2u);
    BOOST_REQUIRE(query.set(block, database::context{ 0, 1, 0 }, {}, false, false));
    BOOST_REQUIRE_EQUAL(query.tx_records(), 3u);
    BOOST_REQUIRE_EQUAL(query.duplicate_records(), 0u);
    BOOST_REQUIRE(query.to_tx(test::tx4.hash(false)) == pooled);

    const auto link = query.to_header(block.hash());
    const auto pointer = query.get_block(link, true);
    BOOST_REQUIRE(pointer);
    BOOST_REQUIRE_EQUAL(pointer->transactions(), 2u);
    BOOST_REQUIRE_EQUAL(pointer->transactions_ptr()->back()->hash(false), test::tx4.hash(false));
    BOOST_REQUIRE(!query.is_strong_tx(pooled));
    BOOST_REQUIRE(query.set_strong(link));
    BOOST_REQUIRE(query.is_strong_tx(pooled));
    BOOST_REQUIRE(query.set_unstrong(link));
    BOOST_REQUIRE(!query.is_strong_tx(pooled));
}

BOOST_AUTO_TEST_CASE(query_chain_writer__set_block__pooling_malleated_tx__written)
{
    using namespace system::chain;
    auto data = test::tx4.to_data(true);
    data.at(data.size() - 5) ^= 0xff;
    const transaction malleated{ data, true };
    const block block{ header{ 0x31323334, test::block0_hash, system::hash_digest{ 0xf6 }, 0x41424344, 0x51525354, 0x61626364 }, transactions{ transaction{ 0xb1, inputs{ input{ point{}, script{ { { opcode::size } } }, witness{}, 0xb1 } }, outputs{ output{ 0x42, script{ { { opcode::pick } } } } }, 0xc1 }, malleated } };

    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    store.set_pooling();

    tx_link pooled{};
    BOOST_REQUIRE_EQUAL(malleated.hash(false), test::tx4.hash(false));
    BOOST_REQUIRE_NE(malleated.hash(true), test::tx4.hash(true));
    BOOST_REQUIRE_EQUAL(query.set_code(pooled, test::tx4), error::success);
    BOOST_REQUIRE(query.set(block, database::context{ 0, 1, 0 }, {}, false, false));
    BOOST_REQUIRE_EQUAL(query.tx_records(), 4u);

    const auto link = query.to_header(block.hash());
    const auto pointer = query.get_block(link, true);
    BOOST_REQUIRE(pointer);
    BOOST_REQUIRE(query.to_transactions(link).back() != pooled);
    BOOST_REQUIRE_EQUAL(pointer->transactions_ptr()->back()->hash(true), malleated.hash(true));
}

BOOST_AUTO_TEST_CASE(query_chain_writer__set_header__unstorable_work__header_work)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));

    header_link link{};
    const uint256_t work = uint256_t{ 1 } << to_bits(schema::work);
    BOOST_REQUIRE_EQUAL(query.set_code(link, test::block1.header(), context{}, work, false), error::header_work);
    BOOST_REQUIRE(link.is_terminal());
    BOOST_REQUIRE_EQUAL(query.header_records(), 1u);
    BOOST_REQUIRE_EQUAL(query.set_code(link, test::block1.header(), context{}, sub1(work), false), error::success);
    BOOST_REQUIRE(!link.is_terminal());
}

BOOST_AUTO_TEST_CASE(query_chain_writer__set_links__archived_txs__associated)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));

    const auto& block = test::block1a;
    const auto& txs = *block.transactions_ptr();
    header_link key{};
    BOOST_REQUIRE(!query.set_code(key, block.header(), context{ 0, 1, 0 }, {}, false));

    tx_links links{};
    for (const auto& tx: txs)
    {
        tx_link link{};
        BOOST_REQUIRE(!query.set_code(link, *tx));
        links.push_back(link);
    }

    BOOST_REQUIRE(!query.is_associated(key));
    BOOST_REQUIRE_EQUAL(query.set_code(key, links, false), error::success);
    BOOST_REQUIRE(query.is_associated(key));
    BOOST_REQUIRE_EQUAL(query.to_transactions(key), links);

    size_t light{}, heavy{};
    BOOST_REQUIRE(query.get_block_sizes(light, heavy, key));
    BOOST_REQUIRE_EQUAL(light, block.serialized_size(false));
    BOOST_REQUIRE_EQUAL(heavy, block.serialized_size(true));

    const auto pointer = query.get_block(key, true);
    BOOST_REQUIRE(pointer);
    BOOST_REQUIRE(*pointer == block);
}

BOOST_AUTO_TEST_CASE(query_chain_writer__set_links__empty_or_terminal__error)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));

    BOOST_REQUIRE_EQUAL(query.set_code(header_link{}, tx_links{ 0 }, false), error::txs_header);
    BOOST_REQUIRE_EQUAL(query.set_code(header_link{ 0 }, tx_links{}, false), error::txs_empty);
}

BOOST_AUTO_TEST_SUITE_END()
