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

struct query_confirm_setup_fixture
{
    DELETE_COPY_MOVE(query_confirm_setup_fixture);
    BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

    query_confirm_setup_fixture() NOEXCEPT
    {
        BOOST_REQUIRE(test::clear(test::directory));
    }

    ~query_confirm_setup_fixture() NOEXCEPT
    {
        BOOST_REQUIRE(test::clear(test::directory));
    }

    BC_POP_WARNING()
};

BOOST_FIXTURE_TEST_SUITE(query_confirm_tests, query_confirm_setup_fixture)

// nop event handler.
const auto events_handler = [](auto, auto) {};

// This asserts.
////BOOST_AUTO_TEST_CASE(query_confirm__pop__zero__false)
////{
////    settings settings{};
////    settings.path = TEST_DIRECTORY;
////    test::chunk_store store{ settings };
////    test::query_accessor query{ store };
////    BOOST_REQUIRE_EQUAL(store.create(events_handler), error::success);
////    BOOST_REQUIRE(!query.pop_candidate());
////    BOOST_REQUIRE(!query.pop_confirmed());
////}

BOOST_AUTO_TEST_CASE(query_confirm__is_candidate_block__push_pop_candidate__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(events_handler), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, context{ 0, 1, 0 }, false, false));
    BOOST_REQUIRE(query.set(test::block2, context{ 0, 2, 0 }, false, false));
    BOOST_REQUIRE(query.is_candidate_header(0));
    BOOST_REQUIRE(!query.is_candidate_header(1));
    BOOST_REQUIRE(!query.is_candidate_header(2));

    BOOST_REQUIRE(query.push_candidate(1));
    BOOST_REQUIRE(query.is_candidate_header(1));

    BOOST_REQUIRE(query.push_candidate(2));
    BOOST_REQUIRE(query.is_candidate_header(2));

    BOOST_REQUIRE(query.pop_candidate());
    BOOST_REQUIRE(query.is_candidate_header(0));
    BOOST_REQUIRE(query.is_candidate_header(1));
    BOOST_REQUIRE(!query.is_candidate_header(2));

    BOOST_REQUIRE(query.pop_candidate());
    BOOST_REQUIRE(query.is_candidate_header(0));
    BOOST_REQUIRE(!query.is_candidate_header(1));
    BOOST_REQUIRE(!query.is_candidate_header(2));

    // Terminal returns false.
    BOOST_REQUIRE(!query.is_candidate_header(database::header_link::terminal));
}

BOOST_AUTO_TEST_CASE(query_confirm__is_confirmed_block__push_pop_confirmed__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(events_handler), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, context{ 0, 1, 0 }, false, false));
    BOOST_REQUIRE(query.set(test::block2, context{ 0, 2, 0 }, false, false));
    BOOST_REQUIRE(query.is_confirmed_block(0));
    BOOST_REQUIRE(!query.is_confirmed_block(1));
    BOOST_REQUIRE(!query.is_confirmed_block(2));

    BOOST_REQUIRE(query.push_confirmed(1));
    BOOST_REQUIRE(query.is_confirmed_block(1));

    BOOST_REQUIRE(query.push_confirmed(2));
    BOOST_REQUIRE(query.is_confirmed_block(2));

    BOOST_REQUIRE(query.pop_confirmed());
    BOOST_REQUIRE(query.is_confirmed_block(0));
    BOOST_REQUIRE(query.is_confirmed_block(1));
    BOOST_REQUIRE(!query.is_confirmed_block(2));

    BOOST_REQUIRE(query.pop_confirmed());
    BOOST_REQUIRE(query.is_confirmed_block(0));
    BOOST_REQUIRE(!query.is_confirmed_block(1));
    BOOST_REQUIRE(!query.is_confirmed_block(2));
}

BOOST_AUTO_TEST_CASE(query_confirm__is_confirmed_tx__confirm__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(events_handler), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, context{ 0, 1, 0 }, false, false));
    BOOST_REQUIRE(query.set(test::block2, context{ 0, 2, 0 }, false, false));
    BOOST_REQUIRE(query.is_confirmed_tx(0));
    BOOST_REQUIRE(!query.is_confirmed_tx(1));
    BOOST_REQUIRE(!query.is_confirmed_tx(2));

    BOOST_REQUIRE(query.push_confirmed(1));
    BOOST_REQUIRE(!query.is_confirmed_tx(1));
    BOOST_REQUIRE(query.set_strong(1));
    BOOST_REQUIRE(query.is_confirmed_tx(1));

    // Ordering between strong and confirmed is unimportant.
    BOOST_REQUIRE(query.set_strong(2));
    BOOST_REQUIRE(!query.is_confirmed_tx(2));
    BOOST_REQUIRE(query.push_confirmed(2));
    BOOST_REQUIRE(query.is_confirmed_tx(2));
}

