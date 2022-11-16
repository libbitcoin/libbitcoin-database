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

BOOST_AUTO_TEST_SUITE(arraymap_tests)

template <typename Link, size_t Size>
class arraymap_
  : public arraymap<Link, Size>
{
public:
    using arraymap<Link, Size>::arraymap;

    reader_ptr at_(const Link& record) const NOEXCEPT
    {
        return base::at(record);
    }

    writer_ptr push_(const Link& size=one) NOEXCEPT
    {
        return base::push(size);
    }

private:
    using base = arraymap<Link, Size>;
};

// There is no internal linkage, but we still have primary key domain.
using link5 = linkage<5>;
struct slab0 { static constexpr size_t size = max_size_t; };
struct record4 { static constexpr size_t size = 4; };
using slab_table = arraymap_<link5, slab0::size>;
using record_table = arraymap_<link5, record4::size>;

// record arraymap
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(arraymap__record_construct__empty__expected)
{
    data_chunk body_file;
    test::storage body_store{ body_file };
    const record_table instance{ body_store };
    BOOST_REQUIRE(body_file.empty());
}

BOOST_AUTO_TEST_CASE(arraymap__record_construct__non_empty__expected)
{
    constexpr auto body_size = 12345u;
    data_chunk body_file(body_size);
    test::storage body_store{ body_file };
    const record_table instance{ body_store };
    BOOST_REQUIRE_EQUAL(body_file.size(), body_size);
}

BOOST_AUTO_TEST_CASE(arraymap__record_at__terminal__false)
{
    data_chunk body_file;
    test::storage body_store{ body_file };
    const record_table instance{ body_store };
    BOOST_REQUIRE(!instance.at_(link5::terminal));
}

BOOST_AUTO_TEST_CASE(arraymap__record_at__empty__exhausted)
{
    data_chunk body_file;
    test::storage body_store{ body_file };
    const record_table instance{ body_store };
    BOOST_REQUIRE(instance.at_(0)->is_exhausted());
    BOOST_REQUIRE(instance.at_(19)->is_exhausted());
}

// slab arraymap
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(arraymap__slab_construct__empty__expected)
{
    data_chunk body_file;
    test::storage body_store{ body_file };
    const slab_table instance{ body_store };
    BOOST_REQUIRE(body_file.empty());
}

BOOST_AUTO_TEST_CASE(arraymap__slab_construct__non_empty__expected)
{
    constexpr auto body_size = 12345u;
    data_chunk body_file(body_size);
    test::storage body_store{ body_file };
    const slab_table instance{ body_store };
    BOOST_REQUIRE_EQUAL(body_file.size(), body_size);
}

BOOST_AUTO_TEST_CASE(arraymap__slab_at__terminal__false)
{
    data_chunk body_file;
    test::storage body_store{ body_file };
    const slab_table instance{ body_store };
    BOOST_REQUIRE(!instance.at_(link5::terminal));
}

BOOST_AUTO_TEST_CASE(arraymap__slab_at__empty__exhausted)
{
    data_chunk body_file;
    test::storage body_store{ body_file };
    const slab_table instance{ body_store };
    BOOST_REQUIRE(instance.at_(0)->is_exhausted());
    BOOST_REQUIRE(instance.at_(19)->is_exhausted());
}

// push/found/at (protected interface positive tests)
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(arraymap__record_readers__empty__expected)
{
    data_chunk body_file;
    test::storage body_store{ body_file };
    record_table instance{ body_store };

    auto stream0 = instance.push_();
    BOOST_REQUIRE_EQUAL(body_file.size(), record4::size);
    BOOST_REQUIRE(!stream0->is_exhausted());
    BOOST_REQUIRE(instance.at_(0));
    stream0.reset();

    auto stream1 = instance.push_();
    BOOST_REQUIRE_EQUAL(body_file.size(), 2u * record4::size);
    BOOST_REQUIRE(!stream1->is_exhausted());
    BOOST_REQUIRE(instance.at_(1));
    stream1.reset();

    // Past end is valid pointer but exhausted stream.
    BOOST_REQUIRE(instance.at_(2));
    BOOST_REQUIRE(instance.at_(2)->is_exhausted());

    // record (assumes zero fill)
    // =================================
    // 00000000 [0]
    // 00000000 [1]
}

