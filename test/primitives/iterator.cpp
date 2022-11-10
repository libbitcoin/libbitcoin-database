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
    iterator_(const memory_ptr& file, const Key& key) NOEXCEPT
      : iterator_(file, Link::terminal, key)
    {
    }

    iterator_(const memory_ptr& file, const Link& link, const Key& key) NOEXCEPT
      : iterator<Link, Key, Size>(file, link, key)
    {
    }

    bool is_match_() const NOEXCEPT
    {
        return base::is_match();
    }

    Link get_next_() const NOEXCEPT
    {
        return base::get_next();
    }

private:
    using base = iterator<Link, Key, Size>;
};

BOOST_AUTO_TEST_CASE(iterator__get_next__empty__terminal)
{
    using link = linkage<4>;
    using key = data_array<0>;
    using slab_iterate = iterator_<link, key, 0>;

    constexpr key key0{};
    test::storage file;
    const slab_iterate iterator{ file.get(), key0 };
    BOOST_REQUIRE(iterator.get_next_().is_terminal());
}

BOOST_AUTO_TEST_CASE(iterator__get__offset0__expected)
{
    using link = linkage<4>;
    using key = data_array<0>;
    using slab_iterate = iterator_<link, key, 0>;

    constexpr auto start = 0;
    constexpr key key0{};
    data_chunk data
    {
        0x00, 0x01, 0x02, 0x03,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00
    };
    test::storage file{ data };
    const slab_iterate iterator{ file.get(), start, key0 };
    BOOST_REQUIRE_EQUAL(iterator.get_next_(), 0x03020100_u32);
}

BOOST_AUTO_TEST_CASE(iterator__get__offset1__expected)
{
    using link = linkage<4>;
    using key = data_array<0>;
    using slab_iterate = iterator_<link, key, 0>;

    constexpr auto start = 8;
    constexpr key key0{};
    data_chunk data
    {
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x01, 0x02, 0x03
    };
    test::storage file{ data };
    const slab_iterate iterator{ file.get(), start, key0 };
    BOOST_REQUIRE_EQUAL(iterator.get_next_(), 0x03020100_u32);
}

BOOST_AUTO_TEST_CASE(iterator__get__offset_to_back__expected)
{
    using link = linkage<1>;
    using key = data_array<0>;
    using slab_iterate = iterator_<link, key, 0>;

    constexpr auto start = 41;
    constexpr key key0{};
    data_chunk data(42u, 0xff);
    test::storage file{ data };
    const slab_iterate iterator{ file.get(), start, key0 };
    BOOST_REQUIRE_EQUAL(iterator.get_next_(), 0xffu);
}

BOOST_AUTO_TEST_CASE(iterator__next__self__expected)
{
    using link = linkage<1>;
    using key = data_array<2>;
    using record_iterate = iterator_<link, key, 1>;

    constexpr auto start = 0;
    constexpr key key2{ 0x1a, 0x2a };
    data_chunk data
    {
        0x01, 0x1a, 0x2a, 0xee,
        0x02, 0x1a, 0x2a, 0xee,
        0xff, 0xcc, 0xcc, 0xee
    };
    test::storage file{ data };
    record_iterate iterator1{ file.get(), start, key2 };

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
    using link = linkage<1>;
    using key = data_array<2>;
    using slab_iterate = iterator_<link, key, 0>;
    constexpr auto start = 0;

    constexpr key key2{ 0x1a, 0x2a };
    data_chunk data
    {
        0x04, 0x1a, 0x2a, 0xee,
        0x08, 0x1a, 0x2a, 0xee,
        0xff, 0xcc, 0xcc, 0xee
    };
    test::storage file{ data };
    slab_iterate iterator{ file.get(), start, key2 };

    BOOST_REQUIRE(!iterator.self().is_terminal());
    BOOST_REQUIRE(iterator.next());
    BOOST_REQUIRE(!iterator.self().is_terminal());
    BOOST_REQUIRE(!iterator.next());
    BOOST_REQUIRE(iterator.self().is_terminal());
}

BOOST_AUTO_TEST_SUITE_END()