BOOST_AUTO_TEST_CASE(query_confirm__is_confirmed_input__confirm__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(events_handler), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, context{ 0, 1, 0 }, false, false));
    BOOST_REQUIRE(query.set(test::block2, context{ 0, 2, 0 }, false, false));

    BOOST_REQUIRE(query.is_confirmed_input(query.to_spend(0, 0)));
    BOOST_REQUIRE(!query.is_confirmed_input(query.to_spend(1, 0)));
    BOOST_REQUIRE(!query.is_confirmed_input(query.to_spend(2, 0)));

    BOOST_REQUIRE(query.push_confirmed(1));
    BOOST_REQUIRE(!query.is_confirmed_input(query.to_spend(1, 0)));
    BOOST_REQUIRE(query.set_strong(1));
    BOOST_REQUIRE(query.is_confirmed_input(query.to_spend(1, 0)));

    BOOST_REQUIRE(query.set_strong(2));
    BOOST_REQUIRE(!query.is_confirmed_input(query.to_spend(2, 0)));
    BOOST_REQUIRE(query.push_confirmed(2));
    BOOST_REQUIRE(query.is_confirmed_input(query.to_spend(2, 0)));
}

BOOST_AUTO_TEST_CASE(query_confirm__is_confirmed_output__confirm__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(events_handler), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, context{ 0, 1, 0 }, false, false));
    BOOST_REQUIRE(query.set(test::block2, context{ 0, 2, 0 }, false, false));

    BOOST_REQUIRE(query.is_confirmed_output(query.to_output(0, 0)));
    BOOST_REQUIRE(!query.is_confirmed_output(query.to_output(1, 0)));
    BOOST_REQUIRE(!query.is_confirmed_output(query.to_output(2, 0)));

    BOOST_REQUIRE(query.push_confirmed(1));
    BOOST_REQUIRE(!query.is_confirmed_output(query.to_output(1, 0)));
    BOOST_REQUIRE(query.set_strong(1));
    BOOST_REQUIRE(query.is_confirmed_output(query.to_output(1, 0)));

    BOOST_REQUIRE(query.set_strong(2));
    BOOST_REQUIRE(!query.is_confirmed_output(query.to_output(2, 0)));
    BOOST_REQUIRE(query.push_confirmed(2));
    BOOST_REQUIRE(query.is_confirmed_output(query.to_output(2, 0)));
}

BOOST_AUTO_TEST_CASE(query_confirm__is_spent_output__genesis__false)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(events_handler), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(!query.is_spent_output(query.to_output(0, 0)));
    BOOST_REQUIRE(!query.is_spent_output(query.to_output(1, 1)));
}

BOOST_AUTO_TEST_CASE(query_confirm__is_spent_output__strong_confirmed__true)
{
    settings settings{};
    settings.minimize = true;
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(events_handler), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1a, context{ 0, 1, 0 }, false, false));
    BOOST_REQUIRE(query.set(test::block2a, context{ 0, 2, 0 }, false, false));
    BOOST_REQUIRE(query.set(test::block3a, context{ 0, 3, 0 }, false, false));
    BOOST_REQUIRE(!query.is_spent_output(query.to_output(0, 0))); // genesis
    BOOST_REQUIRE(!query.is_spent_output(query.to_output(1, 0))); // block1a
    BOOST_REQUIRE(!query.is_spent_output(query.to_output(1, 1))); // block1a
    BOOST_REQUIRE(query.set_strong(1));
    BOOST_REQUIRE(query.set_strong(2));
    BOOST_REQUIRE(query.set_strong(3));
    BOOST_REQUIRE(!query.is_spent_output(query.to_output(0, 0))); // genesis
    BOOST_REQUIRE(!query.is_spent_output(query.to_output(1, 0))); // block1a
    BOOST_REQUIRE(!query.is_spent_output(query.to_output(1, 1))); // block1a
    BOOST_REQUIRE(query.push_confirmed(1));
    BOOST_REQUIRE(query.push_confirmed(2));
    BOOST_REQUIRE(query.push_confirmed(3));
    BOOST_REQUIRE(!query.is_spent_output(query.to_output(0, 0))); // genesis
    BOOST_REQUIRE(query.is_spent_output(query.to_output(1, 0)));  // block1a
    BOOST_REQUIRE(query.is_spent_output(query.to_output(1, 1)));  // block1a
}

