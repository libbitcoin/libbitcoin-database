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

BC_AUTO_TEST_SUITE(pool_tests)

using namespace system;
using word = table::pool_word;
using body_storages = test::chunk_storages<schema::pool::minrow,
    schema::pool_id0::size, schema::pool_id0::size,
    schema::pool_id0::size, schema::pool_id0::size>;
static const body_storages::paths body_paths
{
    "state", "id0", "id1", "id2", "id3"
};

constexpr auto buckets = 8u;
const table::pool::key key1{ 0x01, 0x02, 0x03, 0x04 };
const table::pool::key key2{ 0xa1, 0xa2, 0xa3, 0xa4 };
const table::pool::record in1
{
    {},
    {
        0x11223344, // flags
        0x00667788, // height
        0x99aabbcc  // mtp
    },
    0x1122334455667788, // fee
    0x00345678,         // sigops
    0x01020304,         // spends_fk
    0x5f6e7d8c          // time
};
const table::pool::record in2
{
    {},
    {
        0xaabbccdd, // flags
        0x00332211, // height
        0xabcdef99  // mtp
    },
    0x0000000000000042, // fee
    0x00000055,         // sigops
    0x0a0b0c0d,         // spends_fk
    0x00000042          // time
};

BC_AUTO_TEST_CASE(pool__put__two__found)
{
    test::chunk_storage head_store{ test::terminal_fill };
    body_storages body_store{ body_paths };
    table::pool instance{ head_store, body_store, buckets };
    BC_REQUIRE(instance.create());

    table::pool::link link1{};
    table::pool::link link2{};
    BC_REQUIRE(instance.put_link(link1, key1, in1));
    BC_REQUIRE(instance.put_link(link2, key2, in2));
    BC_REQUIRE_EQUAL(link1, 0u);
    BC_REQUIRE_EQUAL(link2, 1u);
    BC_REQUIRE_EQUAL(instance.count(), 2u);
    BC_REQUIRE_EQUAL(instance.first(key1), link1);
    BC_REQUIRE_EQUAL(instance.first(key2), link2);

    table::pool::record out{};
    BC_REQUIRE(instance.get(link1, out));
    BC_REQUIRE(out == in1);
    BC_REQUIRE(instance.get(link2, out));
    BC_REQUIRE(out == in2);
}

BC_AUTO_TEST_CASE(pool__put__spine__expected_row)
{
    test::chunk_storage head_store{ test::terminal_fill };
    body_storages body_store{ body_paths };
    table::pool instance{ head_store, body_store, buckets };
    BC_REQUIRE(instance.create());
    BC_REQUIRE(instance.put(key1, in1));

    const auto expected_row = base16_chunk
    (
        "ffffffff"         // next->end
        "01020304"         // key1
        "44332211"         // flags
        "887766"           // height
        "ccbbaa99"         // mtp
        "8877665544332211" // fee
        "785634"           // sigops
        "04030201"         // spends_fk
        "8c7d6e5f"         // time
    );
    BC_REQUIRE_EQUAL(body_store.buffers_.at(0), expected_row);
}

BC_AUTO_TEST_CASE(pool__put__columns_then_commit__expected)
{
    test::chunk_storage head_store{ test::terminal_fill };
    body_storages body_store{ body_paths };
    table::pool instance{ head_store, body_store, buckets };
    BC_REQUIRE(instance.create());

    const auto link = instance.allocate(1);
    BC_REQUIRE_EQUAL(link, 0u);

    auto guard = instance.get_memory();
    BC_REQUIRE(guard);
    BC_REQUIRE(instance.id0.put(link, word{ {}, 0x0706050403020100 }));
    BC_REQUIRE(instance.id1.put(link, word{ {}, 0x0f0e0d0c0b0a0908 }));
    BC_REQUIRE(instance.id2.put(link, word{ {}, 0x1716151413121110 }));
    BC_REQUIRE(instance.id3.put(link, word{ {}, 0x1f1e1d1c1b1a1918 }));
    guard.reset();

    BC_REQUIRE(instance.first(key1).is_terminal());
    BC_REQUIRE(instance.put(link, key1, in1));
    BC_REQUIRE_EQUAL(instance.first(key1), link);

    BC_REQUIRE_EQUAL(body_store.buffers_.at(1), base16_chunk("0001020304050607"));
    BC_REQUIRE_EQUAL(body_store.buffers_.at(2), base16_chunk("08090a0b0c0d0e0f"));
    BC_REQUIRE_EQUAL(body_store.buffers_.at(3), base16_chunk("1011121314151617"));
    BC_REQUIRE_EQUAL(body_store.buffers_.at(4), base16_chunk("18191a1b1c1d1e1f"));

    word out{};
    BC_REQUIRE(instance.id2.get(link, out));
    BC_REQUIRE_EQUAL(out.word, 0x1716151413121110u);
}

BC_AUTO_TEST_SUITE_END()
