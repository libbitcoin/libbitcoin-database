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

BOOST_AUTO_TEST_SUITE(iterator_tests)

template <typename Link, typename Key, size_t Size>
class iterator_
  : public iterator<Link, Key, Size>
{
public:
    iterator_(manager<Link, Size>& manage, const Key& key) NOEXCEPT
      : iterator_(manage, Link::terminal, key)
    {
    }

    iterator_(manager<Link, Size>& manage, const Link& link,
        const Key& key) NOEXCEPT
      : iterator<Link, Key, Size>(manage, link, key)
    {
    }

    memory_ptr get_(size_t offset) const NOEXCEPT
    {
        return base::get(offset);
    }

private:
    using base = iterator<Link, Key, Size>;
};

#define DECLARE(link_size, key_size, data_size) \
using link = linkage<link_size>; \
using key = data_array<key_size>; \
using manage = manager<link, data_size>; \
using access = iterator_<link, key, data_size>; \
constexpr key key0{}

BOOST_AUTO_TEST_CASE(iterator__get__empty__nullptr)
{
    DECLARE(4, 0, 0);

    test::storage file;
    manage manager{ file };
    const access iterator{ manager, key0 };
    BOOST_REQUIRE(!iterator.get_(0));
    BOOST_REQUIRE(!iterator.get_(1));
}

BOOST_AUTO_TEST_CASE(iterator__get__no_offset__expected)
{
    DECLARE(4, 0, 0);

    data_chunk data{ 0x00, 0x01, 0x02, 0x03 };
    test::storage file{ data };
    manage manager{ file };
    const access iterator{ manager, 1u, key0 };
    BOOST_REQUIRE_EQUAL(*iterator.get_(0)->begin(), 0x01u);
}

BOOST_AUTO_TEST_CASE(iterator__get__offset1__expected)
{
    DECLARE(4, 0, 0);

    data_chunk data{ 0x00, 0x01, 0x02, 0x03 };
    test::storage file{ data };
    manage manager{ file };
    const access iterator{ manager, 2u, key0 };
    BOOST_REQUIRE_EQUAL(*iterator.get_(1)->begin(), 0x03u);
}

BOOST_AUTO_TEST_CASE(iterator__get__offset_to_back__expected)
{
    DECLARE(4, 0, 0);

    data_chunk data(42u, 0xff);
    test::storage file{ data };
    manage manager{ file };
    const access iterator{ manager, 0, key0 };
    BOOST_REQUIRE_EQUAL(*iterator.get_(41)->begin(), 0xffu);
}

BOOST_AUTO_TEST_CASE(iterator__next__self__expected)
{
    DECLARE(1, 2, 2);

    data_chunk data{ 0x01, 0x1a, 0x2a, 0x02, 0x1a, 0x2a, 0xff, 0xcc, 0xcc };
    test::storage file{ data };
    manage manager{ file };

    constexpr key key1{ 0x1a, 0x2a };
    access iterator1{ manager, 0, key1 };

    // First link is zero, matched.
    BOOST_REQUIRE_EQUAL(iterator1.self(), 0x00u);

    // Sets self/link to 0x01 (data[3]), matched.
    BOOST_REQUIRE(iterator1.next());
    BOOST_REQUIRE(!iterator1.self().is_terminal());
    BOOST_REQUIRE_EQUAL(iterator1.self(), 0x01u);

    // No more matches.
    BOOST_REQUIRE(!iterator1.next());
    BOOST_REQUIRE_EQUAL(iterator1.self(), link::terminal);
}

BOOST_AUTO_TEST_CASE(iterator__next__true__non_terminal)
{
    DECLARE(1, 2, 2);

    data_chunk data{ 0x01, 0x1a, 0x2a, 0x02, 0x1a, 0x2a, 0xff, 0xcc, 0xcc };
    test::storage file{ data };
    manage manager{ file };

    constexpr key key1{ 0x1a, 0x2a };
    access iterator{ manager, 0, key1 };

    while (iterator.next())
    {
        BOOST_REQUIRE(!iterator.self().is_terminal());
    }
}

BOOST_AUTO_TEST_SUITE_END()
