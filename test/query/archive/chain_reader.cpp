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

BOOST_FIXTURE_TEST_SUITE(query_chain_reader_tests, test::directory_setup_fixture)

// get_header

BOOST_AUTO_TEST_CASE(query_chain_reader__get_header__invalid_parent__expected)
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
        "119192939495969798999a9b9c9d9e9f229192939495969798999a9b9c9d9e9f" // merkle_root
        "00000000000000000000000000000000" // work
        "00"); // header flags (milestone false)

    settings settings{};
    settings.header.buckets = 16;
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_CHECK(!store.create(test::events_handler));

    store.header_head() = expected_header_head;
    store.header_body() = expected_header_body;
    BOOST_CHECK(!query.get_header(query.to_header(block_hash)));
    BOOST_CHECK(!query.get_header(header_link::terminal));
}

BOOST_AUTO_TEST_CASE(query_chain_reader__get_header__default__expected)
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
        "ffff7f"   // previous_block_hash (header_fk - terminal)
        "34333231" // version
        "44434241" // timestamp
        "54535251" // bits
        "64636261" // nonce
        "119192939495969798999a9b9c9d9e9f229192939495969798999a9b9c9d9e9f" // merkle_root
        "00000000000000000000000000000000" // work
        "01"); // header flags (milestone)

    settings settings{};
    settings.header.buckets = 16;
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_CHECK(!store.create(test::events_handler));

    ////store.header_head() = expected_header_head;
    ////store.header_body() = expected_header_body;
    BOOST_CHECK(query.set(header, context{ 0x11121314, 0x01020304, 0x21222324 }, {}, true));
    BOOST_CHECK_EQUAL(store.header_head(), expected_header_head);
    BOOST_CHECK_EQUAL(store.header_body(), expected_header_body);

    const auto foo = query.to_header(block_hash);
    const auto pointer1 = query.get_header(foo);
    BOOST_CHECK(pointer1);
    BOOST_CHECK(*pointer1 == header);

    // Verify hash caching.
    BOOST_CHECK_EQUAL(pointer1->hash(), block_hash);
}

BOOST_AUTO_TEST_CASE(query_chain_reader__get_witness__not_found__nullptr)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_CHECK(!store.create(test::events_handler));
    BOOST_CHECK(!query.get_witness(query.to_point({}, {})));
    BOOST_CHECK(!store.close(test::events_handler));
}

BOOST_AUTO_TEST_CASE(query_chain_reader__get_input_script__not_found__nullptr)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_CHECK(!store.create(test::events_handler));
    BOOST_CHECK(!query.get_input_script(query.to_point(tx_link{}, {})));
    BOOST_CHECK(!store.close(test::events_handler));
}

BOOST_AUTO_TEST_CASE(query_chain_reader__get_input__not_found__nullptr)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_CHECK(!store.create(test::events_handler));
    BOOST_CHECK(!query.get_input(query.to_point(tx_link{}, {}), false));
    BOOST_CHECK(!query.get_input(query.to_tx(system::null_hash), {}, false));
    BOOST_CHECK(!store.close(test::events_handler));
}

BOOST_AUTO_TEST_CASE(query_chain_reader__get_input__genesis__expected)
{
    settings settings{};
    settings.header.buckets = 8;
    settings.tx.buckets = 8;
    settings.txs.buckets = 16;
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_CHECK(!store.create(test::events_handler));
    BOOST_CHECK(query.set(test::genesis, test::context, {}, false, false));

    const auto tx = test::genesis.transactions_ptr()->front();
    const auto input = tx->inputs_ptr()->front();
    const auto point = query.to_point(query.to_tx(tx->hash(false)), 0u);
    BOOST_CHECK(*input == *query.get_input(point, false));
    BOOST_CHECK(input->witness() == *query.get_witness(point));
    BOOST_CHECK(input->script() == *query.get_input_script(point));
}

BOOST_AUTO_TEST_CASE(query_chain_reader__get_inputs__tx_not_found__nullptr)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_CHECK(!store.create(test::events_handler));
    BOOST_CHECK(query.initialize(test::genesis));
    BOOST_CHECK(!query.get_inputs(1, false));
}

BOOST_AUTO_TEST_CASE(query_chain_reader__get_inputs__found__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_CHECK(!store.create(test::events_handler));
    BOOST_CHECK(query.initialize(test::genesis));
    BOOST_CHECK(query.set(test::tx4));
    BOOST_CHECK_EQUAL(query.get_inputs(1, false)->size(), 2u);
}

