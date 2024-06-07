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
const auto events_handler = [](auto, auto) {};

// to_candidate

BOOST_AUTO_TEST_CASE(query_translate__to_candidate__always__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(events_handler), error::success);

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
    BOOST_REQUIRE_EQUAL(store.create(events_handler), error::success);

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
    BOOST_REQUIRE_EQUAL(store.create(events_handler), error::success);
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
    BOOST_REQUIRE_EQUAL(store.create(events_handler), error::success);

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
    BOOST_REQUIRE_EQUAL(store.create(events_handler), error::success);
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
    BOOST_REQUIRE_EQUAL(store.create(events_handler), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, test::context));
    BOOST_REQUIRE(query.set(test::block2, test::context));

    // All four blocks have one transaction.
    BOOST_REQUIRE_EQUAL(query.to_tx(test::genesis.transactions_ptr()->front()->hash(true)), 0u);
    BOOST_REQUIRE_EQUAL(query.to_tx(test::block1.transactions_ptr()->front()->hash(true)), 1u);
    BOOST_REQUIRE_EQUAL(query.to_tx(test::block2.transactions_ptr()->front()->hash(true)), 2u);
    BOOST_REQUIRE_EQUAL(query.to_tx(test::block3.transactions_ptr()->front()->hash(true)), tx_link::terminal);
}

// to_spend_tx/to_spend/to_tx_spends/to_spend_key/to_non_coinbase_spends

