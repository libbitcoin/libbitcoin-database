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

struct query_confirmation_setup_fixture
{
    DELETE4(query_confirmation_setup_fixture);
    BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

    query_confirmation_setup_fixture() NOEXCEPT
    {
        BOOST_REQUIRE(test::clear(test::directory));
    }

    ~query_confirmation_setup_fixture() NOEXCEPT
    {
        BOOST_REQUIRE(test::clear(test::directory));
    }

    BC_POP_WARNING()
};

BOOST_FIXTURE_TEST_SUITE(query_confirmation_tests, query_confirmation_setup_fixture)

BOOST_AUTO_TEST_CASE(query_confirmation__pop__zero__false)
{
    settings settings{};
    settings.dir = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(), error::success);
    BOOST_REQUIRE(!query.pop_candidate());
    BOOST_REQUIRE(!query.pop_confirmed());
}

BOOST_AUTO_TEST_CASE(query_confirmation__is_candidate_block__push_pop_candidate__expected)
{
    settings settings{};
    settings.dir = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, { 0, 1, 0 }));
    BOOST_REQUIRE(query.set(test::block2, { 0, 2, 0 }));
    BOOST_REQUIRE(query.is_candidate_block(0));
    BOOST_REQUIRE(!query.is_candidate_block(1));
    BOOST_REQUIRE(!query.is_candidate_block(2));

    BOOST_REQUIRE(query.push_candidate(1));
    BOOST_REQUIRE(query.is_candidate_block(1));

    BOOST_REQUIRE(query.push_candidate(2));
    BOOST_REQUIRE(query.is_candidate_block(2));

    BOOST_REQUIRE(query.pop_candidate());
    BOOST_REQUIRE(query.is_candidate_block(0));
    BOOST_REQUIRE(query.is_candidate_block(1));
    BOOST_REQUIRE(!query.is_candidate_block(2));

    BOOST_REQUIRE(query.pop_candidate());
    BOOST_REQUIRE(query.is_candidate_block(0));
    BOOST_REQUIRE(!query.is_candidate_block(1));
    BOOST_REQUIRE(!query.is_candidate_block(2));
}

BOOST_AUTO_TEST_CASE(query_confirmation__is_confirmed_block__push_pop_confirmed__expected)
{
    settings settings{};
    settings.dir = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, { 0, 1, 0 }));
    BOOST_REQUIRE(query.set(test::block2, { 0, 2, 0 }));
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

BOOST_AUTO_TEST_CASE(query_confirmation__is_confirmed_tx__confirm__expected)
{
    settings settings{};
    settings.dir = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, { 0, 1, 0 }));
    BOOST_REQUIRE(query.set(test::block2, { 0, 2, 0 }));
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

BOOST_AUTO_TEST_CASE(query_confirmation__is_confirmed_input__confirm__expected)
{
    settings settings{};
    settings.dir = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, { 0, 1, 0 }));
    BOOST_REQUIRE(query.set(test::block2, { 0, 2, 0 }));

    BOOST_REQUIRE(query.is_confirmed_input(query.to_input(0, 0)));
    BOOST_REQUIRE(!query.is_confirmed_input(query.to_input(1, 0)));
    BOOST_REQUIRE(!query.is_confirmed_input(query.to_input(2, 0)));

    BOOST_REQUIRE(query.push_confirmed(1));
    BOOST_REQUIRE(!query.is_confirmed_input(query.to_input(1, 0)));
    BOOST_REQUIRE(query.set_strong(1));
    BOOST_REQUIRE(query.is_confirmed_input(query.to_input(1, 0)));

    BOOST_REQUIRE(query.set_strong(2));
    BOOST_REQUIRE(!query.is_confirmed_input(query.to_input(2, 0)));
    BOOST_REQUIRE(query.push_confirmed(2));
    BOOST_REQUIRE(query.is_confirmed_input(query.to_input(2, 0)));
}

BOOST_AUTO_TEST_CASE(query_confirmation__is_confirmed_output__confirm__expected)
{
    settings settings{};
    settings.dir = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, { 0, 1, 0 }));
    BOOST_REQUIRE(query.set(test::block2, { 0, 2, 0 }));

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