BOOST_AUTO_TEST_CASE(arraymap__slab_readers__empty__expected)
{
    data_chunk body_file;
    test::storage body_store{ body_file };
    slab_table instance{ body_store };

    auto stream0 = instance.push_(record4::size);
    BOOST_REQUIRE_EQUAL(body_file.size(), record4::size);
    BOOST_REQUIRE(!stream0->is_exhausted());
    BOOST_REQUIRE(instance.at_(0));
    stream0.reset();

    auto stream1 = instance.push_(record4::size);
    BOOST_REQUIRE_EQUAL(body_file.size(), 2u * record4::size);
    BOOST_REQUIRE(!stream1->is_exhausted());
    BOOST_REQUIRE(instance.at_(record4::size));
    stream1.reset();

    // Past end is valid pointer but exhausted stream.
    BOOST_REQUIRE(instance.at_(2u * record4::size));
    BOOST_REQUIRE(instance.at_(2u * record4::size)->is_exhausted());

    // record (assumes zero fill)
    // =================================
    // 00000000 [0]
    // 00000000 [1]
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

    bool to_data(database::writer& sink) const NOEXCEPT
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

    bool to_data(database::writer& sink) const NOEXCEPT
    {
        sink.write_big_endian(value);
        return sink;
    }

    uint32_t value{ 0 };
};

BOOST_AUTO_TEST_CASE(arraymap__record_get__terminal__invalid)
{
    data_chunk body_file;
    test::storage body_store{ body_file };
    const arraymap<link5, little_record::size> instance{ body_store };

    little_record record{};
    BOOST_REQUIRE(!instance.get(link5::terminal, record));
}

BOOST_AUTO_TEST_CASE(arraymap__record_get__empty__invalid)
{
    data_chunk body_file;
    test::storage body_store{ body_file };
    const arraymap<link5, little_record::size> instance{ body_store };

    little_record record{};
    BOOST_REQUIRE(!instance.get(0, record));
}

BOOST_AUTO_TEST_CASE(arraymap__record_get__populated__valid)
{
    data_chunk body_file{ 0x01, 0x02, 0x03, 0x04 };
    test::storage body_store{ body_file };
    const arraymap<link5, little_record::size> instance{ body_store };

    little_record record{};
    BOOST_REQUIRE(instance.get(0, record));
    BOOST_REQUIRE_EQUAL(record.value, 0x04030201_u32);
}

BOOST_AUTO_TEST_CASE(arraymap__record_put__get__expected)
{
    data_chunk body_file;
    test::storage body_store{ body_file };
    arraymap<link5, big_record::size> instance{ body_store };

    BOOST_REQUIRE(instance.put(big_record{ 0xa1b2c3d4_u32 }));

    big_record record{};
    BOOST_REQUIRE(instance.get(0, record));
    BOOST_REQUIRE_EQUAL(record.value, 0xa1b2c3d4_u32);

    const data_chunk expected_file{ 0xa1, 0xb2, 0xc3, 0xd4 };
    BOOST_REQUIRE_EQUAL(body_file, expected_file);
}

BOOST_AUTO_TEST_CASE(arraymap__record_put__multiple__expected)
{
    data_chunk body_file;
    test::storage body_store{ body_file };
    arraymap<link5, big_record::size> instance{ body_store };

    BOOST_REQUIRE(instance.put(big_record{ 0xa1b2c3d4_u32 }));
    BOOST_REQUIRE(instance.put(little_record{ 0xa1b2c3d4_u32 }));

    big_record record1{};
    BOOST_REQUIRE(instance.get(0, record1));
    BOOST_REQUIRE_EQUAL(record1.value, 0xa1b2c3d4_u32);

    little_record record2{};
    BOOST_REQUIRE(instance.get(1, record2));
    BOOST_REQUIRE_EQUAL(record2.value, 0xa1b2c3d4_u32);

    const data_chunk expected_file{ 0xa1, 0xb2, 0xc3, 0xd4, 0xd4, 0xc3, 0xb2, 0xa1 };
    BOOST_REQUIRE_EQUAL(body_file, expected_file);
}

class little_slab
{
public:
    static constexpr size_t size = max_size_t;
    static constexpr link5 count() NOEXCEPT { return sizeof(uint32_t); }

    bool from_data(database::reader& source) NOEXCEPT
    {
        value = source.read_little_endian<uint32_t>();
        return source;
    }

