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
#include "../test.hpp"
#include "../mocks/chunk_storage.hpp"

BOOST_AUTO_TEST_SUITE(arraymap_tests)

using namespace system;

template <typename Link, size_t Size>
class arraymap_
  : public arraymap<Link, Size>
{
public:
    using base = arraymap<Link, Size>;
    using arraymap<Link, Size>::arraymap;
};

using link3 = linkage<3>;
struct record4 { static constexpr size_t size = 4; };
using record_table = arraymap_<link3, record4::size>;

struct slab0 { static constexpr size_t size = max_size_t; };
using slab_table = arraymap_<link3, slab0::size>;

// Bucket count is one less than link count, due to header.size field.
constexpr auto initial_buckets = 18;
constexpr auto initial_header_size = add1(initial_buckets) * link3::size;
constexpr auto initial_links = initial_header_size / link3::size;
static_assert(initial_links == 19u);

////std::cout << head_store.buffer() << std::endl << std::endl;
////std::cout << body_store.buffer() << std::endl << std::endl;

// record arraymap_
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(arraymap__record_construct__empty__expected)
{
    test::chunk_storage head_store{};
    test::chunk_storage body_store{};
    const record_table instance{ head_store, body_store, initial_buckets };
    BOOST_REQUIRE(body_store.buffer().empty());
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(arraymap__record_construct__non_empty__expected)
{
    constexpr auto body_size = 12345u;
    test::chunk_storage head_store{};
    test::chunk_storage body_store{};
    body_store.buffer().resize(body_size);
    const record_table instance{ head_store, body_store, initial_buckets };
    BOOST_REQUIRE_EQUAL(body_store.buffer().size(), body_size);
    BOOST_REQUIRE(!instance.get_fault());
}
// slab arraymap_
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(arraymap___slab_construct__empty__expected)
{
    test::chunk_storage head_store{};
    test::chunk_storage body_store{};
    const slab_table instance{ head_store, body_store, initial_buckets };
    BOOST_REQUIRE(body_store.buffer().empty());
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(arraymap___slab_construct__non_empty__expected_enabled)
{
    constexpr auto body_size = 12345u;
    test::chunk_storage head_store{};
    test::chunk_storage body_store{};
    body_store.buffer().resize(body_size);
    const slab_table instance{ head_store, body_store, initial_buckets };
    BOOST_REQUIRE_EQUAL(body_store.buffer().size(), body_size);
    BOOST_REQUIRE(instance.enabled());
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(arraymap___enabled__non_empty_slab_zero_buckets__false)
{
    constexpr auto body_size = 12345u;
    test::chunk_storage head_store{};
    test::chunk_storage body_store{};
    body_store.buffer().resize(body_size);
    const slab_table instance{ head_store, body_store, 0 };
    BOOST_REQUIRE(!instance.enabled());
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(arraymap___enabled__empty_slab_one_bucket__false)
{
    test::chunk_storage head_store{};
    test::chunk_storage body_store{};
    slab_table instance{ head_store, body_store, 1 };
    BOOST_REQUIRE(!instance.enabled());
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(arraymap___enabled__empty_slab_nonzero_buckets__true)
{
    test::chunk_storage head_store{};
    test::chunk_storage body_store{};
    slab_table instance{ head_store, body_store, initial_buckets };
    BOOST_REQUIRE(instance.enabled());
    BOOST_REQUIRE(!instance.get_fault());
}

// get/put
// ----------------------------------------------------------------------------

class little_record
{
public:
    // record bytes or zero for slab (for template).
    static constexpr size_t size = sizeof(uint32_t);

    // record count or bytes count for slab (for allocate).
    static constexpr link3 count() NOEXCEPT { return 1; }

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

class big_record
{
public:
    static constexpr size_t size = sizeof(uint32_t);
    static constexpr link3 count() NOEXCEPT { return 1; }

    bool from_data(database::reader& source) NOEXCEPT
    {
        value = source.read_big_endian<uint32_t>();
        return source;
    }

    bool to_data(database::finalizer& sink) const NOEXCEPT
    {
        sink.write_big_endian(value);
        return sink;
    }

    uint32_t value{ 0 };
};

BOOST_AUTO_TEST_CASE(arraymap__record_get__terminal__invalid)
{
    test::chunk_storage head_store{};
    test::chunk_storage body_store{};
    const arraymap<link3, little_record::size> instance{ head_store, body_store, initial_buckets };

    little_record record{};
    BOOST_REQUIRE(!instance.get(link3::terminal, record));
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(arraymap__record_get__empty__invalid)
{
    test::chunk_storage head_store{};
    test::chunk_storage body_store{};
    const arraymap<link3, little_record::size> instance{ head_store, body_store, initial_buckets };

    little_record record{};
    BOOST_REQUIRE(!instance.get(0, record));
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(arraymap__record_get__big_end_populated__expected)
{
    const data_chunk head_file
    {
        0x00, 0x00, 0x10, // initial body size.
                          // --------------------
        0xff, 0xff, 0xff, // head[0]  -> null
        0x02, 0x00, 0x00, // head[1]  -> link[2]
        0x03, 0x00, 0x00, // head[2]  -> link[3]
        0xff, 0xff, 0xff, // head[3]  -> null
        0xff, 0xff, 0xff, // head[4]  -> null
        0xff, 0xff, 0xff, // head[5]  -> null
        0xff, 0xff, 0xff, // head[6]  -> null
        0xff, 0xff, 0xff, // head[7]  -> null
        0xff, 0xff, 0xff, // head[8]  -> null
        0x01, 0x00, 0x00, // head[9]  -> link[1]
        0xff, 0xff, 0xff, // head[10] -> null
        0xff, 0xff, 0xff, // head[11] -> null
        0x00, 0x00, 0x00, // link[12] -> link[0]
        0xff, 0xff, 0xff, // head[13] -> null
        0xff, 0xff, 0xff, // head[14] -> null
        0xff, 0xff, 0xff, // head[15] -> null
        0xff, 0xff, 0xff, // head[16] -> null
        0xff, 0xff, 0xff  // head[17] -> null
    };

    const data_chunk body_file
    {
        0xa1, 0xa2, 0xa3, 0xa4, // 0xa1a2a3a4 (big endian read/write)
        0xb1, 0xb2, 0xb3, 0xb4, // 0xb1b2b3b4
        0xb5, 0xb6, 0xb7, 0xb8, // 0xb5b6b7b8
        0x01, 0x02, 0x03, 0x04  // 0x01020304
    };

    auto head_file_copy = head_file;
    auto body_file_copy = body_file;
    test::chunk_storage head_store{ head_file_copy };
    test::chunk_storage body_store{ body_file_copy };
    const arraymap<link3, big_record::size> instance{ head_store, body_store, initial_buckets };

    big_record record{};

    // Get the link.
    BOOST_REQUIRE(instance.get(0, record));
    BOOST_REQUIRE_EQUAL(record.value, 0xa1a2a3a4_u32);
    BOOST_REQUIRE(instance.get(1, record));
    BOOST_REQUIRE_EQUAL(record.value, 0xb1b2b3b4_u32);
    BOOST_REQUIRE(instance.get(2, record));
    BOOST_REQUIRE_EQUAL(record.value, 0xb5b6b7b8_u32);
    BOOST_REQUIRE(instance.get(3, record));
    BOOST_REQUIRE_EQUAL(record.value, 0x01020304_u32);
    BOOST_REQUIRE(!instance.get(4, record));;

    // Get from key.
    BOOST_REQUIRE(!instance.at(0, record));
    BOOST_REQUIRE( instance.at(1, record));
    BOOST_REQUIRE_EQUAL(record.value, 0xb5b6b7b8_u32);
    BOOST_REQUIRE( instance.at(2, record));
    BOOST_REQUIRE_EQUAL(record.value, 0x01020304_u32);
    BOOST_REQUIRE(!instance.at(3, record));
    BOOST_REQUIRE(!instance.at(4, record));
    BOOST_REQUIRE(!instance.at(5, record));
    BOOST_REQUIRE(!instance.at(6, record));
    BOOST_REQUIRE(!instance.at(7, record));
    BOOST_REQUIRE(!instance.at(8, record));
    BOOST_REQUIRE( instance.at(9, record));
    BOOST_REQUIRE_EQUAL(record.value, 0xb1b2b3b4_u32);
    BOOST_REQUIRE(!instance.at(10, record));
    BOOST_REQUIRE(!instance.at(11, record));
    BOOST_REQUIRE( instance.at(12, record));
    BOOST_REQUIRE_EQUAL(record.value, 0xa1a2a3a4_u32);
    BOOST_REQUIRE(!instance.at(13, record));
    BOOST_REQUIRE(!instance.at(14, record));
    BOOST_REQUIRE(!instance.at(15, record));
    BOOST_REQUIRE(!instance.at(16, record));
    BOOST_REQUIRE(!instance.at(17, record));

    BOOST_REQUIRE(!instance.get_fault());
}

class little_slab
{
public:
    static constexpr size_t size = max_size_t;
    static constexpr link3 count() NOEXCEPT
    {
        return sizeof(uint32_t);
    }

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

class big_slab
{
public:
    static constexpr size_t size = max_size_t;
    static constexpr link3 count() NOEXCEPT
    {
        return sizeof(uint32_t);
    }

    bool from_data(database::reader& source) NOEXCEPT
    {
        value = source.read_big_endian<uint32_t>();
        return source;
    }

    bool to_data(database::finalizer& sink) const NOEXCEPT
    {
        sink.write_big_endian(value);
        return sink;
    }

    uint32_t value{ 0 };
};

BOOST_AUTO_TEST_CASE(hashmap__slab_put__multiple__expected)
{
    test::chunk_storage head_store{};
    test::chunk_storage body_store{};

    arraymap<link3, big_slab::size> instance{ head_store, body_store, initial_buckets };
    BOOST_REQUIRE(instance.create());

    constexpr link3::integer key_big{ 0 };
    constexpr link3::integer key_little{ 1 };
    BOOST_REQUIRE(instance.put(key_big, big_slab{ 0xa1b2c3d4_u32 }));
    BOOST_REQUIRE(instance.put(key_little, little_slab{ 0xa1b2c3d4_u32 }));

    big_slab slab1{};
    BOOST_REQUIRE(instance.get(0, slab1));
    BOOST_REQUIRE_EQUAL(slab1.value, 0xa1b2c3d4_u32);

    little_slab slab2{};
    BOOST_REQUIRE(instance.get(big_slab::count(), slab2));
    BOOST_REQUIRE_EQUAL(slab2.value, 0xa1b2c3d4_u32);

    // This expectation relies on the fact of no hash table conflict between 0x41 and 0x42.
    const data_chunk expected_file
    {
        0xa1, 0xb2, 0xc3, 0xd4,
        0xd4, 0xc3, 0xb2, 0xa1
    };
    BOOST_REQUIRE_EQUAL(body_store.buffer(), expected_file);
    BOOST_REQUIRE(!instance.get_fault());
}

// advertises 32 but reads/writes 64
class record_excess
{
public:
    static constexpr size_t size = sizeof(uint32_t);
    static constexpr link3 count() NOEXCEPT { return 1; }

    bool from_data(database::reader& source) NOEXCEPT
    {
        value = source.read_big_endian<uint64_t>();
        return source;
    }

    bool to_data(database::finalizer& sink) const NOEXCEPT
    {
        sink.write_big_endian(value);
        return sink;
    }

    uint64_t value{ 0 };
};

BOOST_AUTO_TEST_CASE(arraymap__record_put__excess__false)
{
    test::chunk_storage head_store{};
    test::chunk_storage body_store{};
    arraymap<link3, big_record::size> instance{ head_store, body_store, initial_buckets };
    BOOST_REQUIRE(instance.create());
    BOOST_REQUIRE(!instance.put(0, record_excess{ 0xa1b2c3d4_u32 }));

    record_excess record{};
    BOOST_REQUIRE(!instance.get(0, record));
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(arraymap__record_get__excess__false)
{
    test::chunk_storage head_store{};
    test::chunk_storage body_store{};
    arraymap<link3, big_record::size> instance{ head_store, body_store, initial_buckets };
    BOOST_REQUIRE(instance.create());
    BOOST_REQUIRE(instance.put(0, big_record{ 0xa1b2c3d4_u32 }));

    record_excess record{};
    BOOST_REQUIRE(!instance.get(0, record));
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(arraymap__record_put__big_end_no_expansion__expected)
{
    test::chunk_storage head_store{};
    test::chunk_storage body_store{};
    arraymap<link3, big_record::size> instance{ head_store, body_store, initial_buckets };
    BOOST_REQUIRE(instance.create());

    const data_chunk expected_head = base16_chunk
    (
        "000000"
        "ffffff"
        "ffffff"
        "ffffff"
        "ffffff"
        "ffffff"
        "ffffff"
        "ffffff"
        "ffffff"
        "ffffff"
        "ffffff"
        "ffffff"
        "ffffff"
        "ffffff"
        "ffffff"
        "ffffff"
        "ffffff"
        "ffffff"
        "ffffff"
    );

    BOOST_REQUIRE_EQUAL(head_store.buffer(), expected_head);
    BOOST_REQUIRE(body_store.buffer().empty());

    const data_chunk expected_head1 = base16_chunk
    (
        "000000"
        "ffffff"
        "ffffff"
        "ffffff"
        "ffffff"
        "ffffff"
        "000000"
        "ffffff"
        "ffffff"
        "ffffff"
        "ffffff"
        "ffffff"
        "ffffff"
        "ffffff"
        "ffffff"
        "ffffff"
        "ffffff"
        "ffffff"
        "ffffff"
    );

    const data_chunk expected_body1 = base16_chunk
    (
        "a1b2c3d4"
    );

    BOOST_REQUIRE(instance.put(5_size, big_record{ 0xa1b2c3d4_u32 }));
    BOOST_REQUIRE_EQUAL(head_store.buffer(), expected_head1);
    BOOST_REQUIRE_EQUAL(body_store.buffer(), expected_body1);

    const data_chunk expected_head2 = base16_chunk
    (
        "000000"
        // -----
        "020000"
        "010000"
        "ffffff"
        "ffffff"
        "ffffff"
        "000000"
        "ffffff"
        "ffffff"
        "ffffff"
        "ffffff"
        "ffffff"
        "ffffff"
        "ffffff"
        "ffffff"
        "ffffff"
        "ffffff"
        "ffffff"
        "030000"
    );

    const data_chunk expected_body2 = base16_chunk
    (
        "a1b2c3d4"
        "d4c3b2a1"
        "42424242"
        "12345678"
    );

    BOOST_REQUIRE(instance.put(1_size, big_record{ 0xd4c3b2a1_u32 }));
    BOOST_REQUIRE(instance.put(0_size, big_record{ 0x42424242_u32 }));
    BOOST_REQUIRE(instance.put(17_size, big_record{ 0x12345678_u32 }));
    BOOST_REQUIRE_EQUAL(head_store.buffer(), expected_head2);
    BOOST_REQUIRE_EQUAL(body_store.buffer(), expected_body2);

    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(arraymap__record_put__little_end_default_expansion__expected)
{
    test::chunk_storage head_store{};
    test::chunk_storage body_store{};
    arraymap<link3, little_record::size> instance{ head_store, body_store, initial_buckets };
    BOOST_REQUIRE(instance.create());

    const data_chunk expected_head = base16_chunk
    (
        "000000"
        "ffffff"
        "ffffff"
        "ffffff"
        "ffffff"
        "ffffff"
        "ffffff"
        "ffffff"
        "ffffff"
        "ffffff"
        "ffffff"
        "ffffff"
        "ffffff"
        "ffffff"
        "ffffff"
        "ffffff"
        "ffffff"
        "ffffff"
        "ffffff"
    );

    BOOST_REQUIRE_EQUAL(head_store.buffer(), expected_head);
    BOOST_REQUIRE(body_store.buffer().empty());

    const data_chunk expected_head1 = base16_chunk
    (
        "000000"
        "ffffff"
        "ffffff"
        "ffffff"
        "ffffff"
        "ffffff"
        "000000"
        "ffffff"
        "ffffff"
        "ffffff"
        "ffffff"
        "ffffff"
        "ffffff"
        "ffffff"
        "ffffff"
        "ffffff"
        "ffffff"
        "ffffff"
        "ffffff"
    );

    const data_chunk expected_body1 = base16_chunk
    (
        "d4c3b2a1"
    );

    BOOST_REQUIRE(instance.put(5_size, little_record{ 0xa1b2c3d4_u32 }));
    BOOST_REQUIRE_EQUAL(head_store.buffer(), expected_head1);
    BOOST_REQUIRE_EQUAL(body_store.buffer(), expected_body1);

    const data_chunk expected_head2 = base16_chunk
    (
        "000000"
        // -----
        "020000"
        "010000"
        "ffffff"
        "ffffff"
        "ffffff"
        "000000"
        "ffffff"
        "ffffff"
        "ffffff"
        "ffffff"
        "ffffff"
        "ffffff"
        "ffffff"
        "ffffff"
        "ffffff"
        "ffffff"
        "ffffff"
        "ffffff"
        "ffffff"
        "ffffff"
        "030000"
    );

    const data_chunk expected_body2 = base16_chunk
    (
        "d4c3b2a1"
        "a1b2c3d4"
        "42424242"
        "78563412"
    );

    BOOST_REQUIRE(instance.put(1_size, little_record{ 0xd4c3b2a1_u32 }));
    BOOST_REQUIRE(instance.put(0_size, little_record{ 0x42424242_u32 }));
    BOOST_REQUIRE(instance.put(20_size, little_record{ 0x12345678_u32 }));
    BOOST_REQUIRE_EQUAL(head_store.buffer(), expected_head2);
    BOOST_REQUIRE_EQUAL(body_store.buffer(), expected_body2);

    const data_chunk expected_head3 = base16_chunk
    (
        "000000"
        // -----
        "020000"
        "010000"
        "ffffff"
        "ffffff"
        "ffffff"
        "000000"
        "ffffff"
        "ffffff"
        "ffffff"
        "ffffff"
        "ffffff"
        "ffffff"
        "ffffff"
        "ffffff"
        "ffffff"
        "ffffff"
        "ffffff"
        "ffffff"
        "ffffff"
        "ffffff"
        "030000"
        "ffffff"
        "050000"
        "040000"
    );

    const data_chunk expected_body3 = base16_chunk
    (
        "d4c3b2a1"
        "a1b2c3d4"
        "42424242"
        "78563412"
        "88664422"
        "77553311"
    );

    BOOST_REQUIRE(instance.put(23_size, little_record{ 0x22446688_u32 }));
    BOOST_REQUIRE(instance.put(22_size, little_record{ 0x11335577_u32 }));
    BOOST_REQUIRE_EQUAL(head_store.buffer(), expected_head3);
    BOOST_REQUIRE_EQUAL(body_store.buffer(), expected_body3);

    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(arraymap__record_at__existing__expected)
{
    data_chunk store_head = base16_chunk
    (
        "000000"
        // -----
        "020000"
        "010000"
        "ffffff"
        "ffffff"
        "ffffff"
        "000000"
        "ffffff"
        "ffffff"
        "ffffff"
        "ffffff"
        "ffffff"
        "ffffff"
        "ffffff"
        "ffffff"
        "ffffff"
        "ffffff"
        "ffffff"
        "ffffff"
        "ffffff"
        "ffffff"
        "030000"
        "ffffff"
        "050000"
        "040000"
    );

    data_chunk store_body = base16_chunk
    (
        "d4c3b2a1"
        "a1b2c3d4"
        "42424242"
        "78563412"
        "88664422"
        "77553311"
    );

    test::chunk_storage head_store{ store_head };
    test::chunk_storage body_store{ store_body };
    arraymap<link3, big_record::size> instance{ head_store, body_store, initial_buckets };

    BOOST_REQUIRE_EQUAL(instance.at(0), 2u);
    BOOST_REQUIRE_EQUAL(instance.at(1), 1u);
    BOOST_REQUIRE_EQUAL(instance.at(2), link3::terminal);
    BOOST_REQUIRE_EQUAL(instance.at(3), link3::terminal);
    BOOST_REQUIRE_EQUAL(instance.at(4), link3::terminal);
    BOOST_REQUIRE_EQUAL(instance.at(5), 0u);
    BOOST_REQUIRE_EQUAL(instance.at(6), link3::terminal);
    BOOST_REQUIRE_EQUAL(instance.at(7), link3::terminal);
    BOOST_REQUIRE_EQUAL(instance.at(8), link3::terminal);
    BOOST_REQUIRE_EQUAL(instance.at(9), link3::terminal);
    BOOST_REQUIRE_EQUAL(instance.at(10), link3::terminal);
    BOOST_REQUIRE_EQUAL(instance.at(11), link3::terminal);
    BOOST_REQUIRE_EQUAL(instance.at(12), link3::terminal);
    BOOST_REQUIRE_EQUAL(instance.at(13), link3::terminal);
    BOOST_REQUIRE_EQUAL(instance.at(14), link3::terminal);
    BOOST_REQUIRE_EQUAL(instance.at(15), link3::terminal);
    BOOST_REQUIRE_EQUAL(instance.at(16), link3::terminal);
    BOOST_REQUIRE_EQUAL(instance.at(17), link3::terminal);
    BOOST_REQUIRE_EQUAL(instance.at(18), link3::terminal);
    BOOST_REQUIRE_EQUAL(instance.at(19), link3::terminal);
    BOOST_REQUIRE_EQUAL(instance.at(20), 3u);
    BOOST_REQUIRE_EQUAL(instance.at(21), link3::terminal);
    BOOST_REQUIRE_EQUAL(instance.at(22), 5u);
    BOOST_REQUIRE_EQUAL(instance.at(23), 4u);

    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(arraymap__record_at__exists__true)
{
    test::chunk_storage head_store{};
    test::chunk_storage body_store{};
    arraymap<link3, big_record::size> instance{ head_store, body_store, initial_buckets };
    BOOST_REQUIRE(instance.create());

    constexpr uint32_t expected = 0xa1b2c3d4;
    BOOST_REQUIRE(instance.at(1).is_terminal());
    BOOST_REQUIRE(instance.put(1, big_record{ expected }));
    BOOST_REQUIRE_EQUAL(instance.at(1), 0u);

    big_record record{};
    BOOST_REQUIRE(instance.get(0, record));
    BOOST_REQUIRE_EQUAL(record.value, expected);
    BOOST_REQUIRE(!instance.get_fault());
}

// record create/close/backup/restore/verify
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(arraymap__record_verify__empty_files__expected)
{
    test::chunk_storage head_store{};
    test::chunk_storage body_store{};
    record_table instance{ head_store, body_store, initial_buckets };
    BOOST_REQUIRE(!instance.verify());
    BOOST_REQUIRE(instance.create());
    BOOST_REQUIRE(instance.verify());
    BOOST_REQUIRE_EQUAL(head_store.buffer().size(), initial_header_size);
    BOOST_REQUIRE(body_store.buffer().empty());
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(arraymap__record_create__non_empty_head_file__failure)
{
    data_chunk head_file{ 0x42 };
    data_chunk body_file;
    test::chunk_storage head_store{ head_file };
    test::chunk_storage body_store{ body_file };
    record_table instance{ head_store, body_store, initial_buckets };
    BOOST_REQUIRE(!instance.verify());
    BOOST_REQUIRE(!instance.create());
    BOOST_REQUIRE_EQUAL(head_file.size(), 1u);
    BOOST_REQUIRE(body_file.empty());
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(arraymap__record_create__non_empty_body_file__body_zeroed)
{
    data_chunk head_file;
    data_chunk body_file{ 0x42 };
    test::chunk_storage head_store{ head_file };
    test::chunk_storage body_store{ body_file };
    record_table instance{ head_store, body_store, initial_buckets };
    BOOST_REQUIRE(!instance.verify());
    BOOST_REQUIRE(instance.create());
    BOOST_REQUIRE(instance.verify());
    BOOST_REQUIRE_EQUAL(head_file.size(), initial_header_size);
    BOOST_REQUIRE(body_file.empty());
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(arraymap__record_body_count__create__zero)
{
    test::chunk_storage head_store{};
    test::chunk_storage body_store{};
    record_table instance{ head_store, body_store, initial_buckets };
    BOOST_REQUIRE(instance.create());
    BOOST_REQUIRE_EQUAL(head_store.buffer(),
        base16_chunk
        (
            "000000"
            // -----
            "ffffff"
            "ffffff"
            "ffffff"
            "ffffff"
            "ffffff"
            "ffffff"
            "ffffff"
            "ffffff"
            "ffffff"
            "ffffff"
            "ffffff"
            "ffffff"
            "ffffff"
            "ffffff"
            "ffffff"
            "ffffff"
            "ffffff"
            "ffffff"
        ));
    BOOST_REQUIRE(body_store.buffer().empty());
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(arraymap__record_body_count__empty_close__zero)
{
    auto head_file = base16_chunk("123456");
    data_chunk body_file;
    test::chunk_storage head_store{ head_file };
    test::chunk_storage body_store{ body_file };
    record_table instance{ head_store, body_store, initial_buckets };
    BOOST_REQUIRE(instance.close());
    BOOST_REQUIRE_EQUAL(head_file, base16_chunk("000000"));
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(arraymap__record_body_count__nine_close__nine)
{
    test::chunk_storage head_store{};
    test::chunk_storage body_store{};
    record_table instance{ head_store, body_store, initial_buckets };
    BOOST_REQUIRE(instance.create());
    body_store.buffer() = base16_chunk("112233441122334411223344112233441122334411223344112233441122334411223344");
    BOOST_REQUIRE(instance.close());
    BOOST_REQUIRE_EQUAL(head_store.buffer(), base16_chunk("090000ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"));
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(arraymap__reset__nine_close__zero)
{
    test::chunk_storage head_store{};
    test::chunk_storage body_store{};
    record_table instance{ head_store, body_store, initial_buckets };
    BOOST_REQUIRE(instance.create());
    body_store.buffer() = base16_chunk("112233441122334411223344112233441122334411223344112233441122334411223344");
    BOOST_REQUIRE(instance.reset());
    BOOST_REQUIRE(instance.close());
    BOOST_REQUIRE_EQUAL(head_store.buffer(), base16_chunk("000000ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"));
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(arraymap__record_body_count__nine_backup__nine)
{
    test::chunk_storage head_store{};
    test::chunk_storage body_store{};
    record_table instance{ head_store, body_store, initial_buckets };
    BOOST_REQUIRE(instance.create());
    body_store.buffer() = base16_chunk("112233441122334411223344112233441122334411223344112233441122334411223344");
    BOOST_REQUIRE(instance.backup());
    BOOST_REQUIRE_EQUAL(head_store.buffer(), base16_chunk("090000ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"));
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(arraymap__record_body_count__empty_restore__truncates)
{
    test::chunk_storage head_store{};
    test::chunk_storage body_store{};
    record_table instance{ head_store, body_store, initial_buckets };
    BOOST_REQUIRE(instance.create());
    body_store.buffer() = base16_chunk("1234567812345678");
    BOOST_REQUIRE(instance.restore());
    BOOST_REQUIRE(body_store.buffer().empty());
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(arraymap__record_body_count__non_empty_restore__truncates)
{
    test::chunk_storage head_store{};
    test::chunk_storage body_store{};
    record_table instance{ head_store, body_store, initial_buckets };
    BOOST_REQUIRE(instance.create());
    head_store.buffer() = base16_chunk("090000ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
    body_store.buffer() = base16_chunk("112233441122334411223344112233441122334411223344112233441122334411223344abababababababab");
    BOOST_REQUIRE(instance.restore());
    BOOST_REQUIRE_EQUAL(body_store.buffer(), base16_chunk("112233441122334411223344112233441122334411223344112233441122334411223344"));
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_SUITE_END()
