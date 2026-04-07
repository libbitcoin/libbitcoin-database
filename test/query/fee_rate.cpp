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

// TODO:
// get_tx_virtual_size
// get_block_virtual_size

BOOST_FIXTURE_TEST_SUITE(query_fee_rate_tests, test::directory_setup_fixture)

// get_tx_fee
// get_tx_fees
// get_tx_value
// get_tx_spend
// get_tx_virtual_size

BOOST_AUTO_TEST_CASE(query_fee_rate__get_tx_fee__invalid__false)
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

BOOST_AUTO_TEST_CASE(query_fee_rate__get_tx_fee__missing_prevouts__false)
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
    BOOST_CHECK(query.set(test::block1a, test::context, false, false));
    BOOST_CHECK(query.set(test::block2a, test::context, false, false));

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

BOOST_AUTO_TEST_CASE(query_fee_rate__get_tx_fee__coinbase__zero)
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

BOOST_AUTO_TEST_CASE(query_fee_rate__get_tx_fee__valid_non_coinbase__expected)
{
    uint64_t out{};
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_CHECK(!store.create(test::events_handler));
    BOOST_CHECK(query.initialize(test::genesis));
    BOOST_CHECK(query.set(test::block1b, test::context, false, false));
    BOOST_CHECK(query.set(test::block_valid_spend_internal_2b, test::context, false, false));
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
}

// get_block_fee
// get_block_fees
// get_block_value
// get_block_spend
// get_block_virtual_size

BOOST_AUTO_TEST_CASE(query_fee_rate__get_block_fee__invalid__false)
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

BOOST_AUTO_TEST_CASE(query_fee_rate__get_block_fee__block_missing_prevout__false)
{
    uint64_t out{};
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_CHECK(!store.create(test::events_handler));
    BOOST_CHECK(query.initialize(test::genesis));
    BOOST_CHECK(query.set(test::block1b, test::context, false, false));
    BOOST_CHECK(query.set(test::block_missing_prevout_2b, test::context, false, false));
    BOOST_CHECK(!query.get_block_fee(out, 2));

    fee_rates rates{};
    BOOST_CHECK(!query.get_block_fees(rates, 2));
}

BOOST_AUTO_TEST_CASE(query_fee_rate__get_block_fee__coinbases__zero)
{
    uint64_t out{};
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_CHECK(!store.create(test::events_handler));
    BOOST_CHECK(query.initialize(test::genesis));
    BOOST_CHECK(query.set(test::block1, test::context, false, false));
    BOOST_CHECK(query.set(test::block2, test::context, false, false));
    BOOST_CHECK(query.get_block_fee(out, 2));
    BOOST_CHECK_EQUAL(out, 0u);

    fee_rates rates{};
    BOOST_CHECK(query.get_block_fees(rates, 2));
    BOOST_CHECK(rates.empty());
}

BOOST_AUTO_TEST_CASE(query_fee_rate__get_block_fee__valid__expected)
{
    uint64_t out{};
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_CHECK(!store.create(test::events_handler));
    BOOST_CHECK(query.initialize(test::genesis));
    BOOST_CHECK(query.set(test::block1b, test::context, false, false));
    BOOST_CHECK(query.set(test::block_valid_spend_internal_2b, test::context, false, false));
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

BOOST_AUTO_TEST_CASE(query_fee_rate__get_block_fee__genesis__zero)
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

// get_branch_fees

BOOST_AUTO_TEST_CASE(query_fee_rate__get_branch_fees__zero__true_empty)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_CHECK(!store.create(test::events_handler));
    BOOST_CHECK(query.initialize(test::genesis));

    std::atomic_bool cancel{};
    fee_rate_sets rates_sets{};
    BOOST_CHECK(query.get_branch_fees(cancel, rates_sets, 0, 0));
    BOOST_CHECK(rates_sets.empty());
}

BOOST_AUTO_TEST_CASE(query_fee_rate__get_branch_fees__genesis__true_one_empty)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_CHECK(!store.create(test::events_handler));
    BOOST_CHECK(query.initialize(test::genesis));

    std::atomic_bool cancel{};
    fee_rate_sets rates_sets{};
    BOOST_CHECK(query.get_branch_fees(cancel, rates_sets, 0, 1));
    BOOST_CHECK_EQUAL(rates_sets.size(), 1u);
    BOOST_CHECK(rates_sets.front().empty());
}

