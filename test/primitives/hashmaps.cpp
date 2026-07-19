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
#include "../test.hpp"
#include "../mocks/chunk_storage.hpp"

BOOST_AUTO_TEST_SUITE(hashmaps_tests)

using namespace system;
using link5 = linkage<5>;
using key1 = data_array<1>;

constexpr auto buckets = 16_size;
constexpr auto head_size = add1(buckets) * link5::size;

// Column zero is the keyed spine, column one an 8 byte satellite record.
constexpr auto spine_size = sizeof(uint32_t);
constexpr auto satellite_size = sizeof(uint64_t);
constexpr auto spine_row = link5::size + array_count<key1> + spine_size;

using table = hashmaps<link5, key1, spine_size, link5::size, satellite_size>;
using body_storages = test::chunk_storages<spine_row, satellite_size>;
static const body_storages::paths body_paths{ "spine", "satellite" };

static_assert(table::width<0> == spine_size);
static_assert(table::width<1> == satellite_size);

class little_record
{
public:
    // record bytes or zero for slab (for template).
    static constexpr size_t size = spine_size;

    // record count or bytes count for slab (for allocate).
    static constexpr link5 count() NOEXCEPT { return 1; }

    bool from_data(database::reader& source) NOEXCEPT
    {
        value = source.read_little_endian<uint32_t>();
        return source;
    }

    bool to_data(database::finalizer& sink) const NOEXCEPT
    {
        sink.write_little_endian(value);
        return sink;
    }

    uint32_t value{ 0 };
};

class little_satellite
{
public:
    static constexpr size_t size = satellite_size;
    static constexpr link5 count() NOEXCEPT { return 1; }

    bool from_data(database::reader& source) NOEXCEPT
    {
        value = source.read_little_endian<uint64_t>();
        return source;
    }

    bool to_data(database::flipper& sink) const NOEXCEPT
    {
        sink.write_little_endian(value);
        return sink;
    }

    uint64_t value{ 0 };
};