BOOST_AUTO_TEST_CASE(query_translate__to_spend_tx__to_spend__expected)
{
    class accessor
      : public test::query_accessor
    {
    public:
        using test::query_accessor::query_accessor;
        spend_links to_tx_spends_(uint32_t& version,
            const tx_link& link) const NOEXCEPT
        {
            return test::query_accessor::to_tx_spends(version, link);
        }
    };

    settings settings{};
    settings.path = TEST_DIRECTORY;
    settings.spend_buckets = 5;
    test::chunk_store store{ settings };
    accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(events_handler), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, test::context));
    BOOST_REQUIRE(query.set(test::block2, test::context));
    BOOST_REQUIRE(query.set(test::block3, test::context));

    // block1a has no true coinbase.
    BOOST_REQUIRE(query.set(test::block1a, test::context));

    // First 4 blocks have one transaction with 1 input, block1a has 3.
    BOOST_REQUIRE_EQUAL(query.to_spend_tx(0), 0u);
    BOOST_REQUIRE_EQUAL(query.to_spend_tx(1), 1u);
    BOOST_REQUIRE_EQUAL(query.to_spend_tx(2), 2u);
    BOOST_REQUIRE_EQUAL(query.to_spend_tx(3), 3u);
    BOOST_REQUIRE_EQUAL(query.to_spend_tx(4), 4u);
    BOOST_REQUIRE_EQUAL(query.to_spend_tx(5), 4u);
    BOOST_REQUIRE_EQUAL(query.to_spend_tx(6), 4u);

    BOOST_REQUIRE_EQUAL(query.to_spend(0, 0), 0u);
    BOOST_REQUIRE_EQUAL(query.to_spend(1, 0), 1u);
    BOOST_REQUIRE_EQUAL(query.to_spend(2, 0), 2u);
    BOOST_REQUIRE_EQUAL(query.to_spend(3, 0), 3u);
    BOOST_REQUIRE_EQUAL(query.to_spend(4, 0), 4u);
    BOOST_REQUIRE_EQUAL(query.to_spend(4, 1), 5u);
    BOOST_REQUIRE_EQUAL(query.to_spend(4, 2), 6u);

    using namespace system;
    BOOST_REQUIRE_EQUAL(query.to_spend_key(query.to_spend(0, 0)), base16_array("ffffffffffffff"));
    BOOST_REQUIRE_EQUAL(query.to_spend_key(query.to_spend(1, 0)), base16_array("ffffffffffffff"));
    BOOST_REQUIRE_EQUAL(query.to_spend_key(query.to_spend(2, 0)), base16_array("ffffffffffffff"));
    BOOST_REQUIRE_EQUAL(query.to_spend_key(query.to_spend(3, 0)), base16_array("ffffffffffffff"));
    BOOST_REQUIRE_EQUAL(query.to_spend_key(query.to_spend(4, 0)), base16_array("00000000180000"));
    BOOST_REQUIRE_EQUAL(query.to_spend_key(query.to_spend(4, 1)), base16_array("000000002a0000"));
    BOOST_REQUIRE_EQUAL(query.to_spend_key(query.to_spend(4, 2)), base16_array("010000002b0000"));

    const spend_links expected_links4{ 4, 5, 6 };
    BOOST_REQUIRE_EQUAL(query.to_tx_spends(0), spend_links{ 0 });
    BOOST_REQUIRE_EQUAL(query.to_tx_spends(1), spend_links{ 1 });
    BOOST_REQUIRE_EQUAL(query.to_tx_spends(2), spend_links{ 2 });
    BOOST_REQUIRE_EQUAL(query.to_tx_spends(3), spend_links{ 3 });
    BOOST_REQUIRE_EQUAL(query.to_tx_spends(4), expected_links4);

    uint32_t version{};
    BOOST_REQUIRE_EQUAL(query.to_tx_spends_(version, 0), spend_links{ 0 });
    BOOST_REQUIRE_EQUAL(version, test::genesis.transactions_ptr()->front()->version());
    BOOST_REQUIRE_EQUAL(query.to_tx_spends_(version, 1), spend_links{ 1 });
    BOOST_REQUIRE_EQUAL(version, test::block1.transactions_ptr()->front()->version());
    BOOST_REQUIRE_EQUAL(query.to_tx_spends_(version, 2), spend_links{ 2 });
    BOOST_REQUIRE_EQUAL(version, test::block2.transactions_ptr()->front()->version());
    BOOST_REQUIRE_EQUAL(query.to_tx_spends_(version, 3), spend_links{ 3 });
    BOOST_REQUIRE_EQUAL(version, test::block3.transactions_ptr()->front()->version());
    BOOST_REQUIRE_EQUAL(query.to_tx_spends_(version, 4), expected_links4);
    BOOST_REQUIRE_EQUAL(version, test::block1a.transactions_ptr()->front()->version());

    // TODO: All blocks have one transaction.
    BOOST_REQUIRE_EQUAL(query.to_non_coinbase_spends(0), spend_links{});
    BOOST_REQUIRE_EQUAL(query.to_non_coinbase_spends(1), spend_links{});
    BOOST_REQUIRE_EQUAL(query.to_non_coinbase_spends(2), spend_links{});
    BOOST_REQUIRE_EQUAL(query.to_non_coinbase_spends(3), spend_links{});
    BOOST_REQUIRE_EQUAL(query.to_non_coinbase_spends(4), spend_links{});

    // Past end.
    BOOST_REQUIRE_EQUAL(query.to_spend_tx(7), tx_link::terminal);
    BOOST_REQUIRE_EQUAL(query.to_spend(5, 0), spend_link::terminal);
    BOOST_REQUIRE_EQUAL(query.to_spend_key(spend_link::terminal), foreign_point{});
    BOOST_REQUIRE_EQUAL(query.to_spend_key(query.to_spend(5, 0)), foreign_point{});
    BOOST_REQUIRE(query.to_tx_spends(5).empty());
    BOOST_REQUIRE(query.to_non_coinbase_spends(5).empty());

    // Verify expectations.
    const auto spend_head = base16_chunk
    (
        "00000000"   // size
        "03000000"   // pk->3
        "ffffffff"
        "05000000"   // pk->5
        "ffffffff"
        "04000000"   // pk->4
    );
    const auto spend_body = base16_chunk
    (
        "ffffffff"   // terminal->
        "ffffffff"   // fp: point_fk->
        "ffffff"     // fp: point_index (null)
        "00000000"   // parent_fk->
        "ffffffff"   // sequence
        "0000000000" // input_fk->

        "00000000"   // pk->0
        "ffffffff"   // fp: point_fk->
        "ffffff"     // fp: point_index (null)
        "01000000"   // parent_fk->
        "ffffffff"   // sequence
        "4f00000000" // input_fk->

        "01000000"   // pk->1
        "ffffffff"   // fp: point_fk->
        "ffffff"     // fp: point_index (null)
        "02000000"   // parent_fk->
        "ffffffff"   // sequence
        "5800000000" // input_fk->

        "02000000"   // pk->2
        "ffffffff"   // fp: point_fk->
        "ffffff"     // fp: point_index (null)
        "03000000"   // parent_fk->
        "ffffffff"   // sequence
        "6100000000" // input_fk->

        "06000000"   // pk->6
        "00000000"   // fp: point_fk->
        "180000"     // fp: point_index
        "04000000"   // parent_fk->
        "2a000000"   // sequence
        "6a00000000" // input_fk->

        "ffffffff"   // terminal->
        "00000000"   // fp: point_fk->
        "2a0000"     // fp: point_index
        "04000000"   // parent_fk->
        "18000000"   // sequence
        "7200000000" // input_fk->

        "ffffffff"   // terminal->
        "01000000"   // fp: point_fk->
        "2b0000"     // fp: point_index
        "04000000"   // parent_fk->
        "19000000"   // sequence
        "7a00000000" // input_fk->
    );
    const auto input_head = base16_chunk
    (
        "0000000000"
    );
    const auto input_body = base16_chunk
    (
        "4d04ffff001d0104455468652054696d65732030332f4a616e2f32303039204368616e63656c6c6f72206f6e206272696e6b206f66207365636f6e64206261696c6f757420666f722062616e6b7300"
        "0704ffff001d010400"
        "0704ffff001d010b00"
        "0704ffff001d010e00"
        "026a790103242424"
        "026a7a0103313131"
        "026a7a0103424242"
    );
    BOOST_REQUIRE_EQUAL(store.spend_head(), spend_head);
    BOOST_REQUIRE_EQUAL(store.spend_body(), spend_body);
    BOOST_REQUIRE_EQUAL(store.input_head(), input_head);
    BOOST_REQUIRE_EQUAL(store.input_body(), input_body);
}

