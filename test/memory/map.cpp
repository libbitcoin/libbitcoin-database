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

struct map_setup_fixture
{
    DELETE4(map_setup_fixture);
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

constexpr auto default_minimum_capacity = 1u;

BOOST_AUTO_TEST_CASE(map__open__no_file__false)
{
    const std::string file = TEST_PATH;
    map instance(file);
    BOOST_REQUIRE(!test::exists(file));
    BOOST_REQUIRE(!instance.open());
    BOOST_REQUIRE(instance.close());
}

BOOST_AUTO_TEST_CASE(map__properties__open_close__expected)
{
    const std::string file = TEST_PATH;
    BOOST_REQUIRE(test::create(file));

    map instance(file);
    BOOST_REQUIRE(!instance.is_open());
    BOOST_REQUIRE(!instance.is_mapped());
    BOOST_REQUIRE_EQUAL(instance.size(), 0u);
    BOOST_REQUIRE_EQUAL(instance.capacity(), 0u);

    BOOST_REQUIRE(instance.open());
    BOOST_REQUIRE(instance.is_open());
    BOOST_REQUIRE(!instance.is_mapped());
    BOOST_REQUIRE_EQUAL(instance.size(), 0u);
    BOOST_REQUIRE_EQUAL(instance.capacity(), 0u);

    BOOST_REQUIRE(instance.close());
    BOOST_REQUIRE(!instance.is_open());
    BOOST_REQUIRE(!instance.is_mapped());
    BOOST_REQUIRE_EQUAL(instance.size(), 0u);
    BOOST_REQUIRE_EQUAL(instance.capacity(), 0u);

    BOOST_REQUIRE(test::exists(file));
}

BOOST_AUTO_TEST_CASE(map__properties__map_unmap__expected)
{
    const std::string file = TEST_PATH;
    BOOST_REQUIRE(test::create(file));

    map instance(file);
    BOOST_REQUIRE(instance.open());
    BOOST_REQUIRE(!instance.is_mapped());
    BOOST_REQUIRE_EQUAL(instance.size(), 0u);
    BOOST_REQUIRE_EQUAL(instance.capacity(), 0u);

    BOOST_REQUIRE(instance.load());
    BOOST_REQUIRE(instance.is_mapped());
    BOOST_REQUIRE_EQUAL(instance.size(), 0u);
    BOOST_REQUIRE_EQUAL(instance.capacity(), default_minimum_capacity);

    BOOST_REQUIRE(instance.unload());
    BOOST_REQUIRE(!instance.is_mapped());
    BOOST_REQUIRE_EQUAL(instance.size(), 0u);
    BOOST_REQUIRE_EQUAL(instance.capacity(), 0u);

    BOOST_REQUIRE(instance.close());
    BOOST_REQUIRE(!instance.is_open());
    BOOST_REQUIRE(!instance.is_mapped());
    BOOST_REQUIRE_EQUAL(instance.size(), 0u);
    BOOST_REQUIRE_EQUAL(instance.capacity(), 0u);

    BOOST_REQUIRE(test::exists(file));
}


BOOST_AUTO_TEST_CASE(map__load_map__unmapped__true)
{
    const std::string file = TEST_PATH;
    BOOST_REQUIRE(test::create(file));
    map instance(file);
    BOOST_REQUIRE(instance.open());
    BOOST_REQUIRE(instance.load());
    BOOST_REQUIRE(instance.unload());
    BOOST_REQUIRE(instance.close());
}

BOOST_AUTO_TEST_CASE(map__load_map__mapped__false)
{
    const std::string file = TEST_PATH;
    BOOST_REQUIRE(test::create(file));
    map instance(file);
    BOOST_REQUIRE(instance.open());
    BOOST_REQUIRE(instance.load());
    BOOST_REQUIRE(!instance.load());
    BOOST_REQUIRE(instance.unload());
    BOOST_REQUIRE(instance.close());
}

BOOST_AUTO_TEST_CASE(map__unload_map__unmapped__true)
{
    const std::string file = TEST_PATH;
    BOOST_REQUIRE(test::create(file));
    map instance(file);
    BOOST_REQUIRE(instance.open());
    BOOST_REQUIRE(instance.unload());
    BOOST_REQUIRE(instance.close());
}

BOOST_AUTO_TEST_CASE(map__unload_map__mapped__true)
{
    const std::string file = TEST_PATH;
    BOOST_REQUIRE(test::create(file));
    map instance(file);
    BOOST_REQUIRE(instance.open());
    BOOST_REQUIRE(instance.load());
    BOOST_REQUIRE(instance.unload());
    BOOST_REQUIRE(instance.close());
}

BOOST_AUTO_TEST_CASE(map__capacity__default__expected)
{
    const std::string file = TEST_PATH;
    BOOST_REQUIRE(test::create(file));
    map instance(file);
    BOOST_REQUIRE(instance.open());
    BOOST_REQUIRE_EQUAL(instance.capacity(), zero);
    BOOST_REQUIRE(instance.load());
    BOOST_REQUIRE_EQUAL(instance.capacity(), default_minimum_capacity);
    BOOST_REQUIRE(instance.unload());
    BOOST_REQUIRE(instance.close());
}

BOOST_AUTO_TEST_CASE(map__resize__unmapped__false)
{
    const std::string file = TEST_PATH;
    BOOST_REQUIRE(test::create(file));
    map instance(file);
    BOOST_REQUIRE(instance.open());
    BOOST_REQUIRE(!instance.resize(42));
    BOOST_REQUIRE(instance.close());
}

BOOST_AUTO_TEST_CASE(map__resize__mapped__expected)
{
    constexpr auto size = 42u;
    const std::string file = TEST_PATH;
    BOOST_REQUIRE(test::create(file));
    map instance(file, 1, 50);
    BOOST_REQUIRE(instance.open());
    BOOST_REQUIRE(instance.load());
    BOOST_REQUIRE_EQUAL(instance.allocate(size), zero);
    constexpr auto capacity = size + to_half(size);
    BOOST_REQUIRE_EQUAL(instance.capacity(), capacity);
    BOOST_REQUIRE(instance.resize(to_half(size)));
    BOOST_REQUIRE_EQUAL(instance.size(), to_half(size));
    BOOST_REQUIRE(instance.unload());
    BOOST_REQUIRE(instance.close());
}

BOOST_AUTO_TEST_CASE(map__allocate__unmapped__false)
{
    const std::string file = TEST_PATH;
    BOOST_REQUIRE(test::create(file));
    map instance(file);
    BOOST_REQUIRE(instance.open());
    BOOST_REQUIRE_EQUAL(instance.allocate(42), map::eof);
    BOOST_REQUIRE(instance.close());
}

BOOST_AUTO_TEST_CASE(map__allocate__mapped__expected_capacity)
{
    constexpr auto size = 100u;
    const std::string file = TEST_PATH;
    BOOST_REQUIRE(test::create(file));
    map instance(file, 1, 50);
    BOOST_REQUIRE(instance.open());
    BOOST_REQUIRE(instance.load());
    BOOST_REQUIRE_EQUAL(instance.allocate(size), zero);
    constexpr auto capacity = size + to_half(size);
    BOOST_REQUIRE_EQUAL(instance.capacity(), capacity);
    BOOST_REQUIRE(instance.unload());
    BOOST_REQUIRE(instance.close());
}

BOOST_AUTO_TEST_CASE(map__allocate__add_overflow__eof)
{
    const std::string file = TEST_PATH;
    BOOST_REQUIRE(test::create(file));
    map instance(file);
    BOOST_REQUIRE(instance.open());
    BOOST_REQUIRE(instance.load());
    BOOST_REQUIRE_EQUAL(instance.allocate(100), zero);
    BOOST_REQUIRE_EQUAL(instance.allocate(max_size_t), storage::eof);
    BOOST_REQUIRE(instance.unload());
    BOOST_REQUIRE(instance.close());
}

BOOST_AUTO_TEST_CASE(map__allocate__minimum_no_expansion__expected_capacity)
{
    constexpr auto size = 42u;
    constexpr auto minimum = 100u;
    const std::string file = TEST_PATH;
    BOOST_REQUIRE(test::create(file));
    map instance(file, minimum, 0);
    BOOST_REQUIRE(instance.open());
    BOOST_REQUIRE(instance.load());
    BOOST_REQUIRE_EQUAL(instance.allocate(size), zero);
    constexpr auto capacity = std::max(minimum, size);
    BOOST_REQUIRE_EQUAL(instance.capacity(), capacity);
    BOOST_REQUIRE(instance.unload());
    BOOST_REQUIRE(instance.close());
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
    BOOST_REQUIRE(instance.open());
    BOOST_REQUIRE(instance.load());
    BOOST_REQUIRE_EQUAL(instance.allocate(size), zero);

    // These add only because size is 100.
    constexpr auto capacity = size + rate;
    BOOST_REQUIRE_EQUAL(instance.capacity(), capacity);
    BOOST_REQUIRE(instance.unload());
    BOOST_REQUIRE(instance.close());
}

BOOST_AUTO_TEST_CASE(map__get__unmapped__false)
{
    const std::string file = TEST_PATH;
    BOOST_REQUIRE(test::create(file));
    map instance(file);
    BOOST_REQUIRE(instance.open());
    BOOST_REQUIRE(!instance.get());
    BOOST_REQUIRE(instance.close());
}

BOOST_AUTO_TEST_CASE(map__get__mapped__success)
{
    const std::string file = TEST_PATH;
    BOOST_REQUIRE(test::create(file));
    map instance(file);
    BOOST_REQUIRE(instance.open());
    BOOST_REQUIRE(instance.load());
    BOOST_REQUIRE(instance.get(instance.allocate(1)));
    BOOST_REQUIRE(instance.unload());
    BOOST_REQUIRE(instance.close());
}

BOOST_AUTO_TEST_CASE(map__flush_map__unmapped__false)
{
    const std::string file = TEST_PATH;
    BOOST_REQUIRE(test::create(file));
    map instance(file);
    BOOST_REQUIRE(instance.open());
    BOOST_REQUIRE(!instance.flush());
    BOOST_REQUIRE(instance.close());
}

BOOST_AUTO_TEST_CASE(map__flush_map__mapped__true)
{
    const std::string file = TEST_PATH;
    BOOST_REQUIRE(test::create(file));
    map instance(file);
    BOOST_REQUIRE(instance.open());
    BOOST_REQUIRE(instance.load());
    BOOST_REQUIRE(instance.flush());
    BOOST_REQUIRE(instance.unload());
    BOOST_REQUIRE(instance.close());
}

BOOST_AUTO_TEST_CASE(map__write__read__expected)
{
    constexpr uint64_t expected = 0x0102030405060708_u64;
    const std::string file = TEST_PATH;
    BOOST_REQUIRE(test::create(file));
    map instance(file);
    BOOST_REQUIRE(instance.open());
    BOOST_REQUIRE(instance.load());
    auto memory = instance.get(instance.allocate(sizeof(uint64_t)));
    BOOST_REQUIRE(memory);
    system::unsafe_to_little_endian<uint64_t>(memory->begin(), expected);
    memory.reset();
    BOOST_REQUIRE(instance.flush());
    memory = instance.get();
    BOOST_REQUIRE(memory);
    BOOST_REQUIRE_EQUAL(system::unsafe_from_little_endian<uint64_t>(memory->begin()), expected);
    memory.reset();
    BOOST_REQUIRE(instance.unload());
    BOOST_REQUIRE(instance.close());
}

BOOST_AUTO_TEST_CASE(map__unload_map__pending_accessor__false)
{
    const std::string file = TEST_PATH;
    BOOST_REQUIRE(test::create(file));
    map instance(file);
    BOOST_REQUIRE(instance.open());
    BOOST_REQUIRE(instance.load());
    auto memory = instance.get(instance.allocate(1));
    BOOST_REQUIRE(memory);
    BOOST_REQUIRE(!instance.unload());
    memory.reset();
    BOOST_REQUIRE(instance.unload());
    BOOST_REQUIRE(instance.close());
}

BOOST_AUTO_TEST_CASE(map__close__pending_map__false)
{
    const std::string file = TEST_PATH;
    BOOST_REQUIRE(test::create(file));
    map instance(file);
    BOOST_REQUIRE(instance.open());
    BOOST_REQUIRE(instance.load());
    auto memory = instance.get(instance.allocate(1));
    BOOST_REQUIRE(memory);
    BOOST_REQUIRE(!instance.close());
    memory.reset();
    BOOST_REQUIRE(!instance.close());
    BOOST_REQUIRE(instance.unload());
    BOOST_REQUIRE(instance.close());
}

BOOST_AUTO_TEST_SUITE_END()
