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
#include "../../test.hpp"
#include "../../mocks/chunk_storage.hpp"

BOOST_AUTO_TEST_SUITE(prevout_tests)

using namespace system;
constexpr table::prevout::record record1{ {}, true, 0x01020304, 0xbaadf00d };
constexpr table::prevout::record record2{ {}, false, 0xbaadf00d, 0x01020304 };

BOOST_AUTO_TEST_CASE(header__put__at1__expected)
{
    table::prevout::record element{};
    test::chunk_storage head_store{};
    test::chunk_storage body_store{};
    table::prevout instance{ head_store, body_store, 5 };
    BOOST_REQUIRE(instance.create());

    // Put at key.
    BOOST_REQUIRE(instance.put(3, record1));
    BOOST_REQUIRE(instance.put(42, record2));

    // Dereference at key to get link.
    BOOST_REQUIRE(instance.at(0).is_terminal());
    BOOST_REQUIRE_EQUAL(instance.at(3), 0u);
    BOOST_REQUIRE_EQUAL(instance.at(42), 1u);
}

BOOST_AUTO_TEST_CASE(header__put__at2__expected)
{
    table::prevout::record element{};
    test::chunk_storage head_store{};
    test::chunk_storage body_store{};
    table::prevout instance{ head_store, body_store, 5 };
    BOOST_REQUIRE(instance.create());

    // Put at key.
    BOOST_REQUIRE(instance.put(3, record1));
    BOOST_REQUIRE(instance.put(42, record2));

    // Dereference at key to get element.
    BOOST_REQUIRE(!instance.at(0, element));
    BOOST_REQUIRE(instance.at(3, element));
    BOOST_REQUIRE(element == record1);
    BOOST_REQUIRE(instance.at(42, element));
    BOOST_REQUIRE(element == record2);
}

BOOST_AUTO_TEST_CASE(header__put__exists__expected)
{
    table::prevout::record element{};
    test::chunk_storage head_store{};
    test::chunk_storage body_store{};
    table::prevout instance{ head_store, body_store, 5 };
    BOOST_REQUIRE(instance.create());

    // Put at key.
    BOOST_REQUIRE(instance.put(3, record1));
    BOOST_REQUIRE(instance.put(42, record2));

    // Exists at key.
    BOOST_REQUIRE(!instance.exists(0));
    BOOST_REQUIRE(instance.exists(3));
    BOOST_REQUIRE(instance.exists(42));
}

BOOST_AUTO_TEST_CASE(header__put__get__expected)
{
    table::prevout::record element{};
    test::chunk_storage head_store{};
    test::chunk_storage body_store{};
    table::prevout instance{ head_store, body_store, 5 };
    BOOST_REQUIRE(instance.create());

    // Put at key.
    BOOST_REQUIRE(instance.put(3, record1));
    BOOST_REQUIRE(instance.put(42, record2));

    // Get at link.
    BOOST_REQUIRE(!instance.get(2, element));
    BOOST_REQUIRE(instance.get(0, element));
    BOOST_REQUIRE(element == record1);
    BOOST_REQUIRE(instance.get(1, element));
    BOOST_REQUIRE(element == record2);
}

BOOST_AUTO_TEST_SUITE_END()