BOOST_AUTO_TEST_CASE(query_translate__to_non_coinbase_spends__populated__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(events_handler), error::success);

    // coinbase only (null and first).
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.to_non_coinbase_spends(0).empty());
    BOOST_REQUIRE(query.to_non_coinbase_spends(1).empty());
    BOOST_REQUIRE(query.to_non_coinbase_spends(2).empty());

    BOOST_REQUIRE_EQUAL(store.point_body(), system::base16_chunk(""));
    BOOST_REQUIRE_EQUAL(store.spend_body(),
        system::base16_chunk("ffffffffffffffffffffff00000000ffffffff0000000000"));
    BOOST_REQUIRE_EQUAL(store.input_body(),
        system::base16_chunk("4d04ffff001d0104455468652054696d65732030332f4a616e2f32303039204368616e63656c6c6f72206f6e206272696e6b206f66207365636f6e64206261696c6f757420666f722062616e6b73""00"));
    BOOST_REQUIRE_EQUAL(store.output_body(),
        system::base16_chunk("00000000""ff00f2052a01000000""434104678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38c4f35504e51ec112de5c384df7ba0b8d578a4c702b6bf11d5fac"));

    // coinbase only (null and first).
    BOOST_REQUIRE(query.set(test::block1b, context{ 0, 1, 0 }));
    BOOST_REQUIRE(query.to_non_coinbase_spends(0).empty());
    BOOST_REQUIRE(query.to_non_coinbase_spends(1).empty());
    BOOST_REQUIRE(query.to_non_coinbase_spends(2).empty());

    BOOST_REQUIRE_EQUAL(store.point_body(), system::base16_chunk(""));
    BOOST_REQUIRE_EQUAL(store.spend_body(),
        system::base16_chunk("ffffffffffffffffffffff00000000ffffffff0000000000"
                             "00000000ffffffffffffff01000000b10000004f00000000"));
    BOOST_REQUIRE_EQUAL(store.input_body(),
        system::base16_chunk("4d04ffff001d0104455468652054696d65732030332f4a616e2f32303039204368616e63656c6c6f72206f6e206272696e6b206f66207365636f6e64206261696c6f757420666f722062616e6b73""00"
                             "02ae82""00"));
    BOOST_REQUIRE_EQUAL(store.output_body(),
        system::base16_chunk("00000000""ff00f2052a01000000""434104678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38c4f35504e51ec112de5c384df7ba0b8d578a4c702b6bf11d5fac"
                             "01000000""b1""0179"
                             "01000000""b1""0179"));

    // 2 inputs (block1b and tx2b).
    BOOST_REQUIRE(query.set(test::block_spend_internal_2b, context{ 0, 101, 0 }));
    BOOST_REQUIRE(query.to_non_coinbase_spends(0).empty());
    BOOST_REQUIRE(query.to_non_coinbase_spends(1).empty());

    // Two points because non-null, but only one is non-first (also coinbase criteria).
    // block_spend_internal_2b first tx (tx2b) is first but with non-null input.
    // block_spend_internal_2b second tx spends block1b. Archival keys on pont-nullness.
    BOOST_REQUIRE_EQUAL(store.point_body(),
        system::base16_chunk("ffffffff""730460db96b2968de1fe1fbf5cc5aa229f936943ac400ea0047899af03c89ac9"
                             "ffffffff""cad29b6decf1a6d4d47482e7f2c3b50e0a757ca618f7f9e01d4373ab3437c02d"));
    BOOST_REQUIRE_EQUAL(store.spend_body(),
        system::base16_chunk("ffffffff""ffffffffffffff""00000000""ffffffff""0000000000"
                             "00000000""ffffffffffffff""01000000""b1000000""4f00000000"
                             "ffffffff""00000000000000""02000000""b1000000""5300000000"
                             "ffffffff""01000000000000""03000000""b2000000""5700000000"));
    BOOST_REQUIRE_EQUAL(store.input_body(),
        system::base16_chunk("4d04ffff001d0104455468652054696d65732030332f4a616e2f32303039204368616e63656c6c6f72206f6e206272696e6b206f66207365636f6e64206261696c6f757420666f722062616e6b73""00"
                             "02ae82""00"
                             "02ae82""00"
                             "02ae82""00"));
    BOOST_REQUIRE_EQUAL(store.output_body(),
        system::base16_chunk("00000000""ff00f2052a01000000""434104678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38c4f35504e51ec112de5c384df7ba0b8d578a4c702b6bf11d5fac"
                             "01000000""b1""0179"
                             "01000000""b1""0179"
                             "02000000""b1""0179"
                             "03000000""b2""0179"));

    // to_non_coinbase_spends keys on first-tx-ness, so only one input despite two points (second point).
    BOOST_REQUIRE_EQUAL(query.to_non_coinbase_spends(2), spend_links{ 3 });
}

