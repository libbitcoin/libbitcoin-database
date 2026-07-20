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
#include "../../mocks/chunk_storage.hpp"

BOOST_AUTO_TEST_SUITE(ins_tests)

////ffffffff 0000000000000000000000000000000000000000000000000000000000000000 ffffff
////00000000 110102030405060708090a0b0c0d0e0f220102030405060708090a0b0c0d0e0f 420000

using namespace system;
constexpr auto hash = base16_array("110102030405060708090a0b0c0d0e0f220102030405060708090a0b0c0d0e0f");

using body_storages = test::chunk_storages<schema::ins::minrow,
    schema::ins_sequence::size>;
static const body_storages::paths body_paths{ "point", "sequence" };

const data_chunk expected_point_file
{
    // next
    0xff, 0xff, 0xff, 0xff,

    // hash/index
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xff, 0xff, 0xff,

    // --------------------------------------------------------------------------------------------

    // next
    0x00, 0x00, 0x00, 0x00,

    // hash/index
    0x11, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
    0x22, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
    0x42, 0x00, 0x00
};

BOOST_AUTO_TEST_CASE(ins__point_put__get__expected)
{
    test::chunk_storage head_store{};
    body_storages body_store{ body_paths };
    table::ins instance{ head_store, body_store, 5 };
    BOOST_REQUIRE(instance.create());

    table::ins::link link{};
    const system::chain::point null_point{ null_hash, 0x00ffffff_u32 };

    BOOST_REQUIRE(instance.put_link(link, null_point, table::ins_point::record{}));
    BOOST_REQUIRE_EQUAL(link, 0u);

    const system::chain::point key{ hash, 0x00000042_u32 };
    BOOST_REQUIRE(instance.put_link(link, key, table::ins_point::record{}));
    BOOST_REQUIRE_EQUAL(link, 1u);
    BOOST_REQUIRE_EQUAL(body_store.buffers_.at(0), expected_point_file);

    table::ins_point::record element{};
    BOOST_REQUIRE(instance.get(0, element));
    BOOST_REQUIRE_EQUAL(element.hash, null_hash);
    BOOST_REQUIRE(instance.get(1, element));
    BOOST_REQUIRE_EQUAL(element.hash, hash);
}

BOOST_AUTO_TEST_CASE(ins__sequence_put__get__expected)
{
    test::chunk_storage head_store{};
    body_storages body_store{ body_paths };
    table::ins instance{ head_store, body_store, 5 };
    BOOST_REQUIRE(instance.create());

    // Point put allocates the shared point/sequence row.
    table::ins::link link{};
    const system::chain::point key{ hash, 0x00000042_u32 };
    BOOST_REQUIRE(instance.put_link(link, key, table::ins_point::record{}));
    BOOST_REQUIRE_EQUAL(link, 0u);

    // Sequence put is unguarded, the accessor guards its rows against remap.
    auto ptr = instance.get_memory();
    BOOST_REQUIRE(instance.sequence.put(link, table::ins_sequence::record
    {
        {},
        0x01020304_u32,
        0x0505050505_u64,
        0x0a0b0c0d_u32
    }));

    ptr.reset();

    table::ins_sequence::record element{};
    BOOST_REQUIRE(instance.sequence.get(link, element));
    BOOST_REQUIRE_EQUAL(element.sequence, 0x01020304_u32);
    BOOST_REQUIRE_EQUAL(element.input_fk, 0x0505050505_u64);
    BOOST_REQUIRE_EQUAL(element.parent_fk, 0x0a0b0c0d_u32);

    const data_chunk expected_sequence_file
    {
        // sequence
        0x04, 0x03, 0x02, 0x01,

        // input_fk
        0x05, 0x05, 0x05, 0x05, 0x05,

        // parent_fk
        0x0d, 0x0c, 0x0b, 0x0a
    };
    BOOST_REQUIRE_EQUAL(body_store.buffers_.at(1), expected_sequence_file);
}

BOOST_AUTO_TEST_SUITE_END()
