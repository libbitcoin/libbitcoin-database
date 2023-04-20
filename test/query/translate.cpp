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
#include "../test.hpp"
#include "../mocks/blocks.hpp"
#include "../mocks/chunk_store.hpp"

struct query_translate_setup_fixture
{
    DELETE_COPY_MOVE(query_translate_setup_fixture);
    BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

    query_translate_setup_fixture() NOEXCEPT
    {
        BOOST_REQUIRE(test::clear(test::directory));
    }

    ~query_translate_setup_fixture() NOEXCEPT
    {
        BOOST_REQUIRE(test::clear(test::directory));
    }

    BC_POP_WARNING()
};

BOOST_FIXTURE_TEST_SUITE(query_translate_tests, query_translate_setup_fixture)

// nop event handler.
const auto events = [](auto, auto) {};

// to_candidate

BOOST_AUTO_TEST_CASE(query_translate__to_candidate__always__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(events), error::success);

    // initialize pushes the genesis candidate. 
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, test::context));
    BOOST_REQUIRE(query.set(test::block2, test::context));
    BOOST_REQUIRE_EQUAL(query.to_candidate(0), 0u);
    BOOST_REQUIRE_EQUAL(query.to_candidate(1), header_link::terminal);
    BOOST_REQUIRE_EQUAL(query.to_candidate(2), header_link::terminal);
    BOOST_REQUIRE_EQUAL(query.to_candidate(3), header_link::terminal);
    BOOST_REQUIRE_EQUAL(query.to_candidate(4), header_link::terminal);
    BOOST_REQUIRE_EQUAL(query.to_confirmed(0), 0u);
    BOOST_REQUIRE_EQUAL(query.to_confirmed(1), header_link::terminal);
    BOOST_REQUIRE_EQUAL(query.to_confirmed(2), header_link::terminal);
    BOOST_REQUIRE_EQUAL(query.to_confirmed(3), header_link::terminal);
    BOOST_REQUIRE_EQUAL(query.to_confirmed(4), header_link::terminal);

    // key-link translate of actual candidates.
    BOOST_REQUIRE(query.push_candidate(1));
    BOOST_REQUIRE(query.push_candidate(2));
    BOOST_REQUIRE_EQUAL(query.to_candidate(0), 0u);
    BOOST_REQUIRE_EQUAL(query.to_candidate(1), 1u);
    BOOST_REQUIRE_EQUAL(query.to_candidate(2), 2u);
    BOOST_REQUIRE_EQUAL(query.to_candidate(3), header_link::terminal);
    BOOST_REQUIRE_EQUAL(query.to_candidate(4), header_link::terminal);
    BOOST_REQUIRE_EQUAL(query.to_confirmed(0), 0u);
    BOOST_REQUIRE_EQUAL(query.to_confirmed(1), header_link::terminal);
    BOOST_REQUIRE_EQUAL(query.to_confirmed(2), header_link::terminal);
    BOOST_REQUIRE_EQUAL(query.to_confirmed(3), header_link::terminal);
    BOOST_REQUIRE_EQUAL(query.to_confirmed(4), header_link::terminal);
}

// to_confirmed

BOOST_AUTO_TEST_CASE(query_translate__to_confirmed__always__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(events), error::success);

    // initialize pushes the genesis confirmed. 
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, test::context));
    BOOST_REQUIRE(query.set(test::block2, test::context));
    BOOST_REQUIRE_EQUAL(query.to_confirmed(0), 0u);
    BOOST_REQUIRE_EQUAL(query.to_confirmed(1), header_link::terminal);
    BOOST_REQUIRE_EQUAL(query.to_confirmed(2), header_link::terminal);
    BOOST_REQUIRE_EQUAL(query.to_confirmed(3), header_link::terminal);
    BOOST_REQUIRE_EQUAL(query.to_confirmed(4), header_link::terminal);
    BOOST_REQUIRE_EQUAL(query.to_candidate(0), 0u);
    BOOST_REQUIRE_EQUAL(query.to_candidate(1), header_link::terminal);
    BOOST_REQUIRE_EQUAL(query.to_candidate(2), header_link::terminal);
    BOOST_REQUIRE_EQUAL(query.to_candidate(3), header_link::terminal);
    BOOST_REQUIRE_EQUAL(query.to_candidate(4), header_link::terminal);

    // key-link translate of actual confirmeds.
    BOOST_REQUIRE(query.push_confirmed(1));
    BOOST_REQUIRE(query.push_confirmed(2));
    BOOST_REQUIRE_EQUAL(query.to_confirmed(0), 0u);
    BOOST_REQUIRE_EQUAL(query.to_confirmed(1), 1u);
    BOOST_REQUIRE_EQUAL(query.to_confirmed(2), 2u);
    BOOST_REQUIRE_EQUAL(query.to_confirmed(3), header_link::terminal);
    BOOST_REQUIRE_EQUAL(query.to_confirmed(4), header_link::terminal);
    BOOST_REQUIRE_EQUAL(query.to_candidate(0), 0u);
    BOOST_REQUIRE_EQUAL(query.to_candidate(1), header_link::terminal);
    BOOST_REQUIRE_EQUAL(query.to_candidate(2), header_link::terminal);
    BOOST_REQUIRE_EQUAL(query.to_candidate(3), header_link::terminal);
    BOOST_REQUIRE_EQUAL(query.to_candidate(4), header_link::terminal);
}