// to_output_tx/to_output/to_tx_outputs/to_block_outputs

BOOST_AUTO_TEST_CASE(query_translate__to_output_tx__to_output__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(events_handler), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, test::context));
    BOOST_REQUIRE(query.set(test::block2, test::context));
    BOOST_REQUIRE(query.set(test::block3, test::context));
    BOOST_REQUIRE(query.set(test::block1a, test::context));

    // All 5 blocks have one transaction with 1 output.
    BOOST_REQUIRE_EQUAL(query.to_output_tx(0 * 0x51), 0u);
    BOOST_REQUIRE_EQUAL(query.to_output_tx(1 * 0x51), 1u);
    BOOST_REQUIRE_EQUAL(query.to_output_tx(2 * 0x51), 2u);
    BOOST_REQUIRE_EQUAL(query.to_output_tx(3 * 0x51), 3u);
    BOOST_REQUIRE_EQUAL(query.to_output_tx(4 * 0x51), 4u);
    BOOST_REQUIRE_EQUAL(query.to_output_tx(4 * 0x51 + 7u), 4u);

    BOOST_REQUIRE_EQUAL(query.to_output(0, 0), 0u * 0x51u);
    BOOST_REQUIRE_EQUAL(query.to_output(1, 0), 1u * 0x51u);
    BOOST_REQUIRE_EQUAL(query.to_output(2, 0), 2u * 0x51u);
    BOOST_REQUIRE_EQUAL(query.to_output(3, 0), 3u * 0x51u);
    BOOST_REQUIRE_EQUAL(query.to_output(4, 0), 4u * 0x51u);
    BOOST_REQUIRE_EQUAL(query.to_output(4, 1), 4u * 0x51u + 7u);

    const output_links expected_links4{ 4 * 0x51, 4 * 0x51 + 7 };
    BOOST_REQUIRE_EQUAL(query.to_tx_outputs(0), output_links{ 0 * 0x51 });
    BOOST_REQUIRE_EQUAL(query.to_tx_outputs(1), output_links{ 1 * 0x51 });
    BOOST_REQUIRE_EQUAL(query.to_tx_outputs(2), output_links{ 2 * 0x51 });
    BOOST_REQUIRE_EQUAL(query.to_tx_outputs(3), output_links{ 3 * 0x51 });
    BOOST_REQUIRE_EQUAL(query.to_tx_outputs(4), expected_links4);

    // TODO: All blocks have one transaction.
    BOOST_REQUIRE_EQUAL(query.to_block_outputs(0), output_links{ 0 * 0x51 });
    BOOST_REQUIRE_EQUAL(query.to_block_outputs(1), output_links{ 1 * 0x51 });
    BOOST_REQUIRE_EQUAL(query.to_block_outputs(2), output_links{ 2 * 0x51 });
    BOOST_REQUIRE_EQUAL(query.to_block_outputs(3), output_links{ 3 * 0x51 });
    BOOST_REQUIRE_EQUAL(query.to_block_outputs(4), expected_links4);

    // Past end.
    BOOST_REQUIRE_EQUAL(query.to_output_tx(4 * 0x51 + 14), tx_link::terminal);
    BOOST_REQUIRE_EQUAL(query.to_output(5, 0), output_link::terminal);
    BOOST_REQUIRE(query.to_tx_outputs(5).empty());
    BOOST_REQUIRE(query.to_block_outputs(5).empty());

    // Verify expectations.
    const auto output_body = system::base16_chunk
    (
        // 0, 1, 2, 3, 4, 4
        "00000000""ff00f2052a01000000434104678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38c4f35504e51ec112de5c384df7ba0b8d578a4c702b6bf11d5fac"
        "01000000""ff00f2052a0100000043410496b538e853519c726a2c91e61ec11600ae1390813a627c66fb8be7947be63c52da7589379515d4e0a604f8141781e62294721166bf621e73a82cbf2342c858eeac"
        "02000000""ff00f2052a010000004341047211a824f55b505228e4c3d5194c1fcfaa15a456abdf37f9b9d97a4040afc073dee6c89064984f03385237d92167c13e236446b417ab79a0fcae412ae3316b77ac"
        "03000000""ff00f2052a0100000043410494b9d3e76c5b1629ecf97fff95d7a4bbdac87cc26099ada28066c6ff1eb9191223cd897194a08d0c2726c5747f1db49e8cf90e75dc3e3550ae9b30086f3cd5aaac"
        "04000000""180179"
        "04000000""2a017a"
    );
    BOOST_REQUIRE_EQUAL(store.output_body(), output_body);
}

// to_prevout_tx/to_prevout

