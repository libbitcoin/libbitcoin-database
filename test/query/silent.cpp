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
#include "../test.hpp"
#include "../mocks/blocks.hpp"
#include "../mocks/chunk_store.hpp"

BOOST_FIXTURE_TEST_SUITE(query_silent_tests, test::directory_setup_fixture)

using namespace system;

constexpr auto expected_prevouts_summary = base16_array(
    "024ac253c216532e961988e2a8ce266a447c894c781e52ef6cee902361db960004");
constexpr auto expected_block_prevouts_summary = base16_array(
    "0234312c771f033144f850d03442e69047e715bcffb27ceab043f5993d452584f7");
constexpr auto expected_output = base16_array("3e9fce73d4e77a4809908e3c3a2e54ee147b9312dc5044a193d1fc85de46e3c1");
constexpr auto second_output = base16_array("000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f");

static chain::transaction funding_transaction()
{
    const chain::script output_script
    {
        base16_chunk("76a91419c2f3ae0ca3b642bd3e49598b8da89f50c1416188ac"),
        false
    };

    return
    {
        2u,
        { chain::input{} },
        { { 0u, output_script } },
        0u
    };
}

static chain::transaction silent_spend_transaction(
    const hash_digest& prevout_hash)
{
    const chain::script input_script
    {
        base16_chunk("483046022100ad79e6801dd9a8727f342f31c71c4912866f59dc6e7981878e92c5844a0ce929022100fb0d2393e813968648b9753b7e9871d90ab3d815ebf91820d704b19f4ed224d621025a1e61f898173040e20616d43e9f496fba90338a39faa1ed98fcbaeee4dd9be5"),
        false
    };
    const chain::script output_script
    {
        base16_chunk("51203e9fce73d4e77a4809908e3c3a2e54ee147b9312dc5044a193d1fc85de46e3c1"),
        false
    };

    return
    {
        2u,
        {
            {
                { prevout_hash, 0u },
                input_script,
                chain::witness{},
                max_uint32
            }
        },
        { { 0u, output_script } },
        0u
    };
}

static chain::transaction empty_coinbase()
{
    return
    {
        2u,
        { chain::input{} },
        { { 0u, chain::script{} } },
        0u
    };
}

static chain::block make_block(const hash_digest& previous,
    const chain::transactions& transactions, uint32_t nonce)
{
    return
    {
        {
            0x31323334,
            previous,
            hash_digest{ 0x1a },
            0x41424344,
            0x51525354,
            nonce
        },
        transactions
    };
}

BOOST_AUTO_TEST_CASE(query_silent__initialize__active_from_genesis__indexed)
{
    database::settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));

    const auto link = query.to_confirmed(0);
    BOOST_REQUIRE(query.is_silent_indexed(link));

    silent actual{};
    BOOST_REQUIRE(query.get_silent(actual, link));
    BOOST_REQUIRE(actual.records.empty());
}

BOOST_AUTO_TEST_CASE(query_silent__initialize__below_start_height__unindexed)
{
    database::settings settings{};
    settings.path = TEST_DIRECTORY;
    settings.silent_start_height = 1;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));

    const auto link = query.to_confirmed(0);
    BOOST_REQUIRE(!query.is_silent_indexed(link));

    silent actual{};
    BOOST_REQUIRE(!query.get_silent(actual, link));
    BOOST_REQUIRE(query.set_silent(link, test::genesis));
    BOOST_REQUIRE(!query.is_silent_indexed(link));
}

BOOST_AUTO_TEST_CASE(query_silent__set_silent__disabled__unindexed)
{
    database::settings settings{};
    settings.path = TEST_DIRECTORY;
    settings.silent_buckets = 0;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));

    const auto link = query.to_confirmed(0);
    BOOST_REQUIRE(query.set_silent(link, test::genesis));
    BOOST_REQUIRE(!query.is_silent_indexed(link));

    silent actual{};
    BOOST_REQUIRE(!query.get_silent(actual, link));
}

BOOST_AUTO_TEST_CASE(query_silent__set_silent__terminal_link__false)
{
    database::settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));

    silent value{};
    BOOST_REQUIRE(!query.set_silent(header_link::terminal, test::genesis));
    BOOST_REQUIRE(!query.set_silent(header_link::terminal, value));
}

BOOST_AUTO_TEST_CASE(query_silent__set_silent__records__round_trips)
{
    database::settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, context{ 0, 1, 0 }, false, false));

    const auto link = query.to_header(test::block1.hash());
    BOOST_REQUIRE(!link.is_terminal());

    const auto txs = query.to_transactions(link);
    BOOST_REQUIRE_EQUAL(txs.size(), 1u);

    const auto tx = txs.front();
    const silent expected
    {
        {
            {
                tx,
                expected_prevouts_summary,
                { { 1u, expected_output }, { 2u, second_output } }
            }
        }
    };

    BOOST_REQUIRE(!query.is_silent_indexed(link));
    BOOST_REQUIRE(query.set_silent(link, expected));

    silent actual{};
    BOOST_REQUIRE(query.get_silent(actual, link));
    BOOST_REQUIRE_EQUAL(actual.records.size(), 1u);
    BOOST_REQUIRE(actual.records.front().tx == tx);
    BOOST_REQUIRE_EQUAL(actual.records.front().prevouts_summary,
        expected_prevouts_summary);
    BOOST_REQUIRE_EQUAL(actual.records.front().outputs.size(), 2u);
    BOOST_REQUIRE_EQUAL(actual.records.front().outputs[0].index, 1u);
    BOOST_REQUIRE_EQUAL(actual.records.front().outputs[0].key, expected_output);
    BOOST_REQUIRE_EQUAL(actual.records.front().outputs[1].index, 2u);
    BOOST_REQUIRE_EQUAL(actual.records.front().outputs[1].key, second_output);
}