// to_header

BOOST_AUTO_TEST_CASE(query_translate__to_header__always__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(events), error::success);
    BOOST_REQUIRE_EQUAL(query.to_header(test::genesis.hash()), header_link::terminal);
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE_EQUAL(query.to_header(test::genesis.hash()), 0u);
    BOOST_REQUIRE_EQUAL(query.to_header(test::block1.hash()), header_link::terminal);
    BOOST_REQUIRE_EQUAL(query.set_link(test::block1.header(), test::context), 1u);
    BOOST_REQUIRE_EQUAL(query.to_header(test::block1.hash()), 1u);
}

// to_point

BOOST_AUTO_TEST_CASE(query_translate__to_point__null_points__empty_points_table)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(events), error::success);

    // The four blocks have only null points, which are not archived.
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, test::context));
    BOOST_REQUIRE(query.set(test::block2, test::context));
    BOOST_REQUIRE(query.set(test::block3, test::context));
    BOOST_REQUIRE(store.point_body().empty());
}

BOOST_AUTO_TEST_CASE(query_translate__to_point__points__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(events), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1a, test::context));

    const auto point_body = system::base16_chunk
    (
        "ffffffff" // point_link::terminal
        "0100000000000000000000000000000000000000000000000000000000000000" // system::one_hash (x2)
        "ffffffff" // point_link::terminal
        "0200000000000000000000000000000000000000000000000000000000000000" // two_hash
    );
    BOOST_REQUIRE_EQUAL(store.point_body(), point_body);
    BOOST_REQUIRE_EQUAL(query.to_point(system::null_hash), point_link::terminal);
    BOOST_REQUIRE_EQUAL(query.to_point(system::one_hash), 0u);
    BOOST_REQUIRE_EQUAL(query.to_point(test::two_hash), 1u);
}

// to_tx

BOOST_AUTO_TEST_CASE(query_translate__to_tx__txs__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(events), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, test::context));
    BOOST_REQUIRE(query.set(test::block2, test::context));

    // All four blocks have one transaction.
    BOOST_REQUIRE_EQUAL(query.to_tx(test::genesis.transactions_ptr()->front()->hash(true)), 0u);
    BOOST_REQUIRE_EQUAL(query.to_tx(test::block1.transactions_ptr()->front()->hash(true)), 1u);
    BOOST_REQUIRE_EQUAL(query.to_tx(test::block2.transactions_ptr()->front()->hash(true)), 2u);
    BOOST_REQUIRE_EQUAL(query.to_tx(test::block3.transactions_ptr()->front()->hash(true)), tx_link::terminal);
}

// to_input_tx/to_input/to_tx_inputs/to_foreign_point/to_non_coinbase_inputs

