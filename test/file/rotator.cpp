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
 * You should have received a create_file of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "../test.hpp"

BOOST_FIXTURE_TEST_SUITE(rotator_tests, test::directory_setup_fixture)

// ostream instance.
using rotate = file::stream::out::rotator;

// protected method accessor.
class accessor
  : public file::rotator_sink
{
public:
    using rotator_sink::rotator_sink;
    bool start_() NOEXCEPT { return start(); }
    bool stop_() NOEXCEPT { return stop(); }
    bool rotate_() NOEXCEPT { return rotate(); }
};

// test data.
const std::string text = "panopticon";

BOOST_AUTO_TEST_CASE(rotator__write__at_file_limit__single)
{
    const std::string expected(42, 'x');
    rotate splitter(TEST_PATH, TEST_PATH + "_", expected.size());
    splitter << expected;
    BOOST_REQUIRE(splitter);

    // File is initialized, but nothing is written because buffer not overflowed.
    BOOST_REQUIRE_EQUAL(test::size(TEST_PATH), zero);
    BOOST_REQUIRE(!test::exists(TEST_PATH + "_"));

    splitter.flush();
    BOOST_REQUIRE(splitter);
    BOOST_REQUIRE_EQUAL(test::size(TEST_PATH), expected.size());
    BOOST_REQUIRE_EQUAL(test::read_line(TEST_PATH), expected);
    BOOST_REQUIRE(!test::exists(TEST_PATH + "_"));
}

BOOST_AUTO_TEST_CASE(rotator__write__over_file_limit__split)
{
    const std::string text(42, 'x');
    rotate splitter(TEST_PATH, TEST_PATH + "_", sub1(text.size()));
    splitter << text;
    BOOST_REQUIRE(splitter);

    // File is initialized, but nothing is written because buffer not overflowed.
    BOOST_REQUIRE_EQUAL(test::size(TEST_PATH), zero);
    BOOST_REQUIRE(!test::exists(TEST_PATH + "_"));

    splitter.flush();
    BOOST_REQUIRE(splitter);
    BOOST_REQUIRE_EQUAL(test::size(TEST_PATH), one);
    BOOST_REQUIRE_EQUAL(test::size(TEST_PATH + "_"), 41);
}

BOOST_AUTO_TEST_CASE(rotator__construct__empty__expected)
{
    {
        BOOST_REQUIRE(!test::exists(TEST_PATH + "_"));
        BOOST_REQUIRE(!test::exists(TEST_PATH));
        rotate splitter(TEST_PATH, TEST_PATH + "_", 42);
        BOOST_REQUIRE(test::exists(TEST_PATH));
        BOOST_REQUIRE(!test::exists(TEST_PATH + "_"));
    }
    BOOST_REQUIRE_EQUAL(test::size(TEST_PATH), zero);
    BOOST_REQUIRE(!test::exists(TEST_PATH + "_"));
}

BOOST_AUTO_TEST_CASE(rotator__construct__existing1__expected)
{
    {
        BOOST_REQUIRE(!test::exists(TEST_PATH + "_"));
        BOOST_REQUIRE(test::create(TEST_PATH, text));
        rotate splitter(TEST_PATH, TEST_PATH + "_", 42);
        BOOST_REQUIRE(test::exists(TEST_PATH));
        BOOST_REQUIRE(!test::exists(TEST_PATH + "_"));
    }
    BOOST_REQUIRE_EQUAL(test::read_line(TEST_PATH), text);
    BOOST_REQUIRE(!test::exists(TEST_PATH + "_"));
}

BOOST_AUTO_TEST_CASE(rotator__construct__existing2__expected)
{
    {
        BOOST_REQUIRE(test::create(TEST_PATH + "_", text));
        BOOST_REQUIRE(!test::exists(TEST_PATH));
        rotate splitter(TEST_PATH, TEST_PATH + "_", 42);
        BOOST_REQUIRE(test::exists(TEST_PATH));
        BOOST_REQUIRE(test::exists(TEST_PATH + "_"));
    }
    BOOST_REQUIRE_EQUAL(test::size(TEST_PATH), zero);
    BOOST_REQUIRE_EQUAL(test::read_line(TEST_PATH + "_"), text);
}

BOOST_AUTO_TEST_CASE(rotator__construct__both_existing__expected)
{
    const std::string text1 = "panopticon1";
    const std::string text2 = "panopticon2";
    {
        BOOST_REQUIRE(test::create(TEST_PATH + "_", text2));
        BOOST_REQUIRE(test::create(TEST_PATH, text1));
        rotate splitter(TEST_PATH, TEST_PATH + "_", 42);
        BOOST_REQUIRE(test::exists(TEST_PATH));
        BOOST_REQUIRE(test::exists(TEST_PATH + "_"));
    }
    BOOST_REQUIRE_EQUAL(test::read_line(TEST_PATH), text1);
    BOOST_REQUIRE_EQUAL(test::read_line(TEST_PATH + "_"), text2);
}

BOOST_AUTO_TEST_CASE(rotator__start__started__false)
{
    accessor device(TEST_PATH, TEST_PATH + "_", 42);
    BOOST_REQUIRE(!device.start_());
}

BOOST_AUTO_TEST_CASE(rotator__stop__stopped__false)
{
    accessor device(TEST_PATH, TEST_PATH + "_", 42);
    BOOST_REQUIRE(device.stop_());
    BOOST_REQUIRE(!device.stop_());
}

