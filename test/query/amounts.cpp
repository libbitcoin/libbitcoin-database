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
#include "../test.hpp"
#include "../mocks/blocks.hpp"
#include "../mocks/chunk_store.hpp"

BOOST_FIXTURE_TEST_SUITE(query_amounts_tests, test::directory_setup_fixture)

// get_value

BOOST_AUTO_TEST_CASE(query_amounts__get_value__genesis__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_CHECK(!store.create(test::events_handler));
    BOOST_CHECK(query.initialize(test::genesis));

    uint64_t value{};
    BOOST_CHECK(query.get_value(value, query.to_output(0, 0)));
    BOOST_CHECK_EQUAL(value, 5000000000u);
}

// get_tx_fee

BOOST_AUTO_TEST_CASE(query_amounts__get_tx_fee__invalid__false)
{
    uint64_t out{};
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_CHECK(!store.create(test::events_handler));
    BOOST_CHECK(query.initialize(test::genesis));
    BOOST_CHECK(!query.get_tx_fee(out, 42));
    BOOST_CHECK(!query.get_tx_value(out, 42));
    BOOST_CHECK(!query.get_tx_spend(out, 42));

    fee_rate rate{};
    BOOST_CHECK(!query.get_tx_fees(rate, 42));
}

BOOST_AUTO_TEST_CASE(query_amounts__get_tx_fee__missing_prevouts__false)
{
    uint64_t out{};
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_CHECK(!store.create(test::events_handler));
    BOOST_CHECK(query.initialize(test::genesis));
    BOOST_CHECK(query.get_tx_fee(out, 0));
    BOOST_CHECK_EQUAL(out, 0u);
    BOOST_CHECK(query.set(test::block1a, test::context, {}, false, false));
    BOOST_CHECK(query.set(test::block2a, test::context, {}, false, false));

    // Missing prevout fails value (and therefore also fee) but spend is valid.
    BOOST_CHECK(!query.get_tx_fee(out, 3));
    BOOST_CHECK(!query.get_tx_value(out, 3));

    // output is 50btc but not spendable.
    BOOST_CHECK(query.get_tx_spend(out, 0));
    BOOST_CHECK_EQUAL(out, 5'000'000'000u);
    BOOST_CHECK(query.get_tx_spend(out, 1));
    BOOST_CHECK_EQUAL(out, 0x18 + 0x2a);
    BOOST_CHECK(query.get_tx_spend(out, 2));
    BOOST_CHECK_EQUAL(out, 0x81);
    BOOST_CHECK(query.get_tx_spend(out, 3));
    BOOST_CHECK_EQUAL(out, 0x81);
    BOOST_CHECK(!query.get_tx_spend(out, 4));

    fee_rate rate{};
    BOOST_CHECK(!query.get_tx_fees(rate, 3));
}

BOOST_AUTO_TEST_CASE(query_amounts__get_tx_fee__pooled_missing_prevouts__pooled_fee)
{
    uint64_t out{};
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_CHECK(!store.create(test::events_handler));
    BOOST_CHECK(query.initialize(test::genesis));
    BOOST_CHECK(query.set(test::block1a, test::context, {}, false, false));
    BOOST_CHECK(query.set(test::block2a, test::context, {}, false, false));

    // Missing prevout fails value, but a pooled fee is returned directly.
    constexpr uint64_t expected_fee = 42;
    constexpr size_t expected_height = 7;
    BOOST_CHECK(store.pool.put(tx_link{ 3 }, table::pool::record{ {}, { 0, expected_height, 0 }, expected_fee, {}, {} }));
    BOOST_CHECK(!query.get_tx_value(out, 3));
    BOOST_CHECK(query.get_tx_fee(out, 3));
    BOOST_CHECK_EQUAL(out, expected_fee);

    size_t bytes{};
    fee_rate rate{};
    BOOST_CHECK(query.get_tx_virtual_size(bytes, 3));
    BOOST_CHECK(query.get_tx_fees(rate, 3));
    BOOST_CHECK_EQUAL(rate.fee, expected_fee);
    BOOST_CHECK_EQUAL(rate.bytes, bytes);
    BOOST_CHECK_EQUAL(rate.height, expected_height);
}

BOOST_AUTO_TEST_CASE(query_amounts__get_tx_fee__coinbase__zero)
{
    uint64_t out{};
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_CHECK(!store.create(test::events_handler));
    BOOST_CHECK(query.initialize(test::genesis));

    // Coinbase fee and value are always zero.
    BOOST_CHECK(query.get_tx_fee(out, 0));
    BOOST_CHECK_EQUAL(out, 0u);
    BOOST_CHECK(query.get_tx_value(out, 0));
    BOOST_CHECK_EQUAL(out, 0u);
    BOOST_CHECK(query.get_tx_spend(out, 0));
    BOOST_CHECK_EQUAL(out, 5'000'000'000u);

    fee_rate rate{};
    BOOST_CHECK(!query.get_tx_fees(rate, 0));
}

BOOST_AUTO_TEST_CASE(query_amounts__get_tx_fee__valid_non_coinbase__expected)
{
    uint64_t out{};
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_CHECK(!store.create(test::events_handler));
    BOOST_CHECK(query.initialize(test::genesis));
    BOOST_CHECK(query.set(test::block1b, test::context, {}, false, false));
    BOOST_CHECK(query.set(test::block_valid_spend_internal_2b, test::context, {}, false, false));
    BOOST_CHECK(query.get_tx_fee(out, 4));
    BOOST_CHECK_EQUAL(out, (0xb1u + 0xb1u) - 0xb2u);
    BOOST_CHECK(query.get_tx_value(out, 4));
    BOOST_CHECK_EQUAL(out, 0xb1u + 0xb1u);
    BOOST_CHECK(query.get_tx_spend(out, 4));
    BOOST_CHECK_EQUAL(out, 0xb2u);

    size_t virtual_size{};
    BOOST_CHECK(query.get_tx_virtual_size(virtual_size, 2));

    fee_rate rate{};
    BOOST_CHECK(query.get_tx_fees(rate, 2));
    BOOST_CHECK_EQUAL(rate.bytes, virtual_size);
    BOOST_CHECK_EQUAL(rate.bytes, test::tx2b.virtual_size());
    BOOST_CHECK_EQUAL(rate.fee, test::tx2b.fee());
    BOOST_CHECK_EQUAL(rate.height, max_size_t);
}

// get_block_fee

BOOST_AUTO_TEST_CASE(query_amounts__get_block_fee__invalid__false)
{
    uint64_t out{};
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_CHECK(!store.create(test::events_handler));
    BOOST_CHECK(query.initialize(test::genesis));
    BOOST_CHECK(!query.get_block_fee(out, 24));

    fee_rates rates{};
    BOOST_CHECK(!query.get_block_fees(rates, 24));
}

BOOST_AUTO_TEST_CASE(query_amounts__get_block_fee__block_missing_prevout__false)
{
    uint64_t out{};
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_CHECK(!store.create(test::events_handler));
    BOOST_CHECK(query.initialize(test::genesis));
    BOOST_CHECK(query.set(test::block1b, test::context, {}, false, false));
    BOOST_CHECK(query.set(test::block_missing_prevout_2b, test::context, {}, false, false));
    BOOST_CHECK(!query.get_block_fee(out, 2));

    fee_rates rates{};
    BOOST_CHECK(!query.get_block_fees(rates, 2));
}

BOOST_AUTO_TEST_CASE(query_amounts__get_block_fee__coinbases__zero)
{
    uint64_t out{};
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_CHECK(!store.create(test::events_handler));
    BOOST_CHECK(query.initialize(test::genesis));
    BOOST_CHECK(query.set(test::block1, test::context, {}, false, false));
    BOOST_CHECK(query.set(test::block2, test::context, {}, false, false));
    BOOST_CHECK(query.get_block_fee(out, 2));
    BOOST_CHECK_EQUAL(out, 0u);

    fee_rates rates{};
    BOOST_CHECK(query.get_block_fees(rates, 2));
    BOOST_CHECK(rates.empty());
}

BOOST_AUTO_TEST_CASE(query_amounts__get_block_fee__valid__expected)
{
    uint64_t out{};
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_CHECK(!store.create(test::events_handler));
    BOOST_CHECK(query.initialize(test::genesis));
    BOOST_CHECK(query.set(test::block1b, test::context, {}, false, false));
    BOOST_CHECK(query.set(test::block_valid_spend_internal_2b, test::context, {}, false, false));
    BOOST_CHECK(query.get_block_fee(out, 2));
    BOOST_CHECK_EQUAL(out, 0xb1u);

    size_t virtual_size{};
    BOOST_CHECK(query.get_block_virtual_size(virtual_size, 2));
    BOOST_CHECK_EQUAL(virtual_size, test::block_valid_spend_internal_2b.virtual_size());

    // 3 txs - 1 coinbase = 2 rates.
    fee_rates rates{};
    BOOST_CHECK(query.get_block_fees(rates, 2));
    BOOST_CHECK_EQUAL(rates.size(), 2u);
    BOOST_CHECK_EQUAL(rates.at(0).bytes, 63u);
    BOOST_CHECK_EQUAL(rates.at(0).fee, 0x01u);
    BOOST_CHECK_EQUAL(rates.at(1).bytes, 107u);
    BOOST_CHECK_EQUAL(rates.at(1).fee, 0xb0u);
}

BOOST_AUTO_TEST_CASE(query_amounts__get_block_fee__genesis__zero)
{
    uint64_t out{};
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_CHECK(!store.create(test::events_handler));
    BOOST_CHECK(query.initialize(test::genesis));
    BOOST_CHECK(query.get_block_fee(out, 0));
    BOOST_CHECK_EQUAL(out, 0u);

    fee_rates rates{};
    BOOST_CHECK(query.get_block_fees(rates, 0));
    BOOST_CHECK(rates.empty());
}

BOOST_AUTO_TEST_SUITE_END()