BOOST_AUTO_TEST_CASE(query_translate__to_input_tx__to_input__expected)
{
    settings settings{};
    settings.input_buckets = 5;
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(events), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, test::context));
    BOOST_REQUIRE(query.set(test::block2, test::context));
    BOOST_REQUIRE(query.set(test::block3, test::context));

    // block1a has no true coinbase.
    BOOST_REQUIRE(query.set(test::block1a, test::context));

    // First 4 blocks have one transaction with 1 input, block1a has 3.
    BOOST_REQUIRE_EQUAL(query.to_input_tx(0x00), 0u);
    BOOST_REQUIRE_EQUAL(query.to_input_tx(0x63), 1u);
    BOOST_REQUIRE_EQUAL(query.to_input_tx(0x80), 2u);
    BOOST_REQUIRE_EQUAL(query.to_input_tx(0x9d), 3u);
    BOOST_REQUIRE_EQUAL(query.to_input_tx(0xba), 4u);
    BOOST_REQUIRE_EQUAL(query.to_input_tx(0xd6), 4u);
    BOOST_REQUIRE_EQUAL(query.to_input_tx(0xf2), 4u);

    BOOST_REQUIRE_EQUAL(query.to_input(0, 0), 0x00u);
    BOOST_REQUIRE_EQUAL(query.to_input(1, 0), 0x63u);
    BOOST_REQUIRE_EQUAL(query.to_input(2, 0), 0x80u);
    BOOST_REQUIRE_EQUAL(query.to_input(3, 0), 0x9du);
    BOOST_REQUIRE_EQUAL(query.to_input(4, 0), 0xbau);
    BOOST_REQUIRE_EQUAL(query.to_input(4, 1), 0xd6u);
    BOOST_REQUIRE_EQUAL(query.to_input(4, 2), 0xf2u);

    using namespace system;
    BOOST_REQUIRE_EQUAL(query.to_foreign_point(query.to_input(0, 0)), base16_array("ffffffffffffff"));
    BOOST_REQUIRE_EQUAL(query.to_foreign_point(query.to_input(1, 0)), base16_array("ffffffffffffff"));
    BOOST_REQUIRE_EQUAL(query.to_foreign_point(query.to_input(2, 0)), base16_array("ffffffffffffff"));
    BOOST_REQUIRE_EQUAL(query.to_foreign_point(query.to_input(3, 0)), base16_array("ffffffffffffff"));
    BOOST_REQUIRE_EQUAL(query.to_foreign_point(query.to_input(4, 0)), base16_array("00000000180000"));
    BOOST_REQUIRE_EQUAL(query.to_foreign_point(query.to_input(4, 1)), base16_array("000000002a0000"));
    BOOST_REQUIRE_EQUAL(query.to_foreign_point(query.to_input(4, 2)), base16_array("010000002b0000"));

    const input_links expected_links4{ 0xba, 0xd6, 0xf2 };
    BOOST_REQUIRE_EQUAL(query.to_tx_inputs(0), input_links{ 0x00 });
    BOOST_REQUIRE_EQUAL(query.to_tx_inputs(1), input_links{ 0x63 });
    BOOST_REQUIRE_EQUAL(query.to_tx_inputs(2), input_links{ 0x80 });
    BOOST_REQUIRE_EQUAL(query.to_tx_inputs(3), input_links{ 0x9d });
    BOOST_REQUIRE_EQUAL(query.to_tx_inputs(4), expected_links4);

    // TODO: All blocks have one transaction.
    BOOST_REQUIRE_EQUAL(query.to_non_coinbase_inputs(0), input_links{});
    BOOST_REQUIRE_EQUAL(query.to_non_coinbase_inputs(1), input_links{});
    BOOST_REQUIRE_EQUAL(query.to_non_coinbase_inputs(2), input_links{});
    BOOST_REQUIRE_EQUAL(query.to_non_coinbase_inputs(3), input_links{});
    BOOST_REQUIRE_EQUAL(query.to_non_coinbase_inputs(4), input_links{});

    // Past end.
    BOOST_REQUIRE_EQUAL(query.to_input_tx(277), tx_link::terminal);
    BOOST_REQUIRE_EQUAL(query.to_input(5, 0), input_link::terminal);
    BOOST_REQUIRE_EQUAL(query.to_foreign_point(input_link::terminal), foreign_point{});
    BOOST_REQUIRE_EQUAL(query.to_foreign_point(query.to_input(5, 0)), foreign_point{});
    BOOST_REQUIRE(query.to_tx_inputs(5).empty());
    BOOST_REQUIRE(query.to_non_coinbase_inputs(5).empty());

    // Verify expectations.
    const auto input_head = base16_chunk
    (
        "0000000000" // size
        "9d00000000"
        "ffffffffff"
        "d600000000"
        "ffffffffff"
        "f200000000"
    );
    const auto input_body = base16_chunk
    (
        // 0, 1, 2, 3, 4, 4, 4
        // genesis [00]->[terminal]
        "ffffffffff""ffffffffffffff""00000000""ffffffff4d04ffff001d0104455468652054696d65732030332f4a616e2f32303039204368616e63656c6c6f72206f6e206272696e6b206f66207365636f6e64206261696c6f757420666f722062616e6b7300"
        "0000000000""ffffffffffffff""01000000""ffffffff0704ffff001d010400" // block1  [64]->[00]
        "6300000000""ffffffffffffff""02000000""ffffffff0704ffff001d010b00" // block2  [82]->[64]
        "8000000000""ffffffffffffff""03000000""ffffffff0704ffff001d010e00" // block3  [a0]->[82]
        "ffffffffff""00000000180000""04000000""2a000000026a790103242424"   // block1a [be]->[terminal]
        "ffffffffff""000000002a0000""04000000""18000000026a7a0103313131"   // block1a [db]->[terminal]
        "ba00000000""010000002b0000""04000000""19000000026a7a0103424242"   // block1a [f8]->[be]
    );
    BOOST_REQUIRE_EQUAL(store.input_head(), input_head);
    BOOST_REQUIRE_EQUAL(store.input_body(), input_body);
}

// to_output_tx/to_output/to_tx_outputs/to_block_outputs

