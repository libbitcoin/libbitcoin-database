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

template <typename Link, size_t Size>
class arraymap_
  : public arraymap<Link, Size>
{
public:
    using base = arraymap<Link, Size>;
    using base::arraymap;
    ////using reader_ptr = std::shared_ptr<database::reader>;
    ////using writer_ptr = std::shared_ptr<database::writer>;
    ////
    ////reader_ptr getter_(const Link& link) const NOEXCEPT
    ////{
    ////    using namespace system;
    ////    const auto ptr = manager_.get(link);
    ////    if (!ptr)
    ////        return {};
    ////
    ////    istream<memory> stream{ *ptr };
    ////    const auto source = std::make_shared<database::reader>(stream);
    ////    if constexpr (!is_slab) { source->set_limit(Size); }
    ////    return source;
    ////}
    ////
    ////writer_ptr creater_(const Link& size=one) NOEXCEPT
    ////{
    ////    using namespace system;
    ////    const auto link = manager_.allocate(size);
    ////    const auto ptr = manager_.get(link);
    ////    if (!ptr)
    ////        return {};
    ////
    ////    iostream<memory> stream{ *ptr };
    ////    const auto sink = std::make_shared<database::writer>(stream);
    ////    if constexpr (!is_slab) { sink.set_limit(Size * size); }
    ////    return sink;
    ////}
};

// There is no internal linkage, but we still have primary key domain.
using namespace system;
using link5 = linkage<5>;
struct slab0 { static constexpr size_t size = max_size_t; };
struct record4 { static constexpr size_t size = 4; };
using slab_table = arraymap_<link5, slab0::size>;
using record_table = arraymap_<link5, record4::size>;

