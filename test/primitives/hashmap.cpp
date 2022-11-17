/**
 * Copyright (c) 2011-2022 libbitcoin developers (see AUTHORS)
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
#include "../storage.hpp"

BOOST_AUTO_TEST_SUITE(hashmap_tests)

template <typename Link, typename Key, size_t Size>
class hashmap_
  : public hashmap<Link, Key, Size>
{
public:
    using hashmap<Link, Key, Size>::hashmap;

    reader_ptr getter_(const Key& key) const NOEXCEPT
    {
        return hashmap<Link, Key, Size>::getter(key);
    }

    reader_ptr getter_(const Link& record) const NOEXCEPT
    {
        return hashmap<Link, Key, Size>::template streamer<database::reader>(record);
    }

    finalizer_ptr creater_(const Key& key, const Link& size=bc::one) NOEXCEPT
    {
        return hashmap<Link, Key, Size>::creater(key, size);
    }
};

using link5 = linkage<5>;
using key1 = data_array<1>;
using key10 = data_array<10>;

// Key size does not factor into header byte size (for first key only).
constexpr size_t header_size = 105;
constexpr auto links = header_size / link5::size;
static_assert(links == 21u);

// Bucket count is one less than link count, due to header.size field.
constexpr auto buckets = bc::sub1(links);
static_assert(buckets == 20u);

struct slab0 { static constexpr size_t size = max_size_t; };
struct record4 { static constexpr size_t size = 4; };
using slab_table = hashmap_<link5, key10, slab0::size>;
using record_table = hashmap_<link5, key10, record4::size>;

constexpr auto element_size = link5::size + array_count<key10> + record4::size;

// record hashmap
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(hashmap__record_construct__empty_files__expected)
{
    data_chunk head_file;
    data_chunk body_file;
    test::storage head_store{ head_file };
    test::storage body_store{ body_file };
    record_table instance{ head_store, body_store, buckets };

    BOOST_REQUIRE(head_file.empty());
    BOOST_REQUIRE(body_file.empty());

    BOOST_REQUIRE(!instance.verify());
    BOOST_REQUIRE(instance.create());
    BOOST_REQUIRE(instance.verify());
    BOOST_REQUIRE(instance.snap());

    BOOST_REQUIRE_EQUAL(head_file.size(), header_size);
    BOOST_REQUIRE(body_file.empty());
}

BOOST_AUTO_TEST_CASE(hashmap__record_construct__non_empty_head_file__failure)
{
    data_chunk head_file{ 0x42 };
    data_chunk body_file;
    test::storage head_store{ head_file };
    test::storage body_store{ body_file };
    record_table instance{ head_store, body_store, buckets };

    BOOST_REQUIRE_EQUAL(head_file.size(), one);
    BOOST_REQUIRE(body_file.empty());

    BOOST_REQUIRE(!instance.verify());
    BOOST_REQUIRE(!instance.create());
    BOOST_REQUIRE(!instance.verify());

    BOOST_REQUIRE_EQUAL(head_file.size(), one);
    BOOST_REQUIRE(body_file.empty());
}

BOOST_AUTO_TEST_CASE(hashmap__record_construct__multiple_item_body_file__failure)
{
    constexpr auto body_size = 3u * element_size;
    data_chunk head_file;
    data_chunk body_file(body_size, 0x42);
    test::storage head_store{ head_file };
    test::storage body_store{ body_file };
    record_table instance{ head_store, body_store, buckets };

    BOOST_REQUIRE(head_file.empty());
    BOOST_REQUIRE_EQUAL(body_file.size(), body_size);

    BOOST_REQUIRE(!instance.verify());
    BOOST_REQUIRE(!instance.create());
    BOOST_REQUIRE(!instance.verify());

    BOOST_REQUIRE_EQUAL(head_file.size(), header_size);
    BOOST_REQUIRE_EQUAL(body_file.size(), body_size);
}

BOOST_AUTO_TEST_CASE(hashmap__record_construct__multiple_fractional_item_body_file__failure)
{
    constexpr auto body_size = 3u * element_size + 2u;
    data_chunk head_file;
    data_chunk body_file(body_size, 0x42);
    test::storage head_store{ head_file };
    test::storage body_store{ body_file };
    record_table instance{ head_store, body_store, buckets };

    BOOST_REQUIRE(head_file.empty());
    BOOST_REQUIRE_EQUAL(body_file.size(), body_size);

    BOOST_REQUIRE(!instance.verify());
    BOOST_REQUIRE(!instance.create());
    BOOST_REQUIRE(!instance.verify());

    BOOST_REQUIRE_EQUAL(head_file.size(), header_size);
    BOOST_REQUIRE_EQUAL(body_file.size(), body_size);
}

BOOST_AUTO_TEST_CASE(hashmap__record_construct__one_item_body_file__failure)
{
    constexpr auto body_size = element_size;
    data_chunk head_file;
    data_chunk body_file(body_size, 0x42);
    test::storage head_store{ head_file };
    test::storage body_store{ body_file };
    record_table instance{ head_store, body_store, buckets };

    BOOST_REQUIRE(head_file.empty());
    BOOST_REQUIRE_EQUAL(body_file.size(), body_size);

    BOOST_REQUIRE(!instance.verify());
    BOOST_REQUIRE(!instance.create());
    BOOST_REQUIRE(!instance.verify());

    BOOST_REQUIRE_EQUAL(head_file.size(), header_size);
    BOOST_REQUIRE_EQUAL(body_file.size(), body_size);
}

BOOST_AUTO_TEST_CASE(hashmap__record_construct__sub_one_item_body_file__success)
{
    constexpr auto body_size = sub1(element_size);
    data_chunk head_file;
    data_chunk body_file(body_size, 0x42);
    test::storage head_store{ head_file };
    test::storage body_store{ body_file };
    record_table instance{ head_store, body_store, buckets };

    BOOST_REQUIRE(head_file.empty());
    BOOST_REQUIRE_EQUAL(body_file.size(), body_size);

    // With records, count rounds down (truncates).
    // In this case it appears as zero, which is a successful start.
    // Since truncation is consistent, a fractional record is merely overwritten
    // upon allocation, or truncated upon close (with map storage).
    BOOST_REQUIRE(!instance.verify());
    BOOST_REQUIRE(instance.create());
    BOOST_REQUIRE(instance.verify());

    BOOST_REQUIRE_EQUAL(head_file.size(), header_size);
    BOOST_REQUIRE_EQUAL(body_file.size(), body_size);
}

BOOST_AUTO_TEST_CASE(hashmap__record_getter__terminal__false)
{
    data_chunk head_file;
    data_chunk body_file;
    test::storage head_store{ head_file };
    test::storage body_store{ body_file };
    record_table instance{ head_store, body_store, buckets };
    BOOST_REQUIRE(instance.create());
    BOOST_REQUIRE(!instance.getter_(link5::terminal));
    BOOST_REQUIRE(!instance.getter_(link5::terminal));
}

BOOST_AUTO_TEST_CASE(hashmap__record_getter__empty__exhausted)
{
    data_chunk head_file;
    data_chunk body_file;
    test::storage head_store{ head_file };
    test::storage body_store{ body_file };
    record_table instance{ head_store, body_store, buckets };
    BOOST_REQUIRE(instance.create());
    BOOST_REQUIRE(instance.getter_(0)->is_exhausted());
    BOOST_REQUIRE(instance.getter_(19)->is_exhausted());
}

BOOST_AUTO_TEST_CASE(hashmap__record_getter__empty__false)
{
    data_chunk head_file;
    data_chunk body_file;
    test::storage head_store{ head_file };
    test::storage body_store{ body_file };
    record_table instance{ head_store, body_store, buckets };
    BOOST_REQUIRE(instance.create());
    BOOST_REQUIRE(!instance.getter_(key10{ 0x00 }));
    BOOST_REQUIRE(!instance.getter_(key10{ 0x42 }));
}

// slab hashmap
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(hashmap__slab_construct__empty_files__success)
{
    data_chunk head_file;
    data_chunk body_file;
    test::storage head_store{ head_file };
    test::storage body_store{ body_file };
    slab_table instance{ head_store, body_store, buckets };

    BOOST_REQUIRE(head_file.empty());
    BOOST_REQUIRE(body_file.empty());

    BOOST_REQUIRE(!instance.verify());
    BOOST_REQUIRE(instance.create());
    BOOST_REQUIRE(instance.verify());

    BOOST_REQUIRE_EQUAL(head_file.size(), header_size);
    BOOST_REQUIRE(body_file.empty());
}

BOOST_AUTO_TEST_CASE(hashmap__slab_construct__non_empty_head_file__failure)
{
    data_chunk head_file{ 0x42 };
    data_chunk body_file;
    test::storage head_store{ head_file };
    test::storage body_store{ body_file };
    slab_table instance{ head_store, body_store, buckets };

    BOOST_REQUIRE_EQUAL(head_file.size(), one);
    BOOST_REQUIRE(body_file.empty());

    BOOST_REQUIRE(!instance.verify());
    BOOST_REQUIRE(!instance.create());
    BOOST_REQUIRE(!instance.verify());

    BOOST_REQUIRE_EQUAL(head_file.size(), one);
    BOOST_REQUIRE(body_file.empty());
}

BOOST_AUTO_TEST_CASE(hashmap__slab_construct__multiple_item_body_file__failure)
{
    constexpr auto body_size = 3u * element_size;
    data_chunk head_file;
    data_chunk body_file(body_size, 0x42);
    test::storage head_store{ head_file };
    test::storage body_store{ body_file };
    slab_table instance{ head_store, body_store, buckets };

    BOOST_REQUIRE(head_file.empty());
    BOOST_REQUIRE_EQUAL(body_file.size(), body_size);

    BOOST_REQUIRE(!instance.verify());
    BOOST_REQUIRE(!instance.create());
    BOOST_REQUIRE(!instance.verify());

    BOOST_REQUIRE_EQUAL(head_file.size(), header_size);
    BOOST_REQUIRE_EQUAL(body_file.size(), body_size);
}

BOOST_AUTO_TEST_CASE(hashmap__slab_construct__multiple_fractional_item_body_file__failure)
{
    constexpr auto body_size = 3u * element_size + 2u;
    data_chunk head_file;
    data_chunk body_file(body_size, 0x42);
    test::storage head_store{ head_file };
    test::storage body_store{ body_file };
    slab_table instance{ head_store, body_store, buckets };

    BOOST_REQUIRE(head_file.empty());
    BOOST_REQUIRE_EQUAL(body_file.size(), body_size);

    BOOST_REQUIRE(!instance.verify());
    BOOST_REQUIRE(!instance.create());
    BOOST_REQUIRE(!instance.verify());

    BOOST_REQUIRE_EQUAL(head_file.size(), header_size);
    BOOST_REQUIRE_EQUAL(body_file.size(), body_size);
}

BOOST_AUTO_TEST_CASE(hashmap__slab_construct__one_item_body_file__failure)
{
    constexpr auto body_size = element_size;
    data_chunk head_file;
    data_chunk body_file(body_size, 0x42);
    test::storage head_store{ head_file };
    test::storage body_store{ body_file };
    slab_table instance{ head_store, body_store, buckets };

    BOOST_REQUIRE(head_file.empty());
    BOOST_REQUIRE_EQUAL(body_file.size(), body_size);

    BOOST_REQUIRE(!instance.verify());
    BOOST_REQUIRE(!instance.create());
    BOOST_REQUIRE(!instance.verify());

    BOOST_REQUIRE_EQUAL(head_file.size(), header_size);
    BOOST_REQUIRE_EQUAL(body_file.size(), body_size);
}

BOOST_AUTO_TEST_CASE(hashmap__slab_construct__sub_one_item_body_file__failure)
{
    constexpr auto body_size = sub1(element_size);
    data_chunk head_file;
    data_chunk body_file(body_size, 0x42);
    test::storage head_store{ head_file };
    test::storage body_store{ body_file };
    slab_table instance{ head_store, body_store, buckets };

    BOOST_REQUIRE(head_file.empty());
    BOOST_REQUIRE_EQUAL(body_file.size(), body_size);

    // With slabs, count is not divided, so also not rounded down.
    BOOST_REQUIRE(!instance.verify());
    BOOST_REQUIRE(!instance.create());
    BOOST_REQUIRE(!instance.verify());

    BOOST_REQUIRE_EQUAL(head_file.size(), header_size);
    BOOST_REQUIRE_EQUAL(body_file.size(), body_size);
}

BOOST_AUTO_TEST_CASE(slab_hashmap__getter__terminal__false)
{
    data_chunk head_file;
    data_chunk body_file;
    test::storage head_store{ head_file };
    test::storage body_store{ body_file };
    slab_table instance{ head_store, body_store, buckets };
    BOOST_REQUIRE(instance.create());
    BOOST_REQUIRE(!instance.getter_(link5::terminal));
    BOOST_REQUIRE(!instance.getter_(link5::terminal));
}

BOOST_AUTO_TEST_CASE(slab_hashmap__getter__empty__exhausted)
{
    data_chunk head_file;
    data_chunk body_file;
    test::storage head_store{ head_file };
    test::storage body_store{ body_file };
    slab_table instance{ head_store, body_store, buckets };
    BOOST_REQUIRE(instance.create());
    BOOST_REQUIRE(instance.getter_(0)->is_exhausted());
    BOOST_REQUIRE(instance.getter_(19)->is_exhausted());
}

BOOST_AUTO_TEST_CASE(slab_hashmap__getter__empty__false)
{
    data_chunk head_file;
    data_chunk body_file;
    test::storage head_store{ head_file };
    test::storage body_store{ body_file };
    slab_table instance{ head_store, body_store, buckets };
    BOOST_REQUIRE(instance.create());
    BOOST_REQUIRE(!instance.getter_(key10{ 0x00 }));
    BOOST_REQUIRE(!instance.getter_(key10{ 0x42 }));
}

// push/found/at (protected interface positive tests)
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(hashmap__record_readers__empty__expected)
{
    data_chunk head_file;
    data_chunk body_file;
    test::storage head_store{ head_file };
    test::storage body_store{ body_file };
    record_table instance{ head_store, body_store, buckets };
    BOOST_REQUIRE(instance.create());

    constexpr key10 key0{ 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a };
    auto stream0 = instance.creater_(key0);
    BOOST_REQUIRE_EQUAL(body_file.size(), element_size);
    BOOST_REQUIRE(!stream0->is_exhausted());
    BOOST_REQUIRE(!instance.getter_(key0));
    BOOST_REQUIRE(stream0->finalize());
    BOOST_REQUIRE(instance.getter_(key0));
    stream0.reset();

    constexpr key10 key1{ 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a };
    auto stream1 = instance.creater_(key1);
    BOOST_REQUIRE_EQUAL(body_file.size(), 2u * element_size);
    BOOST_REQUIRE(!stream1->is_exhausted());
    BOOST_REQUIRE(!instance.getter_(key1));
    BOOST_REQUIRE(stream1->finalize());
    BOOST_REQUIRE(instance.getter_(key1));
    stream1.reset();

    // Past end is valid pointer but exhausted stream.
    BOOST_REQUIRE(instance.getter_(0));
    BOOST_REQUIRE(!instance.getter_(0)->is_exhausted());
    BOOST_REQUIRE(instance.getter_(1));
    BOOST_REQUIRE(!instance.getter_(1)->is_exhausted());
    BOOST_REQUIRE(instance.getter_(2));
    BOOST_REQUIRE(instance.getter_(2)->is_exhausted());

    // record
    // 0000000000 [body logical size]
    // ---------------------------------
    // 0000000000 [0] [0->0]
    // ffffffffff [1]
    // ffffffffff [2]
    // ffffffffff [3]
    // 0100000000 [4] [4->1]
    // ffffffffff [5]
    // ffffffffff [6]
    // ffffffffff [7]
    // ffffffffff [8]
    // ffffffffff [9]
    // ffffffffff [10]
    // ffffffffff [11]
    // ffffffffff [12]
    // ffffffffff [13]
    // ffffffffff [14]
    // ffffffffff [15]
    // ffffffffff [16]
    // ffffffffff [17]
    // ffffffffff [18]
    // ffffffffff [19]
    // =================================
    // ffffffffff [0]       [terminator]
    // 0102030405060708090a [key]
    // 00000000             [data]
    // ffffffffff [1]       [terminator]
    // 1112131415161718191a [key]
    // 00000000             [data]
}

BOOST_AUTO_TEST_CASE(hashmap__slab_creater_getter__empty__true)
{
    data_chunk head_file;
    data_chunk body_file;
    test::storage head_store{ head_file };
    test::storage body_store{ body_file };
    slab_table instance{ head_store, body_store, buckets };
    BOOST_REQUIRE(instance.create());

    constexpr key10 key0{ 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a };
    BOOST_REQUIRE(!instance.getter_(key0));
    BOOST_REQUIRE(instance.creater_(key0, element_size)->finalize());
    BOOST_REQUIRE_EQUAL(body_file.size(), element_size);
    BOOST_REQUIRE(instance.getter_(key0));

    constexpr key10 key1{ 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a };
    BOOST_REQUIRE(!instance.getter_(key1));
    BOOST_REQUIRE(instance.creater_(key1, element_size)->finalize());
    BOOST_REQUIRE_EQUAL(body_file.size(), 2u * element_size);
    BOOST_REQUIRE(instance.getter_(key1));

    // Past end is valid pointer but exhausted stream.
    BOOST_REQUIRE(instance.getter_(0));
    BOOST_REQUIRE(!instance.getter_(0)->is_exhausted());
    BOOST_REQUIRE(instance.getter_(element_size));
    BOOST_REQUIRE(!instance.getter_(element_size)->is_exhausted());
    BOOST_REQUIRE(instance.getter_(2u * element_size));
    BOOST_REQUIRE(instance.getter_(2u * element_size)->is_exhausted());
 
    // slab (same extents as record above)
    // 0000000000 [body logical size]
    // ---------------------------------
    // 0000000000 [0->0x00]
    // ffffffffff
    // ffffffffff
    // ffffffffff
    // 1300000000 [4->0x13]
    // ffffffffff
    // ffffffffff
    // ffffffffff
    // ffffffffff
    // ffffffffff
    // ffffffffff
    // ffffffffff
    // ffffffffff
    // ffffffffff
    // ffffffffff
    // ffffffffff
    // ffffffffff
    // ffffffffff
    // ffffffffff
    // ffffffffff
    // =================================
    // ffffffffff [0x00]    [terminator]
    // 0102030405060708090a [key]
    // 00000000             [data]
    // ffffffffff [0x13]    [terminator]
    // 1112131415161718191a [key]
    // 00000000             [data]
}

BOOST_AUTO_TEST_CASE(hashmap__record_creater_duplicate_key__getter__true)
{
    data_chunk head_file;
    data_chunk body_file;
    test::storage head_store{ head_file };
    test::storage body_store{ body_file };
    record_table instance{ head_store, body_store, buckets };
    BOOST_REQUIRE(instance.create());

    constexpr key10 key0{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    BOOST_REQUIRE(instance.creater_(key0)->finalize());
    BOOST_REQUIRE_EQUAL(body_file.size(), element_size);
    BOOST_REQUIRE(instance.getter_(key0));

    constexpr key10 key1{ 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a };
    BOOST_REQUIRE(instance.creater_(key1)->finalize());
    BOOST_REQUIRE_EQUAL(body_file.size(), 2u * element_size);
    BOOST_REQUIRE(instance.getter_(key1));

    BOOST_REQUIRE(instance.creater_(key1)->finalize());
    BOOST_REQUIRE_EQUAL(body_file.size(), 3u * element_size);
    BOOST_REQUIRE(instance.getter_(key1));

    // 0000000000 [body logical size]
    // ---------------------------------
    // 0200000000 [0->2]
    // ffffffffff
    // ffffffffff
    // ffffffffff
    // ffffffffff
    // ffffffffff
    // ffffffffff
    // ffffffffff
    // ffffffffff
    // 0000000000 [9->0]
    // ffffffffff
    // ffffffffff
    // ffffffffff
    // ffffffffff
    // ffffffffff
    // ffffffffff
    // ffffffffff
    // ffffffffff
    // ffffffffff
    // ffffffffff
    // =================================
    // ffffffffff [0]       [terminator]
    // 00000000000000000000 [key]
    // 00000000             [data]
    // ffffffffff [1]       [terminator]
    // 0102030405060708090a [key]
    // 00000000             [data]
    // 0100000000 [2->1]    [next]
    // 0102030405060708090a [key]
    // 00000000             [data]
}

// get/put
// ----------------------------------------------------------------------------

class little_record
{
public:
    // record bytes or zero for slab (for template).
    static constexpr size_t size = sizeof(uint32_t);

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

class big_record
{
public:
    static constexpr size_t size = sizeof(uint32_t);
    static constexpr link5 count() NOEXCEPT { return 1; }

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

BOOST_AUTO_TEST_CASE(hashmap__record_get__terminal__invalid)
{
    data_chunk head_file;
    data_chunk body_file;
    test::storage head_store{ head_file };
    test::storage body_store{ body_file };
    const hashmap<link5, key10, little_record::size> instance{ head_store, body_store, buckets };

    little_record record{};
    BOOST_REQUIRE(!instance.get(link5::terminal, record));
}

BOOST_AUTO_TEST_CASE(hashmap__record_get__empty__invalid)
{
    data_chunk head_file;
    data_chunk body_file;
    test::storage head_store{ head_file };
    test::storage body_store{ body_file };
    const hashmap<link5, key10, little_record::size> instance{ head_store, body_store, buckets };

    little_record record{};
    BOOST_REQUIRE(!instance.get(0, record));
}

BOOST_AUTO_TEST_CASE(hashmap__record_get__populated__valid)
{
    data_chunk head_file;
    data_chunk body_file
    {
        0xa1, 0xa2, 0xa3, 0xa4, 0xa5,
        0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba,
        0x01, 0x02, 0x03, 0x04
    };
    test::storage head_store{ head_file };
    test::storage body_store{ body_file };
    const hashmap<link5, key10, little_record::size> instance{ head_store, body_store, buckets };

    little_record record{};
    BOOST_REQUIRE(instance.get(0, record));
    BOOST_REQUIRE_EQUAL(record.value, 0x04030201_u32);
}

BOOST_AUTO_TEST_CASE(hashmap__record_put__get__expected)
{
    data_chunk head_file;
    data_chunk body_file;
    test::storage head_store{ head_file };
    test::storage body_store{ body_file };
    hashmap<link5, key1, big_record::size> instance{ head_store, body_store, buckets };
    BOOST_REQUIRE(instance.create());

    constexpr key1 key{ 0x42 };
    BOOST_REQUIRE(instance.put(key, big_record{ 0xa1b2c3d4_u32 }));

    big_record link_record{};
    BOOST_REQUIRE(instance.get(0, link_record));
    BOOST_REQUIRE_EQUAL(link_record.value, 0xa1b2c3d4_u32);

    big_record key_record{};
    BOOST_REQUIRE(instance.get(key, key_record));
    BOOST_REQUIRE_EQUAL(key_record.value, 0xa1b2c3d4_u32);

    const data_chunk expected_file
    {
        0xff, 0xff, 0xff, 0xff, 0xff,
        0x42,
        0xa1, 0xb2, 0xc3, 0xd4
    };
    BOOST_REQUIRE_EQUAL(body_file, expected_file);
}

BOOST_AUTO_TEST_CASE(hashmap__record_put__multiple__expected)
{
    data_chunk head_file;
    data_chunk body_file;
    test::storage head_store{ head_file };
    test::storage body_store{ body_file };
    hashmap<link5, key1, big_record::size> instance{ head_store, body_store, buckets };
    BOOST_REQUIRE(instance.create());

    constexpr key1 key1_big{ 0x41 };
    constexpr key1 key1_little{ 0x42 };
    BOOST_REQUIRE(instance.put(key1_big, big_record{ 0xa1b2c3d4_u32 }));
    BOOST_REQUIRE(instance.put(key1_little, little_record{ 0xa1b2c3d4_u32 }));

    big_record record1{};
    BOOST_REQUIRE(instance.get(key1_big, record1));
    BOOST_REQUIRE_EQUAL(record1.value, 0xa1b2c3d4_u32);

    little_record record2{};
    BOOST_REQUIRE(instance.get(key1_little, record2));
    BOOST_REQUIRE_EQUAL(record2.value, 0xa1b2c3d4_u32);

    // This expecatation relies on the fact of no hash table conflict between 0x41 and 0x42.
    const data_chunk expected_file
    {
        0xff, 0xff, 0xff, 0xff, 0xff,
        0x41,
        0xa1, 0xb2, 0xc3, 0xd4,

        0xff, 0xff, 0xff, 0xff, 0xff,
        0x42,
        0xd4, 0xc3, 0xb2, 0xa1
    };
    BOOST_REQUIRE_EQUAL(body_file, expected_file);
}

class little_slab
{
public:
    static constexpr size_t size = max_size_t;
    static constexpr link5 count() NOEXCEPT
    {
        return link5::size + array_count<key1> + sizeof(uint32_t);
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
    static constexpr link5 count() NOEXCEPT
    {
        return link5::size + array_count<key1> + sizeof(uint32_t);
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

BOOST_AUTO_TEST_CASE(hashmap__slab_put__get__expected)
{
    data_chunk head_file;
    data_chunk body_file;
    test::storage head_store{ head_file };
    test::storage body_store{ body_file };

    hashmap<link5, key1, big_slab::size> instance{ head_store, body_store, buckets };
    BOOST_REQUIRE(instance.create());

    constexpr key1 key{ 0x42 };
    BOOST_REQUIRE(instance.put(key, big_slab{ 0xa1b2c3d4_u32 }));

    big_slab slab{};
    BOOST_REQUIRE(instance.get(zero, slab));
    BOOST_REQUIRE_EQUAL(slab.value, 0xa1b2c3d4_u32);

    const data_chunk expected_file
    {
        0xff, 0xff, 0xff, 0xff, 0xff,
        0x42,
        0xa1, 0xb2, 0xc3, 0xd4
    };
    BOOST_REQUIRE_EQUAL(body_file, expected_file);
}

BOOST_AUTO_TEST_CASE(hashmap__slab_put__multiple__expected)
{
    data_chunk head_file;
    data_chunk body_file;
    test::storage head_store{ head_file };
    test::storage body_store{ body_file };

    hashmap<link5, key1, big_slab::size> instance{ head_store, body_store, buckets };
    BOOST_REQUIRE(instance.create());

    constexpr key1 key_big{ 0x41 };
    constexpr key1 key_little{ 0x42 };
    BOOST_REQUIRE(instance.put(key_big, big_slab{ 0xa1b2c3d4_u32 }));
    BOOST_REQUIRE(instance.put(key_little, little_slab{ 0xa1b2c3d4_u32 }));

    big_slab slab1{};
    BOOST_REQUIRE(instance.get(zero, slab1));
    BOOST_REQUIRE_EQUAL(slab1.value, 0xa1b2c3d4_u32);

    little_slab slab2{};
    BOOST_REQUIRE(instance.get(big_slab::count(), slab2));
    BOOST_REQUIRE_EQUAL(slab2.value, 0xa1b2c3d4_u32);

    // This expecatation relies on the fact of no hash table conflict between 0x41 and 0x42.
    const data_chunk expected_file
    {
        0xff, 0xff, 0xff, 0xff, 0xff,
        0x41,
        0xa1, 0xb2, 0xc3, 0xd4,

        0xff, 0xff, 0xff, 0xff, 0xff,
        0x42,
        0xd4, 0xc3, 0xb2, 0xa1
    };
    BOOST_REQUIRE_EQUAL(body_file, expected_file);
}

// advertises 32 but reads/writes 64
class record_excess
{
public:
    static constexpr size_t size = sizeof(uint32_t);
    static constexpr link5 count() NOEXCEPT { return 1; }

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

BOOST_AUTO_TEST_CASE(hashmap__record_get__excess__false)
{
    data_chunk head_file;
    data_chunk body_file;
    test::storage head_store{ head_file };
    test::storage body_store{ body_file };
    hashmap<link5, key1, big_record::size> instance{ head_store, body_store, buckets };
    BOOST_REQUIRE(instance.create());

    constexpr key1 key{ 0x41 };
    BOOST_REQUIRE(instance.put(key, big_record{ 0xa1b2c3d4_u32 }));

    record_excess record{};
    BOOST_REQUIRE(!instance.get(zero, record));
}

BOOST_AUTO_TEST_CASE(hashmap__record_put__excess__false)
{
    data_chunk head_file;
    data_chunk body_file;
    test::storage head_store{ head_file };
    test::storage body_store{ body_file };
    hashmap<link5, key1, big_record::size> instance{ head_store, body_store, buckets };
    BOOST_REQUIRE(instance.create());

    constexpr key1 key{ 0x41 };
    BOOST_REQUIRE(!instance.put(key, record_excess{ 0xa1b2c3d4_u32 }));
}

// advertises 32 but reads/writes 64
class slab_excess
{
public:
    static constexpr size_t size = max_size_t;
    static constexpr link5 count() NOEXCEPT { return sizeof(uint32_t); }

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

// advertises 32 but reads 65 (file is 64)/writes 64
class file_excess
{
public:
    static constexpr size_t size = max_size_t;
    static constexpr link5 count() NOEXCEPT { return sizeof(uint32_t); }

    bool from_data(database::reader& source) NOEXCEPT
    {
        value = source.read_big_endian<uint64_t>();
        source.read_byte();
        return source;
    }

    bool to_data(database::finalizer& sink) const NOEXCEPT
    {
        sink.write_big_endian(value);
        return sink;
    }

    uint64_t value{ 0 };
};

BOOST_AUTO_TEST_CASE(hashmap__slab_get__excess__true)
{
    data_chunk head_file;
    data_chunk body_file;
    test::storage head_store{ head_file };
    test::storage body_store{ body_file };
    hashmap<link5, key1, big_slab::size> instance{ head_store, body_store, buckets };
    BOOST_REQUIRE(instance.create());

    constexpr key1 key{ 0x41 };
    BOOST_REQUIRE(instance.put(key, big_slab{ 0xa1b2c3d4_u32 }));
    BOOST_REQUIRE(instance.put(key, big_slab{ 0xa1b2c3d4_u32 }));

    // Excess read allowed to eof here (reader has only knowledge of size).
    slab_excess slab{};
    BOOST_REQUIRE(instance.get<slab_excess>(zero, slab));
}

BOOST_AUTO_TEST_CASE(hashmap__slab_get__file_excess__false)
{
    data_chunk head_file;
    data_chunk body_file;
    test::storage head_store{ head_file };
    test::storage body_store{ body_file };
    hashmap<link5, key1, big_slab::size> instance{ head_store, body_store, buckets };
    BOOST_REQUIRE(instance.create());

    constexpr key1 key{ 0x41 };
    BOOST_REQUIRE(instance.put(key, big_slab{ 0xa1b2c3d4_u32 }));

    // Excess read disallowed to here (past eof).
    slab_excess slab{};
    BOOST_REQUIRE(!instance.get<slab_excess>(zero, slab));
}

BOOST_AUTO_TEST_CASE(hashmap__slab_put__excess__false)
{
    data_chunk head_file;
    data_chunk body_file;
    test::storage head_store{ head_file };
    test::storage body_store{ body_file };
    hashmap<link5, key1, big_slab::size> instance{ head_store, body_store, buckets };
    BOOST_REQUIRE(instance.create());

    constexpr key1 key{ 0x41 };
    BOOST_REQUIRE(!instance.put(key, slab_excess{ 0xa1b2c3d4_u32 }));
}

BOOST_AUTO_TEST_CASE(hashmap__record_exists__exists__true)
{
    data_chunk head_file;
    data_chunk body_file;
    test::storage head_store{ head_file };
    test::storage body_store{ body_file };
    hashmap<link5, key1, big_record::size> instance{ head_store, body_store, buckets };
    BOOST_REQUIRE(instance.create());

    constexpr key1 key{ 0x41 };
    BOOST_REQUIRE(!instance.exists(key));
    BOOST_REQUIRE(instance.put(key, big_record{ 0xa1b2c3d4_u32 }));
    BOOST_REQUIRE(instance.exists(key));
}

BOOST_AUTO_TEST_CASE(hashmap__slab_exists__exists__true)
{
    data_chunk head_file;
    data_chunk body_file;
    test::storage head_store{ head_file };
    test::storage body_store{ body_file };
    hashmap<link5, key1, big_slab::size> instance{ head_store, body_store, buckets };
    BOOST_REQUIRE(instance.create());

    constexpr key1 key{ 0x41 };
    BOOST_REQUIRE(!instance.exists(key));
    BOOST_REQUIRE(instance.put(key, big_slab{ 0xa1b2c3d4_u32 }));
    BOOST_REQUIRE(instance.exists(key));
}

BOOST_AUTO_TEST_CASE(hashmap__record_it__exists__non_terminal)
{
    data_chunk head_file;
    data_chunk body_file;
    test::storage head_store{ head_file };
    test::storage body_store{ body_file };
    hashmap<link5, key1, big_record::size> instance{ head_store, body_store, buckets };
    BOOST_REQUIRE(instance.create());

    constexpr key1 key{ 0x41 };
    BOOST_REQUIRE(instance.it(key).self().is_terminal());
    BOOST_REQUIRE(instance.put(key, big_record{ 0xa1b2c3d4_u32 }));
    BOOST_REQUIRE(!instance.it(key).self().is_terminal());

    big_record record{};
    BOOST_REQUIRE(instance.get(instance.it(key).self(), record));
}

BOOST_AUTO_TEST_CASE(hashmap__record_it__multiple__iterated)
{
    data_chunk head_file;
    data_chunk body_file;
    test::storage head_store{ head_file };
    test::storage body_store{ body_file };
    hashmap<link5, key1, big_record::size> instance{ head_store, body_store, buckets };
    BOOST_REQUIRE(instance.create());

    constexpr key1 key_a{ 0xaa };
    constexpr key1 key_b{ 0xbb };
    constexpr key1 key_c{ 0xcc };

    BOOST_REQUIRE(instance.put(key_a, big_record{ 0x000000a1_u32 }));
    BOOST_REQUIRE(instance.put(key_a, big_record{ 0x000000a2_u32 }));
    BOOST_REQUIRE(instance.put(key_a, big_record{ 0x000000a3_u32 }));
    BOOST_REQUIRE(instance.put(key_b, big_record{ 0x000000b1_u32 }));
    BOOST_REQUIRE(instance.put(key_b, big_record{ 0x000000b2_u32 }));
    BOOST_REQUIRE(instance.put(key_b, big_record{ 0x000000b3_u32 }));
    BOOST_REQUIRE(instance.put(key_c, big_record{ 0x000000c1_u32 }));
    BOOST_REQUIRE(instance.put(key_c, big_record{ 0x000000c2_u32 }));
    BOOST_REQUIRE(instance.put(key_c, big_record{ 0x000000c3_u32 }));

    auto it_a = instance.it(key_a);

    big_record record{};
    BOOST_REQUIRE(instance.get(it_a.self(), record));
    BOOST_REQUIRE_EQUAL(record.value, 0x000000a3_u32);
    BOOST_REQUIRE(it_a.advance());
    BOOST_REQUIRE(instance.get(it_a.self(), record));
    BOOST_REQUIRE_EQUAL(record.value, 0x000000a2_u32);
    BOOST_REQUIRE(it_a.advance());
    BOOST_REQUIRE(instance.get(it_a.self(), record));
    BOOST_REQUIRE_EQUAL(record.value, 0x000000a1_u32);
    BOOST_REQUIRE(!it_a.advance());
    BOOST_REQUIRE(!instance.get(it_a.self(), record));

    auto it_b = instance.it(key_b);

    BOOST_REQUIRE(instance.get(it_b.self(), record));
    BOOST_REQUIRE_EQUAL(record.value, 0x000000b3_u32);
    BOOST_REQUIRE(it_b.advance());
    BOOST_REQUIRE(instance.get(it_b.self(), record));
    BOOST_REQUIRE_EQUAL(record.value, 0x000000b2_u32);
    BOOST_REQUIRE(it_b.advance());
    BOOST_REQUIRE(instance.get(it_b.self(), record));
    BOOST_REQUIRE_EQUAL(record.value, 0x000000b1_u32);
    BOOST_REQUIRE(!it_b.advance());
    BOOST_REQUIRE(!instance.get(it_b.self(), record));

    auto it_c = instance.it(key_c);

    BOOST_REQUIRE(instance.get(it_c.self(), record));
    BOOST_REQUIRE_EQUAL(record.value, 0x000000c3_u32);
    BOOST_REQUIRE(it_c.advance());
    BOOST_REQUIRE(instance.get(it_c.self(), record));
    BOOST_REQUIRE_EQUAL(record.value, 0x000000c2_u32);
    BOOST_REQUIRE(it_c.advance());
    BOOST_REQUIRE(instance.get(it_c.self(), record));
    BOOST_REQUIRE_EQUAL(record.value, 0x000000c1_u32);
    BOOST_REQUIRE(!it_c.advance());
    BOOST_REQUIRE(!instance.get(it_c.self(), record));

    //   [0000000000]
    //[b] 0500000000
    //    ffffffffff
    //    ffffffffff
    //[a] 0200000000
    //    ffffffffff
    //    ffffffffff
    //    ffffffffff
    //    ffffffffff
    //    ffffffffff
    //    ffffffffff
    //    ffffffffff
    //    ffffffffff
    //    ffffffffff
    //    ffffffffff
    //    ffffffffff
    //    ffffffffff
    //    ffffffffff
    //[c] 0800000000
    //    ffffffffff
    //    ffffffffff
    //==================
    //[0] ffffffffff
    //    aa
    //    000000a1
    //
    //[1] 0000000000
    //    aa
    //    000000a2
    //
    //[2] 0100000000
    //    aa
    //    000000a3
    //
    //[3] ffffffffff
    //    bb
    //    000000b1
    //
    //[4] 0300000000
    //    bb
    //    000000b2
    //
    //[5] 0400000000
    //    bb
    //    000000b3
    //
    //[6] ffffffffff
    //    cc
    //    000000c1
    //
    //[7] 0600000000
    //    cc
    //    000000c2
    //
    //[8] 0700000000
    //    cc
    //    000000c3
}

// mutiphase commit.
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(hashmap__allocate__terminal__expected)
{
    data_chunk head_file;
    data_chunk body_file;
    test::storage head_store{ head_file };
    test::storage body_store{ body_file };
    record_table instance{ head_store, body_store, 2 };
    BOOST_REQUIRE(instance.create());
    BOOST_REQUIRE(instance.allocate(link5::terminal).is_terminal());
    BOOST_REQUIRE(body_file.empty());
}

BOOST_AUTO_TEST_CASE(hashmap__allocate__record__expected)
{
    data_chunk head_file;
    data_chunk body_file;
    test::storage head_store{ head_file };
    test::storage body_store{ body_file };
    record_table instance{ head_store, body_store, 2 };
    BOOST_REQUIRE(instance.create());
    BOOST_REQUIRE_EQUAL(instance.allocate(1), 0u);
    BOOST_REQUIRE_EQUAL(body_file.size(), element_size);
}

BOOST_AUTO_TEST_CASE(hashmap__allocate__3_records__expected)
{
    data_chunk head_file;
    data_chunk body_file;
    test::storage head_store{ head_file };
    test::storage body_store{ body_file };
    record_table instance{ head_store, body_store, 2 };
    BOOST_REQUIRE(instance.create());
    BOOST_REQUIRE_EQUAL(instance.allocate(3), 0u);
    BOOST_REQUIRE_EQUAL(body_file.size(), 3u * element_size);
    BOOST_REQUIRE_EQUAL(instance.allocate(1), 3u);
    BOOST_REQUIRE_EQUAL(body_file.size(), 4u * element_size);
}

BOOST_AUTO_TEST_CASE(hashmap__allocate__slab__allocated)
{
    data_chunk head_file;
    data_chunk body_file;
    test::storage head_store{ head_file };
    test::storage body_store{ body_file };
    slab_table instance{ head_store, body_store, 2 };
    BOOST_REQUIRE(instance.create());
    BOOST_REQUIRE_EQUAL(instance.allocate(42), 0u);
    BOOST_REQUIRE_EQUAL(body_file.size(), 42u);
    BOOST_REQUIRE_EQUAL(instance.allocate(24), 42);
    BOOST_REQUIRE_EQUAL(body_file.size(), 42u + 24u);
}

class flex_record
{
public:
    static constexpr size_t size = sizeof(uint32_t);

    template <typename Sinker>
    bool to_data(Sinker& sink) const NOEXCEPT
    {
        sink.write_little_endian(value);
        return sink;
    }

    uint32_t value{ 0 };
};

class flex_slab
{
public:
    static constexpr size_t size = max_size_t;

    template <typename Sinker>
    bool to_data(Sinker& sink) const NOEXCEPT
    {
        sink.write_little_endian(value);
        return sink;
    }

    uint32_t value{ 0 };
};

BOOST_AUTO_TEST_CASE(hashmap__allocate_set_commit__record__expected)
{
    data_chunk head_file;
    data_chunk body_file;
    test::storage head_store{ head_file };
    test::storage body_store{ body_file };
    hashmap<link5, key10, flex_record::size> instance{ head_store, body_store, 2 };
    BOOST_REQUIRE(instance.create());

    const auto link = instance.allocate(1);
    BOOST_REQUIRE_EQUAL(link, 0u);

    constexpr auto size = link5::size + array_count<key10> +flex_record::size;
    BOOST_REQUIRE_EQUAL(body_file.size(), size);

    BOOST_REQUIRE(instance.set(link, flex_record{ 0x01020304_u32 }));
    BOOST_REQUIRE_EQUAL(head_file, base16_chunk("0000000000ffffffffffffffffffff"));
    BOOST_REQUIRE_EQUAL(body_file, base16_chunk("00000000000000000000000000000004030201"));

    constexpr key10 key1{ 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a };
    BOOST_REQUIRE(instance.commit(key1, link));
    BOOST_REQUIRE_EQUAL(head_file, base16_chunk("00000000000000000000ffffffffff"));
    BOOST_REQUIRE_EQUAL(body_file, base16_chunk("ffffffffff0102030405060708090a04030201"));
}

BOOST_AUTO_TEST_CASE(hashmap__allocate_set_commit__slab__expected)
{
    data_chunk head_file;
    data_chunk body_file;
    test::storage head_store{ head_file };
    test::storage body_store{ body_file };
    hashmap<link5, key10, flex_slab::size> instance{ head_store, body_store, 2 };
    BOOST_REQUIRE(instance.create());

    constexpr auto size = link5::size + array_count<key10> + sizeof(uint32_t);
    const auto link = instance.allocate(size);
    BOOST_REQUIRE_EQUAL(link, 0u);
    BOOST_REQUIRE_EQUAL(body_file.size(), size);

    BOOST_REQUIRE(instance.set(link, flex_slab{ 0x01020304_u32 }));
    BOOST_REQUIRE_EQUAL(head_file, base16_chunk("0000000000ffffffffffffffffffff"));
    BOOST_REQUIRE_EQUAL(body_file, base16_chunk("00000000000000000000000000000004030201"));

    constexpr key10 key1{ 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a };
    BOOST_REQUIRE(instance.commit(key1, link));
    BOOST_REQUIRE_EQUAL(head_file, base16_chunk("00000000000000000000ffffffffff"));
    BOOST_REQUIRE_EQUAL(body_file, base16_chunk("ffffffffff0102030405060708090a04030201"));
}

////std::cout << head_file << std::endl << std::endl;
////std::cout << body_file << std::endl << std::endl;

BOOST_AUTO_TEST_SUITE_END()