BOOST_AUTO_TEST_CASE(query_translate__to_output_tx__to_output__expected)
{
    settings settings{};
    settings.input_buckets = 5;
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(events), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, test::context));
    BOOST_REQUIRE(query.set(test::block2, test::context));
    BOOST_REQUIRE(query.set(test::block3, test::context));
    BOOST_REQUIRE(query.set(test::block1a, test::context));

    // All 5 blocks have one transaction with 1 output.
    BOOST_REQUIRE_EQUAL(query.to_output_tx(0 * 0x52), 0u);
    BOOST_REQUIRE_EQUAL(query.to_output_tx(1 * 0x52), 1u);
    BOOST_REQUIRE_EQUAL(query.to_output_tx(2 * 0x52), 2u);
    BOOST_REQUIRE_EQUAL(query.to_output_tx(3 * 0x52), 3u);
    BOOST_REQUIRE_EQUAL(query.to_output_tx(4 * 0x52), 4u);
    BOOST_REQUIRE_EQUAL(query.to_output_tx(4 * 0x52 + 8), 4u);

    BOOST_REQUIRE_EQUAL(query.to_output(0, 0), 0u * 0x52u);
    BOOST_REQUIRE_EQUAL(query.to_output(1, 0), 1u * 0x52u);
    BOOST_REQUIRE_EQUAL(query.to_output(2, 0), 2u * 0x52u);
    BOOST_REQUIRE_EQUAL(query.to_output(3, 0), 3u * 0x52u);
    BOOST_REQUIRE_EQUAL(query.to_output(4, 0), 4u * 0x52u);
    BOOST_REQUIRE_EQUAL(query.to_output(4, 1), 4u * 0x52u + 8u);

    const input_links expected_links4{ 4 * 0x52, 4 * 0x52 + 8 };
    BOOST_REQUIRE_EQUAL(query.to_tx_outputs(0), output_links{ 0 * 0x52 });
    BOOST_REQUIRE_EQUAL(query.to_tx_outputs(1), output_links{ 1 * 0x52 });
    BOOST_REQUIRE_EQUAL(query.to_tx_outputs(2), output_links{ 2 * 0x52 });
    BOOST_REQUIRE_EQUAL(query.to_tx_outputs(3), output_links{ 3 * 0x52 });
    BOOST_REQUIRE_EQUAL(query.to_tx_outputs(4), expected_links4);

    // TODO: All blocks have one transaction.
    BOOST_REQUIRE_EQUAL(query.to_block_outputs(0), output_links{ 0 * 0x52 });
    BOOST_REQUIRE_EQUAL(query.to_block_outputs(1), output_links{ 1 * 0x52 });
    BOOST_REQUIRE_EQUAL(query.to_block_outputs(2), output_links{ 2 * 0x52 });
    BOOST_REQUIRE_EQUAL(query.to_block_outputs(3), output_links{ 3 * 0x52 });
    BOOST_REQUIRE_EQUAL(query.to_block_outputs(4), expected_links4);

    // Past end.
    BOOST_REQUIRE_EQUAL(query.to_output_tx(4 * 0x52 + 16), tx_link::terminal);
    BOOST_REQUIRE_EQUAL(query.to_output(5, 0), output_link::terminal);
    BOOST_REQUIRE(query.to_tx_outputs(5).empty());
    BOOST_REQUIRE(query.to_block_outputs(5).empty());

    // Verify expectations.
    const auto output_body = system::base16_chunk
    (
        // 0, 1, 2, 3, 4, 4
        "00000000""00""ff00f2052a01000000434104678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38c4f35504e51ec112de5c384df7ba0b8d578a4c702b6bf11d5fac"
        "01000000""00""ff00f2052a0100000043410496b538e853519c726a2c91e61ec11600ae1390813a627c66fb8be7947be63c52da7589379515d4e0a604f8141781e62294721166bf621e73a82cbf2342c858eeac"
        "02000000""00""ff00f2052a010000004341047211a824f55b505228e4c3d5194c1fcfaa15a456abdf37f9b9d97a4040afc073dee6c89064984f03385237d92167c13e236446b417ab79a0fcae412ae3316b77ac"
        "03000000""00""ff00f2052a0100000043410494b9d3e76c5b1629ecf97fff95d7a4bbdac87cc26099ada28066c6ff1eb9191223cd897194a08d0c2726c5747f1db49e8cf90e75dc3e3550ae9b30086f3cd5aaac"
        "04000000""00""180179"
        "04000000""01""2a017a"
    );
    BOOST_REQUIRE_EQUAL(store.output_body(), output_body);
}

// to_prevout_tx/to_prevout

