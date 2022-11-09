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

template <typename Link, size_t Size = zero>
class arraymap_
  : public arraymap<Link, Size>
{
public:
    using link = typename Link;
    using base = arraymap<Link, Size>;

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

// There is no internal linkage, but there is still a primary key domain.
constexpr size_t link_size = 5;
using link = linkage<link_size>;

constexpr size_t data_size = 4;
using record_table = arraymap_<link, data_size>;
using slab_table = arraymap_<link, zero>;

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
    BOOST_REQUIRE(!instance.at_(link::terminal));
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
    BOOST_REQUIRE(!instance.at_(link::terminal));
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
    BOOST_REQUIRE_EQUAL(body_file.size(), data_size);
    BOOST_REQUIRE(!stream0->is_exhausted());
    BOOST_REQUIRE(instance.at_(0));
    stream0.reset();

    auto stream1 = instance.push_();
    BOOST_REQUIRE_EQUAL(body_file.size(), 2u * data_size);
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

    auto stream0 = instance.push_(data_size);
    BOOST_REQUIRE_EQUAL(body_file.size(), data_size);
    BOOST_REQUIRE(!stream0->is_exhausted());
    BOOST_REQUIRE(instance.at_(0));
    stream0.reset();

    auto stream1 = instance.push_(data_size);
    BOOST_REQUIRE_EQUAL(body_file.size(), 2u * data_size);
    BOOST_REQUIRE(!stream1->is_exhausted());
    BOOST_REQUIRE(instance.at_(data_size));
    stream1.reset();

    // Past end is valid pointer but exhausted stream.
    BOOST_REQUIRE(instance.at_(2u * data_size));
    BOOST_REQUIRE(instance.at_(2u * data_size)->is_exhausted());

    // record (assumes zero fill)
    // =================================
    // 00000000 [0]
    // 00000000 [1]
}

BOOST_AUTO_TEST_SUITE_END()
