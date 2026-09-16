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
#include "../test.hpp"
#include "../mocks/chunk_storage.hpp"

BOOST_AUTO_TEST_SUITE(headmap_tests)

using namespace system;

using link = linkage<2>;
using test_headmap = headmap<link, sizeof(uint16_t)>;
using small_link = linkage<3>;
using unaligned_headmap = headmap<small_link, small_link::size>;
using slab_link = linkage<5>;
using slab_headmap = headmap<slab_link, max_size_t>;

class little_slab
{
public:
    static constexpr size_t size = max_size_t;
    static constexpr slab_link count() NOEXCEPT { return sizeof(uint32_t); }

    bool from_data(database::reader& source) NOEXCEPT
    {
        value = source.read_little_endian<uint32_t>();
        return source;
    }

    bool to_data(database::flipper& sink) const NOEXCEPT
    {
        sink.write_little_endian(value);
        return sink;
    }

    uint32_t value{ 0 };
};

BOOST_AUTO_TEST_CASE(headmap__create__empty__expected)
{
    data_chunk head_file{};
    test::chunk_storage head_store{ head_file };
    test_headmap instance{ head_store };
    BOOST_REQUIRE(instance.create());
    BOOST_REQUIRE(instance.verify());
    BOOST_REQUIRE(head_file.empty());
    BOOST_REQUIRE_EQUAL(instance.head_size(), zero);
    BOOST_REQUIRE_EQUAL(instance.body_size(), zero);
    BOOST_REQUIRE_EQUAL(instance.count(), 0u);
    BOOST_REQUIRE(instance.at(0).is_terminal());
}

BOOST_AUTO_TEST_CASE(headmap__push__reserved__expected_at)
{
    data_chunk head_file{};
    test::chunk_storage head_store{ head_file };
    test_headmap instance{ head_store };
    BOOST_REQUIRE(instance.create());
    BOOST_REQUIRE(!instance.push(link::terminal));
    BOOST_REQUIRE(instance.reserve(1u));
    BOOST_REQUIRE(instance.push(0x1122_u16));
    BOOST_REQUIRE(instance.reserve(1u));
    BOOST_REQUIRE(instance.push(0x3344_u16));
    BOOST_REQUIRE_EQUAL(instance.count(), 2u);
    BOOST_REQUIRE_EQUAL(instance.at(0), 0x1122_u16);
    BOOST_REQUIRE_EQUAL(instance.at(1), 0x3344_u16);
    BOOST_REQUIRE(instance.at(2).is_terminal());
    BOOST_REQUIRE_EQUAL(head_file, base16_chunk("22114433"));
}

BOOST_AUTO_TEST_CASE(headmap__push__unreserved__false)
{
    data_chunk head_file{};
    test::chunk_storage head_store{ head_file };
    test_headmap instance{ head_store };
    BOOST_REQUIRE(instance.create());
    BOOST_REQUIRE(!instance.push(0x1122_u16));
    BOOST_REQUIRE_EQUAL(instance.count(), 0u);
}

BOOST_AUTO_TEST_CASE(headmap__truncate__push__rewrites_bucket)
{
    data_chunk head_file{};
    test::chunk_storage head_store{ head_file };
    test_headmap instance{ head_store };
    BOOST_REQUIRE(instance.create());
    BOOST_REQUIRE(instance.reserve(2u));
    BOOST_REQUIRE(instance.push(0x1122_u16));
    BOOST_REQUIRE(instance.push(0x3344_u16));
    BOOST_REQUIRE(!instance.truncate(3u));
    BOOST_REQUIRE(instance.truncate(1u));
    BOOST_REQUIRE_EQUAL(instance.count(), 1u);
    BOOST_REQUIRE(instance.at(1).is_terminal());
    BOOST_REQUIRE(instance.reserve(1u));
    BOOST_REQUIRE(instance.push(0x5566_u16));
    BOOST_REQUIRE_EQUAL(instance.count(), 2u);
    BOOST_REQUIRE_EQUAL(instance.at(1), 0x5566_u16);
}

BOOST_AUTO_TEST_CASE(headmap__unaligned__push_at_truncate__expected)
{
    data_chunk head_file{};
    test::chunk_storage head_store{ head_file };
    unaligned_headmap instance{ head_store };
    BOOST_REQUIRE(instance.create());
    BOOST_REQUIRE(instance.reserve(2u));
    BOOST_REQUIRE(instance.push(0x00112233_u32));
    BOOST_REQUIRE(instance.push(0x00445566_u32));
    BOOST_REQUIRE_EQUAL(instance.count(), 2u);
    BOOST_REQUIRE_EQUAL(instance.at(0), 0x00112233_u32);
    BOOST_REQUIRE_EQUAL(instance.at(1), 0x00445566_u32);
    BOOST_REQUIRE(instance.at(2).is_terminal());
    BOOST_REQUIRE_EQUAL(head_file, base16_chunk("332211665544"));
    BOOST_REQUIRE(instance.truncate(1u));
    BOOST_REQUIRE(instance.at(1).is_terminal());
}