BOOST_AUTO_TEST_CASE(query_confirmation__set_strong__unassociated__false)
{
    settings settings{};
    settings.dir = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1.header(), {}));
    BOOST_REQUIRE(!query.set_strong(1));
    BOOST_REQUIRE(!query.set_unstrong(1));
}

BOOST_AUTO_TEST_CASE(query_confirmation__set_strong__set_unstrong__expected)
{
    settings settings{};
    settings.dir = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, { 0, 1, 0 }));
    BOOST_REQUIRE(query.set(test::block2, { 0, 2, 0 }));
    BOOST_REQUIRE(query.push_confirmed(1));
    BOOST_REQUIRE(query.push_confirmed(2));

    BOOST_REQUIRE(query.is_confirmed_tx(0));
    BOOST_REQUIRE(query.is_confirmed_input(query.to_input(0, 0)));
    BOOST_REQUIRE(query.is_confirmed_output(query.to_output(0, 0)));

    BOOST_REQUIRE(!query.is_confirmed_tx(1));
    BOOST_REQUIRE(!query.is_confirmed_input(query.to_input(1, 0)));
    BOOST_REQUIRE(!query.is_confirmed_output(query.to_output(1, 0)));

    BOOST_REQUIRE(!query.is_confirmed_tx(2));
    BOOST_REQUIRE(!query.is_confirmed_input(query.to_input(2, 0)));
    BOOST_REQUIRE(!query.is_confirmed_output(query.to_output(2, 0)));

    BOOST_REQUIRE(query.set_strong(1));
    BOOST_REQUIRE(query.set_strong(2));

    BOOST_REQUIRE(query.is_confirmed_tx(0));
    BOOST_REQUIRE(query.is_confirmed_input(query.to_input(0, 0)));
    BOOST_REQUIRE(query.is_confirmed_output(query.to_output(0, 0)));

    BOOST_REQUIRE(query.is_confirmed_tx(1));
    BOOST_REQUIRE(query.is_confirmed_input(query.to_input(1, 0)));
    BOOST_REQUIRE(query.is_confirmed_output(query.to_output(1, 0)));

    BOOST_REQUIRE(query.is_confirmed_tx(2));
    BOOST_REQUIRE(query.is_confirmed_input(query.to_input(2, 0)));
    BOOST_REQUIRE(query.is_confirmed_output(query.to_output(2, 0)));

    BOOST_REQUIRE(query.set_unstrong(1));
    BOOST_REQUIRE(query.set_unstrong(2));

    BOOST_REQUIRE(query.is_confirmed_tx(0));
    BOOST_REQUIRE(query.is_confirmed_input(query.to_input(0, 0)));
    BOOST_REQUIRE(query.is_confirmed_output(query.to_output(0, 0)));

    BOOST_REQUIRE(!query.is_confirmed_tx(1));
    BOOST_REQUIRE(!query.is_confirmed_input(query.to_input(1, 0)));
    BOOST_REQUIRE(!query.is_confirmed_output(query.to_output(1, 0)));

    BOOST_REQUIRE(!query.is_confirmed_tx(2));
    BOOST_REQUIRE(!query.is_confirmed_input(query.to_input(2, 0)));
    BOOST_REQUIRE(!query.is_confirmed_output(query.to_output(2, 0)));
}

BOOST_AUTO_TEST_CASE(query_confirmation__is_spent__unspent__false)
{
    settings settings{};
    settings.dir = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, {}));
    BOOST_REQUIRE(!query.is_spent(query.to_input(0, 0))); // unspendable
    BOOST_REQUIRE(!query.is_spent(query.to_input(1, 0))); // unspent
    BOOST_REQUIRE(!query.is_spent(query.to_input(2, 0))); // non-existent
}

////bool is_spent(const input_link& link) NOEXCEPT;
////bool is_mature(const input_link& link, size_t height) NOEXCEPT;
////bool is_confirmable_block(const header_link& link, size_t height) NOEXCEPT;

BOOST_AUTO_TEST_SUITE_END()
