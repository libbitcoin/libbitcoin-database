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

    reader_ptr find_(const Key& key) const NOEXCEPT
    {
        return base::find(key);
    }

    reader_ptr at_(const Link& record) const NOEXCEPT
    {
        return base::at(record);
    }

    finalizer_ptr push_(const Key& key, const Link& size=bc::one) NOEXCEPT
    {
        return base::push(key, size);
    }

private:
    using base = hashmap<Link, Key, Size>;
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

struct record0 { static constexpr size_t size = zero; };
struct record4 { static constexpr size_t size = 4; };
using slab_table = hashmap_<link5, key10, record0::size>;
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

BOOST_AUTO_TEST_CASE(hashmap__record_at__terminal__false)
{
    data_chunk head_file;
    data_chunk body_file;
    test::storage head_store{ head_file };
    test::storage body_store{ body_file };
    record_table instance{ head_store, body_store, buckets };
    BOOST_REQUIRE(instance.create());
    BOOST_REQUIRE(!instance.at_(link5::terminal));
    BOOST_REQUIRE(!instance.at_(link5::terminal));
}

BOOST_AUTO_TEST_CASE(hashmap__record_at__empty__exhausted)
{
    data_chunk head_file;
    data_chunk body_file;
    test::storage head_store{ head_file };
    test::storage body_store{ body_file };
    record_table instance{ head_store, body_store, buckets };
    BOOST_REQUIRE(instance.create());
    BOOST_REQUIRE(instance.at_(0)->is_exhausted());
    BOOST_REQUIRE(instance.at_(19)->is_exhausted());
}

