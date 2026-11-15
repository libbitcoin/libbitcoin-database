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

BC_AUTO_TEST_SUITE(spends_tests)

using namespace system;
const table::spends::record in1{ {}, 0x12345678 };
const table::spends::record in2{ {}, 0xabcdef12 };
const auto expected_head = base16_chunk
(
    "00000000"
);
const auto closed_head = base16_chunk
(
    "02000000"
);
const auto expected_body = base16_chunk
(
    "78563412" // parent_fk1
    "12efcdab" // parent_fk2
);

BC_AUTO_TEST_CASE(spends__put__two__expected)
{
    test::chunk_storage head_store{};
    test::chunk_storage body_store{};
    table::spends instance{ head_store, body_store };
    BC_REQUIRE(instance.create());

    table::spends::link link1{};
    BC_REQUIRE(instance.put_link(link1, in1));
    BC_REQUIRE_EQUAL(link1, 0u);

    table::spends::link link2{};
    BC_REQUIRE(instance.put_link(link2, in2));
    BC_REQUIRE_EQUAL(link2, 1u);

    BC_REQUIRE_EQUAL(head_store.buffer(), expected_head);
    BC_REQUIRE_EQUAL(body_store.buffer(), expected_body);
    BC_REQUIRE(instance.close());
    BC_REQUIRE_EQUAL(head_store.buffer(), closed_head);
}

BC_AUTO_TEST_CASE(spends__get__two__expected)
{
    auto head = expected_head;
    auto body = expected_body;
    test::chunk_storage head_store{ head };
    test::chunk_storage body_store{ body };
    table::spends instance{ head_store, body_store };

    table::spends::record out{};
    BC_REQUIRE(instance.get(0u, out));
    BC_REQUIRE(out == in1);
    BC_REQUIRE(instance.get(1u, out));
    BC_REQUIRE(out == in2);
}

BC_AUTO_TEST_CASE(spends__put_refs__two__expected)
{
    using parents = table::spends::put_refs::parents;

    test::chunk_storage head_store{};
    test::chunk_storage body_store{};
    table::spends instance{ head_store, body_store };
    BC_REQUIRE(instance.create());

    const parents fks{ 0x12345678, 0xabcdef12 };
    BC_REQUIRE(instance.put(table::spends::put_refs{ {}, fks }));

    BC_REQUIRE_EQUAL(body_store.buffer(), expected_body);
    BC_REQUIRE(instance.close());
    BC_REQUIRE_EQUAL(head_store.buffer(), closed_head);
}

BC_AUTO_TEST_CASE(spends__put_refs_get_refs__second_run__expected)
{
    using put_parents = table::spends::put_refs::parents;
    using get_parents = table::spends::get_refs::parents;

    test::chunk_storage head_store{};
    test::chunk_storage body_store{};
    table::spends instance{ head_store, body_store };
    BC_REQUIRE(instance.create());

    const put_parents first{ 0x00000001, 0x00000002 };
    const put_parents second{ 0x00000003, 0x80000004, 0x00000005 };

    table::spends::link link1{};
    table::spends::link link2{};
    BC_REQUIRE(instance.put_link(link1, table::spends::put_refs{ {}, first }));
    BC_REQUIRE(instance.put_link(link2, table::spends::put_refs{ {}, second }));
    BC_REQUIRE_EQUAL(link1, 0u);
    BC_REQUIRE_EQUAL(link2, 2u);

    get_parents result(second.size());
    table::spends::get_refs reader{ {}, result };
    BC_REQUIRE(instance.get(link2, reader));
    BC_REQUIRE(result == second);
}

// value heads
// ----------------------------------------------------------------------------

BC_AUTO_TEST_CASE(spends_rows__set__two__expected)
{
    data_chunk head_file{};
    test::chunk_storage head_store{ head_file };
    table::spends_rows instance{ head_store };
    const auto expected = base16_chunk("07000000" "ffffffff");
    BC_REQUIRE(instance.create());
    BC_REQUIRE(instance.set(0u, table::spends_rows::replace{ {}, 0x00000007 }));
    BC_REQUIRE(instance.set(1u, table::spends_rows::replace{ {}, 0xffffffff }));
    BC_REQUIRE_EQUAL(instance.count(), 2u);
    BC_REQUIRE_EQUAL(instance.at(0u), 7u);
    BC_REQUIRE_EQUAL(instance.at(1u), 0xffffffffu);
    BC_REQUIRE_EQUAL(head_file, expected);
}

BC_AUTO_TEST_CASE(spends_rivals__set__ring__expected)
{
    data_chunk head_file{};
    test::chunk_storage head_store{ head_file };
    table::spends_rivals instance{ head_store };
    BC_REQUIRE(instance.create());
    BC_REQUIRE(instance.set(0u, table::spends_rivals::replace{ {}, 2u }));
    BC_REQUIRE(instance.set(1u, table::spends_rivals::replace{ {}, 0xffffffff }));
    BC_REQUIRE(instance.set(2u, table::spends_rivals::replace{ {}, 0u }));
    BC_REQUIRE_EQUAL(instance.count(), 3u);
    BC_REQUIRE_EQUAL(instance.at(0u), 2u);
    BC_REQUIRE_EQUAL(instance.at(1u), 0xffffffffu);
    BC_REQUIRE_EQUAL(instance.at(2u), 0u);
}

BC_AUTO_TEST_SUITE_END()
