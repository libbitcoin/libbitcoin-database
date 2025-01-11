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
constexpr table::prevout::record record1{ {}, 0x01020304_u32 };
constexpr table::prevout::record record2{ {}, 0xbaadf00d_u32 };

BOOST_AUTO_TEST_CASE(prevout__put__at1__expected)
{
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

BOOST_AUTO_TEST_CASE(prevout__put__at2__expected)
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

BOOST_AUTO_TEST_CASE(prevout__put__exists__expected)
{
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

BOOST_AUTO_TEST_CASE(prevout__put__get__expected)
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

// values

BOOST_AUTO_TEST_CASE(prevout__put__isolated_values__expected)
{
    constexpr auto bits = sub1(to_bits(linkage<schema::tx>::size));

    test::chunk_storage head_store{};
    test::chunk_storage body_store{};
    table::prevout instance{ head_store, body_store, 5 };
    BOOST_REQUIRE(instance.create());

    constexpr auto cb_only = table::prevout::record{ {}, 0b10000000'00000000'00000000'00000000_u32 };
    BOOST_REQUIRE(instance.put(3, cb_only));

    constexpr auto tx_only = table::prevout::record{ {}, 0b01010101'01010101'01010101'01010101_u32 };
    BOOST_REQUIRE(instance.put(42, tx_only));

    table::prevout::record element1{};
    BOOST_REQUIRE(instance.at(3, element1));
    BOOST_REQUIRE(element1.coinbase());
    BOOST_REQUIRE_EQUAL(element1.coinbase(), cb_only.coinbase());
    BOOST_REQUIRE_EQUAL(element1.output_tx_fk(), cb_only.output_tx_fk());
    BOOST_REQUIRE_EQUAL(element1.output_tx_fk(), set_right(cb_only.value, bits, false));

    table::prevout::record element2{};
    BOOST_REQUIRE(instance.at(42, element2));
    BOOST_REQUIRE(!element2.coinbase());
    BOOST_REQUIRE_EQUAL(element2.coinbase(), tx_only.coinbase());
    BOOST_REQUIRE_EQUAL(element2.output_tx_fk(), tx_only.output_tx_fk());
    BOOST_REQUIRE_EQUAL(element2.output_tx_fk(), set_right(tx_only.value, bits, false));
}

BOOST_AUTO_TEST_CASE(prevout__put__merged_values__expected)
{
    test::chunk_storage head_store{};
    test::chunk_storage body_store{};
    table::prevout instance{ head_store, body_store, 5 };
    BOOST_REQUIRE(instance.create());

    constexpr auto expected_cb = true;
    constexpr auto expected_tx = 0b01010101'01010101'01010101'01010101_u32;
    auto record = table::prevout::record{ {}, 0_u32 };
    record.set(expected_cb, expected_tx);
    BOOST_REQUIRE(instance.put(3, record));

    table::prevout::record element{};
    BOOST_REQUIRE(instance.at(3, element));
    BOOST_REQUIRE_EQUAL(element.coinbase(), expected_cb);
    BOOST_REQUIRE_EQUAL(element.output_tx_fk(), expected_tx);
}

BOOST_AUTO_TEST_SUITE_END()