BOOST_AUTO_TEST_CASE(rotator__start_stopped__stop_started__true)
{
    accessor device(TEST_PATH, TEST_PATH + "_", 42);
    BOOST_REQUIRE(device.stop_());
    BOOST_REQUIRE(device.start_());
}

BOOST_AUTO_TEST_CASE(rotator__rotate__existing__expected)
{
    BOOST_REQUIRE(test::create(TEST_PATH, text));

    accessor instance(TEST_PATH, TEST_PATH + "_", 42);
    BOOST_REQUIRE_EQUAL(test::size(TEST_PATH), text.size());
    BOOST_REQUIRE(!test::exists(TEST_PATH + "_"));
    BOOST_REQUIRE(instance.rotate_());
    BOOST_REQUIRE_EQUAL(test::size(TEST_PATH + "_"), text.size());
    BOOST_REQUIRE(test::exists(TEST_PATH));
    BOOST_REQUIRE_EQUAL(test::size(TEST_PATH), zero);
}

BOOST_AUTO_TEST_CASE(rotator__write__below_limit__written)
{
    rotate splitter(TEST_PATH, TEST_PATH + "_", add1(text.size()));
    splitter.write(text.data(), text.size());
    splitter.flush();
    BOOST_REQUIRE(splitter);
    BOOST_REQUIRE_EQUAL(test::size(TEST_PATH), text.size());
    BOOST_REQUIRE(!test::exists(TEST_PATH + "_"));
}

BOOST_AUTO_TEST_CASE(rotator__write__at_limit__written)
{
    rotate splitter(TEST_PATH, TEST_PATH + "_", text.size());
    splitter.write(text.data(), text.size());
    splitter.flush();
    BOOST_REQUIRE(splitter);
    BOOST_REQUIRE_EQUAL(test::size(TEST_PATH), text.size());
    BOOST_REQUIRE(!test::exists(TEST_PATH + "_"));
}

BOOST_AUTO_TEST_CASE(rotator__write__above_limit___split)
{
    rotate splitter(TEST_PATH, TEST_PATH + "_", sub1(text.size()));
    BOOST_REQUIRE(splitter);

    splitter.write(text.data(), text.size());
    BOOST_REQUIRE(splitter);

    splitter.flush();
    BOOST_REQUIRE(splitter);
    BOOST_REQUIRE_EQUAL(test::size(TEST_PATH), one);
    BOOST_REQUIRE_EQUAL(test::size(TEST_PATH + "_"), sub1(text.size()));
}

BOOST_AUTO_TEST_CASE(rotator__write__twice_at_limit__split)
{
    rotate splitter(TEST_PATH, TEST_PATH + "_", text.size());
    splitter.write(text.data(), text.size());
    splitter.write(text.data(), text.size());
    splitter.flush();
    BOOST_REQUIRE(splitter);
    BOOST_REQUIRE_EQUAL(test::size(TEST_PATH), text.size());
    BOOST_REQUIRE_EQUAL(test::size(TEST_PATH + "_"), text.size());
}

BOOST_AUTO_TEST_CASE(rotator__write__thrice_at_limit__stacked)
{
    const std::string text1 = "panopticon1";
    const std::string text2 = "panopticon2";
    rotate splitter(TEST_PATH, TEST_PATH + "_", text1.size());
    splitter.write(text1.data(), text1.size());
    splitter.write(text2.data(), text2.size());
    splitter.write(text1.data(), text1.size());
    splitter.flush();
    BOOST_REQUIRE(splitter);
    BOOST_REQUIRE_EQUAL(test::read_line(TEST_PATH), text1);
    BOOST_REQUIRE_EQUAL(test::read_line(TEST_PATH + "_"), text2);
}

BOOST_AUTO_TEST_CASE(rotator__write__parts__accumulated_and_rotated)
{
    // Binary mode on Windows ensures that \n nor replaced with \r\n.
    rotate splitter(TEST_PATH, TEST_PATH + "_", 6);
    splitter.write("abc", 3);
    splitter.write("def", 3);
    splitter.write("ghi", 3);
    splitter.write("jkl", 3);
    splitter.write("mn\n", 3);
    splitter.write("pqr", 3);
    splitter.flush();
    BOOST_REQUIRE_EQUAL(test::size(TEST_PATH), 6);
    BOOST_REQUIRE_EQUAL(test::read_line(TEST_PATH, 0), "mn");
    BOOST_REQUIRE_EQUAL(test::read_line(TEST_PATH, 1), "pqr");
    BOOST_REQUIRE_EQUAL(test::read_line(TEST_PATH + "_"), "ghijkl");
}

BOOST_AUTO_TEST_CASE(rotator__write__existing__appends)
{
    BOOST_REQUIRE(test::create(TEST_PATH, text));

    rotate splitter(TEST_PATH, TEST_PATH + "_", 42);
    splitter.write("\n", 1);
    splitter.write("abc", 3);
    splitter.flush();
    BOOST_REQUIRE_EQUAL(test::read_line(TEST_PATH, 0), "panopticon");
    BOOST_REQUIRE_EQUAL(test::read_line(TEST_PATH, 1), "abc");
}

BOOST_AUTO_TEST_SUITE_END()
