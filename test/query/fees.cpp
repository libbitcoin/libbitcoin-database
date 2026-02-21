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

BOOST_FIXTURE_TEST_SUITE(query_fees_tests, test::directory_setup_fixture)

// nop event handler.
const auto events_handler = [](auto, auto) {};

// get_tx_fee
// get_tx_fees

BOOST_AUTO_TEST_CASE(query_fees__get_tx_fee__invalid__max_uint64)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_CHECK(!store.create(events_handler));
    BOOST_CHECK(query.initialize(test::genesis));
    BOOST_CHECK_EQUAL(query.get_tx_fee(42), max_uint64);

    fee_rate rate{};
    BOOST_CHECK(!query.get_tx_fees(rate, 42));
}

BOOST_AUTO_TEST_CASE(query_fees__get_tx_fee__missing_prevouts__max_uint64)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_CHECK(!store.create(events_handler));
    BOOST_CHECK(query.initialize(test::genesis));
    BOOST_CHECK_EQUAL(query.get_tx_fee(0), 0u);
    BOOST_CHECK(query.set(test::block1a, test::context, false, false));
    BOOST_CHECK(query.set(test::block2a, test::context, false, false));
    BOOST_CHECK_EQUAL(query.get_tx_fee(3), max_uint64);

    fee_rate rate{};
    BOOST_CHECK(!query.get_tx_fees(rate, 3));
}

BOOST_AUTO_TEST_CASE(query_fees__get_tx_fee__genesis__zero)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_CHECK(!store.create(events_handler));
    BOOST_CHECK(query.initialize(test::genesis));
    BOOST_CHECK_EQUAL(query.get_tx_fee(0), 0u);

    fee_rate rate{};
    BOOST_CHECK(!query.get_tx_fees(rate, 0u));
}

BOOST_AUTO_TEST_CASE(query_fees__get_tx_fee__valid_non_coinbase__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_CHECK(!store.create(events_handler));
    BOOST_CHECK(query.initialize(test::genesis));
    BOOST_CHECK(query.set(test::block1b, test::context, false, false));
    BOOST_CHECK(query.set(test::tx2b));
    BOOST_CHECK_EQUAL(query.get_tx_fee(2), 0u);

    fee_rate rate{};
    BOOST_CHECK(query.get_tx_fees(rate, 2));
    BOOST_CHECK_EQUAL(rate.bytes, test::tx2b.virtual_size());
    BOOST_CHECK_EQUAL(rate.fee, test::tx2b.fee());
}

// get_block_fee
// get_block_fees

BOOST_AUTO_TEST_CASE(query_fees__get_block_fee__invalid__max_uint64)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_CHECK(!store.create(events_handler));
    BOOST_CHECK(query.initialize(test::genesis));
    BOOST_CHECK_EQUAL(query.get_block_fee(24), max_uint64);

    fee_rates rates{};
    BOOST_CHECK(!query.get_block_fees(rates, 24));
}

BOOST_AUTO_TEST_CASE(query_fees__get_block_fee__block_missing_prevout__max_uint64)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_CHECK(!store.create(events_handler));
    BOOST_CHECK(query.initialize(test::genesis));
    BOOST_CHECK(query.set(test::block1b, test::context, false, false));
    BOOST_CHECK(query.set(test::block_missing_prevout_2b, test::context, false, false));
    BOOST_CHECK_EQUAL(query.get_block_fee(2), max_uint64);

    fee_rates rates{};
    BOOST_CHECK(!query.get_block_fees(rates, 2));
}

BOOST_AUTO_TEST_CASE(query_fees__get_block_fee__coinbases__zero)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_CHECK(!store.create(events_handler));
    BOOST_CHECK(query.initialize(test::genesis));
    BOOST_CHECK(query.set(test::block1, test::context, false, false));
    BOOST_CHECK(query.set(test::block2, test::context, false, false));
    BOOST_CHECK_EQUAL(query.get_block_fee(2), zero);

    fee_rates rates{};
    BOOST_CHECK(query.get_block_fees(rates, 2));
    BOOST_CHECK(rates.empty());
}

BOOST_AUTO_TEST_CASE(query_fees__get_block_fee__valid__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_CHECK(!store.create(events_handler));
    BOOST_CHECK(query.initialize(test::genesis));
    BOOST_CHECK(query.set(test::block1b, test::context, false, false));
    BOOST_CHECK(query.set(test::block_valid_spend_internal_2b, test::context, false, false));
    BOOST_CHECK_EQUAL(query.get_block_fee(2), 0xb1u);

    // 3 txs - 1 coinbase = 2 rates.
    fee_rates rates{};
    BOOST_CHECK(query.get_block_fees(rates, 2));
    BOOST_CHECK_EQUAL(rates.size(), 2u);
    BOOST_CHECK_EQUAL(rates.at(0).bytes, 63u);
    BOOST_CHECK_EQUAL(rates.at(0).fee, 0x01u);
    BOOST_CHECK_EQUAL(rates.at(1).bytes, 107u);
    BOOST_CHECK_EQUAL(rates.at(1).fee, 0xb0u);
}