BOOST_AUTO_TEST_CASE(query_fee_rate__get_branch_fees__unconfirmed_blocks__false)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_CHECK(!store.create(test::events_handler));
    BOOST_CHECK(query.initialize(test::genesis));
    BOOST_CHECK(query.set(test::block1, test::context, false, false));
    BOOST_CHECK(query.set(test::block2, test::context, false, false));

    std::atomic_bool cancel{};
    fee_rate_sets rates_sets{};
    BOOST_CHECK(!query.get_branch_fees(cancel, rates_sets, 0, 3));
}

BOOST_AUTO_TEST_CASE(query_fee_rate__get_branch_fees__confirmed_overflow__false)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_CHECK(!store.create(test::events_handler));
    BOOST_CHECK(query.initialize(test::genesis));
    BOOST_CHECK(query.set(test::block1, test::context, false, false));
    BOOST_CHECK(query.set(test::block2, test::context, false, false));
    BOOST_CHECK(query.push_confirmed(1, true));
    BOOST_CHECK(query.push_confirmed(2, true));

    std::atomic_bool cancel{};
    fee_rate_sets rates_sets{};
    BOOST_CHECK(!query.get_branch_fees(cancel, rates_sets, 0, 4));
    BOOST_CHECK(!query.get_branch_fees(cancel, rates_sets, 1, 3));
    BOOST_CHECK(!query.get_branch_fees(cancel, rates_sets, 2, 2));
    BOOST_CHECK(!query.get_branch_fees(cancel, rates_sets, 3, 1));
}

BOOST_AUTO_TEST_CASE(query_fee_rate__get_branch_fees__zero_over_top__true)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_CHECK(!store.create(test::events_handler));
    BOOST_CHECK(query.initialize(test::genesis));

    // A start of 1 is over the chain top, but requested count is zero (ok).
    std::atomic_bool cancel{};
    fee_rate_sets rates_sets{};
    BOOST_CHECK(query.get_branch_fees(cancel, rates_sets, 1, 0));
}

BOOST_AUTO_TEST_CASE(query_fee_rate__get_branch_fees__confirmed_empty_blocks__all_empty)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_CHECK(!store.create(test::events_handler));
    BOOST_CHECK(query.initialize(test::genesis));
    BOOST_CHECK(query.set(test::block1, test::context, false, false));
    BOOST_CHECK(query.set(test::block2, test::context, false, false));
    BOOST_CHECK(query.push_confirmed(1, true));
    BOOST_CHECK(query.push_confirmed(2, true));

    std::atomic_bool cancel{};
    fee_rate_sets rates_sets{};
    BOOST_CHECK(query.get_branch_fees(cancel, rates_sets, 0, 3));
    BOOST_CHECK_EQUAL(rates_sets.size(), 3u);
    BOOST_CHECK(rates_sets.at(0).empty());
    BOOST_CHECK(rates_sets.at(1).empty());
    BOOST_CHECK(rates_sets.at(2).empty());

    rates_sets.clear();
    BOOST_CHECK(query.get_branch_fees(cancel, rates_sets, 1, 2));
    BOOST_CHECK_EQUAL(rates_sets.size(), 2u);
    BOOST_CHECK(rates_sets.at(0).empty());
    BOOST_CHECK(rates_sets.at(1).empty());

    rates_sets.clear();
    BOOST_CHECK(query.get_branch_fees(cancel, rates_sets, 2, 1));
    BOOST_CHECK_EQUAL(rates_sets.size(), 1u);
    BOOST_CHECK(rates_sets.at(0).empty());

    rates_sets.clear();
    BOOST_CHECK(query.get_branch_fees(cancel, rates_sets, 3, 0));
    BOOST_CHECK(rates_sets.empty());
}