BOOST_AUTO_TEST_CASE(query_confirm__is_strong_spend__strong__true)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(events_handler), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.is_strong_spend(query.to_spend(0, 0)));
}

BOOST_AUTO_TEST_CASE(query_confirm__is_strong_spend__weak__false)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(events_handler), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, context{}, false, false));
    BOOST_REQUIRE(!query.is_strong_spend(query.to_spend(1, 0)));
    BOOST_REQUIRE(query.set_strong(1));
    BOOST_REQUIRE(query.is_strong_spend(query.to_spend(1, 0)));
}

// The coinbase tx is strong.
BOOST_AUTO_TEST_CASE(query_confirm__is_strong__strong__true)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(events_handler), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.is_strong_tx(0));
    BOOST_REQUIRE(query.is_strong_block(0));
}

BOOST_AUTO_TEST_CASE(query_confirm__is_strong__weak__false)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(events_handler), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, context{}, false, false));
    BOOST_REQUIRE(!query.is_strong_tx(1));
    BOOST_REQUIRE(!query.is_strong_block(1));
    BOOST_REQUIRE(query.set_strong(1));
    BOOST_REQUIRE(query.is_strong_tx(1));
    BOOST_REQUIRE(query.is_strong_block(1));
}

BOOST_AUTO_TEST_CASE(query_confirm__is_spent__unspent__false)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(events_handler), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, context{}, false, false));
    BOOST_REQUIRE(!query.is_spent(query.to_spend(0, 0))); // unspendable
    BOOST_REQUIRE(!query.is_spent(query.to_spend(1, 0))); // unspent
    BOOST_REQUIRE(!query.is_spent(query.to_spend(2, 0))); // non-existent
}

BOOST_AUTO_TEST_CASE(query_confirm__is_spent__unconfirmed_double_spend__false)
{
    settings settings{};
    settings.minimize = true;
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(events_handler), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));

    BOOST_REQUIRE(query.set(test::block1a, context{}, false, false));
    BOOST_REQUIRE(query.set_strong(1));
    BOOST_REQUIRE(!query.is_spent(query.to_spend(0, 0))); // unspendable
    BOOST_REQUIRE(!query.is_spent(query.to_spend(1, 0))); // spent by self (and missing)
    BOOST_REQUIRE(!query.is_spent(query.to_spend(1, 1))); // spent by self (and missing)
    BOOST_REQUIRE(!query.is_spent(query.to_spend(1, 2))); // spent by self (and missing)
    BOOST_REQUIRE(!query.is_spent(query.to_spend(2, 0))); // missing tx
    BOOST_REQUIRE(!query.is_spent(query.to_spend(2, 1))); // missing tx

    BOOST_REQUIRE(query.set(test::block2a, context{}, false, false));
    BOOST_REQUIRE(!query.is_spent(query.to_spend(2, 0))); // unconfirmed self
    BOOST_REQUIRE(!query.is_spent(query.to_spend(2, 1))); // unconfirmed self

    BOOST_REQUIRE(query.set_strong(2));
    BOOST_REQUIRE(query.push_confirmed(2));
    BOOST_REQUIRE(!query.is_spent(query.to_spend(2, 0))); // confirmed self
    BOOST_REQUIRE(!query.is_spent(query.to_spend(2, 1))); // confirmed self

    BOOST_REQUIRE(query.set(test::tx4));
    BOOST_REQUIRE(!query.is_spent(query.to_spend(2, 0))); // confirmed self, unconfirmed
    BOOST_REQUIRE(!query.is_spent(query.to_spend(2, 1))); // confirmed self, unconfirmed

    BOOST_REQUIRE(query.set(test::block3a, context{}, false, false));
    BOOST_REQUIRE(!query.is_spent(query.to_spend(2, 0))); // confirmed self, unconfirmed(2)
    BOOST_REQUIRE(!query.is_spent(query.to_spend(2, 1))); // confirmed self, unconfirmed(2)

    BOOST_REQUIRE(query.set_strong(3));
    BOOST_REQUIRE(query.is_spent(query.to_spend(2, 0)));  // confirmed self, unconfirmed, confirmed (double spent)
    BOOST_REQUIRE(query.is_spent(query.to_spend(2, 1)));  // confirmed self, unconfirmed, confirmed (double spent)

    BOOST_REQUIRE(query.set_unstrong(2));
    BOOST_REQUIRE(query.is_spent(query.to_spend(2, 0)));  // unconfirmed self, unconfirmed, confirmed (spent)
    BOOST_REQUIRE(query.is_spent(query.to_spend(2, 1)));  // unconfirmed self, unconfirmed, confirmed (spent)

    BOOST_REQUIRE(query.set_unstrong(3));
    BOOST_REQUIRE(!query.is_spent(query.to_spend(2, 0)));  // unconfirmed self, unconfirmed, unconfirmed (unspent)
    BOOST_REQUIRE(!query.is_spent(query.to_spend(2, 1)));  // unconfirmed self, unconfirmed, unconfirmed (unspent)

    BOOST_REQUIRE(query.set_strong(2));
    BOOST_REQUIRE(!query.is_spent(query.to_spend(2, 0)));  // confirmed self, unconfirmed, confirmed (unspent)
    BOOST_REQUIRE(!query.is_spent(query.to_spend(2, 1)));  // confirmed self, unconfirmed, confirmed (unspent)
}

