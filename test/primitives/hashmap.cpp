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

template <typename Link, typename Key, typename Record>
class hashmap_
  : public hashmap<Link, Key, Record>
{
public:
    using hashmap<Link, Key, Record>::hashmap;

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
    using base = hashmap<Link, Key, Record>;
};

using link5 = linkage<5>;
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
using slab_table = hashmap_<link5, key10, record0>;
using record_table = hashmap_<link5, key10, record4>;

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

    ////std::cout << head_file << std::endl << std::endl;
    ////std::cout << body_file << std::endl << std::endl;

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

BOOST_AUTO_TEST_SUITE_END()
