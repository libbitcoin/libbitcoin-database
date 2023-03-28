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

struct query_context_setup_fixture
{
    DELETE_COPY_MOVE(query_context_setup_fixture);
    BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

    query_context_setup_fixture() NOEXCEPT
    {
        BOOST_REQUIRE(test::clear(test::directory));
    }

    ~query_context_setup_fixture() NOEXCEPT
    {
        BOOST_REQUIRE(test::clear(test::directory));
    }

    BC_POP_WARNING()
};

BOOST_FIXTURE_TEST_SUITE(query_context_tests, query_context_setup_fixture)

const auto events = [](auto, auto) {};

BOOST_AUTO_TEST_CASE(query_context__get_chain_state__genesis__expected)
{
    const system::settings system_settings{};
    database::settings database_settings{};
    database_settings.path = TEST_DIRECTORY;
    test::chunk_store store{ database_settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(events), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));

    const auto state = query.get_chain_state(system_settings);
    BOOST_REQUIRE(state);
    BOOST_REQUIRE_EQUAL(state->height(), zero);
}

BOOST_AUTO_TEST_SUITE_END()