BOOST_AUTO_TEST_CASE(query_confirm__is_spent__confirmed_double_spend__true)
{
    settings settings{};
    settings.minimize = true;
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(events_handler), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1a, context{}, false, false));
    BOOST_REQUIRE(query.set_strong(1));
    BOOST_REQUIRE(query.set(test::block2a, context{}, false, false));
    BOOST_REQUIRE(query.set_strong(2));
    BOOST_REQUIRE(query.set(test::block3a, context{}, false, false));
    BOOST_REQUIRE(query.set_strong(3));
    BOOST_REQUIRE(query.is_spent(query.to_spend(2, 0))); // confirmed self and confirmed (double spent)
    BOOST_REQUIRE(query.is_spent(query.to_spend(2, 1))); // confirmed self and confirmed (double spent)
}

BOOST_AUTO_TEST_CASE(query_confirm__is_mature__spend_genesis__false)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(events_handler), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::tx_spend_genesis));
    BOOST_REQUIRE(!query.is_mature(query.to_spend(1, 0), 0));
    BOOST_REQUIRE(!query.is_mature(query.to_spend(1, 0), 100));
}

BOOST_AUTO_TEST_CASE(query_confirm__is_mature__not_found__false)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(events_handler), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(!query.is_mature(query.to_spend(0, 1), 0));
    BOOST_REQUIRE(!query.is_mature(query.to_spend(42, 24), 1000));
}

BOOST_AUTO_TEST_CASE(query_confirm__is_mature__null_input__true)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(events_handler), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.is_mature(query.to_spend(0, 0), 0));
}

BOOST_AUTO_TEST_CASE(query_confirm__is_mature__non_coinbase_strong_above__true)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(events_handler), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1a, context{ 0, 1, 0 }, false, false));
    BOOST_REQUIRE(query.set(test::tx4));

    // Is not actually mature at height zero, but strong is presumed to always
    // be set at the current height and never above it (set above in this test).
    BOOST_REQUIRE(query.set_strong(1));
    BOOST_REQUIRE(query.is_mature(query.to_spend(2, 0), 0));
}

BOOST_AUTO_TEST_CASE(query_confirm__is_mature__non_coinbase__true)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(events_handler), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1a, context{ 0, 1, 0 }, false, false));
    BOOST_REQUIRE(query.set(test::tx4));
    BOOST_REQUIRE(query.set_strong(1));
    BOOST_REQUIRE(query.is_mature(query.to_spend(2, 0), 1));
}

BOOST_AUTO_TEST_CASE(query_confirm__is_mature__coinbase__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(events_handler), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1b, context{ 0, 1, 0 }, false, false));
    BOOST_REQUIRE(query.set_strong(1));
    BOOST_REQUIRE(query.set(test::tx2b));
    BOOST_REQUIRE(!query.is_mature(query.to_spend(2, 0), 100));
    BOOST_REQUIRE(query.is_mature(query.to_spend(2, 0), 101));
}

