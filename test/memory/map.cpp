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

    constexpr auto default_minimum_capacity = 1_size;
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
    constexpr auto half_rate = 50_size;
    constexpr auto minimum = 42_size;
    constexpr auto size = 100_size;
    const std::string file = TEST_PATH;
    BOOST_REQUIRE(test::create(file));

    map instance(file, minimum, half_rate);
    BOOST_REQUIRE(!instance.open());
    BOOST_REQUIRE(!instance.load());
    BOOST_REQUIRE_EQUAL(instance.capacity(), minimum);
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
    constexpr auto size = 42_size;
    constexpr auto minimum = 100_size;
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
    constexpr auto minimum = 1_size;
    constexpr auto rate = 42_size;
    constexpr auto size = 100_size;
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

BOOST_AUTO_TEST_CASE(map__set__unloaded__false)
{
    const std::string file = TEST_PATH;
    BOOST_REQUIRE(test::create(file));
    map instance(file);
    BOOST_REQUIRE(!instance.open());
    BOOST_REQUIRE(!instance.set(42, 24, 0xff));
    BOOST_REQUIRE(!instance.close());
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(map__set__loaded__expected_capacity)
{
    constexpr auto half_rate = 50_size;
    constexpr auto minimum = 42_size;
    constexpr auto size = 100_size;
    constexpr auto offset = 42_size;
    constexpr auto fill = 0b01010101;
    const std::string file = TEST_PATH;
    BOOST_REQUIRE(test::create(file));

    map instance(file, minimum, half_rate);
    BOOST_REQUIRE(!instance.open());
    BOOST_REQUIRE(!instance.load());
    BOOST_REQUIRE_EQUAL(instance.capacity(), minimum);

    auto memory = instance.set(offset, size, fill);
    BOOST_REQUIRE(memory);

    const auto expected = std::next(instance.get()->data(), offset);
    BOOST_REQUIRE(memory->data() == expected);

    constexpr auto capacity = offset + size + to_half(offset + size);
    BOOST_REQUIRE_EQUAL(instance.capacity(), capacity);
    BOOST_REQUIRE_EQUAL(instance.unload(), error::unload_locked);

    memory.reset();
    BOOST_REQUIRE(!instance.unload());
    BOOST_REQUIRE(!instance.close());
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(map__set__add_overflow__eof)
{
    const std::string file = TEST_PATH;
    BOOST_REQUIRE(test::create(file));

    map instance(file);
    BOOST_REQUIRE(!instance.open());
    BOOST_REQUIRE(!instance.load());
    BOOST_REQUIRE(instance.set(100, 24, 0xff));
    BOOST_REQUIRE(!instance.set(max_size_t, 24, 0xff));
    BOOST_REQUIRE(!instance.unload());
    BOOST_REQUIRE(!instance.close());
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(map__set__minimum_no_expansion__expected_capacity)
{
    constexpr auto rate = 0_size;
    constexpr auto minimum = 42_size;
    constexpr auto size = 100_size;
    constexpr auto offset = 42_size;
    constexpr auto fill = 0b01010101;
    const std::string file = TEST_PATH;
    BOOST_REQUIRE(test::create(file));

    map instance(file, minimum, rate);
    BOOST_REQUIRE(!instance.open());
    BOOST_REQUIRE(!instance.load());
    BOOST_REQUIRE(instance.set(offset, size, fill));

    constexpr auto capacity = std::max(minimum, offset + size);
    BOOST_REQUIRE_EQUAL(instance.capacity(), capacity);
    BOOST_REQUIRE(!instance.unload());
    BOOST_REQUIRE(!instance.close());
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(map__set__no_minimum_expansion__expected_capacity)
{
    // map will fail if minimum is zero.
    constexpr auto rate = 42_size;
    constexpr auto minimum = 1_size;
    constexpr auto size = 51_size;
    constexpr auto offset = 49_size;
    constexpr auto fill = 0b01010101;
    const std::string file = TEST_PATH;
    BOOST_REQUIRE(test::create(file));

    map instance(file, minimum, rate);
    BOOST_REQUIRE(!instance.open());
    BOOST_REQUIRE(!instance.load());
    BOOST_REQUIRE(instance.set(offset, size, fill));

    // These add only because offset + size is 100.
    constexpr auto capacity = offset + size + rate;
    BOOST_REQUIRE_EQUAL(instance.capacity(), capacity);
    BOOST_REQUIRE(!instance.unload());
    BOOST_REQUIRE(!instance.close());
    BOOST_REQUIRE(!instance.get_fault());
}

BOOST_AUTO_TEST_CASE(map__set__loaded__expected_fill)
{
    BC_PUSH_WARNING(NO_POINTER_ARITHMETIC)

    constexpr auto half_rate = 50_size;
    constexpr auto minimum = 1_size;
    constexpr auto size1 = 3_size;
    constexpr auto offset1 = 5_size;
    constexpr auto fill1 = 0b0101'0101;
    const std::string file = TEST_PATH;
    BOOST_REQUIRE(test::create(file));

    map instance(file, minimum, half_rate);
    BOOST_REQUIRE(!instance.open());
    BOOST_REQUIRE(!instance.load());

    auto memory = instance.set(offset1, size1, fill1);
    BOOST_REQUIRE(memory);

    constexpr auto capacity = offset1 + size1 + to_half(offset1 + size1);
    BOOST_REQUIRE_EQUAL(instance.capacity(), capacity);
    BOOST_REQUIRE_EQUAL(capacity, 12u);

    auto data = instance.get()->data();
    ////BOOST_REQUIRE_EQUAL(data[ 0], 0x00_u8); // cannot assume mmap default fill
    ////BOOST_REQUIRE_EQUAL(data[ 1], 0x00_u8); // cannot assume mmap default fill
    ////BOOST_REQUIRE_EQUAL(data[ 2], 0x00_u8); // cannot assume mmap default fill
    ////BOOST_REQUIRE_EQUAL(data[ 3], 0x00_u8); // cannot assume mmap default fill
    ////BOOST_REQUIRE_EQUAL(data[ 4], 0x00_u8); // cannot assume mmap default fill

    BOOST_REQUIRE_EQUAL(data[ 5], fill1);   // offset + 0
    BOOST_REQUIRE_EQUAL(data[ 6], fill1);   // offset + 1
    BOOST_REQUIRE_EQUAL(data[ 7], fill1);   // offset + 2
    BOOST_REQUIRE_EQUAL(data[ 8], fill1);   // expansion
    BOOST_REQUIRE_EQUAL(data[ 9], fill1);   // expansion
    BOOST_REQUIRE_EQUAL(data[10], fill1);   // expansion
    BOOST_REQUIRE_EQUAL(data[11], fill1);   // sub1(offset1 + size1 + to_half(offset1 + size1))

    data[5] = 'a';
    data[6] = 'b';
    data[7] = 'c';

    constexpr auto size2 = 5_size;
    constexpr auto offset2 = 15_size;
    constexpr auto fill2 = 0b1111'0000;
    memory.reset();
    memory = instance.set(offset2, size2, fill2);
    BOOST_REQUIRE(memory);

    constexpr auto capacity2 = offset2 + size2 + to_half(offset2 + size2);
    BOOST_REQUIRE_EQUAL(instance.capacity(), capacity2);
    BOOST_REQUIRE_EQUAL(capacity2, 30u);

    data[15] = 'd';
    data[16] = 'e';
    data[17] = 'f';
    data[18] = 'g';
    data[19] = 'h';

    // Get data again in case it has been remapped by set().
    data = instance.get()->data();
    ////BOOST_REQUIRE_EQUAL(data[ 0], 0x00_u8); // cannot assume mmap default fill
    ////BOOST_REQUIRE_EQUAL(data[ 1], 0x00_u8); // cannot assume mmap default fill
    ////BOOST_REQUIRE_EQUAL(data[ 2], 0x00_u8); // cannot assume mmap default fill
    ////BOOST_REQUIRE_EQUAL(data[ 3], 0x00_u8); // cannot assume mmap default fill
    ////BOOST_REQUIRE_EQUAL(data[ 4], 0x00_u8); // cannot assume mmap default fill

    BOOST_REQUIRE_EQUAL(data[ 5], 'a');     // offset + 0
    BOOST_REQUIRE_EQUAL(data[ 6], 'b');     // offset + 1
    BOOST_REQUIRE_EQUAL(data[ 7], 'c');     // offset + 2
    BOOST_REQUIRE_EQUAL(data[ 8], fill2);   // expansion, resize_ (macos) trims on remap...
    BOOST_REQUIRE_EQUAL(data[ 9], fill2);   // expansion, so it goes to zero if not refilled.
    BOOST_REQUIRE_EQUAL(data[10], fill2);   // expansion
    BOOST_REQUIRE_EQUAL(data[11], fill2);   // sub1(offset + size + to_half(offset + size))
    BOOST_REQUIRE_EQUAL(data[12], fill2);   // cannot assume mmap default fill
    BOOST_REQUIRE_EQUAL(data[13], fill2);   // cannot assume mmap default fill
    BOOST_REQUIRE_EQUAL(data[14], fill2);   // cannot assume mmap default fill
    BOOST_REQUIRE_EQUAL(data[15], 'd');     // offset2 + 0
    BOOST_REQUIRE_EQUAL(data[16], 'e');     // offset2 + 1
    BOOST_REQUIRE_EQUAL(data[17], 'f');     // offset2 + 2
    BOOST_REQUIRE_EQUAL(data[18], 'g');     // offset2 + 3
    BOOST_REQUIRE_EQUAL(data[19], 'h');     // offset2 + 4
    BOOST_REQUIRE_EQUAL(data[20], fill2);   // expansion
    BOOST_REQUIRE_EQUAL(data[21], fill2);   // expansion
    BOOST_REQUIRE_EQUAL(data[22], fill2);   // expansion
    BOOST_REQUIRE_EQUAL(data[23], fill2);   // expansion
    BOOST_REQUIRE_EQUAL(data[24], fill2);   // expansion
    BOOST_REQUIRE_EQUAL(data[25], fill2);   // expansion
    BOOST_REQUIRE_EQUAL(data[26], fill2);   // expansion
    BOOST_REQUIRE_EQUAL(data[27], fill2);   // expansion
    BOOST_REQUIRE_EQUAL(data[28], fill2);   // expansion
    BOOST_REQUIRE_EQUAL(data[29], fill2);   // expansion

    memory.reset();
    BOOST_REQUIRE(!instance.unload());
    BOOST_REQUIRE(!instance.close());
    BOOST_REQUIRE(!instance.get_fault());

    BC_POP_WARNING()
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