BOOST_AUTO_TEST_CASE(query_chain_reader__get_output_script__not_found__nullptr)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_CHECK(!store.create(test::events_handler));
    BOOST_CHECK(!query.get_output_script(query.to_output(system::null_hash, 0u)));
    BOOST_CHECK(!query.get_output(0));
    BOOST_CHECK(!store.close(test::events_handler));
}

BOOST_AUTO_TEST_CASE(query_chain_reader__get_output__not_found__nullptr)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_CHECK(!store.create(test::events_handler));
    BOOST_CHECK(!query.get_output(query.to_tx(system::null_hash), 0u));
    BOOST_CHECK(!query.get_output(query.to_output(system::null_hash, 0u)));
    BOOST_CHECK(!query.get_output(0));
    BOOST_CHECK(!store.close(test::events_handler));
}

BOOST_AUTO_TEST_CASE(query_chain_reader__get_output__genesis__expected)
{
    settings settings{};
    settings.header.buckets = 8;
    settings.tx.buckets = 8;
    settings.txs.buckets = 16;
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_CHECK(!store.create(test::events_handler));
    BOOST_CHECK(query.set(test::genesis, test::context, {}, false, false));

    const auto tx = test::genesis.transactions_ptr()->front();
    const auto output = tx->outputs_ptr()->front();
    BOOST_CHECK(*output == *query.get_output(0));
    BOOST_CHECK(*output == *query.get_output(query.to_tx(tx->hash(false)), 0u));
    BOOST_CHECK(*output == *query.get_output(query.to_output(tx->hash(false), 0u)));
    BOOST_CHECK(output->script() == *query.get_output_script(query.to_output(tx->hash(false), 0u)));
}

BOOST_AUTO_TEST_CASE(query_chain_reader__get_outputs__tx_not_found__nullptr)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_CHECK(!store.create(test::events_handler));
    BOOST_CHECK(query.initialize(test::genesis));
    BOOST_CHECK(!query.get_outputs(1));
}

BOOST_AUTO_TEST_CASE(query_chain_reader__get_outputs__found__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_CHECK(!store.create(test::events_handler));
    BOOST_CHECK(query.initialize(test::genesis));
    BOOST_CHECK(query.set(test::tx4));
    BOOST_CHECK_EQUAL(query.get_outputs(1)->size(), 1u);
}

BOOST_AUTO_TEST_CASE(query_chain_reader__get_transactions__tx_not_found__nullptr)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_CHECK(!store.create(test::events_handler));
    BOOST_CHECK(query.initialize(test::genesis));
    BOOST_CHECK(query.set(test::tx4));
    BOOST_CHECK(!query.get_transactions(3, false));
}

BOOST_AUTO_TEST_CASE(query_chain_reader__get_transactions__found__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_CHECK(!store.create(test::events_handler));
    BOOST_CHECK(query.initialize(test::genesis));
    BOOST_CHECK(query.set(test::block1a, test::context, {}, false, false));
    BOOST_CHECK(query.set(test::block2a, test::context, {}, false, false));
    BOOST_CHECK(query.set(test::tx4));
    BOOST_CHECK_EQUAL(query.get_transactions(0, false)->size(), 1u);
    BOOST_CHECK_EQUAL(query.get_transactions(1, false)->size(), 1u);
    BOOST_CHECK_EQUAL(query.get_transactions(2, false)->size(), 2u);
}

BOOST_AUTO_TEST_CASE(query_chain_reader__get_spenders__unspent_or_not_found__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_CHECK(!store.create(test::events_handler));
    BOOST_CHECK(query.initialize(test::genesis));
    BOOST_CHECK(query.set(test::block1, test::context, {}, false, false));
    BOOST_CHECK(query.set(test::block2, test::context, {}, false, false));
    BOOST_CHECK(query.set(test::block3, test::context, {}, false, false));

    // Caller should always test for nullptr.
    BOOST_CHECK(query.get_spenders(output_link::terminal, true)->empty());
    BOOST_CHECK(query.get_spenders(query.to_output(0, 0), true)->empty());
    BOOST_CHECK(query.get_spenders(query.to_output(0, 1), true)->empty());
    BOOST_CHECK(query.get_spenders(query.to_output(1, 0), true)->empty());
    BOOST_CHECK(query.get_spenders(query.to_output(1, 1), true)->empty());
    BOOST_CHECK(query.get_spenders(query.to_output(2, 0), true)->empty());
    BOOST_CHECK(query.get_spenders(query.to_output(2, 1), true)->empty());
    BOOST_CHECK(query.get_spenders(query.to_output(3, 0), true)->empty());
    BOOST_CHECK(query.get_spenders(query.to_output(3, 1), true)->empty());
}

BOOST_AUTO_TEST_SUITE_END()