constexpr auto bip68 = system::chain::flags::bip68_rule;

BOOST_AUTO_TEST_CASE(query_confirm__block_confirmable__bad_link__integrity)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(events_handler), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, context{ bip68, 1, 0 }, false, false));
    BOOST_REQUIRE_EQUAL(query.block_confirmable(2), error::integrity1);
}

BOOST_AUTO_TEST_CASE(query_confirm__block_confirmable__null_points__success)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(events_handler), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, context{ bip68 }, false, false));
    BOOST_REQUIRE(query.set(test::block2, context{ bip68 }, false, false));
    BOOST_REQUIRE(query.set(test::block3, context{ bip68 }, false, false));

    // ALL COINBASE TXS
    // block1/2/3 at links 1/2/3 confirming at heights 1/2/3.
    // blocks have only coinbase txs, all txs should be set strong before calling
    // confirmable, but these are bip30 default configuration.
    BOOST_REQUIRE_EQUAL(query.block_confirmable(1), error::success);
    BOOST_REQUIRE_EQUAL(query.block_confirmable(2), error::success);
    BOOST_REQUIRE_EQUAL(query.block_confirmable(3), error::success);
}

BOOST_AUTO_TEST_CASE(query_confirm__block_confirmable__missing_prevouts__integrity)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(events_handler), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1a, context{ bip68, 1, 0 }, false, false));

    // ONLY COINBASE TXS
    // block1a is missing all three input prevouts.
    BOOST_REQUIRE(query.set_strong(1));
    BOOST_REQUIRE_EQUAL(query.block_confirmable(1), error::success);
}

BOOST_AUTO_TEST_CASE(query_confirm__block_confirmable__spend_gensis__coinbase_maturity)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(events_handler), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));

    // block_spend_genesis spends the genesis output.
    BOOST_REQUIRE(query.set(test::block_spend_genesis, context{ 0, 101, 0 }, false, false));
    BOOST_REQUIRE(query.set_strong(1));

    // COINBASE TX
    // 1 + 100 = 101 (maturity, except genesis)
    BOOST_REQUIRE_EQUAL(query.block_confirmable(1), error::success);
}

BOOST_AUTO_TEST_CASE(query_confirm__block_confirmable__immature_prevouts__coinbase_maturity)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(events_handler), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));

    // block1b has only a coinbase tx.
    BOOST_REQUIRE(query.set(test::block1b, context{ bip68, 1, 0 }, false, false));
    BOOST_REQUIRE(query.set_strong(1));
    BOOST_REQUIRE_EQUAL(query.block_confirmable(1), error::success);

    // COINBASE TX
    // block2b prematurely spends block1b's coinbase outputs.
    BOOST_REQUIRE(query.set(test::block2b, context{ 0, 100, 0 }, false, false));
    BOOST_REQUIRE(query.set_strong(2));
    BOOST_REQUIRE_EQUAL(query.block_confirmable(2), error::success);
}

BOOST_AUTO_TEST_CASE(query_confirm__block_confirmable__mature_prevouts__success)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(events_handler), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));

    // block1b has only a coinbase tx.
    BOOST_REQUIRE(query.set(test::block1b, context{ bip68, 1, 0 }, false, false));
    BOOST_REQUIRE(query.set_strong(1));
    BOOST_REQUIRE_EQUAL(query.block_confirmable(1), error::success);

    // COINBASE TX
    // block2b spends block1b's coinbase outputs.
    BOOST_REQUIRE(query.set(test::block2b, context{ 0, 101, 0 }, false, false));
    BOOST_REQUIRE(query.set_strong(2));
    BOOST_REQUIRE_EQUAL(query.block_confirmable(2), error::success);
}

BOOST_AUTO_TEST_CASE(query_confirm__block_confirmable__spend_non_coinbase__success)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(events_handler), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));

    // block1a has non-coinbase tx/outputs.
    BOOST_REQUIRE(query.set(test::block1a, context{ 0, 1, 0 }, false, false));
    BOOST_REQUIRE(query.set_strong(1));

    // block_spend_1a spends both block1a outputs.
    BOOST_REQUIRE(query.set(test::block_spend_1a, context{ 0, 2, 0 }, false, false));
    BOOST_REQUIRE(query.set_strong(2));

    // COINBASE TX
    // Maturity applies only to coinbase prevouts.
    BOOST_REQUIRE_EQUAL(query.block_confirmable(2), error::success);
}

