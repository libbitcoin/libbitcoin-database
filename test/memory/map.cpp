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
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "../test.hpp"

// TODO: need to fake map_(), unmap_() and flush_() in order to hit
// error::load_failure, error::flush_failure, error::unload_failure codes, but
// don't want to make this class virtual.

struct map_setup_fixture
{
    DELETE_COPY_MOVE(map_setup_fixture);
    BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

    map_setup_fixture() NOEXCEPT
    {
        BOOST_REQUIRE(test::clear(test::directory));
    }

    ~map_setup_fixture() NOEXCEPT
    {
        BOOST_REQUIRE(test::clear(test::directory));
    }

    BC_POP_WARNING()
};

BOOST_FIXTURE_TEST_SUITE(map_tests, map_setup_fixture)

BOOST_AUTO_TEST_CASE(map__file__always__expected)
{
    const std::string file = TEST_PATH;
    map instance(file);
    BOOST_REQUIRE_EQUAL(instance.file(), file);
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(map__open__opened__false)
{
    const std::string file = TEST_PATH;
    BOOST_REQUIRE(test::create(file));

    map instance(file);
    BOOST_REQUIRE(test::exists(file));
    BOOST_REQUIRE(!instance.open());
    BOOST_REQUIRE_EQUAL(instance.open(), error::open_open);
    BOOST_REQUIRE(!instance.close());
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(map__open__no_file__false)
{
    const std::string file = TEST_PATH;
    map instance(file);
    BOOST_REQUIRE(!test::exists(file));
    BOOST_REQUIRE(instance.open());
    BOOST_REQUIRE(!instance.close());
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(map__close__open__true)
{
    const std::string file = TEST_PATH;
    BOOST_REQUIRE(test::create(file));
    map instance(file);
    BOOST_REQUIRE(!instance.open());
    BOOST_REQUIRE(instance.is_open());
    BOOST_REQUIRE(!instance.close());
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(map__close__closed__true)
{
    const std::string file = TEST_PATH;
    BOOST_REQUIRE(test::create(file));
    map instance(file);
    BOOST_REQUIRE(!instance.is_open());
    BOOST_REQUIRE(!instance.close());
}

BOOST_AUTO_TEST_CASE(map__close__loaded__false)
{
    const std::string file = TEST_PATH;
    BOOST_REQUIRE(test::create(file));
    map instance(file);
    BOOST_REQUIRE(!instance.open());
    BOOST_REQUIRE(!instance.load());
    BOOST_REQUIRE_EQUAL(instance.close(), error::close_loaded);
    BOOST_REQUIRE(!instance.unload());
    BOOST_REQUIRE(!instance.close());
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(map__properties__open_close__expected)
{
    const std::string file = TEST_PATH;
    BOOST_REQUIRE(test::create(file));

    map instance(file);
    BOOST_REQUIRE(!instance.is_open());
    BOOST_REQUIRE(!instance.is_loaded());
    BOOST_REQUIRE_EQUAL(instance.size(), 0u);
    BOOST_REQUIRE_EQUAL(instance.capacity(), 0u);

    BOOST_REQUIRE(!instance.open());
    BOOST_REQUIRE(instance.is_open());
    BOOST_REQUIRE(!instance.is_loaded());
    BOOST_REQUIRE_EQUAL(instance.size(), 0u);
    BOOST_REQUIRE_EQUAL(instance.capacity(), 0u);

    BOOST_REQUIRE(!instance.close());
    BOOST_REQUIRE(!instance.is_open());
    BOOST_REQUIRE(!instance.is_loaded());
    BOOST_REQUIRE_EQUAL(instance.size(), 0u);
    BOOST_REQUIRE_EQUAL(instance.capacity(), 0u);
    BOOST_REQUIRE(!instance.get_fault());

    BOOST_REQUIRE(test::exists(file));
}

BOOST_AUTO_TEST_CASE(map__properties__load_unload__expected)
{
    constexpr auto default_minimum_capacity = 1u;
    const std::string file = TEST_PATH;
    BOOST_REQUIRE(test::create(file));

    map instance(file);
    BOOST_REQUIRE(!instance.open());
    BOOST_REQUIRE(!instance.is_loaded());
    BOOST_REQUIRE_EQUAL(instance.size(), 0u);
    BOOST_REQUIRE_EQUAL(instance.capacity(), 0u);

    BOOST_REQUIRE(!instance.load());
    BOOST_REQUIRE(instance.is_loaded());
    BOOST_REQUIRE_EQUAL(instance.size(), 0u);
    BOOST_REQUIRE_EQUAL(instance.capacity(), default_minimum_capacity);

    BOOST_REQUIRE(!instance.unload());
    BOOST_REQUIRE(!instance.is_loaded());
    BOOST_REQUIRE_EQUAL(instance.size(), 0u);
    BOOST_REQUIRE_EQUAL(instance.capacity(), 0u);

    BOOST_REQUIRE(!instance.close());
    BOOST_REQUIRE(!instance.is_open());
    BOOST_REQUIRE(!instance.is_loaded());
    BOOST_REQUIRE_EQUAL(instance.size(), 0u);
    BOOST_REQUIRE_EQUAL(instance.capacity(), 0u);
    BOOST_REQUIRE(!instance.get_fault());

    BOOST_REQUIRE(test::exists(file));
}

BOOST_AUTO_TEST_CASE(map__load__unloaded__true)
{
    const std::string file = TEST_PATH;
    BOOST_REQUIRE(test::create(file));
    map instance(file);
    BOOST_REQUIRE(!instance.open());
    BOOST_REQUIRE(!instance.load());
    BOOST_REQUIRE(!instance.unload());
    BOOST_REQUIRE(!instance.close());
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(map__load__shared__false)
{
    const std::string file = TEST_PATH;
    BOOST_REQUIRE(test::create(file));
    map instance(file);
    BOOST_REQUIRE(!instance.open());
    BOOST_REQUIRE(!instance.load());
    auto memory = instance.get(instance.allocate(1));
    BOOST_REQUIRE(memory);
    BOOST_REQUIRE_EQUAL(instance.load(), error::load_locked);
    memory.reset();
    BOOST_REQUIRE(!instance.unload());
    BOOST_REQUIRE(!instance.close());
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(map__load__loaded__false)
{
    const std::string file = TEST_PATH;
    BOOST_REQUIRE(test::create(file));
    map instance(file);
    BOOST_REQUIRE(!instance.open());
    BOOST_REQUIRE(!instance.load());
    BOOST_REQUIRE_EQUAL(instance.load(), error::load_loaded);
    BOOST_REQUIRE(!instance.unload());
    BOOST_REQUIRE(!instance.close());
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(map__unload__unloaded__true)
{
    const std::string file = TEST_PATH;
    BOOST_REQUIRE(test::create(file));
    map instance(file);
    BOOST_REQUIRE(!instance.open());
    BOOST_REQUIRE(!instance.unload());
    BOOST_REQUIRE(!instance.close());
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(map__unload__loaded__true)
{
    const std::string file = TEST_PATH;
    BOOST_REQUIRE(test::create(file));
    map instance(file);
    BOOST_REQUIRE(!instance.open());
    BOOST_REQUIRE(!instance.load());
    BOOST_REQUIRE(!instance.unload());
    BOOST_REQUIRE(!instance.close());
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(map__capacity__default__expected)
{

    constexpr auto default_minimum_capacity = 1u;
    const std::string file = TEST_PATH;
    BOOST_REQUIRE(test::create(file));
    map instance(file);
    BOOST_REQUIRE(!instance.open());
    BOOST_REQUIRE_EQUAL(instance.capacity(), zero);
    BOOST_REQUIRE(!instance.load());
    BOOST_REQUIRE_EQUAL(instance.capacity(), default_minimum_capacity);
    BOOST_REQUIRE(!instance.unload());
    BOOST_REQUIRE(!instance.close());
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(map__truncate__unloaded__false)
{
    const std::string file = TEST_PATH;
    BOOST_REQUIRE(test::create(file));
    map instance(file);
    BOOST_REQUIRE(!instance.open());
    BOOST_REQUIRE(!instance.truncate(42));
    BOOST_REQUIRE(!instance.close());
    BOOST_REQUIRE(!instance.get_fault());
}

// Truncate is no longer capacty based.
////BOOST_AUTO_TEST_CASE(map__truncate__loaded__expected)
////{
////    constexpr auto size = 42u;
////    const std::string file = TEST_PATH;
////    BOOST_REQUIRE(test::create(file));
////    map instance(file, 1, 50);
////    BOOST_REQUIRE(!instance.open());
////    BOOST_REQUIRE(!instance.load());
////    BOOST_REQUIRE_EQUAL(instance.allocate(size), zero);
////    constexpr auto capacity = size + to_half(size);
////    BOOST_REQUIRE_EQUAL(instance.capacity(), capacity);
////    BOOST_REQUIRE(instance.truncate(to_half(size)));
////    BOOST_REQUIRE_EQUAL(instance.size(), to_half(size));
////    BOOST_REQUIRE(!instance.unload());
////    BOOST_REQUIRE(!instance.close());
////    BOOST_REQUIRE(!instance.get_fault());
////}

BOOST_AUTO_TEST_CASE(map__allocate__unloaded__false)
{
    const std::string file = TEST_PATH;
    BOOST_REQUIRE(test::create(file));
    map instance(file);
    BOOST_REQUIRE(!instance.open());
    BOOST_REQUIRE_EQUAL(instance.allocate(42), map::eof);
    BOOST_REQUIRE(!instance.close());
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(map__allocate__loaded__expected_capacity)
{
    constexpr auto size = 100u;
    const std::string file = TEST_PATH;
    BOOST_REQUIRE(test::create(file));
    map instance(file, 1, 50);
    BOOST_REQUIRE(!instance.open());
    BOOST_REQUIRE(!instance.load());
    BOOST_REQUIRE_EQUAL(instance.allocate(size), zero);
    constexpr auto capacity = size + to_half(size);
    BOOST_REQUIRE_EQUAL(instance.capacity(), capacity);
    BOOST_REQUIRE(!instance.unload());
    BOOST_REQUIRE(!instance.close());
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(map__allocate__add_overflow__eof)
{
    const std::string file = TEST_PATH;
    BOOST_REQUIRE(test::create(file));
    map instance(file);
    BOOST_REQUIRE(!instance.open());
    BOOST_REQUIRE(!instance.load());
    BOOST_REQUIRE_EQUAL(instance.allocate(100), zero);
    BOOST_REQUIRE_EQUAL(instance.allocate(max_size_t), storage::eof);
    BOOST_REQUIRE(!instance.unload());
    BOOST_REQUIRE(!instance.close());
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(map__allocate__minimum_no_expansion__expected_capacity)
{
    constexpr auto size = 42u;
    constexpr auto minimum = 100u;
    const std::string file = TEST_PATH;
    BOOST_REQUIRE(test::create(file));
    map instance(file, minimum, 0);
    BOOST_REQUIRE(!instance.open());
    BOOST_REQUIRE(!instance.load());
    BOOST_REQUIRE_EQUAL(instance.allocate(size), zero);
    constexpr auto capacity = std::max(minimum, size);
    BOOST_REQUIRE_EQUAL(instance.capacity(), capacity);
    BOOST_REQUIRE(!instance.unload());
    BOOST_REQUIRE(!instance.close());
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(map__allocate__no_minimum_expansion__expected_capacity)
{
    // map will fail if minimum is zero.
    constexpr auto minimum = 1u;
    constexpr auto rate = 42u;
    constexpr auto size = 100u;

    const std::string file = TEST_PATH;
    BOOST_REQUIRE(test::create(file));

    map instance(file, minimum, rate);
    BOOST_REQUIRE(!instance.open());
    BOOST_REQUIRE(!instance.load());
    BOOST_REQUIRE_EQUAL(instance.allocate(size), zero);

    // These add only because size is 100.
    constexpr auto capacity = size + rate;
    BOOST_REQUIRE_EQUAL(instance.capacity(), capacity);
    BOOST_REQUIRE(!instance.unload());
    BOOST_REQUIRE(!instance.close());
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(map__get__unloaded__false)
{
    const std::string file = TEST_PATH;
    BOOST_REQUIRE(test::create(file));
    map instance(file);
    BOOST_REQUIRE(!instance.open());
    BOOST_REQUIRE(!instance.get());
    BOOST_REQUIRE(!instance.close());
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(map__get__loaded__success)
{
    const std::string file = TEST_PATH;
    BOOST_REQUIRE(test::create(file));
    map instance(file);
    BOOST_REQUIRE(!instance.open());
    BOOST_REQUIRE(!instance.load());
    BOOST_REQUIRE(instance.get(instance.allocate(1)));
    BOOST_REQUIRE(!instance.unload());
    BOOST_REQUIRE(!instance.close());
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(map__flush__unloaded__false)
{
    const std::string file = TEST_PATH;
    BOOST_REQUIRE(test::create(file));
    map instance(file);
    BOOST_REQUIRE(!instance.open());
    BOOST_REQUIRE_EQUAL(instance.flush(), error::flush_unloaded);
    BOOST_REQUIRE(!instance.close());
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(map__flush__loaded__true)
{
    const std::string file = TEST_PATH;
    BOOST_REQUIRE(test::create(file));
    map instance(file);
    BOOST_REQUIRE(!instance.open());
    BOOST_REQUIRE(!instance.load());
    BOOST_REQUIRE(!instance.flush());
    BOOST_REQUIRE(!instance.unload());
    BOOST_REQUIRE(!instance.close());
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(map__write__read__expected)
{
    constexpr uint64_t expected = 0x0102030405060708_u64;
    const std::string file = TEST_PATH;
    BOOST_REQUIRE(test::create(file));
    map instance(file);
    BOOST_REQUIRE(!instance.open());
    BOOST_REQUIRE(!instance.load());
    auto memory = instance.get(instance.allocate(sizeof(uint64_t)));
    BOOST_REQUIRE(memory);
    system::unsafe_to_little_endian<uint64_t>(memory->begin(), expected);
    memory.reset();
    BOOST_REQUIRE(!instance.flush());
    memory = instance.get();
    BOOST_REQUIRE(memory);
    BOOST_REQUIRE_EQUAL(system::unsafe_from_little_endian<uint64_t>(memory->begin()), expected);
    memory.reset();
    BOOST_REQUIRE(!instance.unload());
    BOOST_REQUIRE(!instance.close());
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(map__unload__shared__false)
{
    const std::string file = TEST_PATH;
    BOOST_REQUIRE(test::create(file));
    map instance(file);
    BOOST_REQUIRE(!instance.open());
    BOOST_REQUIRE(!instance.load());
    auto memory = instance.get(instance.allocate(1));
    BOOST_REQUIRE(memory);
    BOOST_REQUIRE_EQUAL(instance.unload(), error::unload_locked);
    memory.reset();
    BOOST_REQUIRE(!instance.unload());
    BOOST_REQUIRE(!instance.close());
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_SUITE_END()