BOOST_AUTO_TEST_CASE(query_translate__to_prevout_tx__to_prevout__expected)
{
    settings settings{};
    settings.tx_buckets = 5;
    settings.input_buckets = 5;
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(events), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1a, test::context));
    BOOST_REQUIRE(query.set(test::block2a, test::context));

    ////// inputs in link order.
    ////BOOST_REQUIRE_EQUAL(query.to_prevout_tx(0x00), tx_link::terminal);
    ////BOOST_REQUIRE_EQUAL(query.to_prevout_tx(0x64 + 0 * 29), tx_link::terminal);
    ////BOOST_REQUIRE_EQUAL(query.to_prevout_tx(0x64 + 1 * 29), tx_link::terminal);
    ////BOOST_REQUIRE_EQUAL(query.to_prevout_tx(0x64 + 2 * 29), tx_link::terminal);
    ////BOOST_REQUIRE_EQUAL(query.to_prevout_tx(0x64 + 3 * 29), 1u); // block1a:0 (second serialized tx)
    ////BOOST_REQUIRE_EQUAL(query.to_prevout_tx(0x64 + 4 * 29), 1u); // block1a:0 (second serialized tx)
    ////BOOST_REQUIRE_EQUAL(query.to_prevout_tx(0x64 + 5 * 29), tx_link::terminal);
    ////BOOST_REQUIRE_EQUAL(query.to_prevout_tx(0x64 + 6 * 29), tx_link::terminal);

    ////BOOST_REQUIRE_EQUAL(query.to_prevout(0x00), output_link::terminal);
    ////BOOST_REQUIRE_EQUAL(query.to_prevout(0x64 + 0 * 29), output_link::terminal);
    ////BOOST_REQUIRE_EQUAL(query.to_prevout(0x64 + 1 * 29), output_link::terminal);
    ////BOOST_REQUIRE_EQUAL(query.to_prevout(0x64 + 2 * 29), output_link::terminal);
    ////BOOST_REQUIRE_EQUAL(query.to_prevout(0x64 + 3 * 29), 0x52u); // block1a:0:0 (second serialized tx, first output)
    ////BOOST_REQUIRE_EQUAL(query.to_prevout(0x64 + 4 * 29), 0x5au); // block1a:0:1 (second serialized tx, second output)
    ////BOOST_REQUIRE_EQUAL(query.to_prevout(0x64 + 5 * 29), output_link::terminal);
    ////BOOST_REQUIRE_EQUAL(query.to_prevout(0x64 + 6 * 29), output_link::terminal);

    // Past end.
    BOOST_REQUIRE_EQUAL(query.to_prevout_tx(0x64 + 7 * 29), tx_link::terminal);
    BOOST_REQUIRE_EQUAL(query.to_prevout(0x64 + 7 * 29), output_link::terminal);

    // Verify expectations.
    const auto input_head = system::base16_chunk
    (
        "0000000000" // size
        "0000000000"
        "ffffffffff"
        "ef00000000"
        "0b01000000"
        "9b00000000"
    );
    const auto input_body = system::base16_chunk
    (
        // points: (hash2 is block1a:tx0 [tx:1])
        // null_point, hash0:18, hash0:2a, hash1:2b, hash2:00, hash2:01, hash0:20, hash0:21.
        // transactions:
        // null_point, not_found, not_found, not_found, block1a:0:0, block1a:0:1, not_found, not_found.
        // genesis [00]->[terminal]
        "ffffffffff""ffffffff""ffffff""00000000""ffffffff4d04ffff001d0104455468652054696d65732030332f4a616e2f32303039204368616e63656c6c6f72206f6e206272696e6b206f66207365636f6e64206261696c6f757420666f722062616e6b7300"
        "ffffffffff""00000000""180000""01000000""2a000000026a790103242424" // 1a:0:0 [64]->[terminal]
        "ffffffffff""00000000""2a0000""01000000""18000000026a7a0103313131" // 1a:0:1 [81]->[terminal]
        "6300000000""01000000""2b0000""01000000""19000000026a7a0103424242" // 1a:0:2 [9e]->[64]
        "7f00000000""02000000""000000""02000000""a200000002ae790103242424" // 2a:0:0 [bb]->[81]
        "ffffffffff""02000000""010000""02000000""8100000002ae7a0103313131" // 2a:0:1 [d8]->[terminal]
        "b700000000""00000000""200000""03000000""a200000002ae790103242424" // 2a:1:0 [f5]->[bb]
        "d300000000""00000000""210000""03000000""8100000002ae7a0103313131" // 2a:1:1 [0112]->[d8]
    );
    const auto output_body = system::base16_chunk
    (
        "00000000""00""ff00f2052a01000000434104678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38c4f35504e51ec112de5c384df7ba0b8d578a4c702b6bf11d5fac"
        "01000000""00""180179" // [52]
        "01000000""01""2a017a" // [5a]
        "02000000""00""810179" // [62]
        "03000000""00""810179" // [6a]
    );
    const auto tx_head = system::base16_chunk
    (
        "00000000" // size
        "03000000"
        "ffffffff"
        "ffffffff"
        "01000000"
        "02000000"
    );

    ////// djb2
    ////const auto tx_body = system::base16_chunk
    ////(
    ////    "ffffffff" // genesis:0 [0]->terminal
    ////    "3ba3edfd7a7b12b27ac72c3e67768f617fc81bc3888a51323a9fb8aa4b1e5e4a"
    ////    "01cc0000cc000000000000010000000100000100000000000000"
    ////    "00000000" // block1a:0 [1]->[0]
    ////    "d19c4584d53264e5d0f9d2f852578c4d4382b69abee853bfbd6bc580f84069cf"
    ////    "009f0000b00000180000002a0000000300000200000200000000"
    ////    "ffffffff" // block2a:0 [2]->terminal
    ////    "c67bfbf8f354bd8f26d7a8b60c20b591dddf8760e02a1fcc3fd7af60b4253e67"
    ////    "006a000076000081000000a20000000200000100000700000000"
    ////    "02000000" // block2a:1 [3]->[2]
    ////    "64a86f067651854e2242b6ac9430b6d6806ea2b24dd7edec7b61dd885cf4a40c"
    ////    "006a000076000081000000a20000000200000100000a00000000"
    ////);

    // nosh
    const auto tx_body = system::base16_chunk
    (
        "ffffffff" // genesis:0 [0]->terminal
        "3ba3edfd7a7b12b27ac72c3e67768f617fc81bc3888a51323a9fb8aa4b1e5e4a"
        "01cc0000cc000000000000010000000100000100000000000000"
        "00000000" // block1a:0 [1]->[0]
        "d19c4584d53264e5d0f9d2f852578c4d4382b69abee853bfbd6bc580f84069cf"
        "009f0000b00000180000002a0000000300000200000200000000"
        "ffffffff" // block2a:0 [2]->terminal
        "c67bfbf8f354bd8f26d7a8b60c20b591dddf8760e02a1fcc3fd7af60b4253e67"
        "006a000076000081000000a20000000200000100000700000000"
        "ffffffff" // block2a:1 [3]->terminal
        "64a86f067651854e2242b6ac9430b6d6806ea2b24dd7edec7b61dd885cf4a40c"
        "006a000076000081000000a20000000200000100000a00000000"
    );

    BOOST_REQUIRE_EQUAL(store.input_head(), input_head);
    BOOST_REQUIRE_EQUAL(store.input_body(), input_body);
    BOOST_REQUIRE_EQUAL(store.output_body(), output_body);
    BOOST_REQUIRE_EQUAL(store.tx_head(), tx_head);
    BOOST_REQUIRE_EQUAL(store.tx_body(), tx_body);
}

