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
#include "../utility/utility.hpp"


struct file_storage_setup_fixture
{
    DELETE4(file_storage_setup_fixture);
    BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

    file_storage_setup_fixture() NOEXCEPT
    {
        BOOST_REQUIRE(test::clear(test::directory));
    }

    ~file_storage_setup_fixture() NOEXCEPT
    {
        BOOST_REQUIRE(test::clear(test::directory));
    }

    BC_POP_WARNING()
};

BOOST_FIXTURE_TEST_SUITE(file_storage_tests, file_storage_setup_fixture)

BOOST_AUTO_TEST_CASE(file_storage__constructor1__always__leaves_file)
{
    const std::string file = TEST_PATH;
    BOOST_REQUIRE(test::create(file));
    file_storage instance(file);
    BOOST_REQUIRE(instance.close());
    BOOST_REQUIRE(test::exists(file));
}

BOOST_AUTO_TEST_CASE(file_storage__constructor2__always__leaves_file)
{
    const std::string file = TEST_PATH;
    BOOST_REQUIRE(test::create(file));
    file_storage instance(file, 42, 42);
    BOOST_REQUIRE(instance.close());
    BOOST_REQUIRE(test::exists(file));
}

BOOST_AUTO_TEST_CASE(file_storage__load_map__unmapped__true)
{
    const std::string file = TEST_PATH;
    BOOST_REQUIRE(test::create(file));
    file_storage instance(file);
    BOOST_REQUIRE(instance.load_map());
    BOOST_REQUIRE(instance.unload_map());
    BOOST_REQUIRE(instance.close());
}

BOOST_AUTO_TEST_CASE(file_storage__load_map__mapped__false)
{
    const std::string file = TEST_PATH;
    BOOST_REQUIRE(test::create(file));
    file_storage instance(file);
    BOOST_REQUIRE(instance.load_map());
    BOOST_REQUIRE(!instance.load_map());
    BOOST_REQUIRE(instance.unload_map());
    BOOST_REQUIRE(instance.close());
}

BOOST_AUTO_TEST_CASE(file_storage__unload_map__unmapped__false)
{
    const std::string file = TEST_PATH;
    BOOST_REQUIRE(test::create(file));
    file_storage instance(file);
    BOOST_REQUIRE(!instance.unload_map());
    BOOST_REQUIRE(instance.close());
}

BOOST_AUTO_TEST_CASE(file_storage__unload_map__mapped__true)
{
    const std::string file = TEST_PATH;
    BOOST_REQUIRE(test::create(file));
    file_storage instance(file);
    BOOST_REQUIRE(instance.load_map());
    BOOST_REQUIRE(instance.unload_map());
    BOOST_REQUIRE(instance.close());
}

BOOST_AUTO_TEST_CASE(file_storage__capacity__default__expected)
{
    const std::string file = TEST_PATH;
    BOOST_REQUIRE(test::create(file));
    file_storage instance(file);
    BOOST_REQUIRE_EQUAL(instance.capacity(), zero);
    BOOST_REQUIRE(instance.load_map());
    BOOST_REQUIRE_EQUAL(instance.capacity(), 1u);
    BOOST_REQUIRE(instance.unload_map());
    BOOST_REQUIRE(instance.close());
}

BOOST_AUTO_TEST_CASE(file_storage__resize__unmapped__false)
{
    const std::string file = TEST_PATH;
    BOOST_REQUIRE(test::create(file));
    file_storage instance(file);
    BOOST_REQUIRE(!instance.resize(42));
    BOOST_REQUIRE(instance.close());
}

// TODO: externally verify open/closed file size 42.
BOOST_AUTO_TEST_CASE(file_storage__resize__mapped__expected)
{
    const std::string file = TEST_PATH;
    BOOST_REQUIRE(test::create(file));
    file_storage instance(file);
    BOOST_REQUIRE(instance.load_map());
    BOOST_REQUIRE(instance.resize(42));
    BOOST_REQUIRE_EQUAL(instance.capacity(), 42u);
    BOOST_REQUIRE(instance.unload_map());
    BOOST_REQUIRE(instance.close());
}

BOOST_AUTO_TEST_CASE(file_storage__reserve__unmapped__false)
{
    const std::string file = TEST_PATH;
    BOOST_REQUIRE(test::create(file));
    file_storage instance(file);
    BOOST_REQUIRE(!instance.reserve(42));
    BOOST_REQUIRE(instance.close());
}

// TODO: externally verify open file size 150, closed file size 100.
BOOST_AUTO_TEST_CASE(file_storage__reserve__mapped__expected_capacity)
{
    const std::string file = TEST_PATH;
    BOOST_REQUIRE(test::create(file));
    file_storage instance(file);
    BOOST_REQUIRE(instance.load_map());
    BOOST_REQUIRE(instance.reserve(100));
    BOOST_REQUIRE_EQUAL(instance.capacity(), 150u);
    BOOST_REQUIRE(instance.unload_map());
    BOOST_REQUIRE(instance.close());
}

