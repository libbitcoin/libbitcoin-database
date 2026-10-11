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

BOOST_FIXTURE_TEST_SUITE(query_confirmed_tests, test::directory_setup_fixture)

BOOST_AUTO_TEST_CASE(query_confirmed__is_candidate_block__push_pop_candidate__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, context{ 0, 1, 0 }, {}, false, false));
    BOOST_REQUIRE(query.set(test::block2, context{ 0, 2, 0 }, {}, false, false));
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

BOOST_AUTO_TEST_CASE(query_confirmed__is_candidate_extension__candidates__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1.header(), context{ 0, 1, 0 }, {}, false));
    BOOST_REQUIRE(query.set(test::block2.header(), context{ 0, 2, 0 }, {}, false));
    BOOST_REQUIRE(!query.is_candidate_extension(0));
    BOOST_REQUIRE(!query.is_candidate_extension(1));

    BOOST_REQUIRE(query.push_candidate(1));
    BOOST_REQUIRE(query.push_candidate(2));
    BOOST_REQUIRE(query.is_candidate_extension(1));
    BOOST_REQUIRE(!query.is_candidate_extension(2));

    BOOST_REQUIRE(query.set(test::block1, false, false));
    BOOST_REQUIRE(!query.is_candidate_extension(1));
}

BOOST_AUTO_TEST_CASE(query_confirmed__is_confirmed_block__push_pop_confirmed__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, context{ 0, 1, 0 }, {}, false, false));
    BOOST_REQUIRE(query.set(test::block2, context{ 0, 2, 0 }, {}, false, false));
    BOOST_REQUIRE(query.is_confirmed_block(0));
    BOOST_REQUIRE(!query.is_confirmed_block(1));
    BOOST_REQUIRE(!query.is_confirmed_block(2));

    BOOST_REQUIRE(query.push_confirmed(1, false));
    BOOST_REQUIRE(query.is_confirmed_block(1));

    BOOST_REQUIRE(query.push_confirmed(2, false));
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

BOOST_AUTO_TEST_CASE(query_confirmed__is_confirmed_tx__confirm__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, context{ 0, 1, 0 }, {}, false, false));
    BOOST_REQUIRE(query.set(test::block2, context{ 0, 2, 0 }, {}, false, false));
    BOOST_REQUIRE(query.is_confirmed_tx(0));
    BOOST_REQUIRE(!query.is_confirmed_tx(1));
    BOOST_REQUIRE(!query.is_confirmed_tx(2));

    BOOST_REQUIRE(query.push_confirmed(1, false));
    BOOST_REQUIRE(!query.is_confirmed_tx(1));
    BOOST_REQUIRE(query.set_strong(1));
    BOOST_REQUIRE(query.is_confirmed_tx(1));

    // Ordering between strong and confirmed is unimportant.
    BOOST_REQUIRE(query.set_strong(2));
    BOOST_REQUIRE(!query.is_confirmed_tx(2));
    BOOST_REQUIRE(query.push_confirmed(2, false));
    BOOST_REQUIRE(query.is_confirmed_tx(2));
}

BOOST_AUTO_TEST_CASE(query_confirmed__is_confirmed_input__genesis__true)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.is_confirmed_input(query.to_point(0, 0)));
}

BOOST_AUTO_TEST_CASE(query_confirmed__is_confirmed_input__unconfirmed__false)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, context{ 0, 1, 0 }, {}, false, false));

    BOOST_REQUIRE(!query.is_confirmed_input(query.to_point(1, 0)));
    BOOST_REQUIRE(query.set_strong(1));
    BOOST_REQUIRE(!query.is_confirmed_input(query.to_point(1, 0)));
    BOOST_REQUIRE(query.push_confirmed(1, false));
    BOOST_REQUIRE(query.is_confirmed_input(query.to_point(1, 0)));
}