// to_block/set_strong/set_unstrong

BOOST_AUTO_TEST_CASE(query_translate__to_block__set_strong__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(events), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, test::context));
    BOOST_REQUIRE(query.set(test::block2, test::context));

    // Either not strong or not found, except genesis.
    BOOST_REQUIRE_EQUAL(query.to_block(0), 0u);
    BOOST_REQUIRE_EQUAL(query.to_block(1), header_link::terminal);
    BOOST_REQUIRE_EQUAL(query.to_block(2), header_link::terminal);
    BOOST_REQUIRE_EQUAL(query.to_block(3), header_link::terminal);

    // push_candidate/push_confirmed has no effect.
    BOOST_REQUIRE(query.push_candidate(query.to_header(test::genesis.hash())));
    BOOST_REQUIRE(query.push_confirmed(query.to_header(test::genesis.hash())));
    BOOST_REQUIRE_EQUAL(query.to_block(0), 0u);
    BOOST_REQUIRE_EQUAL(query.to_block(1), header_link::terminal);
    BOOST_REQUIRE_EQUAL(query.to_block(2), header_link::terminal);
    BOOST_REQUIRE_EQUAL(query.to_block(3), header_link::terminal);

    // set_strong sets strong_by (only), and is idempotent.
    BOOST_REQUIRE(query.set_strong(query.to_header(test::genesis.hash())));
    BOOST_REQUIRE(query.set_strong(query.to_header(test::block1.hash())));
    BOOST_REQUIRE_EQUAL(query.to_block(0), 0u);
    BOOST_REQUIRE_EQUAL(query.to_block(1), 1u);
    BOOST_REQUIRE_EQUAL(query.to_block(2), header_link::terminal);
    BOOST_REQUIRE_EQUAL(query.to_block(3), header_link::terminal);

    // candidate/confirmed unaffected.
    BOOST_REQUIRE(query.is_candidate_block(query.to_header(test::genesis.hash())));
    BOOST_REQUIRE(query.is_confirmed_block(query.to_header(test::genesis.hash())));
    BOOST_REQUIRE(!query.is_candidate_block(query.to_header(test::block1.hash())));
    BOOST_REQUIRE(!query.is_confirmed_block(query.to_header(test::block1.hash())));

    // set_unstrong unsets strong_by, and is idempotent.
    BOOST_REQUIRE(query.set_unstrong(query.to_header(test::genesis.hash())));
    BOOST_REQUIRE(query.set_unstrong(query.to_header(test::block1.hash())));
    BOOST_REQUIRE(query.set_unstrong(query.to_header(test::block2.hash())));
    BOOST_REQUIRE_EQUAL(query.to_block(0), header_link::terminal);
    BOOST_REQUIRE_EQUAL(query.to_block(1), header_link::terminal);
    BOOST_REQUIRE_EQUAL(query.to_block(2), header_link::terminal);
    BOOST_REQUIRE_EQUAL(query.to_block(3), header_link::terminal);
}