BOOST_AUTO_TEST_CASE(query_fees__get_block_fee__genesis__zero)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_CHECK(!store.create(events_handler));
    BOOST_CHECK(query.initialize(test::genesis));
    BOOST_CHECK_EQUAL(query.get_block_fee(0), 0u);

    fee_rates rates{};
    BOOST_CHECK(query.get_block_fees(rates, 0));
    BOOST_CHECK(rates.empty());
}

// get_branch_fees

BOOST_AUTO_TEST_CASE(query_fees__get_branch_fees__zero__true_empty)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_CHECK(!store.create(events_handler));
    BOOST_CHECK(query.initialize(test::genesis));

    std::atomic_bool cancel{};
    fee_rate_sets rates_sets{};
    BOOST_CHECK(query.get_branch_fees(cancel, rates_sets, 0, 0));
    BOOST_CHECK(rates_sets.empty());
}

BOOST_AUTO_TEST_CASE(query_fees__get_branch_fees__genesis__true_one_empty)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_CHECK(!store.create(events_handler));
    BOOST_CHECK(query.initialize(test::genesis));

    std::atomic_bool cancel{};
    fee_rate_sets rates_sets{};
    BOOST_CHECK(query.get_branch_fees(cancel, rates_sets, 0, 1));
    BOOST_CHECK_EQUAL(rates_sets.size(), 1u);
    BOOST_CHECK(rates_sets.front().empty());
}

BOOST_AUTO_TEST_CASE(query_fees__get_branch_fees__unconfirmed_blocks__false)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_CHECK(!store.create(events_handler));
    BOOST_CHECK(query.initialize(test::genesis));
    BOOST_CHECK(query.set(test::block1, test::context, false, false));
    BOOST_CHECK(query.set(test::block2, test::context, false, false));

    std::atomic_bool cancel{};
    fee_rate_sets rates_sets{};
    BOOST_CHECK(!query.get_branch_fees(cancel, rates_sets, 0, 3));
}

BOOST_AUTO_TEST_CASE(query_fees__get_branch_fees__confirmed_overflow__false)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_CHECK(!store.create(events_handler));
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

BOOST_AUTO_TEST_CASE(query_fees__get_branch_fees__zero_over_top__true)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_CHECK(!store.create(events_handler));
    BOOST_CHECK(query.initialize(test::genesis));

    // A start of 1 is over the chain top, but requested count is zero (ok).
    std::atomic_bool cancel{};
    fee_rate_sets rates_sets{};
    BOOST_CHECK(query.get_branch_fees(cancel, rates_sets, 1, 0));
}

BOOST_AUTO_TEST_CASE(query_fees__get_branch_fees__confirmed_empty_blocks__all_empty)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_CHECK(!store.create(events_handler));
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

BOOST_AUTO_TEST_CASE(query_fees__get_branch_fees__confirmed_non_empty_blocks__all_expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_CHECK(!store.create(events_handler));
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


BOOST_AUTO_TEST_CASE(query_fees__get_branch_fees__cancel_zero__true_empty)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_CHECK(!store.create(events_handler));
    BOOST_CHECK(query.initialize(test::genesis));

    std::atomic_bool cancel{ true };
    fee_rate_sets rates_sets{};
    BOOST_CHECK(query.get_branch_fees(cancel, rates_sets, 0, 0));
    BOOST_CHECK(rates_sets.empty());
}

BOOST_AUTO_TEST_CASE(query_fees__get_branch_fees__cancel_genesis__false_empty)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_CHECK(!store.create(events_handler));
    BOOST_CHECK(query.initialize(test::genesis));

    std::atomic_bool cancel{ true  };
    fee_rate_sets rates_sets{};
    BOOST_CHECK(!query.get_branch_fees(cancel, rates_sets, 0, 1));
    BOOST_CHECK(rates_sets.empty());
}

BOOST_AUTO_TEST_CASE(query_fees__get_branch_fees__cancel_three_blocks__false_empty)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_CHECK(!store.create(events_handler));
    BOOST_CHECK(query.initialize(test::genesis));
    BOOST_CHECK(query.set(test::block1, test::context, false, false));
    BOOST_CHECK(query.set(test::block2, test::context, false, false));

    std::atomic_bool cancel{ true };
    fee_rate_sets rates_sets{};
    BOOST_CHECK(!query.get_branch_fees(cancel, rates_sets, 0, 3));
    BOOST_CHECK(rates_sets.empty());
}

BOOST_AUTO_TEST_SUITE_END()
