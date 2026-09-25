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

BOOST_AUTO_TEST_SUITE(validated_tx_tests)

using namespace system;
using word = table::validated_tx_word;
using body_storages = test::chunk_storages<schema::validated_tx::minrow,
    schema::validated_tx_id0::size, schema::validated_tx_id0::size,
    schema::validated_tx_id0::size, schema::validated_tx_id0::size>;
static const body_storages::paths body_paths
{
    "state", "id0", "id1", "id2", "id3"
};

constexpr auto buckets = 8u;
const table::validated_tx::key key1{ 0x01, 0x02, 0x03, 0x04 };
const table::validated_tx::key key2{ 0xa1, 0xa2, 0xa3, 0xa4 };
const table::validated_tx::record in1
{
    {},
    {
        0x11223344, // flags
        0x00667788, // height
        0x99aabbcc  // mtp
    },
    0x1122334455667788, // fee
    0x00345678,         // sigops
    0x01020304          // spends_fk
};
const table::validated_tx::record in2
{
    {},
    {
        0xaabbccdd, // flags
        0x00332211, // height
        0xabcdef99  // mtp
    },
    0x0000000000000042, // fee
    0x00000055,         // sigops
    0x0a0b0c0d          // spends_fk
};

BOOST_AUTO_TEST_CASE(validated_tx__put__two__found)
{
    test::chunk_storage head_store{};
    body_storages body_store{ body_paths };
    table::validated_tx instance{ head_store, body_store, buckets };
    BOOST_REQUIRE(instance.create());

    table::validated_tx::link link1{};
    table::validated_tx::link link2{};
    BOOST_REQUIRE(instance.put_link(link1, key1, in1));
    BOOST_REQUIRE(instance.put_link(link2, key2, in2));
    BOOST_REQUIRE_EQUAL(link1, 0u);
    BOOST_REQUIRE_EQUAL(link2, 1u);
    BOOST_REQUIRE_EQUAL(instance.count(), 2u);
    BOOST_REQUIRE_EQUAL(instance.first(key1), link1);
    BOOST_REQUIRE_EQUAL(instance.first(key2), link2);

    table::validated_tx::record out{};
    BOOST_REQUIRE(instance.get(link1, out));
    BOOST_REQUIRE(out == in1);
    BOOST_REQUIRE(instance.get(link2, out));
    BOOST_REQUIRE(out == in2);
}

BOOST_AUTO_TEST_CASE(validated_tx__put__spine__expected_row)
{
    test::chunk_storage head_store{};
    body_storages body_store{ body_paths };
    table::validated_tx instance{ head_store, body_store, buckets };
    BOOST_REQUIRE(instance.create());
    BOOST_REQUIRE(instance.put(key1, in1));

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
    );
    BOOST_REQUIRE_EQUAL(body_store.buffers_.at(0), expected_row);
}

BOOST_AUTO_TEST_CASE(validated_tx__put__columns_then_commit__expected)
{
    test::chunk_storage head_store{};
    body_storages body_store{ body_paths };
    table::validated_tx instance{ head_store, body_store, buckets };
    BOOST_REQUIRE(instance.create());

    const auto row = instance.allocate(1);
    BOOST_REQUIRE_EQUAL(row, 0u);

    auto guard = instance.get_memory();
    BOOST_REQUIRE(guard);
    BOOST_REQUIRE(instance.id0.put(row, word{ {}, 0x0706050403020100 }));
    BOOST_REQUIRE(instance.id1.put(row, word{ {}, 0x0f0e0d0c0b0a0908 }));
    BOOST_REQUIRE(instance.id2.put(row, word{ {}, 0x1716151413121110 }));
    BOOST_REQUIRE(instance.id3.put(row, word{ {}, 0x1f1e1d1c1b1a1918 }));
    guard.reset();

    BOOST_REQUIRE(instance.first(key1).is_terminal());
    BOOST_REQUIRE(instance.put(row, key1, in1));
    BOOST_REQUIRE_EQUAL(instance.first(key1), row);

    BOOST_REQUIRE_EQUAL(body_store.buffers_.at(1), base16_chunk("0001020304050607"));
    BOOST_REQUIRE_EQUAL(body_store.buffers_.at(2), base16_chunk("08090a0b0c0d0e0f"));
    BOOST_REQUIRE_EQUAL(body_store.buffers_.at(3), base16_chunk("1011121314151617"));
    BOOST_REQUIRE_EQUAL(body_store.buffers_.at(4), base16_chunk("18191a1b1c1d1e1f"));

    word out{};
    BOOST_REQUIRE(instance.id2.get(row, out));
    BOOST_REQUIRE_EQUAL(out.word, 0x1716151413121110u);
}

BOOST_AUTO_TEST_SUITE_END()