// TODO: externally verify open file size 100, closed file size 42.
BOOST_AUTO_TEST_CASE(file_storage__reserve__minimum_no_expansion__expected_capacity)
{
    const std::string file = TEST_PATH;
    BOOST_REQUIRE(test::create(file));
    file_storage instance(file, 100, 0);
    BOOST_REQUIRE(instance.load_map());
    BOOST_REQUIRE(instance.reserve(42));
    BOOST_REQUIRE_EQUAL(instance.capacity(), 100u);
    BOOST_REQUIRE(instance.unload_map());
    BOOST_REQUIRE(instance.close());
}

// TODO: externally verify open file size 142, closed file size 100.
BOOST_AUTO_TEST_CASE(file_storage__reserve__no_minimum_expansion__expected_capacity)
{
    const std::string file = TEST_PATH;
    BOOST_REQUIRE(test::create(file));

    // map will fail if minimum is zero.
    file_storage instance(file, 1, 42);
    BOOST_REQUIRE(instance.load_map());
    BOOST_REQUIRE(instance.reserve(100));
    BOOST_REQUIRE_EQUAL(instance.capacity(), 142u);
    BOOST_REQUIRE(instance.unload_map());
    BOOST_REQUIRE(instance.close());
}

BOOST_AUTO_TEST_CASE(file_storage__get__unmapped__false)
{
    const std::string file = TEST_PATH;
    BOOST_REQUIRE(test::create(file));
    file_storage instance(file);
    BOOST_REQUIRE(!instance.get());
    BOOST_REQUIRE(instance.close());
}

// TODO: externally verify file size.
BOOST_AUTO_TEST_CASE(file_storage__get__mapped__success)
{
    const std::string file = TEST_PATH;
    BOOST_REQUIRE(test::create(file));
    file_storage instance(file);
    BOOST_REQUIRE(instance.load_map());
    BOOST_REQUIRE(instance.get());
    BOOST_REQUIRE(instance.unload_map());
    BOOST_REQUIRE(instance.close());
}

BOOST_AUTO_TEST_CASE(file_storage__flush_map__unmapped__false)
{
    const std::string file = TEST_PATH;
    BOOST_REQUIRE(test::create(file));
    file_storage instance(file);
    BOOST_REQUIRE(!instance.flush_map());
    BOOST_REQUIRE(instance.close());
}

BOOST_AUTO_TEST_CASE(file_storage__flush_map__mapped__true)
{
    const std::string file = TEST_PATH;
    BOOST_REQUIRE(test::create(file));
    file_storage instance(file);
    BOOST_REQUIRE(instance.load_map());
    BOOST_REQUIRE(instance.flush_map());
    BOOST_REQUIRE(instance.unload_map());
    BOOST_REQUIRE(instance.close());
}

BOOST_AUTO_TEST_CASE(file_storage__write__read__expected)
{
    constexpr uint64_t expected = 0x0102030405060708_u64;
    const std::string file = TEST_PATH;
    BOOST_REQUIRE(test::create(file));
    file_storage instance(file);
    BOOST_REQUIRE(instance.load_map());
    auto memory = instance.reserve(sizeof(uint64_t));
    BOOST_REQUIRE(memory);
    unsafe_to_little_endian<uint64_t>(memory->buffer(), expected);
    memory.reset();
    BOOST_REQUIRE(instance.flush_map());
    memory = instance.get();
    BOOST_REQUIRE(memory);
    BOOST_REQUIRE_EQUAL(unsafe_from_little_endian<uint64_t>(memory->buffer()), expected);
    memory.reset();
    BOOST_REQUIRE(instance.unload_map());
    BOOST_REQUIRE(instance.close());
}

BOOST_AUTO_TEST_CASE(file_storage__unload_map__pending_accessor__false)
{
    const std::string file = TEST_PATH;
    BOOST_REQUIRE(test::create(file));
    file_storage instance(file);
    BOOST_REQUIRE(instance.load_map());
    auto memory = instance.get();
    BOOST_REQUIRE(memory);
    BOOST_REQUIRE(!instance.unload_map());
    memory.reset();
    BOOST_REQUIRE(instance.unload_map());
    BOOST_REQUIRE(instance.close());
}

BOOST_AUTO_TEST_CASE(file_storage__close__pending_map__false)
{
    const std::string file = TEST_PATH;
    BOOST_REQUIRE(test::create(file));
    file_storage instance(file);
    BOOST_REQUIRE(instance.load_map());
    auto memory = instance.get();
    BOOST_REQUIRE(memory);
    BOOST_REQUIRE(!instance.close());
    memory.reset();
    BOOST_REQUIRE(!instance.close());
    BOOST_REQUIRE(instance.unload_map());
    BOOST_REQUIRE(instance.close());
}

BOOST_AUTO_TEST_SUITE_END()
