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

struct query_optional_setup_fixture
{
    DELETE4(query_optional_setup_fixture);
    BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

    query_optional_setup_fixture() NOEXCEPT
    {
        BOOST_REQUIRE(test::clear(test::directory));
    }

    ~query_optional_setup_fixture() NOEXCEPT
    {
        BOOST_REQUIRE(test::clear(test::directory));
    }

    BC_POP_WARNING()
};

BOOST_FIXTURE_TEST_SUITE(query_optional_tests, query_optional_setup_fixture)

/////// Address (natural-keyed).
/////// Terminal implies not found, false implies fault.
////hash_digest address_hash(const output& output) NOEXCEPT;
////output_link get_address(const hash_digest& key) NOEXCEPT;
////bool set_address(const hash_digest& key, const output_link& link) NOEXCEPT;
////bool set_address(const output& output) NOEXCEPT;
////
/////// Neutrino (foreign-keyed).
/////// Empty/null_hash implies not found, false implies fault.
////filter get_filter(const header_link& link) NOEXCEPT;
////hash_digest get_filter_head(const header_link& link) NOEXCEPT;
////bool set_filter(const header_link& link, const hash_digest& head,
////    const filter& body) NOEXCEPT;
////
/////// Buffer (foreign-keyed).
/////// Null implies not found, false implies fault.
////transaction::cptr get_buffered_tx(const tx_link& link) NOEXCEPT;
////bool set_buffered_tx(const tx_link& link, const transaction& tx) NOEXCEPT;
////
/////// Bootstrap (natural-keyed).
/////// Empty implies empty table, false implies height exceeds confirmed top.
////hashes get_bootstrap() NOEXCEPT;
////bool set_bootstrap(size_t height) NOEXCEPT;

BOOST_AUTO_TEST_CASE(query_optional_test)
{
    BOOST_REQUIRE(true);
}

BOOST_AUTO_TEST_SUITE_END()