BOOST_AUTO_TEST_CASE(query_translate__to_prevout_tx__to_prevout__expected)
{
    settings settings{};
    settings.tx_buckets = 5;
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(events_handler), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1a, test::context));
    BOOST_REQUIRE(query.set(test::block2a, test::context));

    // inputs in link order.
    BOOST_REQUIRE_EQUAL(query.to_prevout_tx(0), tx_link::terminal);
    BOOST_REQUIRE_EQUAL(query.to_prevout_tx(1), tx_link::terminal);
    BOOST_REQUIRE_EQUAL(query.to_prevout_tx(2), tx_link::terminal);
    BOOST_REQUIRE_EQUAL(query.to_prevout_tx(3), tx_link::terminal);
    BOOST_REQUIRE_EQUAL(query.to_prevout_tx(4), 1u); // block1a:0 (second serialized tx)
    BOOST_REQUIRE_EQUAL(query.to_prevout_tx(5), 1u); // block1a:0 (second serialized tx)
    BOOST_REQUIRE_EQUAL(query.to_prevout_tx(6), tx_link::terminal);
    BOOST_REQUIRE_EQUAL(query.to_prevout_tx(7), tx_link::terminal);

    BOOST_REQUIRE_EQUAL(query.to_prevout(0), output_link::terminal);
    BOOST_REQUIRE_EQUAL(query.to_prevout(1), output_link::terminal);
    BOOST_REQUIRE_EQUAL(query.to_prevout(2), output_link::terminal);
    BOOST_REQUIRE_EQUAL(query.to_prevout(3), output_link::terminal);
    BOOST_REQUIRE_EQUAL(query.to_prevout(4), 0x51u);      // block1a:0:0 (second serialized tx:0)
    BOOST_REQUIRE_EQUAL(query.to_prevout(5), 0x51u + 7u); // block1a:0:1 (second serialized tx:1)
    BOOST_REQUIRE_EQUAL(query.to_prevout(6), output_link::terminal);
    BOOST_REQUIRE_EQUAL(query.to_prevout(7), output_link::terminal);

    // Past end.
    BOOST_REQUIRE_EQUAL(query.to_prevout_tx(8), tx_link::terminal);
    BOOST_REQUIRE_EQUAL(query.to_prevout(8), output_link::terminal);

    // Verify expectations.
    const auto input_head = system::base16_chunk
    (
        "0000000000"
    );
    const auto input_body = system::base16_chunk
    (
        "4d04ffff001d0104455468652054696d65732030332f4a616e2f32303039204368616e63656c6c6f72206f6e206272696e6b206f66207365636f6e64206261696c6f757420666f722062616e6b73""00"
        "026a79""0103242424"
        "026a7a""0103313131"
        "026a7a""0103424242"
        "02ae79""0103242424"
        "02ae7a""0103313131"
        "02ae79""0103242424"
        "02ae7a""0103313131"
    );
    const auto output_head = system::base16_chunk
    (
        "0000000000"
    );
    const auto output_body = system::base16_chunk
    (
        // parent  value               script
        "00000000""ff00f2052a01000000""434104678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38c4f35504e51ec112de5c384df7ba0b8d578a4c702b6bf11d5fac"
        "01000000""18""0179"
        "01000000""2a""017a"
        "02000000""81""0179"
        "03000000""81""0179"
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
    const auto tx_body = system::base16_chunk
    (
        "ffffffff" // genesis:0 [0]->terminal
        "3ba3edfd7a7b12b27ac72c3e67768f617fc81bc3888a51323a9fb8aa4b1e5e4a"
        "01cc0000cc00000000000001000000010000010000""0000000000"
        "00000000" // block1a:0 [1]->[0]
        "d19c4584d53264e5d0f9d2f852578c4d4382b69abee853bfbd6bc580f84069cf"
        "009f0000b00000180000002a000000030000020000""0900000000"
        "ffffffff" // block2a:0 [2]->terminal
        "c67bfbf8f354bd8f26d7a8b60c20b591dddf8760e02a1fcc3fd7af60b4253e67"
        "006a000076000081000000a2000000020000010000""1f00000000"
        "ffffffff" // block2a:1 [3]->terminal
        "64a86f067651854e2242b6ac9430b6d6806ea2b24dd7edec7b61dd885cf4a40c"
        "006a000076000081000000a2000000020000010000""2c00000000"
    );

    BOOST_REQUIRE_EQUAL(store.input_head(), input_head);
    BOOST_REQUIRE_EQUAL(store.input_body(), input_body);
    BOOST_REQUIRE_EQUAL(store.output_head(), output_head);
    BOOST_REQUIRE_EQUAL(store.output_body(), output_body);
    BOOST_REQUIRE_EQUAL(store.tx_head(), tx_head);
    BOOST_REQUIRE_EQUAL(store.tx_body(), tx_body);
}

// to_block/set_strong/set_unstrong/to_strong/to_strongs