BOOST_AUTO_TEST_CASE(headmap__setup__lifecycle__expected)
{
    data_chunk head_file{};
    test::chunk_storage head_store{ head_file };
    test_headmap instance{ head_store };
    BOOST_REQUIRE(instance.create());
    BOOST_REQUIRE(instance.reserve(1u));
    BOOST_REQUIRE(instance.push(0x1122_u16));
    BOOST_REQUIRE(instance.close());
    BOOST_REQUIRE(instance.backup());
    BOOST_REQUIRE(instance.restore());
    BOOST_REQUIRE(instance.verify());
    BOOST_REQUIRE_EQUAL(instance.count(), 1u);
    BOOST_REQUIRE(!instance.get_fault());
}

// slab

BOOST_AUTO_TEST_CASE(headmap__slab_create__empty__expected)
{
    data_chunk head_file{};
    test::chunk_storage head_store{ head_file };
    slab_headmap instance{ head_store };
    BOOST_REQUIRE(instance.create());
    BOOST_REQUIRE(instance.verify());
    BOOST_REQUIRE_EQUAL(instance.head_size(), zero);
    BOOST_REQUIRE_EQUAL(instance.body_size(), zero);
}

BOOST_AUTO_TEST_CASE(headmap__slab_put__allocated__expected_get)
{
    data_chunk head_file{};
    test::chunk_storage head_store{ head_file };
    slab_headmap instance{ head_store };
    BOOST_REQUIRE(instance.create());

    const auto link = instance.allocate(little_slab::count());
    BOOST_REQUIRE(!link.is_terminal());
    BOOST_REQUIRE_EQUAL(link, 0u);
    BOOST_REQUIRE_EQUAL(instance.head_size(), sizeof(uint32_t));

    const little_slab in{ 0x11223344_u32 };
    BOOST_REQUIRE(instance.put(link, in));

    little_slab out{};
    BOOST_REQUIRE(instance.get(link, out));
    BOOST_REQUIRE_EQUAL(out.value, in.value);
}

// The link is a byte offset, as with a body slab.
BOOST_AUTO_TEST_CASE(headmap__slab_put__second_element__expected_get)
{
    data_chunk head_file{};
    test::chunk_storage head_store{ head_file };
    slab_headmap instance{ head_store };
    BOOST_REQUIRE(instance.create());

    const auto first = instance.allocate(little_slab::count());
    const auto second = instance.allocate(little_slab::count());
    BOOST_REQUIRE_EQUAL(first, 0u);
    BOOST_REQUIRE_EQUAL(second, sizeof(uint32_t));

    BOOST_REQUIRE(instance.put(first, little_slab{ 0x01020304_u32 }));
    BOOST_REQUIRE(instance.put(second, little_slab{ 0x05060708_u32 }));

    little_slab out{};
    BOOST_REQUIRE(instance.get(first, out));
    BOOST_REQUIRE_EQUAL(out.value, 0x01020304_u32);
    BOOST_REQUIRE(instance.get(second, out));
    BOOST_REQUIRE_EQUAL(out.value, 0x05060708_u32);
}

// The element is rewritten in place, as the head is not appended.
BOOST_AUTO_TEST_CASE(headmap__slab_put__rewrite__expected_get)
{
    data_chunk head_file{};
    test::chunk_storage head_store{ head_file };
    slab_headmap instance{ head_store };
    BOOST_REQUIRE(instance.create());

    const auto link = instance.allocate(little_slab::count());
    BOOST_REQUIRE(instance.put(link, little_slab{ 0x11223344_u32 }));
    BOOST_REQUIRE(instance.put(link, little_slab{ 0x55667788_u32 }));
    BOOST_REQUIRE_EQUAL(instance.head_size(), sizeof(uint32_t));

    little_slab out{};
    BOOST_REQUIRE(instance.get(link, out));
    BOOST_REQUIRE_EQUAL(out.value, 0x55667788_u32);
}

BOOST_AUTO_TEST_CASE(headmap__slab_get__terminal__false)
{
    data_chunk head_file{};
    test::chunk_storage head_store{ head_file };
    slab_headmap instance{ head_store };
    BOOST_REQUIRE(instance.create());

    little_slab out{};
    BOOST_REQUIRE(!instance.get(slab_link::terminal, out));
    BOOST_REQUIRE(!instance.put(slab_link::terminal, little_slab{}));
}

BOOST_AUTO_TEST_CASE(headmap__slab_get__unallocated__false)
{
    data_chunk head_file{};
    test::chunk_storage head_store{ head_file };
    slab_headmap instance{ head_store };
    BOOST_REQUIRE(instance.create());

    little_slab out{};
    BOOST_REQUIRE(!instance.get(0u, out));
    BOOST_REQUIRE(!instance.put(0u, little_slab{}));
}

BOOST_AUTO_TEST_CASE(headmap__slab_setup__lifecycle__expected)
{
    data_chunk head_file{};
    test::chunk_storage head_store{ head_file };
    slab_headmap instance{ head_store };
    BOOST_REQUIRE(instance.create());

    const auto link = instance.allocate(little_slab::count());
    BOOST_REQUIRE(instance.put(link, little_slab{ 0x11223344_u32 }));
    BOOST_REQUIRE(instance.close());
    BOOST_REQUIRE(instance.backup());
    BOOST_REQUIRE(instance.restore());
    BOOST_REQUIRE(instance.verify());
    BOOST_REQUIRE(!instance.get_fault());

    little_slab out{};
    BOOST_REQUIRE(instance.get(link, out));
    BOOST_REQUIRE_EQUAL(out.value, 0x11223344_u32);
}

BOOST_AUTO_TEST_SUITE_END()
