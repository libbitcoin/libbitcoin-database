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
#include "../../mocks/chunk_storage.hpp"

BC_AUTO_TEST_SUITE(outs_tests)

BC_AUTO_TEST_CASE(outs_test)
{
    BC_REQUIRE(true);
}

BC_AUTO_TEST_CASE(outs__allocate__address_disabled__spine_unbacked)
{
    using chunks = test::chunk_storages<0, 5>;
    test::chunk_storage head_store{ test::terminal_fill };
    chunks body_store{ chunks::paths{ "address", "puts" } };
    table::outs instance{ head_store, body_store, 0 };
    BC_REQUIRE(instance.create());

    constexpr auto rows = 2u;
    BC_REQUIRE_EQUAL(instance.allocate(rows), 0u);
    BC_REQUIRE_EQUAL(instance.count(), rows);

    // The unbacked address spine contributes no bytes to the aggregate.
    BC_REQUIRE_EQUAL(instance.body_size(), rows * schema::outs::minrow);
    BC_REQUIRE(body_store.buffer().empty());
    BC_REQUIRE_EQUAL(body_store.buffers_.at(one).size(), rows * schema::outs::minrow);
}

BC_AUTO_TEST_SUITE_END()