BOOST_AUTO_TEST_CASE(query_confirmed__is_confirmed_input__confirmed_weak__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1a, context{ 0, 1, 0 }, {}, false, false));
    BOOST_REQUIRE(query.set(test::block2a, context{ 0, 2, 0 }, {}, false, false));

    // test::genesis has one input, initialize always confirms strong.
    BOOST_REQUIRE(query.is_confirmed_input(query.to_point(0, 0)));

    // test::block1a has three inputs.
    BOOST_REQUIRE(!query.is_confirmed_input(query.to_point(1, 0)));
    BOOST_REQUIRE(!query.is_confirmed_input(query.to_point(1, 1)));
    BOOST_REQUIRE(!query.is_confirmed_input(query.to_point(1, 2)));
    BOOST_REQUIRE(query.push_confirmed(1, false));
    BOOST_REQUIRE(!query.is_confirmed_input(query.to_point(1, 0)));
    BOOST_REQUIRE(!query.is_confirmed_input(query.to_point(1, 1)));
    BOOST_REQUIRE(!query.is_confirmed_input(query.to_point(1, 2)));
    BOOST_REQUIRE(query.set_strong(1));
    BOOST_REQUIRE(query.is_confirmed_input(query.to_point(1, 0)));
    BOOST_REQUIRE(query.is_confirmed_input(query.to_point(1, 1)));
    BOOST_REQUIRE(query.is_confirmed_input(query.to_point(1, 2)));

    // test::block2a has two inputs.
    BOOST_REQUIRE(!query.is_confirmed_input(query.to_point(2, 0)));
    BOOST_REQUIRE(!query.is_confirmed_input(query.to_point(2, 1)));
    BOOST_REQUIRE(query.push_confirmed(2, false));
    BOOST_REQUIRE(!query.is_confirmed_input(query.to_point(2, 0)));
    BOOST_REQUIRE(!query.is_confirmed_input(query.to_point(2, 1)));
    BOOST_REQUIRE(query.set_strong(2));
    BOOST_REQUIRE(query.is_confirmed_input(query.to_point(2, 0)));
    BOOST_REQUIRE(query.is_confirmed_input(query.to_point(2, 1)));
}

BOOST_AUTO_TEST_CASE(query_confirmed__is_confirmed_input__confirmed_strong__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, context{ 0, 1, 0 }, {}, false, false));
    BOOST_REQUIRE(query.set(test::block2, context{ 0, 2, 0 }, {}, false, false));

    // test::genesis has one input, initialize always confirms strong.
    BOOST_REQUIRE(query.is_confirmed_input(query.to_point(0, 0)));

    // test::block1 has one input.
    BOOST_REQUIRE(!query.is_confirmed_input(query.to_point(1, 0)));
    BOOST_REQUIRE(query.push_confirmed(1, true));
    BOOST_REQUIRE(query.is_confirmed_input(query.to_point(1, 0)));

    // test::block2 has two inputs.
    BOOST_REQUIRE(!query.is_confirmed_input(query.to_point(2, 0)));
    BOOST_REQUIRE(query.push_confirmed(2, true));
    BOOST_REQUIRE(query.is_confirmed_input(query.to_point(2, 0)));
}

BOOST_AUTO_TEST_CASE(query_confirmed__is_confirmed_output__confirm__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, context{ 0, 1, 0 }, {}, false, false));
    BOOST_REQUIRE(query.set(test::block2, context{ 0, 2, 0 }, {}, false, false));

    // test::genesis has one output, initialize always confirms strong.
    BOOST_REQUIRE(query.is_confirmed_output(query.to_output(0, 0)));
    BOOST_REQUIRE(!query.is_confirmed_output(query.to_output(1, 0)));
    BOOST_REQUIRE(!query.is_confirmed_output(query.to_output(2, 0)));

    BOOST_REQUIRE(query.push_confirmed(1, false));
    BOOST_REQUIRE(!query.is_confirmed_output(query.to_output(1, 0)));
    BOOST_REQUIRE(query.set_strong(1));
    BOOST_REQUIRE(query.is_confirmed_output(query.to_output(1, 0)));

    // Can set strong before confirm.
    BOOST_REQUIRE(query.set_strong(2));
    BOOST_REQUIRE(!query.is_confirmed_output(query.to_output(2, 0)));
    BOOST_REQUIRE(query.push_confirmed(2, false));
    BOOST_REQUIRE(query.is_confirmed_output(query.to_output(2, 0)));
}

BOOST_AUTO_TEST_CASE(query_confirmed__is_confirmed_spent__genesis__false)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(!query.is_confirmed_spent(query.to_output(0, 0)));
    BOOST_REQUIRE(!query.is_confirmed_spent(query.to_output(1, 1)));
}

