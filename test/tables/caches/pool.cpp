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
    schema::pool_id0::size, schema::pool_id0::size,
    schema::pool_totals::size, schema::pool_edges::size>;
static const body_storages::paths body_paths
{
    "state", "id0", "id1", "id2", "id3", "totals", "edges"
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

BC_AUTO_TEST_CASE(pool__put__scan_columns__expected)
{
    test::chunk_storage head_store{ test::terminal_fill };
    body_storages body_store{ body_paths };
    table::pool instance{ head_store, body_store, buckets };
    BC_REQUIRE(instance.create());

    const auto link = instance.allocate(1);
    BC_REQUIRE_EQUAL(link, 0u);

    const table::pool_total total{ {}, 0x1122334455667788, 0x00000190, 0x00000005 };
    const table::pool_edge edge{ {}, 0x01020304, 0x00000003 };
    auto guard = instance.get_memory();
    BC_REQUIRE(guard);
    BC_REQUIRE(instance.totals.put(link, total));
    BC_REQUIRE(instance.edges.put(link, edge));
    guard.reset();

    BC_REQUIRE(instance.put(link, key1, in1));
    BC_REQUIRE_EQUAL(body_store.buffers_.at(5), base16_chunk("88776655443322119001000005000000"));
    BC_REQUIRE_EQUAL(body_store.buffers_.at(6), base16_chunk("0403020103000000"));

    table::pool_total total_out{};
    table::pool_edge edge_out{};
    BC_REQUIRE(instance.totals.get(link, total_out));
    BC_REQUIRE(instance.edges.get(link, edge_out));
    BC_REQUIRE(total_out == total);
    BC_REQUIRE(edge_out == edge);
}

// value heads
// ----------------------------------------------------------------------------

BC_AUTO_TEST_CASE(pool_cells__set__two__expected)
{
    data_chunk head_file{};
    test::chunk_storage head_store{ head_file };
    table::pool_cells instance{ head_store };
    const auto expected = base16_chunk("00000000" "ffffff3f" "00000040");
    BC_REQUIRE(instance.create());
    BC_REQUIRE(instance.set(1u, table::pool_cells::replace{ {}, 0x3fffffff }));
    BC_REQUIRE(instance.set(2u, table::pool_cells::replace{ {}, 0x40000000 }));
    BC_REQUIRE_EQUAL(instance.count(), 3u);
    BC_REQUIRE_EQUAL(instance.at(0u), 0u);
    BC_REQUIRE_EQUAL(instance.at(1u), 0x3fffffffu);
    BC_REQUIRE_EQUAL(instance.at(2u), 0x40000000u);
    BC_REQUIRE_EQUAL(head_file, expected);
}

BC_AUTO_TEST_CASE(pool_children__set__replace__expected)
{
    data_chunk head_file{};
    test::chunk_storage head_store{ head_file };
    table::pool_children instance{ head_store };
    BC_REQUIRE(instance.create());
    BC_REQUIRE(instance.set(0u, table::pool_children::replace{ {}, 0xffffffff }));
    BC_REQUIRE(instance.set(0u, table::pool_children::replace{ {}, 0x00000007 }));
    BC_REQUIRE_EQUAL(instance.count(), 1u);
    BC_REQUIRE_EQUAL(instance.at(0u), 7u);
}

BC_AUTO_TEST_SUITE_END()
