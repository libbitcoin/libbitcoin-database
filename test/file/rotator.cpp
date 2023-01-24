/**
 * Copyright (c) 2011-2023 libbitcoin developers (see AUTHORS)
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

struct rotator_setup_fixture
{
    DELETE_COPY_MOVE(rotator_setup_fixture);
    BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

    rotator_setup_fixture() NOEXCEPT
    {
        BOOST_REQUIRE(test::clear(test::directory));
    }

    ~rotator_setup_fixture() NOEXCEPT
    {
        BOOST_REQUIRE(test::clear(test::directory));
    }

    BC_POP_WARNING()
};

BOOST_FIXTURE_TEST_SUITE(rotator_tests, rotator_setup_fixture)

class accessor
  : public file::rotator
{
public:
    using rotator::rotator;
    bool rotate_() NOEXCEPT { return rotate();  }
    ////bool set_size_() NOEXCEPT { return set_size(); }
    ////bool set_stream_() NOEXCEPT { return set_stream(); }
};

BOOST_AUTO_TEST_CASE(rotator__start__missing__expected)
{
    file::rotator instance(TEST_PATH, TEST_PATH + "_", 42);
    BOOST_REQUIRE(instance.start());
    BOOST_REQUIRE(test::exists(TEST_PATH));
}

BOOST_AUTO_TEST_CASE(rotator__start__existing__expected)
{
    BOOST_REQUIRE(test::create(TEST_PATH));
    file::rotator instance(TEST_PATH, TEST_PATH + "_", 42);
    BOOST_REQUIRE(instance.start());
    BOOST_REQUIRE(test::exists(TEST_PATH));
}

BOOST_AUTO_TEST_CASE(rotator__stop__stopped__false)
{
    file::rotator instance(TEST_PATH, TEST_PATH + "_", 42);
    BOOST_REQUIRE(!instance.stop());
    BOOST_REQUIRE(!instance.stop());
}

BOOST_AUTO_TEST_CASE(rotator__stop__started__etrue)
{
    file::rotator instance(TEST_PATH, TEST_PATH + "_", 42);
    BOOST_REQUIRE(instance.start());
    BOOST_REQUIRE(instance.stop());
}

BOOST_AUTO_TEST_CASE(rotator__rotate__stopped__false)
{
    accessor instance(TEST_PATH, TEST_PATH + "_", 42);
    BOOST_REQUIRE(!instance.rotate_());
}

BOOST_AUTO_TEST_CASE(rotator__rotate__started_missing__expected)
{
    accessor instance(TEST_PATH, TEST_PATH + "_", 42);
    BOOST_REQUIRE(instance.start());
    BOOST_REQUIRE(test::exists(TEST_PATH));
    BOOST_REQUIRE(!test::exists(TEST_PATH + "_"));
    BOOST_REQUIRE(instance.rotate_());
    BOOST_REQUIRE(test::exists(TEST_PATH + "_"));
    BOOST_REQUIRE(test::exists(TEST_PATH));
    BOOST_REQUIRE(instance.stop());
    BOOST_REQUIRE(test::exists(TEST_PATH + "_"));
    BOOST_REQUIRE(test::exists(TEST_PATH));
    BOOST_REQUIRE_EQUAL(test::size(TEST_PATH + "_"), zero);
    BOOST_REQUIRE_EQUAL(test::size(TEST_PATH), zero);
}

BOOST_AUTO_TEST_CASE(rotator__rotate__started_existing__expected)
{
    const std::string text = "panopticon";
    BOOST_REQUIRE(test::create(TEST_PATH, text));
    BOOST_REQUIRE_EQUAL(test::size(TEST_PATH), text.size());

    accessor instance(TEST_PATH, TEST_PATH + "_", 42);
    BOOST_REQUIRE(instance.start());
    BOOST_REQUIRE_EQUAL(test::size(TEST_PATH), text.size());
    BOOST_REQUIRE(!test::exists(TEST_PATH + "_"));

    BOOST_REQUIRE(instance.rotate_());
    BOOST_REQUIRE_EQUAL(test::size(TEST_PATH + "_"), text.size());
    BOOST_REQUIRE(test::exists(TEST_PATH));
    BOOST_REQUIRE_EQUAL(test::size(TEST_PATH), zero);
}

BOOST_AUTO_TEST_CASE(rotator__write__limit__false)
{
    const std::string text = "panopticon";
    file::rotator instance(TEST_PATH, TEST_PATH + "_", text.size());
    BOOST_REQUIRE(instance.start());
    BOOST_REQUIRE(!instance.write(text));
    BOOST_REQUIRE(instance.stop());
    BOOST_REQUIRE(test::exists(TEST_PATH));
    BOOST_REQUIRE_EQUAL(test::size(TEST_PATH), zero);
    BOOST_REQUIRE(!test::exists(TEST_PATH + "_"));
}

BOOST_AUTO_TEST_CASE(rotator__write__limit_add1__false)
{
    const std::string text = "panopticon";
    file::rotator instance(TEST_PATH, TEST_PATH + "_", sub1(text.size()));
    BOOST_REQUIRE(instance.start());
    BOOST_REQUIRE(!instance.write(text));
    BOOST_REQUIRE(instance.stop());
    BOOST_REQUIRE(test::exists(TEST_PATH));
    BOOST_REQUIRE_EQUAL(test::size(TEST_PATH), zero);
    BOOST_REQUIRE(!test::exists(TEST_PATH + "_"));
}

BOOST_AUTO_TEST_CASE(rotator__write__limit_sub1__true_expected)
{
    const std::string text = "panopticon";
    file::rotator instance(TEST_PATH, TEST_PATH + "_", add1(text.size()));
    BOOST_REQUIRE(instance.start());
    BOOST_REQUIRE(instance.write(text));
    BOOST_REQUIRE(instance.stop());
    BOOST_REQUIRE_EQUAL(test::size(TEST_PATH), text.size());
    BOOST_REQUIRE(!test::exists(TEST_PATH + "_"));
}

BOOST_AUTO_TEST_CASE(rotator__write__limit_sub1_twice__true_expected)
{
    const std::string text = "panopticon";
    file::rotator instance(TEST_PATH, TEST_PATH + "_", add1(text.size()));
    BOOST_REQUIRE(instance.start());
    BOOST_REQUIRE(instance.write(text));
    BOOST_REQUIRE(instance.write(text));
    BOOST_REQUIRE(instance.stop());
    BOOST_REQUIRE_EQUAL(test::size(TEST_PATH), text.size());
    BOOST_REQUIRE_EQUAL(test::size(TEST_PATH + "_"), text.size());
}

BOOST_AUTO_TEST_CASE(rotator__write__limit_sub1_thrice__true_expected)
{
    const std::string text = "panopticon";
    file::rotator instance(TEST_PATH, TEST_PATH + "_", add1(text.size()));
    BOOST_REQUIRE(instance.start());
    BOOST_REQUIRE(instance.write(text));
    BOOST_REQUIRE(instance.write(text));
    BOOST_REQUIRE(instance.write(text));
    BOOST_REQUIRE(instance.stop());
    BOOST_REQUIRE_EQUAL(test::size(TEST_PATH), text.size());
    BOOST_REQUIRE_EQUAL(test::size(TEST_PATH + "_"), text.size());
    BOOST_REQUIRE_EQUAL(test::read_line(TEST_PATH), text);
    BOOST_REQUIRE_EQUAL(test::read_line(TEST_PATH + "_"), text);
}

BOOST_AUTO_TEST_CASE(rotator__write__parts__rotated_and_queued)
{
    file::rotator instance(TEST_PATH, TEST_PATH + "_", 8);
    BOOST_REQUIRE(instance.start());

    // file
    BOOST_REQUIRE(instance.write("abc"));
    BOOST_REQUIRE(instance.write("def"));

    // file
    BOOST_REQUIRE(instance.write("ghi"));
    BOOST_REQUIRE(instance.write("jkl"));

    // file
    BOOST_REQUIRE(instance.write("mno"));
    BOOST_REQUIRE(instance.write("pqr"));

    // Last secondary file.
    BOOST_REQUIRE(instance.write("stu\n"));
    BOOST_REQUIRE(instance.write("vwx"));

    // Last primary file.
    BOOST_REQUIRE(instance.write("yz\n"));
    BOOST_REQUIRE(instance.stop());

    // Binary mode on Windows ensures that \n nor replaced with \r\n.
    BOOST_REQUIRE_EQUAL(test::size(TEST_PATH), 3);
    BOOST_REQUIRE_EQUAL(test::read_line(TEST_PATH), "yz");

    // Binary mode on Windows ensures that \n nor replaced with \r\n.
    BOOST_REQUIRE_EQUAL(test::size(TEST_PATH + "_"), 7);
    BOOST_REQUIRE_EQUAL(test::read_line(TEST_PATH + "_", 0), "stu");
    BOOST_REQUIRE_EQUAL(test::read_line(TEST_PATH + "_", 1), "vwx");
}

BOOST_AUTO_TEST_CASE(rotator__write__existing__appends)
{
    const std::string text = "panopticon";
    BOOST_REQUIRE(test::create(TEST_PATH, text));

    file::rotator instance(TEST_PATH, TEST_PATH + "_", 42);
    BOOST_REQUIRE(instance.start());
    BOOST_REQUIRE(instance.write("\n"));
    BOOST_REQUIRE(instance.write("abc"));
    BOOST_REQUIRE(instance.stop());
    BOOST_REQUIRE_EQUAL(test::read_line(TEST_PATH, 0), "panopticon");
    BOOST_REQUIRE_EQUAL(test::read_line(TEST_PATH, 1), "abc");
}

BOOST_AUTO_TEST_CASE(rotator__flush__existing__expected)
{
    const std::string text = "panopticon";
    BOOST_REQUIRE(test::create(TEST_PATH, text));

    file::rotator instance(TEST_PATH, TEST_PATH + "_", 42);
    BOOST_REQUIRE(instance.start());
    BOOST_REQUIRE(instance.write("\n"));
    BOOST_REQUIRE(instance.write("abc"));
    BOOST_REQUIRE(instance.flush());
    BOOST_REQUIRE_EQUAL(test::read_line(TEST_PATH, 0), "panopticon");
    BOOST_REQUIRE_EQUAL(test::read_line(TEST_PATH, 1), "abc");
}

BOOST_AUTO_TEST_SUITE_END()