BOOST_AUTO_TEST_CASE(query_fee_rate__get_branch_fees__confirmed_non_empty_blocks__all_expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_CHECK(!store.create(test::events_handler));
    BOOST_CHECK(query.initialize(test::genesis));
    BOOST_CHECK(query.set(test::block1b, test::context, false, false));
    BOOST_CHECK(query.set(test::block_valid_spend_internal_2b, test::context, false, false));
    BOOST_CHECK(query.push_confirmed(1, true));
    BOOST_CHECK(query.push_confirmed(2, true));

    std::atomic_bool cancel{};
    fee_rate_sets rates_sets{};
    BOOST_CHECK(query.get_branch_fees(cancel, rates_sets, 0, 3));
    BOOST_CHECK_EQUAL(rates_sets.size(), 3u);
    BOOST_CHECK(rates_sets.at(0).empty());
    BOOST_CHECK(rates_sets.at(1).empty());
    BOOST_CHECK_EQUAL(rates_sets.at(2).size(), 2u);
    BOOST_CHECK_EQUAL(rates_sets.at(2).at(0).bytes, 63u);
    BOOST_CHECK_EQUAL(rates_sets.at(2).at(0).fee, 0x01u);
    BOOST_CHECK_EQUAL(rates_sets.at(2).at(1).bytes, 107u);
    BOOST_CHECK_EQUAL(rates_sets.at(2).at(1).fee, 0xb0u);

    rates_sets.clear();
    BOOST_CHECK(query.get_branch_fees(cancel, rates_sets, 1, 2));
    BOOST_CHECK_EQUAL(rates_sets.size(), 2u);
    BOOST_CHECK(rates_sets.at(0).empty());
    BOOST_CHECK_EQUAL(rates_sets.at(1).size(), 2u);
    BOOST_CHECK_EQUAL(rates_sets.at(1).at(0).bytes, 63u);
    BOOST_CHECK_EQUAL(rates_sets.at(1).at(0).fee, 0x01u);
    BOOST_CHECK_EQUAL(rates_sets.at(1).at(1).bytes, 107u);
    BOOST_CHECK_EQUAL(rates_sets.at(1).at(1).fee, 0xb0u);

    rates_sets.clear();
    BOOST_CHECK(query.get_branch_fees(cancel, rates_sets, 2, 1));
    BOOST_CHECK_EQUAL(rates_sets.size(), 1u);
    BOOST_CHECK_EQUAL(rates_sets.at(0).size(), 2u);
    BOOST_CHECK_EQUAL(rates_sets.at(0).at(0).bytes, 63u);
    BOOST_CHECK_EQUAL(rates_sets.at(0).at(0).fee, 0x01u);
    BOOST_CHECK_EQUAL(rates_sets.at(0).at(1).bytes, 107u);
    BOOST_CHECK_EQUAL(rates_sets.at(0).at(1).fee, 0xb0u);

    rates_sets.clear();
    BOOST_CHECK(query.get_branch_fees(cancel, rates_sets, 3, 0));
    BOOST_CHECK(rates_sets.empty());
}


BOOST_AUTO_TEST_CASE(query_fee_rate__get_branch_fees__cancel_zero__true_empty)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_CHECK(!store.create(test::events_handler));
    BOOST_CHECK(query.initialize(test::genesis));

    std::atomic_bool cancel{ true };
    fee_rate_sets rates_sets{};
    BOOST_CHECK(query.get_branch_fees(cancel, rates_sets, 0, 0));
    BOOST_CHECK(rates_sets.empty());
}

BOOST_AUTO_TEST_CASE(query_fee_rate__get_branch_fees__cancel_genesis__false_empty)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_CHECK(!store.create(test::events_handler));
    BOOST_CHECK(query.initialize(test::genesis));

    std::atomic_bool cancel{ true  };
    fee_rate_sets rates_sets{};
    BOOST_CHECK(!query.get_branch_fees(cancel, rates_sets, 0, 1));
    BOOST_CHECK(rates_sets.empty());
}

BOOST_AUTO_TEST_CASE(query_fee_rate__get_branch_fees__cancel_three_blocks__false_empty)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_CHECK(!store.create(test::events_handler));
    BOOST_CHECK(query.initialize(test::genesis));
    BOOST_CHECK(query.set(test::block1, test::context, false, false));
    BOOST_CHECK(query.set(test::block2, test::context, false, false));

    std::atomic_bool cancel{ true };
    fee_rate_sets rates_sets{};
    BOOST_CHECK(!query.get_branch_fees(cancel, rates_sets, 0, 3));
    BOOST_CHECK(rates_sets.empty());
}

BOOST_AUTO_TEST_SUITE_END()