BOOST_AUTO_TEST_CASE(hashmap__record_find__empty__false)
{
    data_chunk head_file;
    data_chunk body_file;
    test::storage head_store{ head_file };
    test::storage body_store{ body_file };
    record_table instance{ head_store, body_store, buckets };
    BOOST_REQUIRE(instance.create());
    BOOST_REQUIRE(!instance.find_({ 0x00 }));
    BOOST_REQUIRE(!instance.find_({ 0x42 }));
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

BOOST_AUTO_TEST_CASE(slab_hashmap__at__terminal__false)
{
    data_chunk head_file;
    data_chunk body_file;
    test::storage head_store{ head_file };
    test::storage body_store{ body_file };
    slab_table instance{ head_store, body_store, buckets };
    BOOST_REQUIRE(instance.create());
    BOOST_REQUIRE(!instance.at_(link5::terminal));
    BOOST_REQUIRE(!instance.at_(link5::terminal));
}

BOOST_AUTO_TEST_CASE(slab_hashmap__at__empty__exhausted)
{
    data_chunk head_file;
    data_chunk body_file;
    test::storage head_store{ head_file };
    test::storage body_store{ body_file };
    slab_table instance{ head_store, body_store, buckets };
    BOOST_REQUIRE(instance.create());
    BOOST_REQUIRE(instance.at_(0)->is_exhausted());
    BOOST_REQUIRE(instance.at_(19)->is_exhausted());
}

BOOST_AUTO_TEST_CASE(slab_hashmap__find__empty__false)
{
    data_chunk head_file;
    data_chunk body_file;
    test::storage head_store{ head_file };
    test::storage body_store{ body_file };
    slab_table instance{ head_store, body_store, buckets };
    BOOST_REQUIRE(instance.create());
    BOOST_REQUIRE(!instance.find_({ 0x00 }));
    BOOST_REQUIRE(!instance.find_({ 0x42 }));
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
    auto stream0 = instance.push_(key0);
    BOOST_REQUIRE_EQUAL(body_file.size(), element_size);
    BOOST_REQUIRE(!stream0->is_exhausted());
    BOOST_REQUIRE(!instance.find_(key0));
    BOOST_REQUIRE(stream0->finalize());
    BOOST_REQUIRE(instance.find_(key0));
    stream0.reset();

    constexpr key10 key1{ 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a };
    auto stream1 = instance.push_(key1);
    BOOST_REQUIRE_EQUAL(body_file.size(), 2u * element_size);
    BOOST_REQUIRE(!stream1->is_exhausted());
    BOOST_REQUIRE(!instance.find_(key1));
    BOOST_REQUIRE(stream1->finalize());
    BOOST_REQUIRE(instance.find_(key1));
    stream1.reset();

    // Past end is valid pointer but exhausted stream.
    BOOST_REQUIRE(instance.at_(0));
    BOOST_REQUIRE(!instance.at_(0)->is_exhausted());
    BOOST_REQUIRE(instance.at_(1));
    BOOST_REQUIRE(!instance.at_(1)->is_exhausted());
    BOOST_REQUIRE(instance.at_(2));
    BOOST_REQUIRE(instance.at_(2)->is_exhausted());

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

BOOST_AUTO_TEST_CASE(hashmap__slab_push_find__empty__true)
{
    data_chunk head_file;
    data_chunk body_file;
    test::storage head_store{ head_file };
    test::storage body_store{ body_file };
    slab_table instance{ head_store, body_store, buckets };
    BOOST_REQUIRE(instance.create());

    constexpr key10 key0{ 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a };
    BOOST_REQUIRE(!instance.find_(key0));
    BOOST_REQUIRE(instance.push_(key0, element_size)->finalize());
    BOOST_REQUIRE_EQUAL(body_file.size(), element_size);
    BOOST_REQUIRE(instance.find_(key0));

    constexpr key10 key1{ 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a };
    BOOST_REQUIRE(!instance.find_(key1));
    BOOST_REQUIRE(instance.push_(key1, element_size)->finalize());
    BOOST_REQUIRE_EQUAL(body_file.size(), 2u * element_size);
    BOOST_REQUIRE(instance.find_(key1));

    // Past end is valid pointer but exhausted stream.
    BOOST_REQUIRE(instance.at_(0));
    BOOST_REQUIRE(!instance.at_(0)->is_exhausted());
    BOOST_REQUIRE(instance.at_(element_size));
    BOOST_REQUIRE(!instance.at_(element_size)->is_exhausted());
    BOOST_REQUIRE(instance.at_(2u * element_size));
    BOOST_REQUIRE(instance.at_(2u * element_size)->is_exhausted());
 
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

BOOST_AUTO_TEST_CASE(hashmap__record_push_duplicate_key__find__true)
{
    data_chunk head_file;
    data_chunk body_file;
    test::storage head_store{ head_file };
    test::storage body_store{ body_file };
    record_table instance{ head_store, body_store, buckets };
    BOOST_REQUIRE(instance.create());

    constexpr key10 key0{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    BOOST_REQUIRE(instance.push_(key0)->finalize());
    BOOST_REQUIRE_EQUAL(body_file.size(), element_size);
    BOOST_REQUIRE(instance.find_(key0));

    constexpr key10 key1{ 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a };
    BOOST_REQUIRE(instance.push_(key1)->finalize());
    BOOST_REQUIRE_EQUAL(body_file.size(), 2u * element_size);
    BOOST_REQUIRE(instance.find_(key1));

    BOOST_REQUIRE(instance.push_(key1)->finalize());
    BOOST_REQUIRE_EQUAL(body_file.size(), 3u * element_size);
    BOOST_REQUIRE(instance.find_(key1));

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

    little_record from_data(database::reader& source) NOEXCEPT
    {
        value = source.read_little_endian<uint32_t>();
        valid = source;
        return *this;
    }

    bool to_data(database::finalizer& sink) const NOEXCEPT
    {
        sink.write_little_endian(value);
        return sink;
    }

    uint32_t value{ 0 };
    bool valid{ false };
};

class big_record
{
public:
    static constexpr size_t size = sizeof(uint32_t);
    static constexpr link5 count() NOEXCEPT { return 1; }

    big_record from_data(database::reader& source) NOEXCEPT
    {
        value = source.read_big_endian<uint32_t>();
        valid = source;
        return *this;
    }

    bool to_data(database::finalizer& sink) const NOEXCEPT
    {
        sink.write_big_endian(value);
        return sink;
    }

    uint32_t value{ 0 };
    bool valid{ false };
};

BOOST_AUTO_TEST_CASE(hashmap__record_get__empty__invalid)
{
    data_chunk head_file;
    data_chunk body_file;
    test::storage head_store{ head_file };
    test::storage body_store{ body_file };
    const hashmap<link5, key10, little_record::size> instance{ head_store, body_store, buckets };

    const auto record = instance.get<little_record>(0);
    BOOST_REQUIRE(!record.valid);
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

    const auto record = instance.get<little_record>(0);
    BOOST_REQUIRE(record.valid);
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
    BOOST_REQUIRE(instance.put(key, big_record{ 0xa1b2c3d4_u32, true }));

    const auto link_record = instance.get<big_record>(0);
    BOOST_REQUIRE(link_record.valid);
    BOOST_REQUIRE_EQUAL(link_record.value, 0xa1b2c3d4_u32);

    const auto key_record = instance.get<big_record>(key);
    BOOST_REQUIRE(key_record.valid);
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
    BOOST_REQUIRE(instance.put(key1_big, big_record{ 0xa1b2c3d4_u32, true }));
    BOOST_REQUIRE(instance.put(key1_little, little_record{ 0xa1b2c3d4_u32, true }));

    const auto record1 = instance.get<big_record>(key1_big);
    BOOST_REQUIRE(record1.valid);
    BOOST_REQUIRE_EQUAL(record1.value, 0xa1b2c3d4_u32);

    const auto record2 = instance.get<little_record>(key1_little);
    BOOST_REQUIRE(record2.valid);
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

    ////std::cout << head_file << std::endl << std::endl;
    ////std::cout << body_file << std::endl << std::endl;
}

class little_slab
{
public:
    static constexpr size_t size = zero;
    static constexpr link5 count() NOEXCEPT
    {
        return link5::size + array_count<key1> + sizeof(uint32_t);
    }

    little_slab from_data(database::reader& source) NOEXCEPT
    {
        value = source.read_little_endian<uint32_t>();
        valid = source;
        return *this;
    }

    bool to_data(database::finalizer& sink) const NOEXCEPT
    {
        sink.write_little_endian(value);
        return sink;
    }

    uint32_t value{ 0 };
    bool valid{ false };
};

class big_slab
{
public:
    static constexpr size_t size = zero;
    static constexpr link5 count() NOEXCEPT
    {
        return link5::size + array_count<key1> + sizeof(uint32_t);
    }

    big_slab from_data(database::reader& source) NOEXCEPT
    {
        value = source.read_big_endian<uint32_t>();
        valid = source;
        return *this;
    }

    bool to_data(database::finalizer& sink) const NOEXCEPT
    {
        sink.write_big_endian(value);
        return sink;
    }

    uint32_t value{ 0 };
    bool valid{ false };
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
    BOOST_REQUIRE(instance.put(key, big_slab{ 0xa1b2c3d4_u32, true }));

    const auto slab = instance.get<big_slab>(zero);
    BOOST_REQUIRE(slab.valid);
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
    BOOST_REQUIRE(instance.put(key_big, big_slab{ 0xa1b2c3d4_u32, true }));
    BOOST_REQUIRE(instance.put(key_little, little_slab{ 0xa1b2c3d4_u32, true }));

    const auto slab1 = instance.get<big_slab>(zero);
    BOOST_REQUIRE(slab1.valid);
    BOOST_REQUIRE_EQUAL(slab1.value, 0xa1b2c3d4_u32);

    const auto slab2 = instance.get<little_slab>(big_slab::count());
    BOOST_REQUIRE(slab2.valid);
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

    record_excess from_data(database::reader& source) NOEXCEPT
    {
        value = source.read_big_endian<uint64_t>();
        valid = source;
        return *this;
    }

    bool to_data(database::finalizer& sink) const NOEXCEPT
    {
        sink.write_big_endian(value);
        return sink;
    }

    uint64_t value{ 0 };
    bool valid{ false };
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
    BOOST_REQUIRE(instance.put(key, big_record{ 0xa1b2c3d4_u32, true }));

    const auto record = instance.get<record_excess>(zero);
    BOOST_REQUIRE(!record.valid);
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
    BOOST_REQUIRE(!instance.put(key, record_excess{ 0xa1b2c3d4_u32, true }));
}

// advertises 32 but reads/writes 64
class slab_excess
{
public:
    static constexpr size_t size = zero;
    static constexpr link5 count() NOEXCEPT { return sizeof(uint32_t); }

    slab_excess from_data(database::reader& source) NOEXCEPT
    {
        value = source.read_big_endian<uint64_t>();
        valid = source;
        return *this;
    }

    bool to_data(database::finalizer& sink) const NOEXCEPT
    {
        sink.write_big_endian(value);
        return sink;
    }

    uint64_t value{ 0 };
    bool valid{ false };
};

// advertises 32 but reads 65 (file is 64)/writes 64
class file_excess
{
public:
    static constexpr size_t size = zero;
    static constexpr link5 count() NOEXCEPT { return sizeof(uint32_t); }

    file_excess from_data(database::reader& source) NOEXCEPT
    {
        value = source.read_big_endian<uint64_t>();
        source.read_byte();
        valid = source;
        return *this;
    }

    bool to_data(database::finalizer& sink) const NOEXCEPT
    {
        sink.write_big_endian(value);
        return sink;
    }

    uint64_t value{ 0 };
    bool valid{ false };
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
    BOOST_REQUIRE(instance.put(key, big_slab{ 0xa1b2c3d4_u32, true }));
    BOOST_REQUIRE(instance.put(key, big_slab{ 0xa1b2c3d4_u32, true }));

    // Excess read allowed to eof here (reader has only knowledge of size).
    const auto slab = instance.get<slab_excess>(zero);
    BOOST_REQUIRE(slab.valid);
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
    BOOST_REQUIRE(instance.put(key, big_slab{ 0xa1b2c3d4_u32, true }));

    // Excess read disallowed to here (past eof).
    const auto slab = instance.get<slab_excess>(zero);
    BOOST_REQUIRE(!slab.valid);
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
    BOOST_REQUIRE(!instance.put(key, slab_excess{ 0xa1b2c3d4_u32, true }));
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
    BOOST_REQUIRE(instance.put(key, big_record{ 0xa1b2c3d4_u32, true }));
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
    BOOST_REQUIRE(instance.put(key, big_slab{ 0xa1b2c3d4_u32, true }));
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
    BOOST_REQUIRE(instance.put(key, big_record{ 0xa1b2c3d4_u32, true }));
    BOOST_REQUIRE(!instance.it(key).self().is_terminal());
    BOOST_REQUIRE(instance.get<big_record>(instance.it(key).self()).valid);
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

    BOOST_REQUIRE(instance.put(key_a, big_record{ 0x000000a1_u32, true }));
    BOOST_REQUIRE(instance.put(key_a, big_record{ 0x000000a2_u32, true }));
    BOOST_REQUIRE(instance.put(key_a, big_record{ 0x000000a3_u32, true }));
    BOOST_REQUIRE(instance.put(key_b, big_record{ 0x000000b1_u32, true }));
    BOOST_REQUIRE(instance.put(key_b, big_record{ 0x000000b2_u32, true }));
    BOOST_REQUIRE(instance.put(key_b, big_record{ 0x000000b3_u32, true }));
    BOOST_REQUIRE(instance.put(key_c, big_record{ 0x000000c1_u32, true }));
    BOOST_REQUIRE(instance.put(key_c, big_record{ 0x000000c2_u32, true }));
    BOOST_REQUIRE(instance.put(key_c, big_record{ 0x000000c3_u32, true }));

    auto it_a = instance.it(key_a);

    BOOST_REQUIRE(instance.get<big_record>(it_a.self()).valid);
    BOOST_REQUIRE_EQUAL(instance.get<big_record>(it_a.self()).value, 0x000000a3_u32);
    BOOST_REQUIRE(it_a.next());
    BOOST_REQUIRE(instance.get<big_record>(it_a.self()).valid);
    BOOST_REQUIRE_EQUAL(instance.get<big_record>(it_a.self()).value, 0x000000a2_u32);
    BOOST_REQUIRE(it_a.next());
    BOOST_REQUIRE(instance.get<big_record>(it_a.self()).valid);
    BOOST_REQUIRE_EQUAL(instance.get<big_record>(it_a.self()).value, 0x000000a1_u32);
    BOOST_REQUIRE(!it_a.next());
    BOOST_REQUIRE(!instance.get<big_record>(it_a.self()).valid);

    auto it_b = instance.it(key_b);

    BOOST_REQUIRE(instance.get<big_record>(it_b.self()).valid);
    BOOST_REQUIRE_EQUAL(instance.get<big_record>(it_b.self()).value, 0x000000b3_u32);
    BOOST_REQUIRE(it_b.next());
    BOOST_REQUIRE(instance.get<big_record>(it_b.self()).valid);
    BOOST_REQUIRE_EQUAL(instance.get<big_record>(it_b.self()).value, 0x000000b2_u32);
    BOOST_REQUIRE(it_b.next());
    BOOST_REQUIRE(instance.get<big_record>(it_b.self()).valid);
    BOOST_REQUIRE_EQUAL(instance.get<big_record>(it_b.self()).value, 0x000000b1_u32);
    BOOST_REQUIRE(!it_b.next());
    BOOST_REQUIRE(!instance.get<big_record>(it_b.self()).valid);

    auto it_c = instance.it(key_c);

    BOOST_REQUIRE(instance.get<big_record>(it_c.self()).valid);
    BOOST_REQUIRE_EQUAL(instance.get<big_record>(it_c.self()).value, 0x000000c3_u32);
    BOOST_REQUIRE(it_c.next());
    BOOST_REQUIRE(instance.get<big_record>(it_c.self()).valid);
    BOOST_REQUIRE_EQUAL(instance.get<big_record>(it_c.self()).value, 0x000000c2_u32);
    BOOST_REQUIRE(it_c.next());
    BOOST_REQUIRE(instance.get<big_record>(it_c.self()).valid);
    BOOST_REQUIRE_EQUAL(instance.get<big_record>(it_c.self()).value, 0x000000c1_u32);
    BOOST_REQUIRE(!it_c.next());
    BOOST_REQUIRE(!instance.get<big_record>(it_c.self()).valid);

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

    //std::cout << head_file << std::endl << std::endl;
    //std::cout << body_file << std::endl << std::endl;
}

BOOST_AUTO_TEST_SUITE_END()