BOOST_AUTO_TEST_CASE(query_silent__set_silent_tx__coinbase__no_record)
{
    database::settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));

    const auto link = query.to_confirmed(0);
    const auto txs = query.to_transactions(link);
    BOOST_REQUIRE_EQUAL(txs.size(), 1u);

    silent actual{};
    const auto& tx = *test::genesis.transactions_ptr()->front();
    BOOST_REQUIRE(query.set_silent(actual, txs.front(), tx));
    BOOST_REQUIRE(actual.records.empty());
}

BOOST_AUTO_TEST_CASE(query_silent__set_silent_tx__terminal_link__false)
{
    database::settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));

    silent actual{};
    BOOST_REQUIRE(!query.set_silent(actual, tx_link::terminal, empty_coinbase()));
    BOOST_REQUIRE(actual.records.empty());
}

BOOST_AUTO_TEST_CASE(query_silent__set_silent_tx__populates_prevout__record)
{
    database::settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));

    const auto funding_tx = funding_transaction();
    const auto funding = make_block(test::genesis.hash(), { funding_tx }, 0x01);
    BOOST_REQUIRE(query.set(funding, context{ 0, 1, 0 }, false, false));

    const chain::transactions spend_txs
    {
        empty_coinbase(),
        silent_spend_transaction(funding_tx.hash(false))
    };
    auto spend = make_block(funding.hash(), spend_txs, 0x02);
    BOOST_REQUIRE(query.set(spend, context{ 0, 2, 0 }, false, false));

    const auto link = query.to_header(spend.hash());
    const auto txs = query.to_transactions(link);
    BOOST_REQUIRE_EQUAL(txs.size(), 2u);
    const auto& spend_tx = *spend.transactions_ptr()->at(1);
    BOOST_REQUIRE(!spend_tx.inputs_ptr()->front()->prevout);

    silent actual{};
    BOOST_REQUIRE(query.set_silent(actual, txs[1], spend_tx));
    BOOST_REQUIRE(spend_tx.inputs_ptr()->front()->prevout);
    BOOST_REQUIRE_EQUAL(actual.records.size(), 1u);
    BOOST_REQUIRE(actual.records.front().tx == txs[1]);
    BOOST_REQUIRE_EQUAL(actual.records.front().prevouts_summary,
        expected_block_prevouts_summary);
    BOOST_REQUIRE_EQUAL(actual.records.front().outputs.size(), 1u);
    BOOST_REQUIRE_EQUAL(actual.records.front().outputs.front().index, 0u);
    BOOST_REQUIRE_EQUAL(actual.records.front().outputs.front().key,
        expected_output);
}

BOOST_AUTO_TEST_CASE(query_silent__set_silent_block__populates_prevouts__indexed)
{
    database::settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));

    const auto funding_tx = funding_transaction();
    const auto funding = make_block(test::genesis.hash(), { funding_tx }, 0x01);
    BOOST_REQUIRE(query.set(funding, context{ 0, 1, 0 }, false, false));

    const chain::transactions spend_txs
    {
        empty_coinbase(),
        silent_spend_transaction(funding_tx.hash(false))
    };
    auto spend = make_block(funding.hash(), spend_txs, 0x02);
    BOOST_REQUIRE(query.set(spend, context{ 0, 2, 0 }, false, false));

    const auto link = query.to_header(spend.hash());
    const auto txs = query.to_transactions(link);
    BOOST_REQUIRE_EQUAL(txs.size(), 2u);
    const auto& spend_tx = *spend.transactions_ptr()->at(1);
    BOOST_REQUIRE(!spend_tx.inputs_ptr()->front()->prevout);

    BOOST_REQUIRE(query.set_silent(link, spend));
    BOOST_REQUIRE(spend_tx.inputs_ptr()->front()->prevout);

    silent actual{};
    BOOST_REQUIRE(query.get_silent(actual, link));
    BOOST_REQUIRE_EQUAL(actual.records.size(), 1u);
    BOOST_REQUIRE(actual.records.front().tx == txs[1]);
    BOOST_REQUIRE_EQUAL(actual.records.front().prevouts_summary,
        expected_block_prevouts_summary);
    BOOST_REQUIRE_EQUAL(actual.records.front().outputs.size(), 1u);
    BOOST_REQUIRE_EQUAL(actual.records.front().outputs.front().index, 0u);
    BOOST_REQUIRE_EQUAL(actual.records.front().outputs.front().key,
        expected_output);
}

BOOST_AUTO_TEST_CASE(query_silent__set_silent_block__missing_prevout__false)
{
    database::settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));

    const chain::transactions spend_txs
    {
        empty_coinbase(),
        silent_spend_transaction(hash_digest{ 0x42 })
    };
    auto spend = make_block(test::genesis.hash(), spend_txs, 0x03);
    BOOST_REQUIRE(query.set(spend, context{ 0, 1, 0 }, false, false));

    const auto link = query.to_header(spend.hash());
    BOOST_REQUIRE(!link.is_terminal());
    BOOST_REQUIRE(!query.set_silent(link, spend));
    BOOST_REQUIRE(!query.is_silent_indexed(link));
}

BOOST_AUTO_TEST_SUITE_END()
