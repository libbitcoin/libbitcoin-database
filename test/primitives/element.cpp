/**
 * Copyright (c) 2011-2019 libbitcoin developers (see AUTHORS)
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

BOOST_AUTO_TEST_SUITE(element_tests)

template <typename Link, typename Key, size_t Size>
class element_
  : public element<Link, Key, Size>
{
public:
    element_(manager<Link, Size>& manage) NOEXCEPT
      : element_(manage, Link::eof)
    {
    }

    element_(manager<Link, Size>& manage, Link link) NOEXCEPT
      : element<Link, Key, Size>(manage, link)
    {
    }

    // element_ methods

    memory_ptr get_() const NOEXCEPT
    {
        return base::get();
    }

    memory_ptr get_(size_t offset) const NOEXCEPT
    {
        return base::get(offset);
    }

private:
    using base = element<Link, Key, Size>;
};

#define DECLARE(link_size, key_size, data_size) \
using link = linkage<link_size>; \
using key = data_array<key_size>; \
using manage = manager<link, data_size>; \
using access = element_<link, key, data_size>

BOOST_AUTO_TEST_CASE(element__get__eof__nullptr)
{
    DECLARE(4, 0, 0);

    test::storage file;
    manage manager{ file };
    const access element{ manager };

    BOOST_REQUIRE(!element.get_());
    BOOST_REQUIRE(!element.get_(1));
}

BOOST_AUTO_TEST_CASE(element__get__offset__nullptr)
{
    DECLARE(4, 0, 0);

    data_chunk data(42u, 0xff);
    test::storage file{ data };
    manage manager{ file };
    const access element{ manager, 0 };

    BOOST_REQUIRE_EQUAL(*element.get_(41)->begin(), 0xffu);
}

BOOST_AUTO_TEST_CASE(element__get__no_offset__expected)
{
    DECLARE(4, 0, 0);

    data_chunk data{ 0x00, 0x01, 0x02, 0x03 };
    test::storage file{ data };
    manage manager{ file };
    const access element{ manager, 1u };

    BOOST_REQUIRE_EQUAL(*element.get_()->begin(), 0x01u);
}

BOOST_AUTO_TEST_CASE(element__get__offset__expected)
{
    DECLARE(4, 0, 0);

    data_chunk data{ 0x00, 0x01, 0x02, 0x03 };
    test::storage file{ data };
    manage manager{ file };
    const access element{ manager, 2u };

    BOOST_REQUIRE_EQUAL(*element.get_(1)->begin(), 0x03u);
}

BOOST_AUTO_TEST_CASE(element__get_next__linked__false)
{
    DECLARE(1, 0, 0);

    data_chunk data{ 0x02, 0x00, 0xff, 0xcc };
    test::storage file{ data };
    manage manager{ file };
    access element{ manager, 0_u8 };

    // First link is zero.
    BOOST_REQUIRE_EQUAL(element.self(), 0x00u);
    BOOST_REQUIRE_EQUAL(element.get_next(), 0x02u);

    // Sets self/link to 0x02 (data[0]).
    element.advance();
    BOOST_REQUIRE_EQUAL(element.self(), 0x02u);
    BOOST_REQUIRE_EQUAL(element.get_next(), 0xffu);
}

BOOST_AUTO_TEST_CASE(element__advance__linked__false)
{
    DECLARE(1, 0, 0);

    data_chunk data{ 0x02, 0x00, 0xff, 0xcc };
    test::storage file{ data };
    manage manager{ file };
    access element{ manager, 0_u8 };

    // First link is zero.
    BOOST_REQUIRE_EQUAL(element.self(), 0x00u);

    // Sets self/link to 0x02 (data[0]).
    element.advance();
    BOOST_REQUIRE(element);
    BOOST_REQUIRE_EQUAL(element.self(), 0x02u);

    // Sets self/link to eof (data[2]).
    element.advance();
    BOOST_REQUIRE(!element);
    BOOST_REQUIRE_EQUAL(element.self(), link::eof);
}

BOOST_AUTO_TEST_CASE(element__get_key__2_bytes__expected)
{
    DECLARE(1, 2, 0);

    data_chunk data{ 0x03, 0x11, 0x22, 0xff, 0x33, 0x44 };
    test::storage file{ data };
    manage manager{ file };
    access element{ manager, 0_u8 };

    constexpr key expected1{ 0x11, 0x22 };
    BOOST_REQUIRE_EQUAL(element.get_key(), expected1);

    element.advance();
    constexpr key expected2{ 0x33, 0x44 };
    BOOST_REQUIRE_EQUAL(element.get_key(), expected2);

    element.advance();
    BOOST_REQUIRE(!element);
}

BOOST_AUTO_TEST_CASE(element__is_match__2_bytes__expected)
{
    DECLARE(1, 2, 5);

    data_chunk data{ 0x01, 0x1a, 0x2a, 0x3a, 0x4a, 0x5a, 0xff, 0x1b, 0x2b, 0x3b, 0x4b, 0x5b };
    test::storage file{ data };
    manage manager{ file };
    access element{ manager, 0_u8 };

    BOOST_REQUIRE(element.is_match({ 0x1a, 0x2a }));

    element.advance();
    BOOST_REQUIRE(element.is_match({ 0x1b, 0x2b }));

    element.advance();
    BOOST_REQUIRE(!element);
}

BOOST_AUTO_TEST_CASE(element__bool__eof__false)
{
    DECLARE(1, 0, 0);

    test::storage file;
    manage manager{ file };
    const access element{ manager };

    BOOST_REQUIRE(!element);
}

BOOST_AUTO_TEST_CASE(element__bool__not_eof__true)
{
    DECLARE(1, 0, 0);

    test::storage file;
    manage manager{ file };
    const access element{ manager, 0x00_u8 };

    BOOST_REQUIRE(element);
}

BOOST_AUTO_TEST_CASE(element__equality__eof_self__true)
{
    DECLARE(1, 0, 0);

    test::storage file;
    manage manager{ file };
    const access element{ manager };

    BOOST_REQUIRE(element == element);
    BOOST_REQUIRE(!(element != element));
}

BOOST_AUTO_TEST_CASE(element__equality__not_eof_self__true)
{
    DECLARE(1, 0, 0);

    test::storage file;
    manage manager{ file };
    const access element{ manager, 0x00_u8 };

    BOOST_REQUIRE(element == element);
    BOOST_REQUIRE(!(element != element));
}

BOOST_AUTO_TEST_CASE(element__equality__eof_distinct__true)
{
    DECLARE(1, 0, 0);

    test::storage file;
    manage manager{ file };
    const access element1{ manager };
    const access element2{ manager };

    BOOST_REQUIRE(element1 == element2);
    BOOST_REQUIRE(!(element1 != element2));
}

BOOST_AUTO_TEST_CASE(element__equality__same_values_distinct__true)
{
    DECLARE(1, 0, 0);

    test::storage file;
    manage manager{ file };
    const access element1{ manager, 0x01_u8 };
    const access element2{ manager, 0x01_u8 };

    BOOST_REQUIRE(element1 == element2);
    BOOST_REQUIRE(!(element1 != element2));
}

BOOST_AUTO_TEST_CASE(element__equality__different_values__true)
{
    DECLARE(1, 0, 0);

    test::storage file;
    manage manager{ file };
    const access element1{ manager, 0x01_u8 };
    const access element2{ manager, 0x02_u8 };

    BOOST_REQUIRE(element1 != element2);
    BOOST_REQUIRE(!(element1 == element2));
}

BOOST_AUTO_TEST_SUITE_END()
