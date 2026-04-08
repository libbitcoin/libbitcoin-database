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
#include "../../test.hpp"
#include "../../mocks/blocks.hpp"
#include "../../mocks/chunk_store.hpp"

BOOST_FIXTURE_TEST_SUITE(query_address_tests, test::directory_setup_fixture)

// get_confirmed_unspent_outputs

BOOST_AUTO_TEST_CASE(query_address__get_confirmed_unspent_outputs__turbo_genesis__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(test::events_handler), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));

    outpoints out{};
    std::atomic_bool cancel{};
    BOOST_REQUIRE(!query.get_confirmed_unspent_outputs(cancel, out, test::genesis_address, true));
    BOOST_REQUIRE_EQUAL(out.size(), 1u);
    BOOST_REQUIRE(*out.begin() == query.get_outpoint(query.to_output(0, 0)));
}

BOOST_AUTO_TEST_CASE(query_address__get_confirmed_unspent_outputs__genesis__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(test::events_handler), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));

    outpoints out{};
    std::atomic_bool cancel{};
    BOOST_REQUIRE(!query.get_confirmed_unspent_outputs(cancel, out, test::genesis_address));
    BOOST_REQUIRE_EQUAL(out.size(), 1u);
    BOOST_REQUIRE(*out.begin() == query.get_outpoint(query.to_output(0, 0)));
}

// get_minimum_unspent_outputs

BOOST_AUTO_TEST_CASE(query_address__get_minimum_unspent_outputs__turbo_above__excluded)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(test::events_handler), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));

    outpoints out{};
    std::atomic_bool cancel{};
    BOOST_REQUIRE(!query.get_minimum_unspent_outputs(cancel, out, test::genesis_address, 5000000001, true));
    BOOST_REQUIRE(out.empty());
}

BOOST_AUTO_TEST_CASE(query_address__get_minimum_unspent_outputs__above__excluded)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(test::events_handler), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));

    outpoints out{};
    std::atomic_bool cancel{};
    BOOST_REQUIRE(!query.get_minimum_unspent_outputs(cancel, out, test::genesis_address, 5000000001));
    BOOST_REQUIRE(out.empty());
}

BOOST_AUTO_TEST_CASE(query_address__get_minimum_unspent_outputs__at__included)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(test::events_handler), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));

    outpoints out{};
    std::atomic_bool cancel{};
    BOOST_REQUIRE(!query.get_minimum_unspent_outputs(cancel, out, test::genesis_address, 5000000000));
    BOOST_REQUIRE_EQUAL(out.size(), 1u);
    BOOST_REQUIRE(*out.begin() == query.get_outpoint(query.to_output(0, 0)));
}

BOOST_AUTO_TEST_CASE(query_address__get_minimum_unspent_outputs__below__included)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(test::events_handler), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));

    outpoints out{};
    std::atomic_bool cancel{};
    BOOST_REQUIRE(!query.get_minimum_unspent_outputs(cancel, out, test::genesis_address, 0));
    BOOST_REQUIRE_EQUAL(out.size(), 1u);
    BOOST_REQUIRE(*out.begin() == query.get_outpoint(query.to_output(0, 0)));

    BOOST_REQUIRE(!query.get_minimum_unspent_outputs(cancel, out, test::genesis_address, 4999999999));
    BOOST_REQUIRE_EQUAL(out.size(), 1u);
    BOOST_REQUIRE(*out.begin() == query.get_outpoint(query.to_output(0, 0)));
}

// get_address_outputs

BOOST_AUTO_TEST_CASE(query_address__get_address_outputs__turbo_genesis__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(test::events_handler), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));

    outpoints out{};
    std::atomic_bool cancel{};
    BOOST_REQUIRE(!query.get_address_outputs(cancel, out, test::genesis_address, true));
    BOOST_REQUIRE_EQUAL(out.size(), 1u);
    BOOST_REQUIRE(*out.begin() == query.get_outpoint(query.to_output(0, 0)));
}

BOOST_AUTO_TEST_CASE(query_address__get_address_outputs__genesis__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(test::events_handler), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));

    outpoints out{};
    std::atomic_bool cancel{};
    BOOST_REQUIRE(!query.get_address_outputs(cancel, out, test::genesis_address));
    BOOST_REQUIRE_EQUAL(out.size(), 1u);
    BOOST_REQUIRE(*out.begin() == query.get_outpoint(query.to_output(0, 0)));
}

BOOST_AUTO_TEST_CASE(query_address__get_address_outputs__cancel__canceled_false)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(test::events_handler), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));

    outpoints out{};
    std::atomic_bool cancel{ true };
    BOOST_REQUIRE_EQUAL(query.get_address_outputs(cancel, out, test::genesis_address), error::canceled);
    BOOST_REQUIRE(out.empty());
}

BOOST_AUTO_TEST_SUITE_END()
