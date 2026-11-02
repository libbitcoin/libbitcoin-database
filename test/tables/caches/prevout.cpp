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
#include "../../test.hpp"
#include "../../mocks/blocks.hpp"
#include "../../mocks/chunk_storage.hpp"

BC_AUTO_TEST_SUITE(prevout_tests)

using namespace system;
using namespace system::chain;
using tx = schema::transaction::link;
const std::vector<tx::integer> conflicts1{ 0x01020304 };
const std::vector<tx::integer> conflicts2{ 0xbaadf00d, 0x01020304 };

BC_AUTO_TEST_CASE(prevout__put__at1__expected)
{
    const table::prevout::slab_put_ref slab1{ {}, conflicts1, test::bogus_block };
    const table::prevout::slab_put_ref slab2{ {}, conflicts2, test::bogus_block };
    test::chunk_storage head_store{ test::terminal_fill };
    test::chunk_storage body_store{};
    table::prevout instance{ head_store, body_store, 5 };
    BC_REQUIRE(instance.create());

    // Put at key.
    BC_REQUIRE(instance.put(3, slab1));
    BC_REQUIRE(instance.put(42, slab2));

    // Dereference at key to get link.
    BC_REQUIRE_EQUAL(instance.at(3), 0u);
    BC_REQUIRE_EQUAL(instance.at(42), 37u);
    BC_REQUIRE(instance.at(0).is_terminal());
    BC_REQUIRE(instance.at(1).is_terminal());
    BC_REQUIRE(instance.at(2).is_terminal());
    BC_REQUIRE(instance.at(4).is_terminal());
}

BC_AUTO_TEST_CASE(prevout__put__at2__expected)
{
    const table::prevout::slab_put_ref slab1{ {}, conflicts1, test::bogus_block };
    const table::prevout::slab_put_ref slab2{ {}, conflicts2, test::bogus_block };
    table::prevout::slab_get element{};
    test::chunk_storage head_store{ test::terminal_fill };
    test::chunk_storage body_store{};
    table::prevout instance{ head_store, body_store, 5 };
    BC_REQUIRE(instance.create());

    // Put at key.
    BC_REQUIRE(instance.put(3, slab1));
    BC_REQUIRE(instance.put(42, slab2));

    // Dereference at key to get element.
    BC_REQUIRE(instance.at(3, element));
    BC_REQUIRE(element.conflicts == slab1.conflicts);
    ////BC_REQUIRE(element.spends == spends1);
    BC_REQUIRE(instance.at(42, element));
    BC_REQUIRE(element.conflicts == slab2.conflicts);
    ////BC_REQUIRE(element.spends == spends2);
    BC_REQUIRE(!instance.at(0, element));
    BC_REQUIRE(!instance.at(1, element));
    BC_REQUIRE(!instance.at(2, element));
    BC_REQUIRE(!instance.at(4, element));
}

BC_AUTO_TEST_CASE(prevout__put__exists__expected)
{
    const table::prevout::slab_put_ref slab1{ {}, conflicts1, test::bogus_block };
    const table::prevout::slab_put_ref slab2{ {}, conflicts2, test::bogus_block };
    test::chunk_storage head_store{ test::terminal_fill };
    test::chunk_storage body_store{};
    table::prevout instance{ head_store, body_store, 5 };
    BC_REQUIRE(instance.create());

    // Put at key.
    BC_REQUIRE(instance.put(3, slab1));
    BC_REQUIRE(instance.put(42, slab2));

    // Exists at key.
    BC_REQUIRE(instance.exists(3));
    BC_REQUIRE(instance.exists(42));
    BC_REQUIRE(!instance.exists(0));
    BC_REQUIRE(!instance.exists(1));
    BC_REQUIRE(!instance.exists(2));
    BC_REQUIRE(!instance.exists(4));
}

BC_AUTO_TEST_CASE(prevout__put__get__expected)
{
    const table::prevout::slab_put_ref slab1{ {}, conflicts1, test::bogus_block };
    const table::prevout::slab_put_ref slab2{ {}, conflicts2, test::bogus_block };
    table::prevout::slab_get element{};
    test::chunk_storage head_store{ test::terminal_fill };
    test::chunk_storage body_store{};
    table::prevout instance{ head_store, body_store, 5 };
    BC_REQUIRE(instance.create());

    // Put at key.
    BC_REQUIRE(instance.put(3, slab1));
    BC_REQUIRE_EQUAL(instance.at(3), 0u);
    BC_REQUIRE(instance.put(42, slab2));
    BC_REQUIRE_EQUAL(instance.at(42), 37u);

    // Get at link.
    BC_REQUIRE(instance.get(0, element));
    BC_REQUIRE(element.conflicts == slab1.conflicts);
    BC_REQUIRE(instance.get(37u, element));
    BC_REQUIRE(element.conflicts == slab2.conflicts);
}

BC_AUTO_TEST_SUITE_END()