// Duplicate tx hashes not tested (cannot set duplicates with guard in place).
BOOST_AUTO_TEST_CASE(query_translate__to_block__set_strong__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };

    class accessor
      : public test::query_accessor
    {
    public:
        using test::query_accessor::query_accessor;
        header_links to_blocks_(const tx_link& link) const NOEXCEPT
        {
            return to_blocks(link);
        }
        strong_pair to_strong_(const hash_digest& tx_hash) const NOEXCEPT
        {
            return to_strong(tx_hash);
        }
        strong_pairs to_strongs_(const hash_digest& tx_hash) const NOEXCEPT
        {
            return to_strongs(tx_hash);
        }
    };

    accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(events_handler), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, test::context));
    BOOST_REQUIRE(query.set(test::block2, test::context));

    // for to_strong/to_strongs
    const auto hash0 = test::genesis.transaction_hashes(false).front();
    const auto hash1 = test::block1.transaction_hashes(false).front();
    const auto hash2 = test::block2.transaction_hashes(false).front();
    const auto hash3 = test::block3.transaction_hashes(false).front();

    // Either not strong or not found, except genesis.
    BOOST_REQUIRE_EQUAL(query.to_block(0), 0u);
    BOOST_REQUIRE_EQUAL(query.to_block(1), header_link::terminal);
    BOOST_REQUIRE_EQUAL(query.to_block(2), header_link::terminal);
    BOOST_REQUIRE_EQUAL(query.to_block(3), header_link::terminal);
    BOOST_REQUIRE_EQUAL(query.to_strong_(hash0).block, 0u);
    BOOST_REQUIRE_EQUAL(query.to_strong_(hash1).block, header_link::terminal);
    BOOST_REQUIRE_EQUAL(query.to_strong_(hash2).block, header_link::terminal);
    BOOST_REQUIRE_EQUAL(query.to_strong_(hash3).block, header_link::terminal);
    BOOST_REQUIRE_EQUAL(query.to_strongs_(hash0).size(), 1u);
    BOOST_REQUIRE_EQUAL(query.to_strongs_(hash0).at(0).block, 0u);
    BOOST_REQUIRE_EQUAL(query.to_strongs_(hash0).at(0).tx, 0u);
    BOOST_REQUIRE(query.to_strongs_(hash1).empty());
    BOOST_REQUIRE(query.to_strongs_(hash2).empty());
    BOOST_REQUIRE(query.to_strongs_(hash3).empty());

    // push_candidate/push_confirmed has no effect.
    BOOST_REQUIRE(query.push_candidate(query.to_header(test::genesis.hash())));
    BOOST_REQUIRE(query.push_confirmed(query.to_header(test::genesis.hash())));
    BOOST_REQUIRE_EQUAL(query.to_block(0), 0u);
    BOOST_REQUIRE_EQUAL(query.to_block(1), header_link::terminal);
    BOOST_REQUIRE_EQUAL(query.to_block(2), header_link::terminal);
    BOOST_REQUIRE_EQUAL(query.to_block(3), header_link::terminal);
    BOOST_REQUIRE_EQUAL(query.to_strong_(hash0).block, 0u);
    BOOST_REQUIRE_EQUAL(query.to_strong_(hash1).block, header_link::terminal);
    BOOST_REQUIRE_EQUAL(query.to_strong_(hash2).block, header_link::terminal);
    BOOST_REQUIRE_EQUAL(query.to_strong_(hash3).block, header_link::terminal);
    BOOST_REQUIRE_EQUAL(query.to_strongs_(hash0).size(), 1u);
    BOOST_REQUIRE_EQUAL(query.to_strongs_(hash0).at(0).block, 0u);
    BOOST_REQUIRE_EQUAL(query.to_strongs_(hash0).at(0).tx, 0u);
    BOOST_REQUIRE(query.to_strongs_(hash1).empty());
    BOOST_REQUIRE(query.to_strongs_(hash2).empty());
    BOOST_REQUIRE(query.to_strongs_(hash3).empty());

    // set_strong sets strong_by (only), and is idempotent.
    BOOST_REQUIRE(query.set_strong(query.to_header(test::genesis.hash())));
    BOOST_REQUIRE(query.set_strong(query.to_header(test::block1.hash())));
    BOOST_REQUIRE_EQUAL(query.to_block(0), 0u);
    BOOST_REQUIRE_EQUAL(query.to_block(1), 1u);
    BOOST_REQUIRE_EQUAL(query.to_block(2), header_link::terminal);
    BOOST_REQUIRE_EQUAL(query.to_block(3), header_link::terminal);
    BOOST_REQUIRE_EQUAL(query.to_strong_(hash0).block, 0u);
    BOOST_REQUIRE_EQUAL(query.to_strong_(hash1).block, 1u);
    BOOST_REQUIRE_EQUAL(query.to_strong_(hash2).block, header_link::terminal);
    BOOST_REQUIRE_EQUAL(query.to_strong_(hash3).block, header_link::terminal);
    BOOST_REQUIRE_EQUAL(query.to_strongs_(hash0).size(), 1u);
    BOOST_REQUIRE_EQUAL(query.to_strongs_(hash0).at(0).block, 0u);
    BOOST_REQUIRE_EQUAL(query.to_strongs_(hash0).at(0).tx, 0u);
    BOOST_REQUIRE_EQUAL(query.to_strongs_(hash1).size(), 1u);
    BOOST_REQUIRE_EQUAL(query.to_strongs_(hash1).at(0).block, 1u);
    BOOST_REQUIRE_EQUAL(query.to_strongs_(hash1).at(0).tx, 1u);
    BOOST_REQUIRE(query.to_strongs_(hash2).empty());
    BOOST_REQUIRE(query.to_strongs_(hash3).empty());

    // candidate/confirmed unaffected.
    BOOST_REQUIRE(query.is_candidate_header(query.to_header(test::genesis.hash())));
    BOOST_REQUIRE(query.is_confirmed_block(query.to_header(test::genesis.hash())));
    BOOST_REQUIRE(!query.is_candidate_header(query.to_header(test::block1.hash())));
    BOOST_REQUIRE(!query.is_confirmed_block(query.to_header(test::block1.hash())));

    // set_unstrong unsets strong_by, and is idempotent.
    BOOST_REQUIRE(query.set_unstrong(query.to_header(test::genesis.hash())));
    BOOST_REQUIRE(query.set_unstrong(query.to_header(test::block1.hash())));
    BOOST_REQUIRE(query.set_unstrong(query.to_header(test::block2.hash())));
    BOOST_REQUIRE_EQUAL(query.to_block(0), header_link::terminal);
    BOOST_REQUIRE_EQUAL(query.to_block(1), header_link::terminal);
    BOOST_REQUIRE_EQUAL(query.to_block(2), header_link::terminal);
    BOOST_REQUIRE_EQUAL(query.to_block(3), header_link::terminal);
    BOOST_REQUIRE_EQUAL(query.to_strong_(hash0).block, header_link::terminal);
    BOOST_REQUIRE_EQUAL(query.to_strong_(hash1).block, header_link::terminal);
    BOOST_REQUIRE_EQUAL(query.to_strong_(hash2).block, header_link::terminal);
    BOOST_REQUIRE_EQUAL(query.to_strong_(hash3).block, header_link::terminal);
    BOOST_REQUIRE(query.to_strongs_(hash0).empty());
    BOOST_REQUIRE(query.to_strongs_(hash1).empty());
    BOOST_REQUIRE(query.to_strongs_(hash2).empty());
    BOOST_REQUIRE(query.to_strongs_(hash3).empty());
}