// These pas but test vectors need to be updated to create clear test conditions.
////BOOST_AUTO_TEST_CASE(query_confirm__block_confirmable__spend_coinbase_and_internal_immature__coinbase_maturity)
////{
////    settings settings{};
////    settings.path = TEST_DIRECTORY;
////    test::chunk_store store{ settings };
////    test::query_accessor query{ store };
////    BOOST_REQUIRE_EQUAL(store.create(events_handler), error::success);
////    BOOST_REQUIRE(query.initialize(test::genesis));
////
////    // block1b has coinbase tx/outputs.
////    BOOST_REQUIRE(query.set(test::block1b, context{ 0, 1, 0 }, false);
////    BOOST_REQUIRE(query.set_strong(1));
////
////    // block_spend_internal_2b spends first block1a output and first own output.
////    BOOST_REQUIRE(query.set(test::block_spend_internal_2b, context{ 0, 100, 0 }, false, false));
////    ////BOOST_REQUIRE(query.set_strong(2));
////
////    // Not confirmable because tx not strong.
////    BOOST_REQUIRE_EQUAL(query.block_confirmable(2), error::integrity);
////    BOOST_REQUIRE(query.set_strong(2));
////
////    // Not confirmable because lack of maturity.
////    BOOST_REQUIRE_EQUAL(!query.block_confirmable(2), error::success);
////}
////
////BOOST_AUTO_TEST_CASE(query_confirm__block_confirmable__spend_coinbase_and_internal_mature__success)
////{
////    settings settings{};
////    settings.path = TEST_DIRECTORY;
////    test::chunk_store store{ settings };
////    test::query_accessor query{ store };
////    BOOST_REQUIRE_EQUAL(store.create(events_handler), error::success);
////    BOOST_REQUIRE(query.initialize(test::genesis));
////
////    // block1b has coinbase tx/outputs.
////    BOOST_REQUIRE(query.set(test::block1b, context{ 0, 1, 0 }, false, false));
////    BOOST_REQUIRE(query.set_strong(1));
////
////    BOOST_REQUIRE(query.set(test::block_spend_internal_2b, context{ 0, 101, 0 }, false, false));
////    ////BOOST_REQUIRE(query.set_strong(2));
////
////    // Not confirmable because tx not strong.
////    BOOST_REQUIRE_EQUAL(query.block_confirmable(2), error::integrity);
////
////    // block_spend_internal_2b spends first block1b output and first own output.
////    // But first block1b is first tx in block_spend_internal_2b so excluded as coinbase.
////    // It spends only its first own output (coinbase) and that can never be mature.
////    // But spend target is not stored as coinbase because it's not a null point.
////    BOOST_REQUIRE(query.set_strong(2));
////    BOOST_REQUIRE_EQUAL(!query.block_confirmable(2), error::success);
////}
////
////BOOST_AUTO_TEST_CASE(query_confirm__block_confirmable__confirmed_double_spend__confirmed_double_spend)
////{
////    settings settings{};
////    settings.path = TEST_DIRECTORY;
////    test::chunk_store store{ settings };
////    test::query_accessor query{ store };
////    BOOST_REQUIRE_EQUAL(store.create(events_handler), error::success);
////    BOOST_REQUIRE(query.initialize(test::genesis));
////
////    // block1a has non-coinbase tx/outputs.
////    BOOST_REQUIRE(query.set(test::block1a, context{ 0, 1, 0 }, false, false));
////    BOOST_REQUIRE(query.set_strong(1));
////
////    // block2a spends both block1a outputs (though not itself confirmable is set strong).
////    BOOST_REQUIRE(query.set(test::block2a, context{ 0, 2, 0 }, false, false));
////    BOOST_REQUIRE(query.set_strong(2));
////
////    // block_spend_1a (also) spends both block1a outputs (and is otherwise confirmable).
////    BOOST_REQUIRE(query.set(test::block_spend_1a, context{ bip68, 3, 0 }, false, false));
////    BOOST_REQUIRE(query.set_strong(3));
////
////    // Not confirmable because of intervening block2a implies double spend.
////    BOOST_REQUIRE_EQUAL(!query.block_confirmable(3), error::success);
////}
////
////BOOST_AUTO_TEST_CASE(query_confirm__block_confirmable__unconfirmed_double_spend__success)
////{
////    settings settings{};
////    settings.path = TEST_DIRECTORY;
////    test::chunk_store store{ settings };
////    test::query_accessor query{ store };
////    BOOST_REQUIRE_EQUAL(store.create(events_handler), error::success);
////    BOOST_REQUIRE(query.initialize(test::genesis));
////
////    // block1a has non-coinbase tx/outputs.
////    BOOST_REQUIRE(query.set(test::block1a, context{ 0, 1, 0 }, false, false));
////    BOOST_REQUIRE(query.set_strong(1));
////
////    // tx5 spends first block1a output.
////    BOOST_REQUIRE(query.set(test::tx5));
////
////    // block_spend_1a (also) spends both block1a outputs.
////    BOOST_REQUIRE(query.set(test::block_spend_1a, context{ 0, 2, 0 }, false, false));
////    BOOST_REQUIRE(query.set_strong(2));
////
////    // Confirmable because of intervening tx5 is unconfirmed double spend.
////    BOOST_REQUIRE_EQUAL(!query.block_confirmable(2));
////}