// record arraymap
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(arraymap__record_construct__empty__expected)
{
    data_chunk head_file;
    data_chunk body_file;
    test::chunk_storage head_store{ head_file };
    test::chunk_storage body_store{ body_file };
    const record_table instance{ head_store, body_store };
    BOOST_REQUIRE(body_file.empty());
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(arraymap__record_construct__non_empty__expected)
{
    constexpr auto body_size = 12345u;
    data_chunk head_file;
    data_chunk body_file(body_size);
    test::chunk_storage head_store{ head_file };
    test::chunk_storage body_store{ body_file };
    const record_table instance{ head_store, body_store };
    BOOST_REQUIRE_EQUAL(body_file.size(), body_size);
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(arraymap__record_getter__terminal__false)
{
    data_chunk head_file;
    data_chunk body_file;
    test::chunk_storage head_store{ head_file };
    test::chunk_storage body_store{ body_file };
    const record_table instance{ head_store, body_store };
    ////BOOST_REQUIRE(!instance.getter_(link5::terminal));
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(arraymap__record_getter__empty__exhausted)
{
    data_chunk head_file;
    data_chunk body_file;
    test::chunk_storage head_store{ head_file };
    test::chunk_storage body_store{ body_file };
    const record_table instance{ head_store, body_store };
    ////BOOST_REQUIRE(instance.getter_(0)->is_exhausted());
    ////BOOST_REQUIRE(instance.getter_(19)->is_exhausted());
    BOOST_REQUIRE(!instance.get_fault());
}

// slab arraymap
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(arraymap__slab_construct__empty__expected)
{
    data_chunk head_file;
    data_chunk body_file;
    test::chunk_storage head_store{ head_file };
    test::chunk_storage body_store{ body_file };
    const slab_table instance{ head_store, body_store };
    BOOST_REQUIRE(body_file.empty());
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(arraymap__slab_construct__non_empty__expected)
{
    constexpr auto body_size = 12345u;
    data_chunk head_file;
    data_chunk body_file(body_size);
    test::chunk_storage head_store{ head_file };
    test::chunk_storage body_store{ body_file };
    const slab_table instance{ head_store, body_store };
    BOOST_REQUIRE_EQUAL(body_file.size(), body_size);
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(arraymap__slab_getter__terminal__false)
{
    data_chunk head_file;
    data_chunk body_file;
    test::chunk_storage head_store{ head_file };
    test::chunk_storage body_store{ body_file };
    const slab_table instance{ head_store, body_store };
    ////BOOST_REQUIRE(!instance.getter_(link5::terminal));
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(arraymap__slab_getter__empty__exhausted)
{
    data_chunk head_file;
    data_chunk body_file;
    test::chunk_storage head_store{ head_file };
    test::chunk_storage body_store{ body_file };
    const slab_table instance{ head_store, body_store };
    ////BOOST_REQUIRE(instance.getter_(0)->is_exhausted());
    ////BOOST_REQUIRE(instance.getter_(19)->is_exhausted());
    BOOST_REQUIRE(!instance.get_fault());
}

// push/found/at (protected interface positive tests)
// ----------------------------------------------------------------------------

////BOOST_AUTO_TEST_CASE(arraymap__record_readers__empty__expected)
////{
////    data_chunk head_file;
////    data_chunk body_file;
////    test::chunk_storage head_store{ head_file };
////    test::chunk_storage body_store{ body_file };
////    record_table instance{ head_store, body_store };
////
////    auto stream0 = instance.creater_();
////    BOOST_REQUIRE_EQUAL(body_file.size(), record4::size);
////    BOOST_REQUIRE(!stream0->is_exhausted());
////    BOOST_REQUIRE(instance.getter_(0));
////    stream0.reset();
////
////    auto stream1 = instance.creater_();
////    BOOST_REQUIRE_EQUAL(body_file.size(), 2u * record4::size);
////    BOOST_REQUIRE(!stream1->is_exhausted());
////    BOOST_REQUIRE(instance.getter_(1));
////    stream1.reset();
////
////    // Past end is valid pointer but exhausted stream.
////    BOOST_REQUIRE(instance.getter_(2));
////    BOOST_REQUIRE(instance.getter_(2)->is_exhausted());
////
////    // record (assumes zero fill)
////    // =================================
////    // 00000000 [0]
////    // 00000000 [1]
////}
////
////BOOST_AUTO_TEST_CASE(arraymap__slab_readers__empty__expected)
////{
////    data_chunk head_file;
////    data_chunk body_file;
////    test::chunk_storage head_store{ head_file };
////    test::chunk_storage body_store{ body_file };
////    slab_table instance{ head_store, body_store };
////
////    auto stream0 = instance.creater_(record4::size);
////    BOOST_REQUIRE_EQUAL(body_file.size(), record4::size);
////    BOOST_REQUIRE(!stream0->is_exhausted());
////    BOOST_REQUIRE(instance.getter_(0));
////    stream0.reset();
////
////    auto stream1 = instance.creater_(record4::size);
////    BOOST_REQUIRE_EQUAL(body_file.size(), 2u * record4::size);
////    BOOST_REQUIRE(!stream1->is_exhausted());
////    BOOST_REQUIRE(instance.getter_(record4::size));
////    stream1.reset();
////
////    // Past end is valid pointer but exhausted stream.
////    BOOST_REQUIRE(instance.getter_(2u * record4::size));
////    BOOST_REQUIRE(instance.getter_(2u * record4::size)->is_exhausted());
////
////    // record (assumes zero fill)
////    // =================================
////    // 00000000 [0]
////    // 00000000 [1]
////}

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

    bool to_data(database::flipper& sink) const NOEXCEPT
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

    bool to_data(database::flipper& sink) const NOEXCEPT
    {
        sink.write_big_endian(value);
        return sink;
    }

    uint32_t value{ 0 };
};

BOOST_AUTO_TEST_CASE(arraymap__record_get__terminal__invalid)
{
    data_chunk head_file;
    data_chunk body_file;
    test::chunk_storage head_store{ head_file };
    test::chunk_storage body_store{ body_file };
    const arraymap<link5, little_record::size> instance{ head_store, body_store };

    little_record record{};
    BOOST_REQUIRE(!instance.get(link5::terminal, record));
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(arraymap__record_get__empty__invalid)
{
    data_chunk head_file;
    data_chunk body_file;
    test::chunk_storage head_store{ head_file };
    test::chunk_storage body_store{ body_file };
    const arraymap<link5, little_record::size> instance{ head_store, body_store };

    little_record record{};
    BOOST_REQUIRE(!instance.get(0, record));
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(arraymap__record_get__populated__valid)
{
    data_chunk head_file;
    data_chunk body_file{ 0x01, 0x02, 0x03, 0x04 };
    test::chunk_storage head_store{ head_file };
    test::chunk_storage body_store{ body_file };
    const arraymap<link5, little_record::size> instance{ head_store, body_store };

    little_record record{};
    BOOST_REQUIRE(instance.get(0, record));
    BOOST_REQUIRE_EQUAL(record.value, 0x04030201_u32);
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(arraymap__record_put__get__expected)
{
    data_chunk head_file;
    data_chunk body_file;
    test::chunk_storage head_store{ head_file };
    test::chunk_storage body_store{ body_file };
    arraymap<link5, big_record::size> instance{ head_store, body_store };
    BOOST_REQUIRE(instance.put(big_record{ 0xa1b2c3d4_u32 }));

    big_record record{};
    BOOST_REQUIRE(instance.get(0, record));
    BOOST_REQUIRE_EQUAL(record.value, 0xa1b2c3d4_u32);

    const data_chunk expected_file{ 0xa1, 0xb2, 0xc3, 0xd4 };
    BOOST_REQUIRE_EQUAL(body_file, expected_file);
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(arraymap__record_count__truncate__expected)
{
    data_chunk head_file;
    data_chunk body_file;
    test::chunk_storage head_store{ head_file };
    test::chunk_storage body_store{ body_file };
    arraymap<link5, big_record::size> instance{ head_store, body_store };
    BOOST_REQUIRE(instance.put(big_record{ 0xa1b2c3d4_u32 }));
    BOOST_REQUIRE(instance.put(big_record{ 0xa1b2c3d4_u32 }));

    const data_chunk expected_file1{ 0xa1, 0xb2, 0xc3, 0xd4, 0xa1, 0xb2, 0xc3, 0xd4 };
    const data_chunk expected_file2{ 0xa1, 0xb2, 0xc3, 0xd4 };
    BOOST_REQUIRE_EQUAL(instance.count(), 2u);
    BOOST_REQUIRE_EQUAL(body_file, expected_file1);
    BOOST_REQUIRE(instance.truncate(1));
    BOOST_REQUIRE_EQUAL(instance.count(), 1u);
    BOOST_REQUIRE_EQUAL(body_file, expected_file2);
    BOOST_REQUIRE(instance.truncate(0));
    BOOST_REQUIRE_EQUAL(instance.count(), 0u);
    BOOST_REQUIRE(body_file.empty());
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(arraymap__record_put_link__multiple__expected)
{
    data_chunk head_file;
    data_chunk body_file;
    test::chunk_storage head_store{ head_file };
    test::chunk_storage body_store{ body_file };
    arraymap<link5, big_record::size> instance{ head_store, body_store };

    link5 link{};
    BOOST_REQUIRE(instance.put_link(link, big_record{ 0xa1b2c3d4_u32 }));
    BOOST_REQUIRE(!link.is_terminal());
    BOOST_REQUIRE_EQUAL(link, 0u);

    link = instance.put_link(little_record{ 0xa1b2c3d4_u32 });
    BOOST_REQUIRE(!link.is_terminal());
    BOOST_REQUIRE_EQUAL(link, 1u);

    big_record record1{};
    BOOST_REQUIRE(instance.get(0, record1));
    BOOST_REQUIRE_EQUAL(record1.value, 0xa1b2c3d4_u32);

    little_record record2{};
    BOOST_REQUIRE(instance.get(1, record2));
    BOOST_REQUIRE_EQUAL(record2.value, 0xa1b2c3d4_u32);

    const data_chunk expected_file{ 0xa1, 0xb2, 0xc3, 0xd4, 0xd4, 0xc3, 0xb2, 0xa1 };
    BOOST_REQUIRE_EQUAL(body_file, expected_file);
    BOOST_REQUIRE(!instance.get_fault());
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

    bool to_data(database::flipper& sink) const NOEXCEPT
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

    bool to_data(database::flipper& sink) const NOEXCEPT
    {
        sink.write_big_endian(value);
        return sink;
    }

    uint32_t value{ 0 };
};

BOOST_AUTO_TEST_CASE(arraymap__slab_put__get__expected)
{
    data_chunk head_file;
    data_chunk body_file;
    test::chunk_storage head_store{ head_file };
    test::chunk_storage body_store{ body_file };
    arraymap<link5, big_slab::size> instance{ head_store, body_store };
    BOOST_REQUIRE(instance.put(big_slab{ 0xa1b2c3d4_u32 }));

    big_slab slab{};
    BOOST_REQUIRE(instance.get(zero, slab));
    BOOST_REQUIRE_EQUAL(slab.value, 0xa1b2c3d4_u32);

    const data_chunk expected_file{ 0xa1, 0xb2, 0xc3, 0xd4 };
    BOOST_REQUIRE_EQUAL(body_file, expected_file);
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(arraymap__slab_count__truncate__expected)
{
    data_chunk head_file;
    data_chunk body_file;
    test::chunk_storage head_store{ head_file };
    test::chunk_storage body_store{ body_file };
    arraymap<link5, big_slab::size> instance{ head_store, body_store };
    BOOST_REQUIRE(instance.put(big_slab{ 0xa1b2c3d4_u32 }));
    BOOST_REQUIRE(instance.put(big_slab{ 0xa1b2c3d4_u32 }));

    const data_chunk expected_file1{ 0xa1, 0xb2, 0xc3, 0xd4, 0xa1, 0xb2, 0xc3, 0xd4 };
    const data_chunk expected_file2{ 0xa1, 0xb2, 0xc3, 0xd4 };
    BOOST_REQUIRE_EQUAL(instance.count(), 8u);
    BOOST_REQUIRE_EQUAL(body_file, expected_file1);
    BOOST_REQUIRE(instance.truncate(4));
    BOOST_REQUIRE_EQUAL(instance.count(), 4u);
    BOOST_REQUIRE_EQUAL(body_file, expected_file2);
    BOOST_REQUIRE(instance.truncate(0));
    BOOST_REQUIRE_EQUAL(instance.count(), 0u);
    BOOST_REQUIRE(body_file.empty());
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(arraymap__slab_put_link__multiple__expected)
{
    data_chunk head_file;
    data_chunk body_file;
    test::chunk_storage head_store{ head_file };
    test::chunk_storage body_store{ body_file };
    arraymap<link5, big_slab::size> instance{ head_store, body_store };

    link5 link{};
    BOOST_REQUIRE(instance.put_link(link, big_slab{ 0xa1b2c3d4_u32 }));
    BOOST_REQUIRE(!link.is_terminal());
    BOOST_REQUIRE_EQUAL(link, 0u);

    link = instance.put_link(little_slab{ 0xa1b2c3d4_u32 });
    BOOST_REQUIRE(!link.is_terminal());
    BOOST_REQUIRE_EQUAL(link, big_slab::count());

    big_slab slab1{};
    BOOST_REQUIRE(instance.get(zero, slab1));
    BOOST_REQUIRE_EQUAL(slab1.value, 0xa1b2c3d4_u32);

    little_slab slab2{};
    BOOST_REQUIRE(instance.get(little_slab::count(), slab2));
    BOOST_REQUIRE_EQUAL(slab2.value, 0xa1b2c3d4_u32);

    const data_chunk expected_file{ 0xa1, 0xb2, 0xc3, 0xd4, 0xd4, 0xc3, 0xb2, 0xa1 };
    BOOST_REQUIRE_EQUAL(body_file, expected_file);
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

    bool to_data(database::flipper& sink) const NOEXCEPT
    {
        sink.write_big_endian(value);
        return sink;
    }

    uint64_t value{ 0 };
};

BOOST_AUTO_TEST_CASE(arraymap__record_get__excess__false)
{
    data_chunk head_file;
    data_chunk body_file{ 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08 };
    test::chunk_storage head_store{ head_file };
    test::chunk_storage body_store{ body_file };
    const arraymap<link5, record_excess::size> instance{ head_store, body_store };


    record_excess record{};
    BOOST_REQUIRE(!instance.get(zero, record));
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(arraymap__record_put_link__excess__false)
{
    data_chunk head_file;
    data_chunk body_file;
    test::chunk_storage head_store{ head_file };
    test::chunk_storage body_store{ body_file };
    arraymap<link5, record_excess::size> instance{ head_store, body_store };
    BOOST_REQUIRE(instance.put_link(record_excess{ 0xa1b2c3d4_u32 }).is_terminal());
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

    bool to_data(database::flipper& sink) const NOEXCEPT
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

    bool to_data(database::flipper& sink) const NOEXCEPT
    {
        sink.write_big_endian(value);
        return sink;
    }

    uint64_t value{ 0 };
};

BOOST_AUTO_TEST_CASE(arraymap__slab_get__excess__true)
{
    data_chunk head_file;
    data_chunk body_file{ 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08 };
    test::chunk_storage head_store{ head_file };
    test::chunk_storage body_store{ body_file };
    const arraymap<link5, slab_excess::size> instance{ head_store, body_store };

    // Excess read allowed to eof here (reader has only knowledge of size).
    slab_excess record{};
    BOOST_REQUIRE(instance.get(zero, record));
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(arraymap__slab_get__file_excess__false)
{
    data_chunk head_file;
    data_chunk body_file{ 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08 };
    test::chunk_storage head_store{ head_file };
    test::chunk_storage body_store{ body_file };
    const arraymap<link5, file_excess::size> instance{ head_store, body_store };

    // Excess read disallowed to here (past eof).
    file_excess record{};
    BOOST_REQUIRE(!instance.get<file_excess>(zero, record));
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(arraymap__slab_put_link__excess__false)
{
    data_chunk head_file;
    data_chunk body_file;
    test::chunk_storage head_store{ head_file };
    test::chunk_storage body_store{ body_file };
    arraymap<link5, slab_excess::size> instance{ head_store, body_store };
    BOOST_REQUIRE(instance.put_link(slab_excess{ 0xa1b2c3d4_u32 }).is_terminal());
    BOOST_REQUIRE(!instance.get_fault());
}

// record create/close/backup/restore/verify
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(arraymap__record_verify__empty_files__expected)
{
    data_chunk head_file;
    data_chunk body_file;
    test::chunk_storage head_store{ head_file };
    test::chunk_storage body_store{ body_file };
    record_table instance{ head_store, body_store };
    BOOST_REQUIRE(!instance.verify());
    BOOST_REQUIRE(instance.create());
    BOOST_REQUIRE(instance.verify());
    BOOST_REQUIRE_EQUAL(head_file.size(), link5::size);
    BOOST_REQUIRE(body_file.empty());
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(arraymap__record_create__non_empty_head_file__failure)
{
    data_chunk head_file{ 0x42 };
    data_chunk body_file;
    test::chunk_storage head_store{ head_file };
    test::chunk_storage body_store{ body_file };
    record_table instance{ head_store, body_store };
    BOOST_REQUIRE(!instance.verify());
    BOOST_REQUIRE(!instance.create());
    BOOST_REQUIRE_EQUAL(head_file.size(), one);
    BOOST_REQUIRE(body_file.empty());
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(arraymap__record_create__non_empty_body_file__body_zeroed)
{
    data_chunk head_file;
    data_chunk body_file{ 0x42 };
    test::chunk_storage head_store{ head_file };
    test::chunk_storage body_store{ body_file };
    record_table instance{ head_store, body_store };
    BOOST_REQUIRE(!instance.verify());
    BOOST_REQUIRE(instance.create());
    BOOST_REQUIRE(instance.verify());
    BOOST_REQUIRE_EQUAL(head_file.size(), link5::size);
    BOOST_REQUIRE(body_file.empty());
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(arraymap__record_body_count__create__zero)
{
    data_chunk head_file;
    data_chunk body_file;
    test::chunk_storage head_store{ head_file };
    test::chunk_storage body_store{ body_file };
    record_table instance{ head_store, body_store };
    BOOST_REQUIRE(instance.create());
    BOOST_REQUIRE_EQUAL(head_file, base16_chunk("0000000000"));
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(arraymap__record_body_count__empty_close__zero)
{
    auto head_file = base16_chunk("1234567890");
    data_chunk body_file;
    test::chunk_storage head_store{ head_file };
    test::chunk_storage body_store{ body_file };
    record_table instance{ head_store, body_store };
    BOOST_REQUIRE(instance.close());
    BOOST_REQUIRE_EQUAL(head_file, base16_chunk("0000000000"));
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(arraymap__record_body_count__two_close__two)
{
    data_chunk head_file;
    data_chunk body_file;
    test::chunk_storage head_store{ head_file };
    test::chunk_storage body_store{ body_file };
    record_table instance{ head_store, body_store };
    BOOST_REQUIRE(instance.create());
    body_file = base16_chunk("1234567812345678");
    BOOST_REQUIRE(instance.close());
    BOOST_REQUIRE_EQUAL(head_file, base16_chunk("0200000000"));
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(arraymap__record_body_count__two_backup__two)
{
    data_chunk head_file;
    data_chunk body_file;
    test::chunk_storage head_store{ head_file };
    test::chunk_storage body_store{ body_file };
    record_table instance{ head_store, body_store };
    BOOST_REQUIRE(instance.create());
    body_file = base16_chunk("1234567812345678");
    BOOST_REQUIRE(instance.backup());
    BOOST_REQUIRE_EQUAL(head_file, base16_chunk("0200000000"));
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(arraymap__record_body_count__empty_restore__truncates)
{
    data_chunk head_file;
    data_chunk body_file;
    test::chunk_storage head_store{ head_file };
    test::chunk_storage body_store{ body_file };
    record_table instance{ head_store, body_store };
    BOOST_REQUIRE(instance.create());
    body_file = base16_chunk("1234567812345678");
    BOOST_REQUIRE(instance.restore());
    BOOST_REQUIRE(body_file.empty());
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(arraymap__record_body_count__non_empty_restore__truncates)
{
    data_chunk head_file;
    data_chunk body_file;
    test::chunk_storage head_store{ head_file };
    test::chunk_storage body_store{ body_file };
    record_table instance{ head_store, body_store };
    BOOST_REQUIRE(instance.create());
    head_file = base16_chunk("0100000000");
    body_file = base16_chunk("1234567812345678");
    BOOST_REQUIRE(instance.restore());
    BOOST_REQUIRE_EQUAL(body_file, base16_chunk("12345678"));
    BOOST_REQUIRE(!instance.get_fault());
}

// slab create/close/backup/restore/verify
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(arraymap__slab_verify__empty_files__expected)
{
    data_chunk head_file;
    data_chunk body_file;
    test::chunk_storage head_store{ head_file };
    test::chunk_storage body_store{ body_file };
    slab_table instance{ head_store, body_store };
    BOOST_REQUIRE(!instance.verify());
    BOOST_REQUIRE(instance.create());
    BOOST_REQUIRE(instance.verify());
    BOOST_REQUIRE_EQUAL(head_file.size(), link5::size);
    BOOST_REQUIRE(body_file.empty());
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(arraymap__slab_create__non_empty_head_file__failure)
{
    data_chunk head_file{ 0x42 };
    data_chunk body_file;
    test::chunk_storage head_store{ head_file };
    test::chunk_storage body_store{ body_file };
    slab_table instance{ head_store, body_store };
    BOOST_REQUIRE(!instance.verify());
    BOOST_REQUIRE(!instance.create());
    BOOST_REQUIRE_EQUAL(head_file.size(), one);
    BOOST_REQUIRE(body_file.empty());
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(arraymap__slab_create__non_empty_body_file__body_zeroed)
{
    data_chunk head_file;
    data_chunk body_file{ 0x42 };
    test::chunk_storage head_store{ head_file };
    test::chunk_storage body_store{ body_file };
    slab_table instance{ head_store, body_store };
    BOOST_REQUIRE(!instance.verify());
    BOOST_REQUIRE(instance.create());
    BOOST_REQUIRE(instance.verify());
    BOOST_REQUIRE_EQUAL(head_file.size(), link5::size);
    BOOST_REQUIRE(body_file.empty());
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(arraymap__slab_body_count__create__zero)
{
    data_chunk head_file;
    data_chunk body_file;
    test::chunk_storage head_store{ head_file };
    test::chunk_storage body_store{ body_file };
    slab_table instance{ head_store, body_store };
    BOOST_REQUIRE(instance.create());
    BOOST_REQUIRE_EQUAL(head_file, base16_chunk("0000000000"));
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(arraymap__slab_body_count__empty_close__zero)
{
    auto head_file = base16_chunk("1234567890");
    data_chunk body_file;
    test::chunk_storage head_store{ head_file };
    test::chunk_storage body_store{ body_file };
    slab_table instance{ head_store, body_store };
    BOOST_REQUIRE(instance.close());
    BOOST_REQUIRE_EQUAL(head_file, base16_chunk("0000000000"));
}

BOOST_AUTO_TEST_CASE(arraymap__slab_body_count__two_close__two)
{
    data_chunk head_file;
    data_chunk body_file;
    test::chunk_storage head_store{ head_file };
    test::chunk_storage body_store{ body_file };
    slab_table instance{ head_store, body_store };
    BOOST_REQUIRE(instance.create());
    body_file = base16_chunk("1234");
    BOOST_REQUIRE(instance.close());
    BOOST_REQUIRE_EQUAL(head_file, base16_chunk("0200000000"));
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(arraymap__slab_body_count__two_backup__two)
{
    data_chunk head_file;
    data_chunk body_file;
    test::chunk_storage head_store{ head_file };
    test::chunk_storage body_store{ body_file };
    slab_table instance{ head_store, body_store };
    BOOST_REQUIRE(instance.create());
    body_file = base16_chunk("1234");
    BOOST_REQUIRE(instance.backup());
    BOOST_REQUIRE_EQUAL(head_file, base16_chunk("0200000000"));
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(arraymap__slab_body_count__empty_restore__truncates)
{
    data_chunk head_file;
    data_chunk body_file;
    test::chunk_storage head_store{ head_file };
    test::chunk_storage body_store{ body_file };
    slab_table instance{ head_store, body_store };
    BOOST_REQUIRE(instance.create());
    body_file = base16_chunk("1234567812345678");
    BOOST_REQUIRE(instance.restore());
    BOOST_REQUIRE(body_file.empty());
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(arraymap__slab_body_count__non_empty_restore__truncates)
{
    data_chunk head_file;
    data_chunk body_file;
    test::chunk_storage head_store{ head_file };
    test::chunk_storage body_store{ body_file };
    slab_table instance{ head_store, body_store };
    BOOST_REQUIRE(instance.create());
    head_file = base16_chunk("0300000000");
    body_file = base16_chunk("1234567812345678");
    BOOST_REQUIRE(instance.restore());
    BOOST_REQUIRE_EQUAL(body_file, base16_chunk("123456"));
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_SUITE_END()
