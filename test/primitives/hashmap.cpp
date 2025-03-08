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

BOOST_AUTO_TEST_SUITE(hashmap_tests)

template <typename Link, typename Key, size_t Size, bool Align = false>
class hashmap_
  : public hashmap<Link, Key, Size, Align>
{
public:
    using base = hashmap_<Link, Key, Size, Align>;
    using hashmap<Link, Key, Size, Align>::hashmap;
};

using namespace system;
using link5 = linkage<5>;
using key1 = data_array<1>;
using key10 = data_array<10>;

// Key size does not factor into header byte size (for first key only).
constexpr auto bucket_bits = 4_size;
constexpr auto head_size = add1(system::power2(bucket_bits)) * link5::size;
constexpr auto links = head_size / link5::size;
static_assert(links == 17u);

// Bucket count is one less than link count, due to header.size field.
constexpr auto buckets = bc::sub1(links);
static_assert(buckets == 16u);

struct slab0 { static constexpr size_t size = max_size_t; };
struct record4 { static constexpr size_t size = 4; };
using slab_table = hashmap_<link5, key10, slab0::size>;
using record_table = hashmap_<link5, key10, record4::size>;

constexpr auto element_size = link5::size + array_count<key10> + record4::size;

// record hashmap
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(hashmap__record_construct__empty__expected)
{
    test::chunk_storage head_store{};
    test::chunk_storage body_store{};
    const record_table instance{ head_store, body_store, bucket_bits };
    BOOST_REQUIRE(body_store.buffer().empty());
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(hashmap__record_construct__non_empty__expected)
{
    constexpr auto body_size = 12345u;
    test::chunk_storage head_store{};
    test::chunk_storage body_store{};
    body_store.buffer().resize(body_size);
    const record_table instance{ head_store, body_store, bucket_bits };
    BOOST_REQUIRE_EQUAL(body_store.buffer().size(), body_size);
    BOOST_REQUIRE(!instance.get_fault());
}

// slab hashmap
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(hashmap__slab_construct__empty__expected)
{
    test::chunk_storage head_store{};
    test::chunk_storage body_store{};
    const slab_table instance{ head_store, body_store, bucket_bits };
    BOOST_REQUIRE(body_store.buffer().empty());
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(hashmap__slab_construct__non_empty__expected_enabled)
{
    constexpr auto body_size = 12345u;
    test::chunk_storage head_store{};
    test::chunk_storage body_store{};
    body_store.buffer().resize(body_size);
    const slab_table instance{ head_store, body_store, bucket_bits };
    BOOST_REQUIRE_EQUAL(body_store.buffer().size(), body_size);
    BOOST_REQUIRE(instance.enabled());
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(hashmap__enabled__non_empty_slab_zero_buckets__false)
{
    constexpr auto body_size = 12345u;
    test::chunk_storage head_store{};
    test::chunk_storage body_store{};
    body_store.buffer().resize(body_size);
    const slab_table instance{ head_store, body_store, 0 };
    BOOST_REQUIRE(!instance.enabled());
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(hashmap__enabled__empty_slab_one_bucket__true)
{
    test::chunk_storage head_store{};
    test::chunk_storage body_store{};
    slab_table instance{ head_store, body_store, 1 };
    BOOST_REQUIRE(instance.enabled());
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(hashmap__enabled__empty_slab_nonzero_buckets__true)
{
    test::chunk_storage head_store{};
    test::chunk_storage body_store{};
    slab_table instance{ head_store, body_store, bucket_bits };
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
    test::chunk_storage head_store{};
    test::chunk_storage body_store{};
    const hashmap_<link5, key10, little_record::size> instance{ head_store, body_store, bucket_bits };

    little_record record{};
    BOOST_REQUIRE(!instance.get(link5::terminal, record));
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(hashmap__record_get__empty__invalid)
{
    test::chunk_storage head_store{};
    test::chunk_storage body_store{};
    const hashmap_<link5, key10, little_record::size> instance{ head_store, body_store, bucket_bits };

    little_record record{};
    BOOST_REQUIRE(!instance.get(0, record));
    BOOST_REQUIRE(!instance.get_fault());
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
    test::chunk_storage head_store{ head_file };
    test::chunk_storage body_store{ body_file };
    const hashmap_<link5, key10, little_record::size> instance{ head_store, body_store, bucket_bits };

    little_record record{};
    BOOST_REQUIRE(instance.get(0, record));
    BOOST_REQUIRE_EQUAL(record.value, 0x04030201_u32);
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(hashmap__record_put__multiple__expected)
{
    test::chunk_storage head_store{};
    test::chunk_storage body_store{};
    hashmap_<link5, key1, big_record::size> instance{ head_store, body_store, bucket_bits };
    BOOST_REQUIRE(instance.create());

    constexpr key1 key1_big{ 0x41 };
    constexpr key1 key1_little{ 0x42 };

    link5 link{};
    BOOST_REQUIRE(instance.put_link(link, key1_big, big_record{ 0xa1b2c3d4_u32 }));
    BOOST_REQUIRE(!link.is_terminal());
    BOOST_REQUIRE_EQUAL(link, 0u);

    link = instance.put_link(key1_little, little_record{ 0xa1b2c3d4_u32 });
    BOOST_REQUIRE(!link.is_terminal());
    BOOST_REQUIRE_EQUAL(link, 1u);

    big_record record1{};
    BOOST_REQUIRE(instance.get(0, record1));
    BOOST_REQUIRE_EQUAL(record1.value, 0xa1b2c3d4_u32);

    little_record record2{};
    BOOST_REQUIRE(instance.get(1, record2));
    BOOST_REQUIRE_EQUAL(record2.value, 0xa1b2c3d4_u32);

    // This expectation relies on the fact of no hash table conflict between 0x41 and 0x42.
    const data_chunk expected_file
    {
        0xff, 0xff, 0xff, 0xff, 0xff,
        0x41,
        0xa1, 0xb2, 0xc3, 0xd4,

        0xff, 0xff, 0xff, 0xff, 0xff,
        0x42,
        0xd4, 0xc3, 0xb2, 0xa1
    };
    BOOST_REQUIRE_EQUAL(body_store.buffer(), expected_file);
    BOOST_REQUIRE(!instance.get_fault());
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

BOOST_AUTO_TEST_CASE(hashmap__slab_put__multiple__expected)
{
    test::chunk_storage head_store{};
    test::chunk_storage body_store{};

    hashmap_<link5, key1, big_slab::size> instance{ head_store, body_store, bucket_bits };
    BOOST_REQUIRE(instance.create());

    constexpr key1 key_big{ 0x41 };
    constexpr key1 key_little{ 0x42 };

    link5 link{};
    BOOST_REQUIRE(instance.put_link(link, key_big, big_slab{ 0xa1b2c3d4_u32 }));
    BOOST_REQUIRE(!link.is_terminal());
    BOOST_REQUIRE_EQUAL(link, 0u);

    link = instance.put_link(key_little, little_slab{ 0xa1b2c3d4_u32 });
    BOOST_REQUIRE(!link.is_terminal());
    BOOST_REQUIRE_EQUAL(link, big_slab::count());

    big_slab slab1{};
    BOOST_REQUIRE(instance.get(0, slab1));
    BOOST_REQUIRE_EQUAL(slab1.value, 0xa1b2c3d4_u32);

    little_slab slab2{};
    BOOST_REQUIRE(instance.get(big_slab::count(), slab2));
    BOOST_REQUIRE_EQUAL(slab2.value, 0xa1b2c3d4_u32);

    // This expectation relies on the fact of no hash table conflict between 0x41 and 0x42.
    const data_chunk expected_file
    {
        0xff, 0xff, 0xff, 0xff, 0xff,
        0x41,
        0xa1, 0xb2, 0xc3, 0xd4,

        0xff, 0xff, 0xff, 0xff, 0xff,
        0x42,
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
    test::chunk_storage head_store{};
    test::chunk_storage body_store{};
    hashmap_<link5, key1, big_record::size> instance{ head_store, body_store, bucket_bits };
    BOOST_REQUIRE(instance.create());

    constexpr key1 key{ 0x41 };
    BOOST_REQUIRE(!instance.put_link(key, big_record{ 0xa1b2c3d4_u32 }).is_terminal());

    record_excess record{};
    BOOST_REQUIRE(!instance.get(0, record));
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(hashmap__record_get_key__exists__expected)
{
    test::chunk_storage head_store{};
    test::chunk_storage body_store{};
    hashmap_<link5, key1, big_record::size> instance{ head_store, body_store, bucket_bits };
    BOOST_REQUIRE(instance.create());

    constexpr key1 key{ 0x41 };
    BOOST_REQUIRE(!instance.put_link(key, big_record{ 0xa1b2c3d4_u32 }).is_terminal());
    BOOST_REQUIRE_EQUAL(instance.get_key(0), key);
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(hashmap__record_put__excess__false)
{
    test::chunk_storage head_store{};
    test::chunk_storage body_store{};
    hashmap_<link5, key1, big_record::size> instance{ head_store, body_store, bucket_bits };
    BOOST_REQUIRE(instance.create());

    constexpr key1 key{ 0x41 };
    BOOST_REQUIRE(instance.put_link(key, record_excess{ 0xa1b2c3d4_u32 }).is_terminal());
    BOOST_REQUIRE(!instance.get_fault());
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
    test::chunk_storage head_store{};
    test::chunk_storage body_store{};
    hashmap_<link5, key1, big_slab::size> instance{ head_store, body_store, bucket_bits };
    BOOST_REQUIRE(instance.create());

    constexpr key1 key{ 0x41 };
    BOOST_REQUIRE(!instance.put_link(key, big_slab{ 0xa1b2c3d4_u32 }).is_terminal());
    BOOST_REQUIRE(!instance.put_link(key, big_slab{ 0xa1b2c3d4_u32 }).is_terminal());

    // Excess read allowed to eof here (reader has only knowledge of size).
    slab_excess slab{};
    BOOST_REQUIRE(instance.get<slab_excess>(0, slab));
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(hashmap__slab_get_key__exists__expected)
{
    test::chunk_storage head_store{};
    test::chunk_storage body_store{};
    hashmap_<link5, key1, big_slab::size> instance{ head_store, body_store, bucket_bits };
    BOOST_REQUIRE(instance.create());

    constexpr key1 key{ 0x41 };
    BOOST_REQUIRE(!instance.put_link(key, big_slab{ 0xa1b2c3d4_u32 }).is_terminal());
    BOOST_REQUIRE(!instance.put_link(key, big_slab{ 0xa1b2c3d4_u32 }).is_terminal());
    BOOST_REQUIRE_EQUAL(instance.get_key(0), key);
    BOOST_REQUIRE_EQUAL(instance.get_key(10), key);
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(hashmap__slab_get__file_excess__false)
{
    test::chunk_storage head_store{};
    test::chunk_storage body_store{};
    hashmap_<link5, key1, big_slab::size> instance{ head_store, body_store, bucket_bits };
    BOOST_REQUIRE(instance.create());

    constexpr key1 key{ 0x41 };
    BOOST_REQUIRE(!instance.put_link(key, big_slab{ 0xa1b2c3d4_u32 }).is_terminal());

    // Excess read disallowed to here (past eof).
    slab_excess slab{};
    BOOST_REQUIRE(!instance.get<slab_excess>(0, slab));
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(hashmap__slab_put__excess__false)
{
    test::chunk_storage head_store{};
    test::chunk_storage body_store{};
    hashmap_<link5, key1, big_slab::size> instance{ head_store, body_store, bucket_bits };
    BOOST_REQUIRE(instance.create());

    constexpr key1 key{ 0x41 };
    BOOST_REQUIRE(instance.put_link(key, slab_excess{ 0xa1b2c3d4_u32 }).is_terminal());
}

BOOST_AUTO_TEST_CASE(hashmap__record_top__default__terminal)
{
    test::chunk_storage head_store{};
    test::chunk_storage body_store{};
    hashmap_<link5, key1, big_record::size> instance{ head_store, body_store, bucket_bits };
    BOOST_REQUIRE(instance.create());
    BOOST_REQUIRE(instance.top(0).is_terminal());
    BOOST_REQUIRE(instance.top(19).is_terminal());
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(hashmap__record_top__past_end__terminal)
{
    test::chunk_storage head_store{};
    test::chunk_storage body_store{};
    hashmap_<link5, key1, big_record::size> instance{ head_store, body_store, bucket_bits };
    BOOST_REQUIRE(instance.create());
    BOOST_REQUIRE(instance.top(20).is_terminal());
    BOOST_REQUIRE(instance.top(21).is_terminal());
}

BOOST_AUTO_TEST_CASE(hashmap__record_top__existing__expected)
{
    test::chunk_storage head_store{};
    test::chunk_storage body_store{};
    hashmap_<link5, key1, big_record::size> instance{ head_store, body_store, bucket_bits };
    BOOST_REQUIRE(instance.create());

    // 0x40 is masked by 4 bucket bits.
    BOOST_REQUIRE(!instance.put_link({ 0x41 }, big_record{ 0xa1b2c3d4_u32 }).is_terminal());
    BOOST_REQUIRE(!instance.put_link({ 0x42 }, big_record{ 0xa2b2c3d4_u32 }).is_terminal());
    BOOST_REQUIRE(!instance.put_link({ 0x44 }, big_record{ 0xa3b2c3d4_u32 }).is_terminal());
    BOOST_REQUIRE(instance.top(0).is_terminal());
    BOOST_REQUIRE_EQUAL(instance.top(1), 0u);
    BOOST_REQUIRE_EQUAL(instance.top(2), 1u);
    BOOST_REQUIRE(instance.top(3).is_terminal());
    BOOST_REQUIRE_EQUAL(instance.top(4), 2u);
    BOOST_REQUIRE(instance.top(5).is_terminal());
    BOOST_REQUIRE(instance.top(6).is_terminal());
    BOOST_REQUIRE(instance.top(7).is_terminal());
    BOOST_REQUIRE(instance.top(8).is_terminal());
    BOOST_REQUIRE(instance.top(9).is_terminal());
    BOOST_REQUIRE(instance.top(10).is_terminal());
    BOOST_REQUIRE(instance.top(11).is_terminal());
    BOOST_REQUIRE(instance.top(12).is_terminal());
    BOOST_REQUIRE(instance.top(13).is_terminal());
    BOOST_REQUIRE(instance.top(14).is_terminal());
    BOOST_REQUIRE(instance.top(15).is_terminal());
    BOOST_REQUIRE(instance.top(16).is_terminal());
    BOOST_REQUIRE(instance.top(17).is_terminal());
    BOOST_REQUIRE(instance.top(18).is_terminal());
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(hashmap__record_exists__exists__true)
{
    test::chunk_storage head_store{};
    test::chunk_storage body_store{};
    hashmap_<link5, key1, big_record::size> instance{ head_store, body_store, bucket_bits };
    BOOST_REQUIRE(instance.create());

    constexpr key1 key{ 0x41 };
    BOOST_REQUIRE(!instance.exists(key));
    BOOST_REQUIRE(!instance.put_link(key, big_record{ 0xa1b2c3d4_u32 }).is_terminal());
    BOOST_REQUIRE(instance.exists(key));
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(hashmap__slab_exists__exists__true)
{
    test::chunk_storage head_store{};
    test::chunk_storage body_store{};
    hashmap_<link5, key1, big_slab::size> instance{ head_store, body_store, bucket_bits };
    BOOST_REQUIRE(instance.create());

    constexpr key1 key{ 0x41 };
    BOOST_REQUIRE(!instance.exists(key));
    BOOST_REQUIRE(!instance.put_link(key, big_slab{ 0xa1b2c3d4_u32 }).is_terminal());
    BOOST_REQUIRE(instance.exists(key));
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(hashmap__record_first__exists__true)
{
    test::chunk_storage head_store{};
    test::chunk_storage body_store{};
    hashmap_<link5, key1, big_record::size> instance{ head_store, body_store, bucket_bits };
    BOOST_REQUIRE(instance.create());

    constexpr key1 key{ 0x41 };
    BOOST_REQUIRE(instance.first(key).is_terminal());
    const auto link = instance.put_link(key, big_record{ 0xa1b2c3d4_u32 });
    BOOST_REQUIRE(!link.is_terminal());
    BOOST_REQUIRE_EQUAL(instance.first(key), link);
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(hashmap__slab_first__exists__true)
{
    test::chunk_storage head_store{};
    test::chunk_storage body_store{};
    hashmap_<link5, key1, big_slab::size> instance{ head_store, body_store, bucket_bits };
    BOOST_REQUIRE(instance.create());

    constexpr key1 key{ 0x41 };
    BOOST_REQUIRE(instance.first(key).is_terminal());
    const auto link = instance.put_link(key, big_slab{ 0xa1b2c3d4_u32 });
    BOOST_REQUIRE(!link.is_terminal());
    BOOST_REQUIRE_EQUAL(instance.first(key), link);
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(hashmap__record_it__exists_copy__non_terminal)
{
    test::chunk_storage head_store{};
    test::chunk_storage body_store{};
    hashmap_<link5, key1, big_record::size> instance{ head_store, body_store, bucket_bits };
    BOOST_REQUIRE(instance.create());

    constexpr key1 key{ 0x41 };
    BOOST_REQUIRE(instance.it(key).self().is_terminal());
    BOOST_REQUIRE(!instance.put_link(key, big_record{ 0xa1b2c3d4_u32 }).is_terminal());
    BOOST_REQUIRE(!instance.it(key).self().is_terminal());

    big_record record{};
    BOOST_REQUIRE(instance.get(instance.it(key).self(), record));
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(hashmap__record_it__exists_move__non_terminal)
{
    test::chunk_storage head_store{};
    test::chunk_storage body_store{};
    hashmap_<link5, key1, big_record::size> instance{ head_store, body_store, bucket_bits };
    BOOST_REQUIRE(instance.create());

    constexpr key1 key{ 0x41 };
    BOOST_REQUIRE(instance.it(key1{ 0x41 }).self().is_terminal());
    BOOST_REQUIRE(!instance.put_link(key, big_record{ 0xa1b2c3d4_u32 }).is_terminal());
    BOOST_REQUIRE(!instance.it(key1{ 0x41 }).self().is_terminal());

    big_record record{};
    BOOST_REQUIRE(instance.get(instance.it(key1{ 0x41 }).self(), record));
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(hashmap__record_it__multiple__iterated)
{
    test::chunk_storage head_store{};
    test::chunk_storage body_store{};
    hashmap_<link5, key1, big_record::size> instance{ head_store, body_store, bucket_bits };
    BOOST_REQUIRE(instance.create());

    constexpr key1 key_a{ 0xaa };
    constexpr key1 key_b{ 0xbb };
    constexpr key1 key_c{ 0xcc };

    BOOST_REQUIRE(!instance.put_link(key_a, big_record{ 0x000000a1_u32 }).is_terminal());
    BOOST_REQUIRE(!instance.put_link(key_a, big_record{ 0x000000a2_u32 }).is_terminal());
    BOOST_REQUIRE(!instance.put_link(key_a, big_record{ 0x000000a3_u32 }).is_terminal());
    BOOST_REQUIRE(!instance.put_link(key_b, big_record{ 0x000000b1_u32 }).is_terminal());
    BOOST_REQUIRE(!instance.put_link(key_b, big_record{ 0x000000b2_u32 }).is_terminal());
    BOOST_REQUIRE(!instance.put_link(key_b, big_record{ 0x000000b3_u32 }).is_terminal());
    BOOST_REQUIRE(!instance.put_link(key_c, big_record{ 0x000000c1_u32 }).is_terminal());
    BOOST_REQUIRE(!instance.put_link(key_c, big_record{ 0x000000c2_u32 }).is_terminal());
    BOOST_REQUIRE(!instance.put_link(key_c, big_record{ 0x000000c3_u32 }).is_terminal());

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
    BOOST_REQUIRE(!instance.get_fault());

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
    test::chunk_storage head_store{};
    test::chunk_storage body_store{};
    record_table instance{ head_store, body_store, 2 };
    BOOST_REQUIRE(instance.create());
    BOOST_REQUIRE(instance.allocate(link5::terminal).is_terminal());
    BOOST_REQUIRE(body_store.buffer().empty());
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(hashmap__allocate__record__expected)
{
    test::chunk_storage head_store{};
    test::chunk_storage body_store{};
    record_table instance{ head_store, body_store, 2 };
    BOOST_REQUIRE(instance.create());
    BOOST_REQUIRE_EQUAL(instance.allocate(1), 0u);
    BOOST_REQUIRE_EQUAL(body_store.buffer().size(), element_size);
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(hashmap__allocate__3_records__expected)
{
    test::chunk_storage head_store{};
    test::chunk_storage body_store{};
    record_table instance{ head_store, body_store, 2 };
    BOOST_REQUIRE(instance.create());
    BOOST_REQUIRE_EQUAL(instance.allocate(3), 0u);
    BOOST_REQUIRE_EQUAL(body_store.buffer().size(), 3u * element_size);
    BOOST_REQUIRE_EQUAL(instance.allocate(1), 3u);
    BOOST_REQUIRE_EQUAL(body_store.buffer().size(), 4u * element_size);
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(hashmap__allocate__slab__allocated)
{
    test::chunk_storage head_store{};
    test::chunk_storage body_store{};
    slab_table instance{ head_store, body_store, 2 };
    BOOST_REQUIRE(instance.create());
    BOOST_REQUIRE_EQUAL(instance.allocate(42), 0u);
    BOOST_REQUIRE_EQUAL(body_store.buffer().size(), 42u);
    BOOST_REQUIRE_EQUAL(instance.allocate(24), 42);
    BOOST_REQUIRE_EQUAL(body_store.buffer().size(), 42u + 24u);
    BOOST_REQUIRE(!instance.get_fault());
}

class flex_record
{
public:
    static constexpr size_t size = sizeof(uint32_t);
    static constexpr link5 count() NOEXCEPT { return 1; }

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
    static constexpr link5 count() NOEXCEPT
    {
        return link5::size + array_count<key10> + sizeof(uint32_t);
    }

    template <typename Sinker>
    bool to_data(Sinker& sink) const NOEXCEPT
    {
        sink.write_little_endian(value);
        return sink;
    }

    uint32_t value{ 0 };
};

BOOST_AUTO_TEST_CASE(hashmap__set_commit__record__expected)
{
    test::chunk_storage head_store{};
    test::chunk_storage body_store{};
    hashmap_<link5, key10, flex_record::size> instance{ head_store, body_store, 2 };
    BOOST_REQUIRE(instance.create());

    constexpr key10 key1{ 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a };
    constexpr auto size = link5::size + array_count<key10> + flex_record::size;
    const auto link = instance.set_link(key1, flex_record{ 0x01020304_u32 });
    BOOST_REQUIRE(!link.is_terminal());
    BOOST_REQUIRE_EQUAL(link, 0u);
    BOOST_REQUIRE_EQUAL(body_store.buffer().size(), size);
    BOOST_REQUIRE_EQUAL(head_store.buffer(), base16_chunk("0000000000ffffffffffffffffffffffffffffffffffffffff"));
    BOOST_REQUIRE_EQUAL(body_store.buffer(), base16_chunk("00000000000102030405060708090a04030201"));

    BOOST_REQUIRE(instance.commit(link, key1));
    BOOST_REQUIRE_EQUAL(head_store.buffer(), base16_chunk("0000000000ffffffffff0000000000ffffffffffffffffffff"));
    BOOST_REQUIRE_EQUAL(body_store.buffer(), base16_chunk("ffffffffff0102030405060708090a04030201"));
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(hashmap__allocate_set_commit__record__expected)
{
    data_chunk head_file;
    test::chunk_storage head_store{};
    test::chunk_storage body_store{};
    hashmap_<link5, key10, flex_record::size> instance{ head_store, body_store, 2 };
    BOOST_REQUIRE(instance.create());

    constexpr key10 key1{ 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a };
    constexpr auto size = link5::size + array_count<key10> + flex_record::size;
    const auto link = instance.allocate(1);
    BOOST_REQUIRE_EQUAL(link, 0u);
    BOOST_REQUIRE_EQUAL(body_store.buffer().size(), size);

    BOOST_REQUIRE(instance.set(link, key1, flex_record{ 0x01020304_u32 }));
    BOOST_REQUIRE_EQUAL(head_store.buffer(), base16_chunk("0000000000ffffffffffffffffffffffffffffffffffffffff"));
    BOOST_REQUIRE_EQUAL(body_store.buffer(), base16_chunk("00000000000102030405060708090a04030201"));

    BOOST_REQUIRE(instance.commit(link, key1));
    BOOST_REQUIRE_EQUAL(head_store.buffer(), base16_chunk("0000000000ffffffffff0000000000ffffffffffffffffffff"));
    BOOST_REQUIRE_EQUAL(body_store.buffer(), base16_chunk("ffffffffff0102030405060708090a04030201"));
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(hashmap__allocate_put1__record__expected)
{
    test::chunk_storage head_store{};
    test::chunk_storage body_store{};
    hashmap_<link5, key10, flex_record::size> instance{ head_store, body_store, 2 };
    BOOST_REQUIRE(instance.create());

    constexpr key10 key1{ 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a };
    constexpr auto size = link5::size + array_count<key10> + sizeof(uint32_t);
    const auto link = instance.allocate(1);
    BOOST_REQUIRE_EQUAL(link, 0u);
    BOOST_REQUIRE_EQUAL(body_store.buffer().size(), size);

    BOOST_REQUIRE(instance.put(link, key1, flex_record{ 0x01020304_u32 }));
    BOOST_REQUIRE_EQUAL(head_store.buffer(), base16_chunk("0000000000ffffffffff0000000000ffffffffffffffffffff"));
    BOOST_REQUIRE_EQUAL(body_store.buffer(), base16_chunk("ffffffffff0102030405060708090a04030201"));
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(hashmap__allocate_put2__record__expected)
{
    test::chunk_storage head_store{};
    test::chunk_storage body_store{};
    hashmap_<link5, key10, flex_record::size> instance{ head_store, body_store, 2 };
    BOOST_REQUIRE(instance.create());

    constexpr key10 key1{ 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a };
    BOOST_REQUIRE(instance.put(key1, flex_record{ 0x01020304_u32 }));
    BOOST_REQUIRE_EQUAL(head_store.buffer(), base16_chunk("0000000000ffffffffff0000000000ffffffffffffffffffff"));
    BOOST_REQUIRE_EQUAL(body_store.buffer(), base16_chunk("ffffffffff0102030405060708090a04030201"));
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(hashmap__set_commit_link__slab__expected)
{
    test::chunk_storage head_store{};
    test::chunk_storage body_store{};
    hashmap_<link5, key10, flex_slab::size> instance{ head_store, body_store, 2 };
    BOOST_REQUIRE(instance.create());

    constexpr key10 key1{ 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a };
    constexpr auto size = link5::size + array_count<key10> + sizeof(uint32_t);
    link5 link{};
    BOOST_REQUIRE(instance.set_link(link, key1, flex_slab{ 0x01020304_u32 }));
    BOOST_REQUIRE(!link.is_terminal());
    BOOST_REQUIRE_EQUAL(link, 0u);
    BOOST_REQUIRE_EQUAL(body_store.buffer().size(), size);
    BOOST_REQUIRE_EQUAL(head_store.buffer(), base16_chunk("0000000000ffffffffffffffffffffffffffffffffffffffff"));
    BOOST_REQUIRE_EQUAL(body_store.buffer(), base16_chunk("00000000000102030405060708090a04030201"));

    BOOST_REQUIRE(!instance.commit_link(link, key1).is_terminal());
    BOOST_REQUIRE_EQUAL(head_store.buffer(), base16_chunk("0000000000ffffffffff0000000000ffffffffffffffffffff"));
    BOOST_REQUIRE_EQUAL(body_store.buffer(), base16_chunk("ffffffffff0102030405060708090a04030201"));
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(hashmap__allocate_set_commit__slab__expected)
{
    test::chunk_storage head_store{};
    test::chunk_storage body_store{};
    hashmap_<link5, key10, flex_slab::size> instance{ head_store, body_store, 2 };
    BOOST_REQUIRE(instance.create());

    constexpr key10 key1{ 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a };
    constexpr auto size = link5::size + array_count<key10> + sizeof(uint32_t);
    const auto link = instance.allocate(size);
    BOOST_REQUIRE_EQUAL(link, 0u);
    BOOST_REQUIRE_EQUAL(body_store.buffer().size(), size);

    BOOST_REQUIRE(instance.set(link, key1, flex_slab{ 0x01020304_u32 }));
    BOOST_REQUIRE_EQUAL(head_store.buffer(), base16_chunk("0000000000ffffffffffffffffffffffffffffffffffffffff"));
    BOOST_REQUIRE_EQUAL(body_store.buffer(), base16_chunk("00000000000102030405060708090a04030201"));

    BOOST_REQUIRE(instance.commit(link, key1));
    BOOST_REQUIRE_EQUAL(head_store.buffer(), base16_chunk("0000000000ffffffffff0000000000ffffffffffffffffffff"));
    BOOST_REQUIRE_EQUAL(body_store.buffer(), base16_chunk("ffffffffff0102030405060708090a04030201"));
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(hashmap__allocate_put1__slab__expected)
{
    data_chunk head_file;
    data_chunk body_file;
    test::chunk_storage head_store{ head_file };
    test::chunk_storage body_store{ body_file };
    hashmap_<link5, key10, flex_slab::size> instance{ head_store, body_store, 2 };
    BOOST_REQUIRE(instance.create());

    constexpr auto size = link5::size + array_count<key10> + sizeof(uint32_t);
    const auto link = instance.allocate(size);
    BOOST_REQUIRE_EQUAL(link, 0u);
    BOOST_REQUIRE_EQUAL(body_file.size(), size);

    constexpr key10 key1{ 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a };
    BOOST_REQUIRE(instance.put(link, key1, flex_slab{ 0x01020304_u32 }));
    BOOST_REQUIRE_EQUAL(head_file, base16_chunk("0000000000ffffffffff0000000000ffffffffffffffffffff"));
    BOOST_REQUIRE_EQUAL(body_file, base16_chunk("ffffffffff0102030405060708090a04030201"));
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(hashmap__allocate_put2__slab__expected)
{
    data_chunk head_file;
    data_chunk body_file;
    test::chunk_storage head_store{ head_file };
    test::chunk_storage body_store{ body_file };
    hashmap_<link5, key10, flex_slab::size> instance{ head_store, body_store, 2 };
    BOOST_REQUIRE(instance.create());

    constexpr key10 key1{ 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a };
    BOOST_REQUIRE(instance.put(key1, flex_slab{ 0x01020304_u32 }));
    BOOST_REQUIRE_EQUAL(head_file, base16_chunk("0000000000ffffffffff0000000000ffffffffffffffffffff"));
    BOOST_REQUIRE_EQUAL(body_file, base16_chunk("ffffffffff0102030405060708090a04030201"));
    BOOST_REQUIRE(!instance.get_fault());
}

// record create/close/backup/restore/verify
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(hashmap__record_verify__empty_files__expected)
{
    test::chunk_storage head_store{};
    test::chunk_storage body_store{};
    record_table instance{ head_store, body_store, bucket_bits };
    BOOST_REQUIRE(!instance.verify());
    BOOST_REQUIRE(instance.create());
    BOOST_REQUIRE(instance.verify());
    BOOST_REQUIRE_EQUAL(head_store.buffer().size(), head_size);
    BOOST_REQUIRE(body_store.buffer().empty());
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(hashmap__record_create__non_empty_head_file__failure)
{
    data_chunk head_file{ 0x42 };
    data_chunk body_file;
    test::chunk_storage head_store{ head_file };
    test::chunk_storage body_store{ body_file };
    record_table instance{ head_store, body_store, bucket_bits };
    BOOST_REQUIRE(!instance.verify());
    BOOST_REQUIRE(!instance.create());
    BOOST_REQUIRE_EQUAL(head_file.size(), 1u);
    BOOST_REQUIRE(body_file.empty());
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(hashmap__record_create__non_empty_body_file__body_zeroed)
{
    data_chunk head_file;
    data_chunk body_file{ 0x42 };
    test::chunk_storage head_store{ head_file };
    test::chunk_storage body_store{ body_file };
    record_table instance{ head_store, body_store, bucket_bits };
    BOOST_REQUIRE(!instance.verify());
    BOOST_REQUIRE(instance.create());
    BOOST_REQUIRE(instance.verify());
    BOOST_REQUIRE_EQUAL(head_file.size(), head_size);
    BOOST_REQUIRE(body_file.empty());
}

BOOST_AUTO_TEST_CASE(hashmap__record_body_count__create__zero)
{
    test::chunk_storage head_store{};
    test::chunk_storage body_store{};
    record_table instance{ head_store, body_store, bucket_bits };
    BOOST_REQUIRE(instance.create());
    BOOST_REQUIRE_EQUAL(head_store.buffer(), base16_chunk("0000000000ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"));
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(hashmap__record_body_count__empty_close__zero)
{
    auto head_file = base16_chunk("1234567890");
    data_chunk body_file;
    test::chunk_storage head_store{ head_file };
    test::chunk_storage body_store{ body_file };
    record_table instance{ head_store, body_store, bucket_bits };
    BOOST_REQUIRE(instance.close());
    BOOST_REQUIRE_EQUAL(head_file, base16_chunk("0000000000"));
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(hashmap__record_body_count__two_close__two)
{
    test::chunk_storage head_store{};
    test::chunk_storage body_store{};
    record_table instance{ head_store, body_store, bucket_bits };
    BOOST_REQUIRE(instance.create());
    body_store.buffer() = base16_chunk("1122334455667788990011223344556677889911223344556677889900112233445566778899");
    BOOST_REQUIRE(instance.close());
    BOOST_REQUIRE_EQUAL(head_store.buffer(), base16_chunk("0200000000ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"));
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(hashmap__record_body_count__two_backup__two)
{
    test::chunk_storage head_store{};
    test::chunk_storage body_store{};
    record_table instance{ head_store, body_store, bucket_bits };
    BOOST_REQUIRE(instance.create());
    body_store.buffer() = base16_chunk("1122334455667788990011223344556677889911223344556677889900112233445566778899");
    BOOST_REQUIRE(instance.backup());
    BOOST_REQUIRE_EQUAL(head_store.buffer(), base16_chunk("0200000000ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"));
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(hashmap__record_body_count__empty_restore__truncates)
{
    test::chunk_storage head_store{};
    test::chunk_storage body_store{};
    record_table instance{ head_store, body_store, bucket_bits };
    BOOST_REQUIRE(instance.create());
    body_store.buffer() = base16_chunk("1234567812345678");
    BOOST_REQUIRE(instance.restore());
    BOOST_REQUIRE(body_store.buffer().empty());
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(hashmap__record_body_count__non_empty_restore__truncates)
{
    test::chunk_storage head_store{};
    test::chunk_storage body_store{};
    record_table instance{ head_store, body_store, bucket_bits };
    BOOST_REQUIRE(instance.create());
    head_store.buffer() = base16_chunk("0100000000ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
    body_store.buffer() = base16_chunk("1122334455667788990011223344556677889911223344556677889900112233445566778899");
    BOOST_REQUIRE(instance.restore());
    BOOST_REQUIRE_EQUAL(body_store.buffer(), base16_chunk("11223344556677889900112233445566778899"));
    BOOST_REQUIRE(!instance.get_fault());
}

// slab create/close/backup/restore/verify
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(hashmap__slab_verify__empty_files__expected)
{
    test::chunk_storage head_store{};
    test::chunk_storage body_store{};
    slab_table instance{ head_store, body_store, bucket_bits };
    BOOST_REQUIRE(!instance.verify());
    BOOST_REQUIRE(instance.create());
    BOOST_REQUIRE(instance.verify());
    BOOST_REQUIRE_EQUAL(head_store.buffer().size(), head_size);
    BOOST_REQUIRE(body_store.buffer().empty());
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(hashmap__slab_create__non_empty_head_file__failure)
{
    data_chunk head_file{ 0x42 };
    data_chunk body_file;
    test::chunk_storage head_store{ head_file };
    test::chunk_storage body_store{ body_file };
    slab_table instance{ head_store, body_store, bucket_bits };
    BOOST_REQUIRE(!instance.verify());
    BOOST_REQUIRE(!instance.create());
    BOOST_REQUIRE_EQUAL(head_file.size(), 1u);
    BOOST_REQUIRE(body_file.empty());
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(hashmap__slab_create__non_empty_body_file__body_zeroed)
{
    data_chunk head_file;
    data_chunk body_file{ 0x42 };
    test::chunk_storage head_store{ head_file };
    test::chunk_storage body_store{ body_file };
    slab_table instance{ head_store, body_store, bucket_bits };
    BOOST_REQUIRE(!instance.verify());
    BOOST_REQUIRE(instance.create());
    BOOST_REQUIRE(instance.verify());
    BOOST_REQUIRE_EQUAL(head_file.size(), head_size);
    BOOST_REQUIRE(body_file.empty());
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(hashmap__slab_body_count__create__zero)
{
    test::chunk_storage head_store{};
    test::chunk_storage body_store{};
    slab_table instance{ head_store, body_store, bucket_bits };
    BOOST_REQUIRE(instance.create());
    BOOST_REQUIRE_EQUAL(head_store.buffer(), base16_chunk("0000000000ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"));
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(hashmap__slab_body_count__empty_close__zero)
{
    auto head_file = base16_chunk("1234567890");
    data_chunk body_file;
    test::chunk_storage head_store{ head_file };
    test::chunk_storage body_store{ body_file };
    slab_table instance{ head_store, body_store, bucket_bits };
    BOOST_REQUIRE(instance.close());
    BOOST_REQUIRE_EQUAL(head_file, base16_chunk("0000000000"));
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(hashmap__slab_body_count__two_close__two)
{
    test::chunk_storage head_store{};
    test::chunk_storage body_store{};
    slab_table instance{ head_store, body_store, bucket_bits };
    BOOST_REQUIRE(instance.create());
    body_store.buffer() = base16_chunk("1234");
    BOOST_REQUIRE(instance.close());
    BOOST_REQUIRE_EQUAL(head_store.buffer(), base16_chunk("0200000000ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"));
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(hashmap__slab_body_count__two_backup__two)
{
    test::chunk_storage head_store{};
    test::chunk_storage body_store{};
    slab_table instance{ head_store, body_store, bucket_bits };
    BOOST_REQUIRE(instance.create());
    body_store.buffer() = base16_chunk("1234");
    BOOST_REQUIRE(instance.backup());
    BOOST_REQUIRE_EQUAL(head_store.buffer(), base16_chunk("0200000000ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"));
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(hashmap__slab_body_count__empty_restore__truncates)
{
    test::chunk_storage head_store{};
    test::chunk_storage body_store{};
    slab_table instance{ head_store, body_store, bucket_bits };
    BOOST_REQUIRE(instance.create());
    body_store.buffer() = base16_chunk("1234567812345678");
    BOOST_REQUIRE(instance.restore());
    BOOST_REQUIRE(body_store.buffer().empty());
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(hashmap__slab_body_count__non_empty_restore__truncates)
{
    test::chunk_storage head_store{};
    test::chunk_storage body_store{};
    slab_table instance{ head_store, body_store, bucket_bits };
    BOOST_REQUIRE(instance.create());
    head_store.buffer() = base16_chunk("0300000000ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
    body_store.buffer() = base16_chunk("1234567812345678");
    BOOST_REQUIRE(instance.restore());
    BOOST_REQUIRE_EQUAL(body_store.buffer(), base16_chunk("123456"));
    BOOST_REQUIRE(!instance.get_fault());
}

////std::cout << head_file << std::endl << std::endl;
////std::cout << body_file << std::endl << std::endl;

BOOST_AUTO_TEST_SUITE_END()