BOOST_AUTO_TEST_CASE(query_confirm__set_strong__unassociated__false)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(events_handler), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1.header(), context{}, false));
    BOOST_REQUIRE(!query.set_strong(1));
    BOOST_REQUIRE(!query.set_unstrong(1));
}

BOOST_AUTO_TEST_CASE(query_confirm__set_strong__set_unstrong__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(events_handler), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, context{ 0, 1, 0 }, false, false));
    BOOST_REQUIRE(query.set(test::block2, context{ 0, 2, 0 }, false, false));
    BOOST_REQUIRE(query.push_confirmed(1));
    BOOST_REQUIRE(query.push_confirmed(2));

    BOOST_REQUIRE(query.is_confirmed_tx(0));
    BOOST_REQUIRE(query.is_confirmed_input(query.to_spend(0, 0)));
    BOOST_REQUIRE(query.is_confirmed_output(query.to_output(0, 0)));

    BOOST_REQUIRE(!query.is_confirmed_tx(1));
    BOOST_REQUIRE(!query.is_confirmed_input(query.to_spend(1, 0)));
    BOOST_REQUIRE(!query.is_confirmed_output(query.to_output(1, 0)));

    BOOST_REQUIRE(!query.is_confirmed_tx(2));
    BOOST_REQUIRE(!query.is_confirmed_input(query.to_spend(2, 0)));
    BOOST_REQUIRE(!query.is_confirmed_output(query.to_output(2, 0)));

    BOOST_REQUIRE(query.set_strong(1));
    BOOST_REQUIRE(query.set_strong(2));

    BOOST_REQUIRE(query.is_confirmed_tx(0));
    BOOST_REQUIRE(query.is_confirmed_input(query.to_spend(0, 0)));
    BOOST_REQUIRE(query.is_confirmed_output(query.to_output(0, 0)));

    BOOST_REQUIRE(query.is_confirmed_tx(1));
    BOOST_REQUIRE(query.is_confirmed_input(query.to_spend(1, 0)));
    BOOST_REQUIRE(query.is_confirmed_output(query.to_output(1, 0)));

    BOOST_REQUIRE(query.is_confirmed_tx(2));
    BOOST_REQUIRE(query.is_confirmed_input(query.to_spend(2, 0)));
    BOOST_REQUIRE(query.is_confirmed_output(query.to_output(2, 0)));

    BOOST_REQUIRE(query.set_unstrong(1));
    BOOST_REQUIRE(query.set_unstrong(2));

    BOOST_REQUIRE(query.is_confirmed_tx(0));
    BOOST_REQUIRE(query.is_confirmed_input(query.to_spend(0, 0)));
    BOOST_REQUIRE(query.is_confirmed_output(query.to_output(0, 0)));

    BOOST_REQUIRE(!query.is_confirmed_tx(1));
    BOOST_REQUIRE(!query.is_confirmed_input(query.to_spend(1, 0)));
    BOOST_REQUIRE(!query.is_confirmed_output(query.to_output(1, 0)));

    BOOST_REQUIRE(!query.is_confirmed_tx(2));
    BOOST_REQUIRE(!query.is_confirmed_input(query.to_spend(2, 0)));
    BOOST_REQUIRE(!query.is_confirmed_output(query.to_output(2, 0)));
}

BOOST_AUTO_TEST_SUITE_END()