// _to_parent

BOOST_AUTO_TEST_CASE(query_translate__to_parent__always__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(events), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, test::context));
    BOOST_REQUIRE(query.set(test::block2, test::context));
    BOOST_REQUIRE(query.set(test::block1a, test::context));
    BOOST_REQUIRE(query.set(test::block2a, test::context));
    BOOST_REQUIRE_EQUAL(query.to_parent(0), header_link::terminal);
    BOOST_REQUIRE_EQUAL(query.to_parent(1), 0u);
    BOOST_REQUIRE_EQUAL(query.to_parent(2), 1u);
    BOOST_REQUIRE_EQUAL(query.to_parent(3), 0u);
    BOOST_REQUIRE_EQUAL(query.to_parent(4), 3u);
    BOOST_REQUIRE_EQUAL(query.to_parent(5), header_link::terminal);
}

// to_txs

BOOST_AUTO_TEST_CASE(query_translate__to_txs__always__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(events), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1a, test::context));
    BOOST_REQUIRE(query.set(test::block2a, test::context));

    const tx_links expected_links2{ 2, 3 };
    BOOST_REQUIRE_EQUAL(query.to_txs(0), tx_links{ 0 });
    BOOST_REQUIRE_EQUAL(query.to_txs(1), tx_links{ 1 });
    BOOST_REQUIRE_EQUAL(query.to_txs(2), expected_links2);
    BOOST_REQUIRE(query.to_txs(3).empty());
}

// to_spenders