BOOST_AUTO_TEST_CASE(query_confirmed__is_confirmed_spent__strong_confirmed__true)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1a, context{ 0, 1, 0 }, {}, false, false));
    BOOST_REQUIRE(query.set(test::block2a, context{ 0, 2, 0 }, {}, false, false));
    BOOST_REQUIRE(query.set(test::block3a, context{ 0, 3, 0 }, {}, false, false));
    BOOST_REQUIRE(!query.is_confirmed_spent(query.to_output(0, 0))); // genesis
    BOOST_REQUIRE(!query.is_confirmed_spent(query.to_output(1, 0))); // block1a
    BOOST_REQUIRE(!query.is_confirmed_spent(query.to_output(1, 1))); // block1a
    BOOST_REQUIRE(query.set_strong(1));
    BOOST_REQUIRE(query.set_strong(2));
    BOOST_REQUIRE(query.set_strong(3));
    BOOST_REQUIRE(!query.is_confirmed_spent(query.to_output(0, 0))); // genesis
    BOOST_REQUIRE(!query.is_confirmed_spent(query.to_output(1, 0))); // block1a
    BOOST_REQUIRE(!query.is_confirmed_spent(query.to_output(1, 1))); // block1a
    BOOST_REQUIRE(query.push_confirmed(1, false));
    BOOST_REQUIRE(query.push_confirmed(2, false));
    BOOST_REQUIRE(query.push_confirmed(3, false));
    BOOST_REQUIRE(!query.is_confirmed_spent(query.to_output(0, 0))); // genesis
    BOOST_REQUIRE(query.is_confirmed_spent(query.to_output(1, 0)));  // block1a
    BOOST_REQUIRE(query.is_confirmed_spent(query.to_output(1, 1)));  // block1a
}

// is_confirmed_all_prevouts

BOOST_AUTO_TEST_CASE(query_confirmed__is_confirmed_all_prevouts__genesis__true)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.is_confirmed_all_prevouts(0));
}

BOOST_AUTO_TEST_CASE(query_confirmed__is_confirmed_all_prevouts__unconfirmed_coinbase__false)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));

    // Block 1b has single null input so archived as (strong) coinbase.
    BOOST_REQUIRE(query.set(test::block1b, context{ 0, 1, 0 }, {}, false, true));
    BOOST_REQUIRE(!query.is_confirmed_all_prevouts(1));
}

BOOST_AUTO_TEST_CASE(query_confirmed__is_confirmed_all_prevouts__confirmed_coinbase__true)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));

    // Block 1b has single null input so archived as (strong) coinbase.
    BOOST_REQUIRE(query.set(test::block1b, context{ 0, 1, 0 }, {}, false, true));
    BOOST_REQUIRE(query.push_confirmed(1, false));
    BOOST_REQUIRE(query.is_confirmed_all_prevouts(1));
}

BOOST_AUTO_TEST_CASE(query_confirmed__is_confirmed_all_prevouts__missing_prevouts__false)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));

    // Tx1 is of block1a and consists of three inputs that do not exist.
    BOOST_REQUIRE(query.set(test::block1a, context{ 0, 1, 0 }, {}, false, true));
    BOOST_REQUIRE(!query.is_confirmed_all_prevouts(1));
}

BOOST_AUTO_TEST_CASE(query_confirmed__is_confirmed_all_prevouts__prevouts_confirmed__true)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));

    // Block 1a has 1 tx(1) with 2 outputs.
    BOOST_REQUIRE(query.set(test::block1a, context{ 0, 1, 0 }, {}, false, true));

    // Block 2a first tx(2) spends both block 1a outputs.
    BOOST_REQUIRE(query.set(test::block2a, context{ 0, 2, 0 }, {}, false, true));

    // Block 1a tx(1) is strong and not confirmed.
    BOOST_REQUIRE(!query.is_confirmed_all_prevouts(2));

    // Block 1a tx(1) is strong and confirmed.
    BOOST_REQUIRE(query.push_confirmed(1, false));
    BOOST_REQUIRE(query.is_confirmed_all_prevouts(2));
}

BOOST_AUTO_TEST_SUITE_END()
