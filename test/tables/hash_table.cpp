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

BOOST_AUTO_TEST_SUITE(hash_table_tests)

constexpr auto link_size = 5_size;
constexpr auto key_size = 10_size;
constexpr auto header_size = 105_size;

// Key size does not factor into header byte size (for search key only).
constexpr auto links = header_size / link_size;
static_assert(links == 21u);

// Bucket count is one less than link count, due to header.size field.
constexpr auto buckets = sub1(links);
static_assert(buckets == 20u);

// Record size includes key but not link.
// Slab allocation includes key and link.
constexpr auto record_size = key_size + 4_size;

using link = linkage<link_size>;
using key = data_array<key_size>;

using record_item = element<link, key, record_size>;
using record_table = hash_table<record_item>;

// record_hash_table__create_verify

BOOST_AUTO_TEST_CASE(record_hash_table__create_verify__empty_files__success)
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

BOOST_AUTO_TEST_CASE(record_hash_table__create_verify__non_empty_head_file__failure)
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

BOOST_AUTO_TEST_CASE(record_hash_table__create_verify__multiple_element_body_file__failure)
{
    constexpr auto body_size = 3u * (link_size + record_size);
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

BOOST_AUTO_TEST_CASE(record_hash_table__create_verify__multiple_fractional_element_body_file__failure)
{
    constexpr auto body_size = 3u * (link_size + record_size) + 2u;
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

BOOST_AUTO_TEST_CASE(record_hash_table__create_verify__one_element_body_file__failure)
{
    constexpr auto body_size = link_size + record_size;
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

BOOST_AUTO_TEST_CASE(record_hash_table__create_verify__sub_one_element_body_file__success)
{
    constexpr auto body_size = sub1(link_size + record_size);
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

// slab_hash_table__create_verify

constexpr auto slab_size = link_size + key_size + 4_size;
using slab_item = element<link, key, zero>;
using slab_table = hash_table<slab_item>;

BOOST_AUTO_TEST_CASE(slab_hash_table__create_verify__empty_files__success)
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

BOOST_AUTO_TEST_CASE(slab_hash_table__create_verify__non_empty_head_file__failure)
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

BOOST_AUTO_TEST_CASE(slab_hash_table__create_verify__multiple_element_body_file__failure)
{
    constexpr auto body_size = 3u * slab_size;
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

BOOST_AUTO_TEST_CASE(slab_hash_table__create_verify__multiple_fractional_element_body_file__failure)
{
    constexpr auto body_size = 3u * slab_size + 2u;
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

BOOST_AUTO_TEST_CASE(slab_hash_table__create_verify__one_element_body_file__failure)
{
    constexpr auto body_size = slab_size;
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

BOOST_AUTO_TEST_CASE(slab_hash_table__create_verify__sub_one_element_body_file__failure)
{
    constexpr auto body_size = sub1(slab_size);
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

// at(terminal)

BOOST_AUTO_TEST_CASE(record_hash_table__at__terminal__false)
{
    data_chunk head_file;
    data_chunk body_file;
    test::storage head_store{ head_file };
    test::storage body_store{ body_file };
    record_table instance{ head_store, body_store, buckets };
    BOOST_REQUIRE(instance.create());
    BOOST_REQUIRE(instance.verify());
    BOOST_REQUIRE(!instance.at(link::terminal));
    BOOST_REQUIRE(!instance.at(link::terminal));
}

BOOST_AUTO_TEST_CASE(slab_hash_table__at__terminal__false)
{
    data_chunk head_file;
    data_chunk body_file;
    test::storage head_store{ head_file };
    test::storage body_store{ body_file };
    slab_table instance{ head_store, body_store, buckets };
    BOOST_REQUIRE(instance.create());
    BOOST_REQUIRE(instance.verify());
    BOOST_REQUIRE(!instance.at(link::terminal));
    BOOST_REQUIRE(!instance.at(link::terminal));
}


// at(exhausted)

BOOST_AUTO_TEST_CASE(record_hash_table__at__empty__exhausted)
{
    data_chunk head_file;
    data_chunk body_file;
    test::storage head_store{ head_file };
    test::storage body_store{ body_file };
    record_table instance{ head_store, body_store, buckets };
    BOOST_REQUIRE(instance.create());
    BOOST_REQUIRE(instance.verify());
    BOOST_REQUIRE(instance.at(0)->is_exhausted());
    BOOST_REQUIRE(instance.at(19)->is_exhausted());
}

BOOST_AUTO_TEST_CASE(slab_hash_table__at__empty__exhausted)
{
    data_chunk head_file;
    data_chunk body_file;
    test::storage head_store{ head_file };
    test::storage body_store{ body_file };
    slab_table instance{ head_store, body_store, buckets };
    BOOST_REQUIRE(instance.create());
    BOOST_REQUIRE(instance.verify());
    BOOST_REQUIRE(instance.at(0)->is_exhausted());
    BOOST_REQUIRE(instance.at(19)->is_exhausted());
}

// find(not found)

BOOST_AUTO_TEST_CASE(record_hash_table__find__empty__false)
{
    data_chunk head_file;
    data_chunk body_file;
    test::storage head_store{ head_file };
    test::storage body_store{ body_file };
    record_table instance{ head_store, body_store, buckets };
    BOOST_REQUIRE(instance.create());
    BOOST_REQUIRE(instance.verify());
    BOOST_REQUIRE(!instance.find({ 0x00 }));
    BOOST_REQUIRE(!instance.find({ 0x42 }));
}

BOOST_AUTO_TEST_CASE(slab_hash_table__find__empty__false)
{
    data_chunk head_file;
    data_chunk body_file;
    test::storage head_store{ head_file };
    test::storage body_store{ body_file };
    slab_table instance{ head_store, body_store, buckets };
    BOOST_REQUIRE(instance.create());
    BOOST_REQUIRE(instance.verify());
    BOOST_REQUIRE(!instance.find({ 0x00 }));
    BOOST_REQUIRE(!instance.find({ 0x42 }));
}

// push(terminal)

BOOST_AUTO_TEST_CASE(record_hash_table__push__terminal_false)
{
    data_chunk head_file;
    data_chunk body_file;
    test::storage head_store{ head_file };
    test::storage body_store{ body_file };
    record_table instance{ head_store, body_store, buckets };
    BOOST_REQUIRE(instance.create());
    BOOST_REQUIRE(instance.verify());
    BOOST_REQUIRE(!instance.push(key{ 0x00 }, link::terminal));
    BOOST_REQUIRE(!instance.push(key{ 0x42 }, link::terminal));
}

BOOST_AUTO_TEST_CASE(slab_hash_table__push__terminal_false)
{
    data_chunk head_file;
    data_chunk body_file;
    test::storage head_store{ head_file };
    test::storage body_store{ body_file };
    slab_table instance{ head_store, body_store, buckets };
    BOOST_REQUIRE(instance.create());
    BOOST_REQUIRE(instance.verify());
    BOOST_REQUIRE(!instance.push(key{ 0x00 }, link::terminal));
    BOOST_REQUIRE(!instance.push(key{ 0x42 }, link::terminal));
}

// push(valid)/find(found)

BOOST_AUTO_TEST_CASE(record_hash_table__push_find__empty__true)
{
    data_chunk head_file;
    data_chunk body_file;
    test::storage head_store{ head_file };
    test::storage body_store{ body_file };
    record_table instance{ head_store, body_store, buckets };
    BOOST_REQUIRE(instance.create());
    BOOST_REQUIRE(instance.verify());

    BOOST_REQUIRE(!instance.find({ 0x00 }));
    BOOST_REQUIRE(instance.push(key{ 0x00 }));
    BOOST_REQUIRE(instance.find({ 0x00 }));

    BOOST_REQUIRE(!instance.find({ 0x42 }));
    BOOST_REQUIRE(instance.push(key{ 0x42 }));
    //BOOST_REQUIRE(instance.find({ 0x42 }));
}

BOOST_AUTO_TEST_CASE(slab_hash_table__push_find__empty__true)
{
    data_chunk head_file;
    data_chunk body_file;
    test::storage head_store{ head_file };
    test::storage body_store{ body_file };
    slab_table instance{ head_store, body_store, buckets };
    BOOST_REQUIRE(instance.create());
    BOOST_REQUIRE(instance.verify());

    BOOST_REQUIRE(!instance.find({ 0x00 }));
    BOOST_REQUIRE(instance.push(key{ 0x00 }, slab_size));
    BOOST_REQUIRE(instance.find({ 0x00 }));

    BOOST_REQUIRE(!instance.find({ 0x42 }));
    BOOST_REQUIRE(instance.push(key{ 0x42 }, slab_size));
    //BOOST_REQUIRE(instance.find({ 0x42 }));
}

BOOST_AUTO_TEST_SUITE_END()