    bool to_data(database::writer& sink) const NOEXCEPT
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
    static constexpr link5 count() NOEXCEPT { return sizeof(uint32_t); }

    bool from_data(database::reader& source) NOEXCEPT
    {
        value = source.read_big_endian<uint32_t>();
        return source;
    }

    bool to_data(database::writer& sink) const NOEXCEPT
    {
        sink.write_big_endian(value);
        return sink;
    }

    uint32_t value{ 0 };
};

BOOST_AUTO_TEST_CASE(arraymap__slab_put__get__expected)
{
    data_chunk body_file;
    test::storage body_store{ body_file };
    arraymap<link5, big_slab::size> instance{ body_store };

    BOOST_REQUIRE(instance.put(big_slab{ 0xa1b2c3d4_u32 }));

    big_slab slab{};
    BOOST_REQUIRE(instance.get(zero, slab));
    BOOST_REQUIRE_EQUAL(slab.value, 0xa1b2c3d4_u32);

    const data_chunk expected_file{ 0xa1, 0xb2, 0xc3, 0xd4 };
    BOOST_REQUIRE_EQUAL(body_file, expected_file);
}

BOOST_AUTO_TEST_CASE(arraymap__slab_put__multiple__expected)
{
    data_chunk body_file;
    test::storage body_store{ body_file };
    arraymap<link5, big_slab::size> instance{ body_store };

    BOOST_REQUIRE(instance.put(big_slab{ 0xa1b2c3d4_u32 }));
    BOOST_REQUIRE(instance.put(little_slab{ 0xa1b2c3d4_u32 }));

    big_slab slab1{};
    BOOST_REQUIRE(instance.get(zero, slab1));
    BOOST_REQUIRE_EQUAL(slab1.value, 0xa1b2c3d4_u32);

    little_slab slab2{};
    BOOST_REQUIRE(instance.get(little_slab::count(), slab2));
    BOOST_REQUIRE_EQUAL(slab2.value, 0xa1b2c3d4_u32);

    const data_chunk expected_file{ 0xa1, 0xb2, 0xc3, 0xd4, 0xd4, 0xc3, 0xb2, 0xa1 };
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

    bool to_data(database::writer& sink) const NOEXCEPT
    {
        sink.write_big_endian(value);
        return sink;
    }

    uint64_t value{ 0 };
};

BOOST_AUTO_TEST_CASE(arraymap__record_get__excess__false)
{
    data_chunk body_file{ 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08 };
    test::storage body_store{ body_file };
    const arraymap<link5, record_excess::size> instance{ body_store };


    record_excess record{};
    BOOST_REQUIRE(!instance.get(zero, record));
}

BOOST_AUTO_TEST_CASE(arraymap__record_put__excess__false)
{
    data_chunk body_file;
    test::storage body_store{ body_file };
    arraymap<link5, record_excess::size> instance{ body_store };
    BOOST_REQUIRE(!instance.put(record_excess{ 0xa1b2c3d4_u32 }));
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

    bool to_data(database::writer& sink) const NOEXCEPT
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

    bool to_data(database::writer& sink) const NOEXCEPT
    {
        sink.write_big_endian(value);
        return sink;
    }

    uint64_t value{ 0 };
};

BOOST_AUTO_TEST_CASE(arraymap__slab_get__excess__true)
{
    data_chunk body_file{ 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08 };
    test::storage body_store{ body_file };
    const arraymap<link5, slab_excess::size> instance{ body_store };

    // Excess read allowed to eof here (reader has only knowledge of size).
    slab_excess record{};
    BOOST_REQUIRE(instance.get(zero, record));
}

BOOST_AUTO_TEST_CASE(arraymap__slab_get__file_excess__false)
{
    data_chunk body_file{ 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08 };
    test::storage body_store{ body_file };
    const arraymap<link5, file_excess::size> instance{ body_store };

    // Excess read disallowed to here (past eof).
    file_excess record{};
    BOOST_REQUIRE(!instance.get<file_excess>(zero, record));
}

BOOST_AUTO_TEST_CASE(arraymap__slab_put__excess__false)
{
    data_chunk body_file;
    test::storage body_store{ body_file };
    arraymap<link5, slab_excess::size> instance{ body_store };
    BOOST_REQUIRE(!instance.put(slab_excess{ 0xa1b2c3d4_u32 }));
}

////std::cout << body_file << std::endl << std::endl;

BOOST_AUTO_TEST_SUITE_END()
