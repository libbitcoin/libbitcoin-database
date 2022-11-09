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

template <typename Link, size_t Size = zero, typename Record = bool>
class arraymap_
  : public arraymap<Link, Size, Record>
{
public:
    using link = typename Link;
    using base = arraymap<Link, Size, Record>;

    using base::arraymap;

    reader_ptr at_(const link& record) const NOEXCEPT
    {
        return base::at(record);
    }

    writer_ptr push_(const link& size=one) NOEXCEPT
    {
        return base::push(size);
    }
};

constexpr auto link_size = 5_size;
constexpr auto header_size = 105_size;

// Key size does not factor into header byte size (for first key only).
constexpr auto links = header_size / link_size;
static_assert(links == 21u);

// Bucket count is one less than link count, due to header.size field.
constexpr auto buckets = sub1(links);
static_assert(buckets == 20u);

// Record size includes key but not link.
// Slab allocation includes key and link.
constexpr auto data_size = 4_size;

constexpr auto record_size = link_size + data_size;
constexpr auto slab_size = record_size;

using link = linkage<link_size>;
using record_table = arraymap_<link, data_size>;
using slab_table = arraymap_<link, zero>;

// record_arraymap__create_verify

BOOST_AUTO_TEST_CASE(record_arraymap__create_verify__empty_files__success)
{
    data_chunk body_file;
    test::storage body_store{ body_file };
    const record_table instance{ body_store };
    BOOST_REQUIRE(body_file.empty());
}

BOOST_AUTO_TEST_CASE(record_arraymap__create_verify__multiple_iterator_body_file__failure)
{
    constexpr auto body_size = 3u * record_size;
    data_chunk body_file(body_size, 0x42);
    test::storage body_store{ body_file };
    const record_table instance{ body_store };
    BOOST_REQUIRE_EQUAL(body_file.size(), body_size);
}

BOOST_AUTO_TEST_CASE(record_arraymap__create_verify__multiple_fractional_iterator_body_file__failure)
{
    constexpr auto body_size = 3u * record_size + 2u;
    data_chunk body_file(body_size, 0x42);
    test::storage body_store{ body_file };
    const record_table instance{ body_store };
    BOOST_REQUIRE_EQUAL(body_file.size(), body_size);
}

BOOST_AUTO_TEST_CASE(record_arraymap__create_verify__one_iterator_body_file__failure)
{
    constexpr auto body_size = record_size;
    data_chunk body_file(body_size, 0x42);
    test::storage body_store{ body_file };
    const record_table instance{ body_store };
    BOOST_REQUIRE_EQUAL(body_file.size(), body_size);
}

BOOST_AUTO_TEST_CASE(record_arraymap__create_verify__sub_one_iterator_body_file__success)
{
    constexpr auto body_size = sub1(record_size);
    data_chunk body_file(body_size, 0x42);
    test::storage body_store{ body_file };
    const record_table instance{ body_store };
    BOOST_REQUIRE_EQUAL(body_file.size(), body_size);
}

// at(terminal)

BOOST_AUTO_TEST_CASE(record_arraymap__at__terminal__false)
{
    data_chunk body_file;
    test::storage body_store{ body_file };
    const record_table instance{ body_store };
    BOOST_REQUIRE(!instance.at_(link::terminal));
}

// at(exhausted)

BOOST_AUTO_TEST_CASE(record_arraymap__at__empty__exhausted)
{
    data_chunk body_file;
    test::storage body_store{ body_file };
    const record_table instance{ body_store };
    BOOST_REQUIRE(instance.at_(0)->is_exhausted());
    BOOST_REQUIRE(instance.at_(19)->is_exhausted());
}

// slab_arraymap__create_verify

BOOST_AUTO_TEST_CASE(slab_arraymap__create_verify__empty_files__success)
{
    data_chunk body_file;
    test::storage body_store{ body_file };
    const slab_table instance{ body_store };
    BOOST_REQUIRE(body_file.empty());
}

BOOST_AUTO_TEST_CASE(slab_arraymap__create_verify__non_empty_head_file__failure)
{
    data_chunk body_file;
    test::storage body_store{ body_file };
    const slab_table instance{ body_store };
    BOOST_REQUIRE(body_file.empty());
}

BOOST_AUTO_TEST_CASE(slab_arraymap__create_verify__multiple_iterator_body_file__failure)
{
    constexpr auto body_size = 3u * slab_size;
    data_chunk body_file(body_size, 0x42);
    test::storage body_store{ body_file };
    const slab_table instance{ body_store };
    BOOST_REQUIRE_EQUAL(body_file.size(), body_size);
}

BOOST_AUTO_TEST_CASE(slab_arraymap__create_verify__multiple_fractional_iterator_body_file__failure)
{
    constexpr auto body_size = 3u * slab_size + 2u;
    data_chunk body_file(body_size, 0x42);
    test::storage body_store{ body_file };
    const slab_table instance{ body_store };
    BOOST_REQUIRE_EQUAL(body_file.size(), body_size);
}

BOOST_AUTO_TEST_CASE(slab_arraymap__create_verify__one_iterator_body_file__failure)
{
    constexpr auto body_size = slab_size;
    data_chunk body_file(body_size, 0x42);
    test::storage body_store{ body_file };
    const slab_table instance{ body_store };
    BOOST_REQUIRE_EQUAL(body_file.size(), body_size);
}

BOOST_AUTO_TEST_CASE(slab_arraymap__create_verify__sub_one_iterator_body_file__failure)
{
    constexpr auto body_size = sub1(slab_size);
    data_chunk body_file(body_size, 0x42);
    test::storage body_store{ body_file };
    const slab_table instance{ body_store };
    BOOST_REQUIRE_EQUAL(body_file.size(), body_size);
}

// at(terminal)

BOOST_AUTO_TEST_CASE(slab_arraymap__at__terminal__false)
{
    data_chunk body_file;
    test::storage body_store{ body_file };
    const slab_table instance{ body_store };
    BOOST_REQUIRE(!instance.at_(link::terminal));
}

// at(exhausted)

BOOST_AUTO_TEST_CASE(slab_arraymap__at__empty__exhausted)
{
    data_chunk body_file;
    test::storage body_store{ body_file };
    const slab_table instance{ body_store };
    BOOST_REQUIRE(instance.at_(0)->is_exhausted());
    BOOST_REQUIRE(instance.at_(19)->is_exhausted());
}

BOOST_AUTO_TEST_SUITE_END()