BOOST_AUTO_TEST_CASE(query_translate__to_spenders__point__expected)
{
    settings settings{};
    settings.tx_buckets = 5;
    settings.input_buckets = 5;
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(events), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1a, test::context));
    BOOST_REQUIRE(query.set(test::block2a, test::context));
    BOOST_REQUIRE(query.set(test::tx4));

    BOOST_REQUIRE(query.to_spenders({ test::genesis.hash(), 0 }).empty());
    const auto expected0 = input_links{ 0x0127, 0x00b7 };
    const auto expected1 = input_links{ 0x0143, 0x00d3 };
    const auto spenders0a = query.to_spenders({ test::block1a.transactions_ptr()->front()->hash(false), 0 });
    const auto spenders1a = query.to_spenders({ test::block1a.transactions_ptr()->front()->hash(false), 1 });
    BOOST_REQUIRE_EQUAL(spenders0a, expected0);
    BOOST_REQUIRE_EQUAL(spenders1a, expected1);
    BOOST_REQUIRE(query.to_spenders({ test::block2a.transactions_ptr()->front()->hash(false), 0 }).empty());
    BOOST_REQUIRE(query.to_spenders({ test::block2a.transactions_ptr()->back()->hash(false), 0 }).empty());
    BOOST_REQUIRE(query.to_spenders({ test::tx4.hash(false), 0 }).empty());
    BOOST_REQUIRE(query.to_spenders({ test::tx4.hash(false), 1 }).empty()); // n/a, only one output
    BOOST_REQUIRE(query.to_spenders({ system::null_hash, 0xffffffff }).empty());

    BOOST_REQUIRE(query.to_spenders(0x00).empty());
    const auto spenders0b = query.to_spenders(0x52);
    const auto spenders1b = query.to_spenders(0x5a);
    BOOST_REQUIRE_EQUAL(spenders0b, expected0);
    BOOST_REQUIRE_EQUAL(spenders1b, expected1);
    BOOST_REQUIRE(query.to_spenders(0x62).empty());
    BOOST_REQUIRE(query.to_spenders(0x6a).empty());
    BOOST_REQUIRE(query.to_spenders(0x72).empty());
    BOOST_REQUIRE(query.to_spenders(output_link::terminal).empty());

    BOOST_REQUIRE(query.to_spenders(0, 0).empty());
    const auto spenders0c = query.to_spenders(1, 0);
    const auto spenders1c = query.to_spenders(1, 1);
    BOOST_REQUIRE_EQUAL(spenders0c, expected0);
    BOOST_REQUIRE_EQUAL(spenders1c, expected1);
    BOOST_REQUIRE(query.to_spenders(2, 0).empty());
    BOOST_REQUIRE(query.to_spenders(3, 0).empty());
    BOOST_REQUIRE(query.to_spenders(4, 0).empty());
    BOOST_REQUIRE(query.to_spenders(4, 1).empty()); // n/a, only one output
    BOOST_REQUIRE(query.to_spenders(tx_link::terminal, 0).empty());

    // Verify expectations.
    const auto input_head = system::base16_chunk
    (
        "0000000000" // size
        "0000000000"
        "ffffffffff"
        "2701000000"
        "4301000000"
        "9b00000000"
    );
    const auto input_body = system::base16_chunk
    (
        "ffffffffff""ffffffff""ffffff""00000000""ffffffff4d04ffff001d0104455468652054696d65732030332f4a616e2f32303039204368616e63656c6c6f72206f6e206272696e6b206f66207365636f6e64206261696c6f757420666f722062616e6b7300"
        "ffffffffff""00000000""180000""01000000""2a000000026a790103242424" // 1a:0:0 [64]->[terminal]
        "ffffffffff""00000000""2a0000""01000000""18000000026a7a0103313131" // 1a:0:1 [81]->[terminal]
        "6300000000""01000000""2b0000""01000000""19000000026a7a0103424242" // 1a:0:2 [9e]->[64]
        "7f00000000""02000000""000000""02000000""a200000002ae790103242424" // 2a:0:0 [bb]->[81]
        "ffffffffff""02000000""010000""02000000""8100000002ae7a0103313131" // 2a:0:1 [d8]->[terminal]
        "b700000000""00000000""200000""03000000""a200000002ae790103242424" // 2a:1:0 [f5]->[bb]
        "d300000000""00000000""210000""03000000""8100000002ae7a0103313131" // 2a:1:1 [0112]->[d8]
        "ef00000000""02000000""000000""04000000""a500000002ae790103252525" //  tx4:0 [012f]->[f5]
        "0b01000000""02000000""010000""04000000""8500000002ae7a0103353535" //  tx4:1 [014c]->[0112] 
    );
    const auto output_body = system::base16_chunk
    (
        "00000000""00""ff00f2052a01000000434104678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38c4f35504e51ec112de5c384df7ba0b8d578a4c702b6bf11d5fac"
        "01000000""00""180179" // [52]
        "01000000""01""2a017a" // [5a]
        "02000000""00""810179" // [62]
        "03000000""00""810179" // [6a]
        "04000000""00""850179" // [72]
    );
    const auto tx_head = system::base16_chunk
    (
        "00000000" // size
        "03000000"
        "ffffffff"
        "04000000"
        "01000000"
        "02000000"
    );

    ////// djb2
    ////const auto tx_body = system::base16_chunk
    ////(
    ////    "ffffffff" // genesis:0 [0]->terminal
    ////    "3ba3edfd7a7b12b27ac72c3e67768f617fc81bc3888a51323a9fb8aa4b1e5e4a"
    ////    "01cc0000cc000000000000010000000100000100000000000000"
    ////    "00000000" // block1a:0 [1]->[0]
    ////    "d19c4584d53264e5d0f9d2f852578c4d4382b69abee853bfbd6bc580f84069cf"
    ////    "009f0000b00000180000002a0000000300000200000200000000"
    ////    "ffffffff" // block2a:0 [2]->terminal
    ////    "c67bfbf8f354bd8f26d7a8b60c20b591dddf8760e02a1fcc3fd7af60b4253e67"
    ////    "006a000076000081000000a20000000200000100000700000000"
    ////    "02000000" // block2a:1 [3]->[2]
    ////    "64a86f067651854e2242b6ac9430b6d6806ea2b24dd7edec7b61dd885cf4a40c"
    ////    "006a000076000081000000a20000000200000100000a00000000"
    ////    "ffffffff" // tx4 [4]->terminal
    ////    "abee882062e8df25c967717d0f97e0133af9be84861a427dd4e3f7370549c441"
    ////    "006a000076000085000000a50000000200000100000d00000000"
    ////);

    // nosh
    const auto tx_body = system::base16_chunk
    (
        "ffffffff" // genesis:0 [0]->terminal
        "3ba3edfd7a7b12b27ac72c3e67768f617fc81bc3888a51323a9fb8aa4b1e5e4a"
        "01cc0000cc000000000000010000000100000100000000000000"
        "00000000" // block1a:0 [1]->[0]
        "d19c4584d53264e5d0f9d2f852578c4d4382b69abee853bfbd6bc580f84069cf"
        "009f0000b00000180000002a0000000300000200000200000000"
        "ffffffff" // block2a:0 [2]->terminal
        "c67bfbf8f354bd8f26d7a8b60c20b591dddf8760e02a1fcc3fd7af60b4253e67"
        "006a000076000081000000a20000000200000100000700000000"
        "ffffffff" // block2a:1 [3]->terminal
        "64a86f067651854e2242b6ac9430b6d6806ea2b24dd7edec7b61dd885cf4a40c"
        "006a000076000081000000a20000000200000100000a00000000"
        "ffffffff" // tx4 [4]->terminal
        "abee882062e8df25c967717d0f97e0133af9be84861a427dd4e3f7370549c441"
        "006a000076000085000000a50000000200000100000d00000000"
    );

    BOOST_REQUIRE_EQUAL(store.input_head(), input_head);
    BOOST_REQUIRE_EQUAL(store.input_body(), input_body);
    BOOST_REQUIRE_EQUAL(store.output_body(), output_body);
    BOOST_REQUIRE_EQUAL(store.tx_head(), tx_head);
    BOOST_REQUIRE_EQUAL(store.tx_body(), tx_body);
}

BOOST_AUTO_TEST_SUITE_END()
