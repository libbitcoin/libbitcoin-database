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

template <typename Link>
class accessor_element
  : public element<slab_manager<Link>>
{
public:
    accessor_element(slab_manager<Link>& manager) NOEXCEPT
      : accessor_element(manager, base::eof)
    {
    }

    accessor_element(slab_manager<Link>& manager, Link link) NOEXCEPT
      : element<slab_manager<Link>>(manager, link)
    {
    }

    // accessor methods

    memory_ptr get_() const NOEXCEPT
    {
        return base::get();
    }

    memory_ptr get_(size_t offset) const NOEXCEPT
    {
        return base::get(offset);
    }

    memory_ptr allocate_(Link size) NOEXCEPT
    {
        return base::allocate(size);
    }

private:
    using base = element<slab_manager<Link>>;
};

BOOST_AUTO_TEST_CASE(element__get__eof_element__nullptr)
{
    test::storage file;
    slab_manager<uint32_t> manager{ file };
    const accessor_element element{ manager };
    BOOST_REQUIRE(!element.get_());
    BOOST_REQUIRE(!element.get_(1));
}

BOOST_AUTO_TEST_CASE(element__get__offset__nullptr)
{
    data_chunk data(42u, 0xff);
    test::storage file{ data };
    slab_manager<uint32_t> manager{ file };
    const accessor_element element{ manager, 0u };
    BOOST_REQUIRE_EQUAL(*element.get_(41)->data(), 0xff);
}

BOOST_AUTO_TEST_CASE(element__get__no_offset__expected)
{
    data_chunk data{ 0x00, 0x01, 0x02, 0x03 };
    test::storage file{ data };
    slab_manager<uint32_t> manager{ file };
    const accessor_element element{ manager, 1u };
    BOOST_REQUIRE_EQUAL(*element.get_()->data(), 0x01);
}

BOOST_AUTO_TEST_CASE(element__get__offset__expected)
{
    data_chunk data{ 0x00, 0x01, 0x02, 0x03 };
    test::storage file{ data };
    slab_manager<uint32_t> manager{ file };
    const accessor_element element{ manager, 2u };
    BOOST_REQUIRE_EQUAL(*element.get_(1)->data(), 0x03);
}

BOOST_AUTO_TEST_CASE(element__allocate__eof_start__expected)
{
    test::storage file;
    slab_manager<uint32_t> manager{ file };
    accessor_element element{ manager };
    BOOST_REQUIRE(element.allocate_(42));
    BOOST_REQUIRE_EQUAL(element.self(), 0u);
    BOOST_REQUIRE(element.allocate_(33));
    BOOST_REQUIRE_EQUAL(element.self(), 0u + 42u);
    BOOST_REQUIRE(element.allocate_(1));
    BOOST_REQUIRE_EQUAL(element.self(), 0u + 42u + 33u);
}

BOOST_AUTO_TEST_CASE(element__advance__eof__false)
{
    test::storage file;
    slab_manager<uint8_t> manager{ file };
    accessor_element element{ manager };
    BOOST_REQUIRE(!element.advance());
}

BOOST_AUTO_TEST_CASE(element__advance__linked__false)
{
    data_chunk data{ 0x02, 0x00, 0xff, 0xcc };
    test::storage file{ data };
    slab_manager<uint8_t> manager{ file };
    accessor_element element{ manager, 0_u8 };

    // Set first link to byte zero.
    BOOST_REQUIRE_EQUAL(element.self(), 0u);

    // Sets self/link to 0x02 (data[0]).
    BOOST_REQUIRE(element.advance());
    BOOST_REQUIRE_EQUAL(element.self(), 2u);

    // Sets self/link to eof (data[2]).
    BOOST_REQUIRE(element.advance());
    BOOST_REQUIRE_EQUAL(element.self(), element.eof);

    // Cannot advance past eof.
    BOOST_REQUIRE(!element.advance());
}

BOOST_AUTO_TEST_CASE(element__bool__eof__false)
{
    test::storage file;
    slab_manager<uint8_t> manager{ file };
    accessor_element element{ manager };
    BOOST_REQUIRE(!element);
}

BOOST_AUTO_TEST_CASE(element__bool__not_eof__true)
{
    test::storage file;
    slab_manager<uint8_t> manager{ file };
    accessor_element element{ manager, 0x00_u8 };
    BOOST_REQUIRE(element);
}

BOOST_AUTO_TEST_CASE(element__equality__eof_self__true)
{
    test::storage file;
    slab_manager<uint8_t> manager{ file };
    accessor_element element{ manager };
    BOOST_REQUIRE(element == element);
    BOOST_REQUIRE(!(element != element));
}

BOOST_AUTO_TEST_CASE(element__equality__not_eof_self__true)
{
    test::storage file;
    slab_manager<uint8_t> manager{ file };
    accessor_element element{ manager, 0x00_u8 };
    BOOST_REQUIRE(element == element);
    BOOST_REQUIRE(!(element != element));
}

BOOST_AUTO_TEST_CASE(element__equality__eof_distinct__true)
{
    test::storage file;
    slab_manager<uint8_t> manager{ file };
    accessor_element element1{ manager };
    accessor_element element2{ manager };
    BOOST_REQUIRE(element1 == element2);
    BOOST_REQUIRE(!(element1 != element2));
}

BOOST_AUTO_TEST_CASE(element__equality__same_values_distinct__true)
{
    test::storage file;
    slab_manager<uint8_t> manager{ file };
    accessor_element element1{ manager, 0x01_u8 };
    accessor_element element2{ manager, 0x01_u8 };
    BOOST_REQUIRE(element1 == element2);
    BOOST_REQUIRE(!(element1 != element2));
}

BOOST_AUTO_TEST_CASE(element__equality__different_values__true)
{
    test::storage file;
    slab_manager<uint8_t> manager{ file };
    accessor_element element1{ manager, 0x01_u8 };
    accessor_element element2{ manager, 0x02_u8 };
    BOOST_REQUIRE(element1 != element2);
    BOOST_REQUIRE(!(element1 == element2));
}

BOOST_AUTO_TEST_SUITE_END()