// construct/create
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(hashmaps__construct__empty__expected)
{
    test::chunk_storage head_store{};
    body_storages body_store{ body_paths };
    const table instance{ head_store, body_store, buckets };
    BOOST_REQUIRE(body_store.buffer().empty());
    BOOST_REQUIRE_EQUAL(instance.count(), 0u);
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(hashmaps__create__empty__expected)
{
    test::chunk_storage head_store{};
    body_storages body_store{ body_paths };
    table instance{ head_store, body_store, buckets };
    BOOST_REQUIRE(instance.create());
    BOOST_REQUIRE(instance.verify());
    BOOST_REQUIRE(instance.enabled());
    BOOST_REQUIRE_EQUAL(instance.buckets(), buckets);
    BOOST_REQUIRE_EQUAL(instance.head_size(), head_size);
    BOOST_REQUIRE_EQUAL(instance.count(), 0u);
    BOOST_REQUIRE(!instance.get_fault());
}

// allocate
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(hashmaps__allocate__multiple__shared_row_count)
{
    test::chunk_storage head_store{};
    body_storages body_store{ body_paths };
    table instance{ head_store, body_store, buckets };
    BOOST_REQUIRE(instance.create());

    // One allocation expands all columns to the shared row count.
    BOOST_REQUIRE_EQUAL(instance.allocate(2), 0u);
    BOOST_REQUIRE_EQUAL(instance.count(), 2u);
    BOOST_REQUIRE_EQUAL(instance.body_size(), 2u * (spine_row + satellite_size));
    BOOST_REQUIRE_EQUAL(instance.allocate(1), 2u);
    BOOST_REQUIRE_EQUAL(instance.count(), 3u);
    BOOST_REQUIRE(!instance.get_fault());
}

// spine
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(hashmaps__spine_put__multiple__expected)
{
    test::chunk_storage head_store{};
    body_storages body_store{ body_paths };
    table instance{ head_store, body_store, buckets };
    BOOST_REQUIRE(instance.create());

    constexpr key1 key_first{ 0x41 };
    constexpr key1 key_second{ 0x42 };

    link5 link{};
    BOOST_REQUIRE(instance.put_link(link, key_first, little_record{ 0x04030201_u32 }));
    BOOST_REQUIRE(!link.is_terminal());
    BOOST_REQUIRE_EQUAL(link, 0u);

    link = instance.put_link(key_second, little_record{ 0x08070605_u32 });
    BOOST_REQUIRE(!link.is_terminal());
    BOOST_REQUIRE_EQUAL(link, 1u);

    BOOST_REQUIRE(instance.exists(key_first));
    BOOST_REQUIRE(instance.exists(key_second));
    BOOST_REQUIRE_EQUAL(instance.first(key_first), 0u);
    BOOST_REQUIRE_EQUAL(instance.first(key_second), 1u);
    BOOST_REQUIRE_EQUAL(instance.get_key(0), key_first);
    BOOST_REQUIRE_EQUAL(instance.get_key(1), key_second);

    little_record record{};
    BOOST_REQUIRE(instance.get(0, record));
    BOOST_REQUIRE_EQUAL(record.value, 0x04030201_u32);
    BOOST_REQUIRE(instance.find(key_second, record));
    BOOST_REQUIRE_EQUAL(record.value, 0x08070605_u32);

    // This expectation relies on the fact of no hash table conflict between 0x41 and 0x42.
    const data_chunk expected_spine
    {
        0xff, 0xff, 0xff, 0xff, 0xff,
        0x41,
        0x01, 0x02, 0x03, 0x04,

        0xff, 0xff, 0xff, 0xff, 0xff,
        0x42,
        0x05, 0x06, 0x07, 0x08
    };
    BOOST_REQUIRE_EQUAL(body_store.buffers_.at(0), expected_spine);

    // Spine put allocates the shared row, backfilling satellite columns.
    const data_chunk expected_satellite(2u * satellite_size, 0x00);
    BOOST_REQUIRE_EQUAL(body_store.buffers_.at(1), expected_satellite);
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(hashmaps__spine_put__duplicate_key__detected)
{
    test::chunk_storage head_store{};
    body_storages body_store{ body_paths };
    table instance{ head_store, body_store, buckets };
    BOOST_REQUIRE(instance.create());

    constexpr key1 key_twin{ 0x41 };

    // Allocate before ptr acquisition (allocation must not follow accessor).
    BOOST_REQUIRE_EQUAL(instance.allocate(2), 0u);

    const auto ptr = instance.get_memory();
    bool duplicate{ true };
    BOOST_REQUIRE(instance.put(duplicate, ptr, 0, key_twin, little_record{ 0x04030201_u32 }));
    BOOST_REQUIRE(!duplicate);
    BOOST_REQUIRE(instance.put(duplicate, ptr, 1, key_twin, little_record{ 0x08070605_u32 }));
    BOOST_REQUIRE(duplicate);
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(hashmaps__spine_it__duplicate_key__iterated)
{
    test::chunk_storage head_store{};
    body_storages body_store{ body_paths };
    table instance{ head_store, body_store, buckets };
    BOOST_REQUIRE(instance.create());

    constexpr key1 key_twin{ 0x41 };
    BOOST_REQUIRE(instance.put(key_twin, little_record{ 0x04030201_u32 }));
    BOOST_REQUIRE(instance.put(key_twin, little_record{ 0x08070605_u32 }));

    auto it = instance.it(key_twin);
    BOOST_REQUIRE(it);
    BOOST_REQUIRE_EQUAL(*it, 1u);
    BOOST_REQUIRE(it.advance());
    BOOST_REQUIRE_EQUAL(*it, 0u);
    BOOST_REQUIRE(!it.advance());
    it.reset();
    BOOST_REQUIRE(!instance.get_fault());
}

// satellites
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(hashmaps__satellite_get__terminal__false)
{
    test::chunk_storage head_store{};
    body_storages body_store{ body_paths };
    table instance{ head_store, body_store, buckets };
    BOOST_REQUIRE(instance.create());

    little_satellite satellite{};
    BOOST_REQUIRE(!instance.get<1>(link5::terminal, satellite));
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(hashmaps__satellite_put__spine_allocated__expected)
{
    test::chunk_storage head_store{};
    body_storages body_store{ body_paths };
    table instance{ head_store, body_store, buckets };
    BOOST_REQUIRE(instance.create());

    // Spine put allocates the shared row across all columns.
    constexpr key1 key_first{ 0x41 };
    link5 link{};
    BOOST_REQUIRE(instance.put_link(link, key_first, little_record{ 0x04030201_u32 }));
    BOOST_REQUIRE_EQUAL(link, 0u);

    // Satellite put writes into the spine-allocated row (guard required).
    {
        const auto guard = instance.get_memory<1>();
        BOOST_REQUIRE(instance.put<1>(link, little_satellite{ 0x1122334455667788_u64 }));
    }

    little_satellite satellite{};
    BOOST_REQUIRE(instance.get<1>(link, satellite));
    BOOST_REQUIRE_EQUAL(satellite.value, 0x1122334455667788_u64);

    const data_chunk expected_satellite
    {
        0x88, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11
    };
    BOOST_REQUIRE_EQUAL(body_store.buffers_.at(1), expected_satellite);
    BOOST_REQUIRE(!instance.get_fault());
}

// close/restore
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(hashmaps__close__populated__verifies)
{
    test::chunk_storage head_store{};
    body_storages body_store{ body_paths };
    table instance{ head_store, body_store, buckets };
    BOOST_REQUIRE(instance.create());

    constexpr key1 key_first{ 0x41 };
    BOOST_REQUIRE(instance.put(key_first, little_record{ 0x04030201_u32 }));
    BOOST_REQUIRE(instance.close());
    BOOST_REQUIRE(instance.verify());
    BOOST_REQUIRE(instance.restore());
    BOOST_REQUIRE_EQUAL(instance.count(), 1u);
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_SUITE_END()