// _to_parent

BOOST_AUTO_TEST_CASE(query_translate__to_parent__always__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(events_handler), error::success);
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
    BOOST_REQUIRE_EQUAL(store.create(events_handler), error::success);
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
    settings.point_buckets = 5;
    settings.spend_buckets = 5;
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(events_handler), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1a, test::context));
    BOOST_REQUIRE(query.set(test::block2a, test::context));
    BOOST_REQUIRE(query.set(test::tx4));
    BOOST_REQUIRE(query.to_spenders({ test::genesis.hash(), 0 }).empty());

    const auto point2 = system::base16_array("d19c4584d53264e5d0f9d2f852578c4d4382b69abee853bfbd6bc580f84069cf");
    BOOST_REQUIRE_EQUAL(test::block1a.transactions_ptr()->front()->hash(false), point2);

    // switch to record index
    const auto expected0 = spend_links{ 8, 4 };
    const auto expected1 = spend_links{ 9, 5 };
    const auto spenders0a = query.to_spenders({ point2, 0 });
    const auto spenders1a = query.to_spenders({ point2, 1 });
    BOOST_REQUIRE_EQUAL(spenders0a, expected0);
    BOOST_REQUIRE_EQUAL(spenders1a, expected1);
    BOOST_REQUIRE(query.to_spenders({ test::block2a.transactions_ptr()->front()->hash(false), 0 }).empty());
    BOOST_REQUIRE(query.to_spenders({ test::block2a.transactions_ptr()->back()->hash(false), 0 }).empty());
    BOOST_REQUIRE(query.to_spenders({ test::tx4.hash(false), 0 }).empty());
    BOOST_REQUIRE(query.to_spenders({ test::tx4.hash(false), 1 }).empty()); // n/a, only one output
    BOOST_REQUIRE(query.to_spenders({ system::null_hash, 0xffffffff }).empty());

    BOOST_REQUIRE(query.to_spenders(0x00).empty());
    const auto spenders0b = query.to_spenders(0x51 + 0 * 7);
    const auto spenders1b = query.to_spenders(0x51 + 1 * 7);
    BOOST_REQUIRE_EQUAL(spenders0b, expected0);
    BOOST_REQUIRE_EQUAL(spenders1b, expected1);
    BOOST_REQUIRE(query.to_spenders(0x51 + 2 * 7).empty());
    BOOST_REQUIRE(query.to_spenders(0x51 + 3 * 7).empty());
    BOOST_REQUIRE(query.to_spenders(0x51 + 4 * 7).empty());
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
    const auto point_head = system::base16_chunk
    (
        "00000000" // size
        "ffffffff"
        "00000000"
        "01000000"
        "02000000"
        "ffffffff"
    );
    const auto point_body = system::base16_chunk
    (
        "ffffffff""0100000000000000000000000000000000000000000000000000000000000000" // block1a:tx1(0,1) and block2a:tx2(0,1) invalid spends
        "ffffffff""0200000000000000000000000000000000000000000000000000000000000000" // block1a:tx1(2) invalid spend
        "ffffffff""d19c4584d53264e5d0f9d2f852578c4d4382b69abee853bfbd6bc580f84069cf" // block2a:tx1(0,1) valid spends of block1a:tx1(0,1)
    );
    const auto spend_head = system::base16_chunk
    (
        "00000000" // size
        "00000000"
        "ffffffff"
        "08000000"
        "09000000"
        "01000000"
    );
    const auto spend_body = system::base16_chunk
    (
        "ffffffff""ffffffff""ffffff""00000000""ffffffff""0000000000"
        "03000000""00000000""180000""01000000""2a000000""4f00000000"
        "ffffffff""00000000""2a0000""01000000""18000000""5700000000"
        "ffffffff""01000000""2b0000""01000000""19000000""5f00000000"
        "02000000""02000000""000000""02000000""a2000000""6700000000"
        "ffffffff""02000000""010000""02000000""81000000""6f00000000"
        "04000000""00000000""200000""03000000""a2000000""7700000000"
        "05000000""00000000""210000""03000000""81000000""7f00000000"
        "06000000""02000000""000000""04000000""a5000000""8700000000"
        "07000000""02000000""010000""04000000""85000000""8f00000000"
    );
    const auto input_head = system::base16_chunk
    (
        "0000000000" // size
    );
    const auto input_body = system::base16_chunk
    (
        "4d04ffff001d0104455468652054696d65732030332f4a616e2f32303039204368616e63656c6c6f72206f6e206272696e6b206f66207365636f6e64206261696c6f757420666f722062616e6b73""00"
        "026a7901""03242424" // 1a:0:0
        "026a7a01""03313131" // 1a:0:1
        "026a7a01""03424242" // 1a:0:2
        "02ae7901""03242424" // 2a:0:0
        "02ae7a01""03313131" // 2a:0:1
        "02ae7901""03242424" // 2a:1:0
        "02ae7a01""03313131" // 2a:1:1
        "02ae7901""03252525" //  tx4:0
        "02ae7a01""03353535" //  tx4:1
    );
    const auto output_head = system::base16_chunk
    (
        "0000000000" // size
    );
    const auto output_body = system::base16_chunk
    (
        // parent  value               script
        "00000000""ff00f2052a01000000""434104678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38c4f35504e51ec112de5c384df7ba0b8d578a4c702b6bf11d5fac"
        "01000000""18""0179"
        "01000000""2a""017a"
        "02000000""81""0179"
        "03000000""81""0179"
        "04000000""85""0179"
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
    const auto tx_body = system::base16_chunk
    (
        "ffffffff" // genesis:0 [0]->terminal
        "3ba3edfd7a7b12b27ac72c3e67768f617fc81bc3888a51323a9fb8aa4b1e5e4a"
        "01""cc0000""cc0000""00000000""01000000""010000""010000""0000000000"
        "00000000" // block1a:0 [1]->[0]
        "d19c4584d53264e5d0f9d2f852578c4d4382b69abee853bfbd6bc580f84069cf"
        "00""9f0000""b00000""18000000""2a000000""030000""020000""0900000000"
        "ffffffff" // block2a:0 [2]->terminal
        "c67bfbf8f354bd8f26d7a8b60c20b591dddf8760e02a1fcc3fd7af60b4253e67"
        "00""6a0000""760000""81000000""a2000000""020000""010000""1f00000000"
        "ffffffff" // block2a:1 [3]->terminal
        "64a86f067651854e2242b6ac9430b6d6806ea2b24dd7edec7b61dd885cf4a40c"
        "00""6a0000""760000""81000000""a2000000""020000""010000""2c00000000"
        "ffffffff" // tx4 [4]->terminal
        "abee882062e8df25c967717d0f97e0133af9be84861a427dd4e3f7370549c441"
        "00""6a0000""760000""85000000""a5000000""020000""010000""3900000000"
    );

    BOOST_REQUIRE_EQUAL(store.point_head(), point_head);
    BOOST_REQUIRE_EQUAL(store.point_body(), point_body);
    BOOST_REQUIRE_EQUAL(store.spend_head(), spend_head);
    BOOST_REQUIRE_EQUAL(store.spend_body(), spend_body);
    BOOST_REQUIRE_EQUAL(store.input_head(), input_head);
    BOOST_REQUIRE_EQUAL(store.input_body(), input_body);
    BOOST_REQUIRE_EQUAL(store.output_head(), output_head);
    BOOST_REQUIRE_EQUAL(store.output_body(), output_body);
    BOOST_REQUIRE_EQUAL(store.tx_head(), tx_head);
    BOOST_REQUIRE_EQUAL(store.tx_body(), tx_body);
}

BOOST_AUTO_TEST_SUITE_END()
