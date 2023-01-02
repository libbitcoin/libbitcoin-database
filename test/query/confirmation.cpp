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

/////// These verify strong against confirmation height.
////bool is_candidate_block(const header_link& link) NOEXCEPT;
////bool is_confirmed_block(const header_link& link) NOEXCEPT;
////bool is_confirmed_tx(const tx_link& link) NOEXCEPT;
////bool is_confirmed_input(const input_link& link) NOEXCEPT;
////bool is_confirmed_output(const output_link& link) NOEXCEPT;
////
/////// These rely on stong (use only for confirmation process).
////bool is_spent(const input_link& link) NOEXCEPT;
////bool is_mature(const input_link& link, size_t height) NOEXCEPT;
////bool is_confirmable_block(const header_link& link, size_t height) NOEXCEPT;
////
/////// False implies fault if link associated.
////bool set_strong(const header_link& link) NOEXCEPT;
////bool set_unstrong(const header_link& link) NOEXCEPT;
////
/////// False implies fault.
////bool initialize(const block& genesis) NOEXCEPT;
////bool push_confirmed(const header_link& link) NOEXCEPT;
////bool push_candidate(const header_link& link) NOEXCEPT;
////bool pop_confirmed() NOEXCEPT;
////bool pop_candidate() NOEXCEPT;

BOOST_AUTO_TEST_CASE(query_confirmation_test)
{
    BOOST_REQUIRE(true);
}

BOOST_AUTO_TEST_SUITE_END()
