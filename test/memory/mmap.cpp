/**
 * Copyright (c) 2011-2026 libbitcoin developers
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

#include <numeric>
#include <thread>

// TODO: need to fake map_(), unmap_() and flush_() in order to hit
// error::load_failure, error::flush_failure, error::unload_failure codes, but
// don't want to make this class virtual.

BC_FIXTURE_TEST_SUITE(mmap_tests, test::directory_setup_fixture)

// map is an alias for mmap<one>.

BC_AUTO_TEST_CASE(mmap__file__always__expected)
{
    const std::string file = TEST_PATH;
    map instance(file);
    BC_REQUIRE_EQUAL(instance.file(), file);
    BC_REQUIRE(!instance.get_fault());
}

BC_AUTO_TEST_CASE(mmap__open__opened__open_open)
{
    const std::string file = TEST_PATH;
    BC_REQUIRE(test::create(file));

    map instance(file);
    BC_REQUIRE(test::exists(file));
    BC_REQUIRE(!instance.open());
    BC_REQUIRE_EQUAL(instance.open(), error::open_open);
    BC_REQUIRE(!instance.close());
    BC_REQUIRE(!instance.get_fault());
}

BC_AUTO_TEST_CASE(mmap__open__no_file__false)
{
    const std::string file = TEST_PATH;
    map instance(file);
    BC_REQUIRE(!test::exists(file));
    BC_REQUIRE(instance.open());
    BC_REQUIRE(!instance.close());
    BC_REQUIRE(!instance.get_fault());
}

BC_AUTO_TEST_CASE(mmap__close__open__true)
{
    const std::string file = TEST_PATH;
    BC_REQUIRE(test::create(file));

    map instance(file);
    BC_REQUIRE(!instance.open());
    BC_REQUIRE(instance.is_open());
    BC_REQUIRE(!instance.close());
    BC_REQUIRE(!instance.get_fault());
}

BC_AUTO_TEST_CASE(mmap__close__closed__true)
{
    const std::string file = TEST_PATH;
    BC_REQUIRE(test::create(file));

    map instance(file);
    BC_REQUIRE(!instance.is_open());
    BC_REQUIRE(!instance.close());
}

BC_AUTO_TEST_CASE(mmap__close__loaded__close_loaded)
{
    const std::string file = TEST_PATH;
    BC_REQUIRE(test::create(file));

    map instance(file);
    BC_REQUIRE(!instance.open());
    BC_REQUIRE(!instance.load());
    BC_REQUIRE_EQUAL(instance.close(), error::close_loaded);
    BC_REQUIRE(!instance.unload());
    BC_REQUIRE(!instance.close());
    BC_REQUIRE(!instance.get_fault());
}

BC_AUTO_TEST_CASE(mmap__properties__open_close__expected)
{
    const std::string file = TEST_PATH;
    BC_REQUIRE(test::create(file));

    map instance(file);
    BC_REQUIRE(!instance.is_open());
    BC_REQUIRE(!instance.is_loaded());
    BC_REQUIRE_EQUAL(instance.size(), 0u);
    BC_REQUIRE_EQUAL(instance.capacity(), 0u);

    BC_REQUIRE(!instance.open());
    BC_REQUIRE(instance.is_open());
    BC_REQUIRE(!instance.is_loaded());
    BC_REQUIRE_EQUAL(instance.size(), 0u);
    BC_REQUIRE_EQUAL(instance.capacity(), 0u);

    BC_REQUIRE(!instance.close());
    BC_REQUIRE(!instance.is_open());
    BC_REQUIRE(!instance.is_loaded());
    BC_REQUIRE_EQUAL(instance.size(), 0u);
    BC_REQUIRE_EQUAL(instance.capacity(), 0u);
    BC_REQUIRE(!instance.get_fault());

    BC_REQUIRE(test::exists(file));
}

BC_AUTO_TEST_CASE(mmap__properties__load_unload__expected)
{
    constexpr auto default_minimum_capacity = 1u;
    const std::string file = TEST_PATH;
    BC_REQUIRE(test::create(file));

    map instance(file);
    BC_REQUIRE(!instance.open());
    BC_REQUIRE(!instance.is_loaded());
    BC_REQUIRE_EQUAL(instance.size(), 0u);
    BC_REQUIRE_EQUAL(instance.capacity(), 0u);

    BC_REQUIRE(!instance.load());
    BC_REQUIRE(instance.is_loaded());
    BC_REQUIRE_EQUAL(instance.size(), 0u);
    BC_REQUIRE_EQUAL(instance.capacity(), default_minimum_capacity);

    BC_REQUIRE(!instance.unload());
    BC_REQUIRE(!instance.is_loaded());
    BC_REQUIRE_EQUAL(instance.size(), 0u);
    BC_REQUIRE_EQUAL(instance.capacity(), 0u);

    BC_REQUIRE(!instance.close());
    BC_REQUIRE(!instance.is_open());
    BC_REQUIRE(!instance.is_loaded());
    BC_REQUIRE_EQUAL(instance.size(), 0u);
    BC_REQUIRE_EQUAL(instance.capacity(), 0u);
    BC_REQUIRE(!instance.get_fault());

    BC_REQUIRE(test::exists(file));
}

BC_AUTO_TEST_CASE(mmap__load__unloaded__true)
{
    const std::string file = TEST_PATH;
    BC_REQUIRE(test::create(file));

    map instance(file);
    BC_REQUIRE(!instance.open());
    BC_REQUIRE(!instance.load());
    BC_REQUIRE(!instance.unload());
    BC_REQUIRE(!instance.close());
    BC_REQUIRE(!instance.get_fault());
}

BC_AUTO_TEST_CASE(mmap__load__shared__load_locked)
{
    const std::string file = TEST_PATH;
    BC_REQUIRE(test::create(file));

    map instance(file);
    BC_REQUIRE(!instance.open());
    BC_REQUIRE(!instance.load());
    auto memory = instance.get(instance.allocate(1));

    BC_REQUIRE(memory);
    BC_REQUIRE_EQUAL(instance.load(), error::load_locked);

    memory.reset();
    BC_REQUIRE(!instance.unload());
    BC_REQUIRE(!instance.close());
    BC_REQUIRE(!instance.get_fault());
}

BC_AUTO_TEST_CASE(mmap__load__loaded__load_loaded)
{
    const std::string file = TEST_PATH;
    BC_REQUIRE(test::create(file));

    map instance(file);
    BC_REQUIRE(!instance.open());
    BC_REQUIRE(!instance.load());
    BC_REQUIRE_EQUAL(instance.load(), error::load_loaded);
    BC_REQUIRE(!instance.unload());
    BC_REQUIRE(!instance.close());
    BC_REQUIRE(!instance.get_fault());
}

BC_AUTO_TEST_CASE(mmap__unload__unloaded__true)
{
    const std::string file = TEST_PATH;
    BC_REQUIRE(test::create(file));

    map instance(file);
    BC_REQUIRE(!instance.open());
    BC_REQUIRE(!instance.unload());
    BC_REQUIRE(!instance.close());
    BC_REQUIRE(!instance.get_fault());
}

BC_AUTO_TEST_CASE(mmap__unload__loaded__true)
{
    const std::string file = TEST_PATH;
    BC_REQUIRE(test::create(file));

    map instance(file);
    BC_REQUIRE(!instance.open());
    BC_REQUIRE(!instance.load());
    BC_REQUIRE(!instance.unload());
    BC_REQUIRE(!instance.close());
    BC_REQUIRE(!instance.get_fault());
}

BC_AUTO_TEST_CASE(mmap__shrink__unloaded__shrink_unloaded)
{
    const std::string file = TEST_PATH;
    BC_REQUIRE(test::create(file));

    map instance(file);
    BC_REQUIRE(!instance.open());
    BC_REQUIRE_EQUAL(instance.shrink(), error::shrink_unloaded);
    BC_REQUIRE(!instance.close());
    BC_REQUIRE(!instance.get_fault());
}

BC_AUTO_TEST_CASE(mmap__shrink__loaded__true)
{
    const std::string file = TEST_PATH;
    BC_REQUIRE(test::create(file));

    map instance(file);
    BC_REQUIRE(!instance.open());
    BC_REQUIRE(!instance.load());
    BC_REQUIRE(!instance.shrink());
    BC_REQUIRE(!instance.unload());
    BC_REQUIRE(!instance.close());
    BC_REQUIRE(!instance.get_fault());
}

BC_AUTO_TEST_CASE(mmap__capacity__default__expected)
{

    constexpr auto default_minimum_capacity = 1_size;
    const std::string file = TEST_PATH;
    BC_REQUIRE(test::create(file));

    map instance(file);
    BC_REQUIRE(!instance.open());
    BC_REQUIRE_EQUAL(instance.capacity(), zero);
    BC_REQUIRE(!instance.load());
    BC_REQUIRE_EQUAL(instance.capacity(), default_minimum_capacity);
    BC_REQUIRE(!instance.unload());
    BC_REQUIRE(!instance.close());
    BC_REQUIRE(!instance.get_fault());
}

BC_AUTO_TEST_CASE(mmap__allocate__unloaded__false)
{
    const std::string file = TEST_PATH;
    BC_REQUIRE(test::create(file));

    map instance(file);
    BC_REQUIRE(!instance.open());
    BC_REQUIRE_EQUAL(instance.allocate(42), map::eof);
    BC_REQUIRE(!instance.close());
    BC_REQUIRE(!instance.get_fault());
}

BC_AUTO_TEST_CASE(mmap__allocate__loaded__expected_capacity)
{
    constexpr auto half_rate = 50_size;
    constexpr auto minimum = 42_size;
    constexpr auto size = 100_size;
    const std::string file = TEST_PATH;
    BC_REQUIRE(test::create(file));

    map instance(file, { minimum, half_rate });
    BC_REQUIRE(!instance.open());
    BC_REQUIRE(!instance.load());
    BC_REQUIRE_EQUAL(instance.capacity(), minimum);
    BC_REQUIRE_EQUAL(instance.allocate(size), zero);

    constexpr auto capacity = size + to_half(size);
    BC_REQUIRE_EQUAL(instance.capacity(), capacity);
    BC_REQUIRE(!instance.unload());
    BC_REQUIRE(!instance.close());
    BC_REQUIRE(!instance.get_fault());
}

BC_AUTO_TEST_CASE(mmap__allocate__add_overflow__eof)
{
    const std::string file = TEST_PATH;
    BC_REQUIRE(test::create(file));

    map instance(file);
    BC_REQUIRE(!instance.open());
    BC_REQUIRE(!instance.load());
    BC_REQUIRE_EQUAL(instance.allocate(100), zero);
    BC_REQUIRE_EQUAL(instance.allocate(max_size_t), storage::eof);
    BC_REQUIRE(!instance.unload());
    BC_REQUIRE(!instance.close());
    BC_REQUIRE(!instance.get_fault());
}

BC_AUTO_TEST_CASE(mmap__allocate__minimum_no_expansion__expected_capacity)
{
    constexpr auto size = 42_size;
    constexpr auto minimum = 100_size;
    const std::string file = TEST_PATH;
    BC_REQUIRE(test::create(file));

    map instance(file, { minimum, 0 });
    BC_REQUIRE(!instance.open());
    BC_REQUIRE(!instance.load());
    BC_REQUIRE_EQUAL(instance.allocate(size), zero);

    constexpr auto capacity = std::max(minimum, size);
    BC_REQUIRE_EQUAL(instance.capacity(), capacity);
    BC_REQUIRE(!instance.unload());
    BC_REQUIRE(!instance.close());
    BC_REQUIRE(!instance.get_fault());
}

BC_AUTO_TEST_CASE(mmap__allocate__no_minimum_expansion__expected_capacity)
{
    // map will fail if minimum is zero.
    constexpr auto minimum = 1_size;
    constexpr auto rate = 42_size;
    constexpr auto size = 100_size;
    const std::string file = TEST_PATH;
    BC_REQUIRE(test::create(file));

    map instance(file, { minimum, rate });
    BC_REQUIRE(!instance.open());
    BC_REQUIRE(!instance.load());
    BC_REQUIRE_EQUAL(instance.allocate(size), zero);

    // These add only because size is 100.
    constexpr auto capacity = size + rate;
    BC_REQUIRE_EQUAL(instance.capacity(), capacity);
    BC_REQUIRE(!instance.unload());
    BC_REQUIRE(!instance.close());
    BC_REQUIRE(!instance.get_fault());
}

// Growth admission (headroom).
// ----------------------------------------------------------------------------
// Unsatisfiable headroom refuses every growth ask, which is recoverable
// resource condition (space set, store intact) and never store fault. Staged
// load commits (stage_), so unsatisfiable headroom refuses load itself and
// these cases cannot isolate growth there.

#if !defined(MANAGE_STAGING)

constexpr uint64_t unsatisfiable_headroom = 0x0001000000000000_u64;

BC_AUTO_TEST_CASE(mmap__allocate__unsatisfiable_headroom__eof)
{
    constexpr auto minimum = 42_size;
    constexpr auto rate = 50_size;
    const std::string file = TEST_PATH;
    BC_REQUIRE(test::create(file));

    map instance(file, { minimum, rate, unsatisfiable_headroom });
    BC_REQUIRE(!instance.open());
    BC_REQUIRE(!instance.load());
    BC_REQUIRE_EQUAL(instance.allocate(1000), map::eof);
    BC_REQUIRE(!instance.unload());
    BC_REQUIRE(!instance.close());
}

BC_AUTO_TEST_CASE(mmap__allocate__unsatisfiable_headroom__space_no_fault)
{
    constexpr auto minimum = 42_size;
    constexpr auto rate = 50_size;
    const std::string file = TEST_PATH;
    BC_REQUIRE(test::create(file));

    map instance(file, { minimum, rate, unsatisfiable_headroom });
    BC_REQUIRE(!instance.open());
    BC_REQUIRE(!instance.load());
    BC_REQUIRE_EQUAL(instance.allocate(1000), map::eof);

    BC_REQUIRE_NE(instance.get_space(), zero);
    BC_REQUIRE(!instance.get_fault());
    BC_REQUIRE(instance.is_loaded());
    BC_REQUIRE(!instance.unload());
    BC_REQUIRE(!instance.close());
}

BC_AUTO_TEST_CASE(mmap__allocate__unsatisfiable_headroom__capacity_retained)
{
    constexpr auto minimum = 42_size;
    constexpr auto rate = 50_size;
    const std::string file = TEST_PATH;
    BC_REQUIRE(test::create(file));

    map instance(file, { minimum, rate, unsatisfiable_headroom });
    BC_REQUIRE(!instance.open());
    BC_REQUIRE(!instance.load());
    BC_REQUIRE_EQUAL(instance.capacity(), minimum);
    BC_REQUIRE_EQUAL(instance.allocate(1000), map::eof);

    BC_REQUIRE_EQUAL(instance.capacity(), minimum);
    BC_REQUIRE(!instance.unload());
    BC_REQUIRE(!instance.close());
    BC_REQUIRE(!instance.get_fault());
}

BC_AUTO_TEST_CASE(mmap__allocate__within_capacity_unsatisfiable_headroom__success)
{
    constexpr auto minimum = 100_size;
    constexpr auto rate = 50_size;
    const std::string file = TEST_PATH;
    BC_REQUIRE(test::create(file));

    map instance(file, { minimum, rate, unsatisfiable_headroom });
    BC_REQUIRE(!instance.open());
    BC_REQUIRE(!instance.load());
    BC_REQUIRE_EQUAL(instance.allocate(42), zero);
    BC_REQUIRE(!instance.get_fault());
    BC_REQUIRE_EQUAL(instance.get_space(), zero);
    BC_REQUIRE(!instance.unload());
    BC_REQUIRE(!instance.close());
}

BC_AUTO_TEST_CASE(mmap__reload__after_headroom_refusal__clears_space)
{
    constexpr auto minimum = 42_size;
    constexpr auto rate = 50_size;
    const std::string file = TEST_PATH;
    BC_REQUIRE(test::create(file));

    map instance(file, { minimum, rate, unsatisfiable_headroom });
    BC_REQUIRE(!instance.open());
    BC_REQUIRE(!instance.load());
    BC_REQUIRE_EQUAL(instance.allocate(1000), map::eof);
    BC_REQUIRE_NE(instance.get_space(), zero);

    BC_REQUIRE(!instance.reload());
    BC_REQUIRE_EQUAL(instance.get_space(), zero);
    BC_REQUIRE(!instance.get_fault());
    BC_REQUIRE(!instance.unload());
    BC_REQUIRE(!instance.close());
}

BC_AUTO_TEST_CASE(mmap__expand__unsatisfiable_headroom__false_no_fault)
{
    constexpr auto minimum = 42_size;
    constexpr auto rate = 50_size;
    const std::string file = TEST_PATH;
    BC_REQUIRE(test::create(file));

    map instance(file, { minimum, rate, unsatisfiable_headroom });
    BC_REQUIRE(!instance.open());
    BC_REQUIRE(!instance.load());
    BC_REQUIRE(!instance.expand(1000));
    BC_REQUIRE_NE(instance.get_space(), zero);
    BC_REQUIRE(!instance.get_fault());
    BC_REQUIRE(!instance.unload());
    BC_REQUIRE(!instance.close());
}

BC_AUTO_TEST_CASE(mmap__reserve__unsatisfiable_headroom__false_no_fault)
{
    constexpr auto minimum = 42_size;
    constexpr auto rate = 50_size;
    const std::string file = TEST_PATH;
    BC_REQUIRE(test::create(file));

    map instance(file, { minimum, rate, unsatisfiable_headroom });
    BC_REQUIRE(!instance.open());
    BC_REQUIRE(!instance.load());
    BC_REQUIRE(!instance.reserve(1000));
    BC_REQUIRE_NE(instance.get_space(), zero);
    BC_REQUIRE(!instance.get_fault());
    BC_REQUIRE(!instance.unload());
    BC_REQUIRE(!instance.close());
}

#endif // !MANAGE_STAGING

BC_AUTO_TEST_CASE(mmap__allocate__zero_headroom__growth_admitted)
{
    constexpr auto minimum = 42_size;
    constexpr auto rate = 50_size;
    constexpr auto size = 1000_size;
    const std::string file = TEST_PATH;
    BC_REQUIRE(test::create(file));

    map instance(file, { minimum, rate, 0 });
    BC_REQUIRE(!instance.open());
    BC_REQUIRE(!instance.load());
    BC_REQUIRE_EQUAL(instance.allocate(size), zero);

    constexpr auto capacity = size + to_half(size);
    BC_REQUIRE_EQUAL(instance.capacity(), capacity);
    BC_REQUIRE_EQUAL(instance.get_space(), zero);
    BC_REQUIRE(!instance.unload());
    BC_REQUIRE(!instance.close());
    BC_REQUIRE(!instance.get_fault());
}

BC_AUTO_TEST_CASE(mmap__truncate__unloaded__failure)
{
    const std::string file = TEST_PATH;
    BC_REQUIRE(test::create(file));

    map instance(file);
    BC_REQUIRE(!instance.open());
    BC_REQUIRE(!instance.truncate(42));
    BC_REQUIRE(!instance.close());
    BC_REQUIRE(!instance.get_fault());
}

BC_AUTO_TEST_CASE(mmap__truncate__default_to_zero__success)
{
    const std::string file = TEST_PATH;
    BC_REQUIRE(test::create(file));

    map instance(file);
    BC_REQUIRE(!instance.open());
    BC_REQUIRE(!instance.load());
    BC_REQUIRE( instance.truncate(0));
    BC_REQUIRE(!instance.unload());
    BC_REQUIRE(!instance.close());
    BC_REQUIRE(!instance.get_fault());
}

BC_AUTO_TEST_CASE(mmap__truncate__increase_capacity__failure)
{
    constexpr auto half_rate = 50_size;
    constexpr auto minimum = 42_size;
    const std::string file = TEST_PATH;
    BC_REQUIRE(test::create(file));

    map instance(file, { minimum, half_rate });
    BC_REQUIRE(!instance.open());
    BC_REQUIRE(!instance.load());
    BC_REQUIRE_EQUAL(instance.capacity(), minimum);
    BC_REQUIRE(!instance.truncate(add1(minimum)));
    BC_REQUIRE_EQUAL(instance.capacity(), minimum);
    BC_REQUIRE(!instance.unload());
    BC_REQUIRE(!instance.close());
    BC_REQUIRE(!instance.get_fault());
}

BC_AUTO_TEST_CASE(mmap__truncate__unchanged__success_capacity_unchanged)
{
    constexpr auto half_rate = 50_size;
    constexpr auto minimum = 42_size;
    const std::string file = TEST_PATH;
    BC_REQUIRE(test::create(file));

    map instance(file, { minimum, half_rate });
    BC_REQUIRE(!instance.open());
    BC_REQUIRE(!instance.load());
    BC_REQUIRE_EQUAL(instance.size(), zero);
    BC_REQUIRE_EQUAL(instance.capacity(), minimum);
    BC_REQUIRE( instance.truncate(zero));
    BC_REQUIRE_EQUAL(instance.capacity(), minimum);
    BC_REQUIRE_EQUAL(instance.size(), zero);
    BC_REQUIRE(!instance.unload());
    BC_REQUIRE(!instance.close());
    BC_REQUIRE(!instance.get_fault());
}

BC_AUTO_TEST_CASE(mmap__truncate__decrease__success_logical_decreased_capacity_unchanged)
{
    constexpr auto half_rate = 50_size;
    constexpr auto minimum = 42_size;
    constexpr auto half = to_half(minimum);
    const std::string file = TEST_PATH;
    BC_REQUIRE(test::create(file));

    map instance(file, { minimum, half_rate });
    BC_REQUIRE(!instance.open());
    BC_REQUIRE(!instance.load());
    BC_REQUIRE_EQUAL(instance.allocate(minimum), zero);
    BC_REQUIRE_EQUAL(instance.size(), minimum);
    BC_REQUIRE_EQUAL(instance.capacity(), minimum);
    BC_REQUIRE( instance.truncate(half));
    BC_REQUIRE_EQUAL(instance.capacity(), minimum);
    BC_REQUIRE_EQUAL(instance.size(), half);
    BC_REQUIRE(!instance.unload());
    BC_REQUIRE(!instance.close());
    BC_REQUIRE(!instance.get_fault());
}

BC_AUTO_TEST_CASE(mmap__get_filled__unloaded__false)
{
    const std::string file = TEST_PATH;
    BC_REQUIRE(test::create(file));
    map instance(file);
    BC_REQUIRE(!instance.open());
    BC_REQUIRE(!instance.get_filled(42, 24));
    BC_REQUIRE(!instance.close());
    BC_REQUIRE(!instance.get_fault());
}

BC_AUTO_TEST_CASE(mmap__get_filled__loaded__expected_capacity)
{
    constexpr auto half_rate = 50_size;
    constexpr auto minimum = 42_size;
    constexpr auto size = 100_size;
    constexpr auto offset = 42_size;
    const std::string file = TEST_PATH;
    BC_REQUIRE(test::create(file));

    map instance(file, { minimum, half_rate });
    BC_REQUIRE(!instance.open());
    BC_REQUIRE(!instance.load());
    BC_REQUIRE_EQUAL(instance.capacity(), minimum);

    auto memory = instance.get_filled(offset, size);
    BC_REQUIRE(memory);

    const auto expected = std::next(instance.get().data(), offset);
    BC_REQUIRE(memory.data() == expected);

    constexpr auto capacity = offset + size + to_half(offset + size);
    BC_REQUIRE_EQUAL(instance.capacity(), capacity);
    BC_REQUIRE_EQUAL(instance.unload(), error::unload_locked);

    memory.reset();
    BC_REQUIRE(!instance.unload());
    BC_REQUIRE(!instance.close());
    BC_REQUIRE(!instance.get_fault());
}

BC_AUTO_TEST_CASE(mmap__get_filled__add_overflow__eof)
{
    const std::string file = TEST_PATH;
    BC_REQUIRE(test::create(file));

    map instance(file);
    BC_REQUIRE(!instance.open());
    BC_REQUIRE(!instance.load());
    BC_REQUIRE(instance.get_filled(100, 24));
    BC_REQUIRE(!instance.get_filled(max_size_t, 24));
    BC_REQUIRE(!instance.unload());
    BC_REQUIRE(!instance.close());
    BC_REQUIRE(!instance.get_fault());
}

BC_AUTO_TEST_CASE(mmap__get_filled__minimum_no_expansion__expected_capacity)
{
    constexpr auto rate = 0_size;
    constexpr auto minimum = 42_size;
    constexpr auto size = 100_size;
    constexpr auto offset = 42_size;
    const std::string file = TEST_PATH;
    BC_REQUIRE(test::create(file));

    map instance(file, { minimum, rate });
    BC_REQUIRE(!instance.open());
    BC_REQUIRE(!instance.load());
    BC_REQUIRE(instance.get_filled(offset, size));

    constexpr auto capacity = std::max(minimum, offset + size);
    BC_REQUIRE_EQUAL(instance.capacity(), capacity);
    BC_REQUIRE(!instance.unload());
    BC_REQUIRE(!instance.close());
    BC_REQUIRE(!instance.get_fault());
}

BC_AUTO_TEST_CASE(mmap__get_filled__no_minimum_expansion__expected_capacity)
{
    // map will fail if minimum is zero.
    constexpr auto rate = 42_size;
    constexpr auto minimum = 1_size;
    constexpr auto size = 51_size;
    constexpr auto offset = 49_size;
    const std::string file = TEST_PATH;
    BC_REQUIRE(test::create(file));

    map instance(file, { minimum, rate });
    BC_REQUIRE(!instance.open());
    BC_REQUIRE(!instance.load());
    BC_REQUIRE(instance.get_filled(offset, size));

    // These add only because offset + size is 100.
    constexpr auto capacity = offset + size + rate;
    BC_REQUIRE_EQUAL(instance.capacity(), capacity);
    BC_REQUIRE(!instance.unload());
    BC_REQUIRE(!instance.close());
    BC_REQUIRE(!instance.get_fault());
}

BC_AUTO_TEST_CASE(mmap__get_filled__loaded__expected_fill)
{
    BC_PUSH_WARNING(NO_POINTER_ARITHMETIC)

    constexpr auto half_rate = 50_size;
    constexpr auto minimum = 1_size;
    constexpr auto size1 = 3_size;
    constexpr auto offset1 = 5_size;
    constexpr auto fill1 = 0b0101'0101_u8;
    const std::string file = TEST_PATH;
    BC_REQUIRE(test::create(file));

    map instance(file, { .size = minimum, .rate = half_rate, .fill = fill1 });
    BC_REQUIRE(!instance.open());
    BC_REQUIRE(!instance.load());

    auto memory = instance.get_filled(offset1, size1);
    BC_REQUIRE(memory);

    constexpr auto capacity = offset1 + size1 + to_half(offset1 + size1);
    BC_REQUIRE_EQUAL(instance.capacity(), capacity);
    BC_REQUIRE_EQUAL(capacity, 12u);

    auto data = instance.get().data();
    BC_REQUIRE_EQUAL(data[ 0], fill1);   // load and expansion
    BC_REQUIRE_EQUAL(data[ 1], fill1);   // load and expansion
    BC_REQUIRE_EQUAL(data[ 2], fill1);   // load and expansion
    BC_REQUIRE_EQUAL(data[ 3], fill1);   // load and expansion
    BC_REQUIRE_EQUAL(data[ 4], fill1);   // load and expansion

    BC_REQUIRE_EQUAL(data[ 5], fill1);   // offset + 0
    BC_REQUIRE_EQUAL(data[ 6], fill1);   // offset + 1
    BC_REQUIRE_EQUAL(data[ 7], fill1);   // offset + 2
    BC_REQUIRE_EQUAL(data[ 8], fill1);   // expansion
    BC_REQUIRE_EQUAL(data[ 9], fill1);   // expansion
    BC_REQUIRE_EQUAL(data[10], fill1);   // expansion
    BC_REQUIRE_EQUAL(data[11], fill1);   // sub1(offset1 + size1 + to_half(offset1 + size1))

    data[5] = 'a';
    data[6] = 'b';
    data[7] = 'c';

    constexpr auto size2 = 5_size;
    constexpr auto offset2 = 15_size;
    memory.reset();
    memory = instance.get_filled(offset2, size2);
    BC_REQUIRE(memory);

    constexpr auto capacity2 = offset2 + size2 + to_half(offset2 + size2);
    BC_REQUIRE_EQUAL(instance.capacity(), capacity2);
    BC_REQUIRE_EQUAL(capacity2, 30u);

    data[15] = 'd';
    data[16] = 'e';
    data[17] = 'f';
    data[18] = 'g';
    data[19] = 'h';

    // Get data again in case it has been remapped by get_filled().
    data = instance.get().data();
    ////BC_REQUIRE_EQUAL(data[ 0], 0x00_u8); // cannot assume mmap default fill
    ////BC_REQUIRE_EQUAL(data[ 1], 0x00_u8); // cannot assume mmap default fill
    ////BC_REQUIRE_EQUAL(data[ 2], 0x00_u8); // cannot assume mmap default fill
    ////BC_REQUIRE_EQUAL(data[ 3], 0x00_u8); // cannot assume mmap default fill
    ////BC_REQUIRE_EQUAL(data[ 4], 0x00_u8); // cannot assume mmap default fill

    BC_REQUIRE_EQUAL(data[ 5], 'a');     // offset + 0
    BC_REQUIRE_EQUAL(data[ 6], 'b');     // offset + 1
    BC_REQUIRE_EQUAL(data[ 7], 'c');     // offset + 2
    BC_REQUIRE_EQUAL(data[ 8], fill1);   // expansion, resize_ (macos) trims on remap...
    BC_REQUIRE_EQUAL(data[ 9], fill1);   // expansion, so it goes to zero if not refilled.
    BC_REQUIRE_EQUAL(data[10], fill1);   // expansion
    BC_REQUIRE_EQUAL(data[11], fill1);   // sub1(offset + size + to_half(offset + size))
    BC_REQUIRE_EQUAL(data[12], fill1);   // cannot assume mmap default fill
    BC_REQUIRE_EQUAL(data[13], fill1);   // cannot assume mmap default fill
    BC_REQUIRE_EQUAL(data[14], fill1);   // cannot assume mmap default fill
    BC_REQUIRE_EQUAL(data[15], 'd');     // offset2 + 0
    BC_REQUIRE_EQUAL(data[16], 'e');     // offset2 + 1
    BC_REQUIRE_EQUAL(data[17], 'f');     // offset2 + 2
    BC_REQUIRE_EQUAL(data[18], 'g');     // offset2 + 3
    BC_REQUIRE_EQUAL(data[19], 'h');     // offset2 + 4
    BC_REQUIRE_EQUAL(data[20], fill1);   // expansion
    BC_REQUIRE_EQUAL(data[21], fill1);   // expansion
    BC_REQUIRE_EQUAL(data[22], fill1);   // expansion
    BC_REQUIRE_EQUAL(data[23], fill1);   // expansion
    BC_REQUIRE_EQUAL(data[24], fill1);   // expansion
    BC_REQUIRE_EQUAL(data[25], fill1);   // expansion
    BC_REQUIRE_EQUAL(data[26], fill1);   // expansion
    BC_REQUIRE_EQUAL(data[27], fill1);   // expansion
    BC_REQUIRE_EQUAL(data[28], fill1);   // expansion
    BC_REQUIRE_EQUAL(data[29], fill1);   // expansion

    memory.reset();
    BC_REQUIRE(!instance.unload());
    BC_REQUIRE(!instance.close());
    BC_REQUIRE(!instance.get_fault());

    BC_POP_WARNING()
}

BC_AUTO_TEST_CASE(mmap__get__unloaded__false)
{
    const std::string file = TEST_PATH;
    BC_REQUIRE(test::create(file));

    map instance(file);
    BC_REQUIRE(!instance.open());
    BC_REQUIRE(!instance.get());
    BC_REQUIRE(!instance.close());
    BC_REQUIRE(!instance.get_fault());
}

BC_AUTO_TEST_CASE(mmap__get__loaded__success)
{
    const std::string file = TEST_PATH;
    BC_REQUIRE(test::create(file));

    map instance(file);
    BC_REQUIRE(!instance.open());
    BC_REQUIRE(!instance.load());
    BC_REQUIRE(instance.get(instance.allocate(1)));
    BC_REQUIRE(!instance.unload());
    BC_REQUIRE(!instance.close());
    BC_REQUIRE(!instance.get_fault());
}

BC_AUTO_TEST_CASE(mmap__get__size__expected)
{
    const std::string file = TEST_PATH;
    BC_REQUIRE(test::create(file));

    map instance(file);
    BC_REQUIRE(!instance.open());
    BC_REQUIRE(!instance.load());

    constexpr auto expected = 42u;
    auto ptr = instance.get(instance.allocate(expected));
    BC_CHECK_EQUAL(ptr.size(), system::to_signed(expected));
    BC_CHECK_EQUAL(*ptr.begin(), 0x00u);
    BC_REQUIRE(instance.unload());

    ptr.reset();
    BC_REQUIRE(!instance.unload());
    BC_REQUIRE(!instance.close());
    BC_REQUIRE(!instance.get_fault());
}

BC_AUTO_TEST_CASE(mmap__get_capacity__size__expected)
{
    const std::string file = TEST_PATH;
    BC_REQUIRE(test::create(file));

    // No expansion, so capacity equals the allocation.
    map instance(file, { 1, 0 });
    BC_REQUIRE(!instance.open());
    BC_REQUIRE(!instance.load());

    constexpr auto expected = 42u;
    auto ptr = instance.get_capacity(instance.allocate(expected));
    BC_REQUIRE_EQUAL(ptr.size(), system::to_signed(expected));
    BC_REQUIRE_EQUAL(*ptr.begin(), 0x00u);
    BC_REQUIRE(instance.unload());

    ptr.reset();
    BC_REQUIRE(!instance.unload());
    BC_REQUIRE(!instance.close());
    BC_REQUIRE(!instance.get_fault());
}

BC_AUTO_TEST_CASE(mmap__get_capacity_at__invalid_column__null)
{
    const std::string file = TEST_PATH;
    BC_REQUIRE(test::create(file));

    map instance(file, { 1, 50 });
    BC_REQUIRE(!instance.open());
    BC_REQUIRE(!instance.load());
    BC_REQUIRE_EQUAL(instance.allocate(100), zero);
    BC_REQUIRE(!instance.get_capacity_at(one));
    BC_REQUIRE(!instance.unload());
    BC_REQUIRE(!instance.close());
    BC_REQUIRE(!instance.get_fault());
}

BC_AUTO_TEST_CASE(mmap__get_capacity_at__allocated_while_held__reachable)
{
    const std::string file = TEST_PATH;
    BC_REQUIRE(test::create(file));

    map instance(file, { 1, 50 });
    BC_REQUIRE(!instance.open());
    BC_REQUIRE(!instance.load());
    BC_REQUIRE_EQUAL(instance.allocate(100), zero);
    BC_REQUIRE(instance.capacity() > 104u);

    // Logical extent is fixed when obtained.
    auto memory = instance.get();
    BC_REQUIRE_EQUAL(instance.allocate(2), 100u);
    BC_REQUIRE(is_null(memory.offset(101)));
    memory.reset();

    // Capacity extent reaches rows allocated while held.
    memory = instance.get_capacity_at(zero);
    BC_REQUIRE_EQUAL(instance.allocate(2), 102u);
    BC_REQUIRE(!is_null(memory.offset(103)));
    memory.reset();

    BC_REQUIRE(!instance.unload());
    BC_REQUIRE(!instance.close());
    BC_REQUIRE(!instance.get_fault());
}

BC_AUTO_TEST_CASE(mmap__get_raw__always__nonblocking)
{
    const std::string file = TEST_PATH;
    BC_REQUIRE(test::create(file));

    map instance(file);
    BC_REQUIRE(!instance.open());
    BC_REQUIRE(!instance.load());

    constexpr auto expected = 42u;
    const auto raw = instance.get_raw(instance.allocate(expected));
    BC_REQUIRE(raw != nullptr);
    BC_REQUIRE_EQUAL(*raw, 0x00u);

    // raw pointer does not block remap.
    BC_REQUIRE(!instance.unload());
    BC_REQUIRE(!instance.unload());
    BC_REQUIRE(!instance.close());
    BC_REQUIRE(!instance.get_fault());
}

BC_AUTO_TEST_CASE(mmap__flush__unloaded__flush_unloaded)
{
    const std::string file = TEST_PATH;
    BC_REQUIRE(test::create(file));

    map instance(file);
    BC_REQUIRE(!instance.open());
    BC_REQUIRE_EQUAL(instance.flush(), error::flush_unloaded);
    BC_REQUIRE(!instance.close());
    BC_REQUIRE(!instance.get_fault());
}

BC_AUTO_TEST_CASE(mmap__flush__loaded__true)
{
    const std::string file = TEST_PATH;
    BC_REQUIRE(test::create(file));

    map instance(file);
    BC_REQUIRE(!instance.open());
    BC_REQUIRE(!instance.load());
    BC_REQUIRE(!instance.flush());
    BC_REQUIRE(!instance.unload());
    BC_REQUIRE(!instance.close());
    BC_REQUIRE(!instance.get_fault());
}

BC_AUTO_TEST_CASE(mmap__write__read__expected)
{
    constexpr uint64_t expected = 0x0102030405060708_u64;
    const std::string file = TEST_PATH;
    BC_REQUIRE(test::create(file));

    map instance(file);
    BC_REQUIRE(!instance.open());
    BC_REQUIRE(!instance.load());

    auto memory = instance.get(instance.allocate(sizeof(uint64_t)));
    BC_REQUIRE(memory);

    system::unsafe_to_little_endian<uint64_t>(memory.begin(), expected);
    memory.reset();
    BC_REQUIRE(!instance.flush());

    memory = instance.get();
    BC_REQUIRE(memory);
    BC_REQUIRE_EQUAL(system::unsafe_from_little_endian<uint64_t>(memory.begin()), expected);

    memory.reset();
    BC_REQUIRE(!instance.unload());
    BC_REQUIRE(!instance.close());
    BC_REQUIRE(!instance.get_fault());
}

// Staged parameter engages anonymous staging backend where built (posix);
// elsewhere it is ignored. Content assertions identical either way, so these
// cases validate staging on posix and classic map on win32.

constexpr auto stage_pattern = [](size_t index) NOEXCEPT
{
    return static_cast<uint8_t>(index % 251u);
};

constexpr auto stage_repattern = [](size_t index) NOEXCEPT
{
    return static_cast<uint8_t>((index * 3u) % 241u);
};

// Test vector generation (element iteration is confined to here).
const auto stage_vector = [](auto pattern, size_t size, size_t offset={})
{
    system::data_chunk out(size);
    std::generate(out.begin(), out.end(), [&]() NOEXCEPT
    {
        return pattern(offset++);
    });

    return out;
};

BC_PUSH_WARNING(NO_UNSAFE_COPY_N)

BC_AUTO_TEST_CASE(mmap__staged__write_flush_write_reload__expected)
{
    constexpr size_t first = 10'000;
    constexpr size_t second = 7'000;
    const std::string file = TEST_PATH;
    BC_REQUIRE(test::create(file));

    map instance(file, { 1, 50 }, true, true);
    BC_REQUIRE(!instance.open());
    BC_REQUIRE(!instance.load());

    const auto head = stage_vector(stage_pattern, first);
    const auto tail = stage_vector(stage_pattern, second, first);
    const auto full = stage_vector(stage_pattern, first + second);

    // Write and flush twice, settling from zero and then a page interior.
    auto memory = instance.get(instance.allocate(first));
    BC_REQUIRE(memory);
    std::copy_n(head.begin(), head.size(), memory.begin());

    memory.reset();
    BC_REQUIRE(!instance.flush());

    memory = instance.get(instance.allocate(second));
    BC_REQUIRE(memory);
    std::copy_n(tail.begin(), tail.size(), memory.begin());

    memory.reset();
    BC_REQUIRE(!instance.flush());

    // All content is readable through the settled and unsettled regions.
    memory = instance.get();
    BC_REQUIRE(memory);

    const auto end1 = std::next(memory.begin(), full.size());
    BC_REQUIRE_EQUAL_COLLECTIONS(memory.begin(), end1, full.begin(), full.end());

    memory.reset();
    BC_REQUIRE(!instance.unload());
    BC_REQUIRE(!instance.close());
    BC_REQUIRE(!instance.get_fault());

    // All content persists across close and reopen.
    map reopened(file, { 1, 50 }, true, true);
    BC_REQUIRE(!reopened.open());
    BC_REQUIRE(!reopened.load());
    BC_REQUIRE_EQUAL(reopened.size(), first + second);

    memory = reopened.get();
    BC_REQUIRE(memory);

    const auto end2 = std::next(memory.begin(), full.size());
    BC_REQUIRE_EQUAL_COLLECTIONS(memory.begin(), end2, full.begin(), full.end());

    memory.reset();
    BC_REQUIRE(!reopened.unload());
    BC_REQUIRE(!reopened.close());
    BC_REQUIRE(!reopened.get_fault());
}

BC_AUTO_TEST_CASE(mmap__staged__truncate_below_flush_rewrite__expected)
{
    constexpr size_t initial = 12'000;
    constexpr size_t retained = 5'000;
    constexpr size_t appended = 4'000;
    const std::string file = TEST_PATH;
    BC_REQUIRE(test::create(file));

    map instance(file, { 1, 50 }, true, true);
    BC_REQUIRE(!instance.open());
    BC_REQUIRE(!instance.load());

    const auto original = stage_vector(stage_pattern, initial);
    const auto replaced = stage_vector(stage_repattern, appended, retained);

    // Retained content precedes replacement content.
    auto expected = stage_vector(stage_pattern, retained);
    expected.insert(expected.end(), replaced.begin(), replaced.end());

    auto memory = instance.get(instance.allocate(initial));
    BC_REQUIRE(memory);
    std::copy_n(original.begin(), original.size(), memory.begin());

    memory.reset();
    BC_REQUIRE(!instance.flush());

    // Truncate below the flushed extent and append replacement content.
    BC_REQUIRE(instance.truncate(retained));
    BC_REQUIRE_EQUAL(instance.allocate(appended), retained);

    memory = instance.get(retained);
    BC_REQUIRE(memory);
    std::copy_n(replaced.begin(), replaced.size(), memory.begin());

    memory.reset();
    BC_REQUIRE(!instance.flush());
    BC_REQUIRE(!instance.unload());
    BC_REQUIRE(!instance.close());
    BC_REQUIRE(!instance.get_fault());

    // Retained content precedes replacement content across reopen.
    map reopened(file, { 1, 50 }, true, true);
    BC_REQUIRE(!reopened.open());
    BC_REQUIRE(!reopened.load());
    BC_REQUIRE_EQUAL(reopened.size(), retained + appended);

    memory = reopened.get();
    BC_REQUIRE(memory);

    const auto end = std::next(memory.begin(), expected.size());
    BC_REQUIRE_EQUAL_COLLECTIONS(memory.begin(), end, expected.begin(), expected.end());

    memory.reset();
    BC_REQUIRE(!reopened.unload());
    BC_REQUIRE(!reopened.close());
    BC_REQUIRE(!reopened.get_fault());
}

BC_AUTO_TEST_CASE(mmap__truncate__reallocate__zeroed)
{
    constexpr size_t initial = 12'000;
    constexpr size_t retained = 5'000;
    constexpr size_t appended = 4'000;
    const std::string file = TEST_PATH;
    BC_REQUIRE(test::create(file));

    map instance(file, { 1, 50 });
    BC_REQUIRE(!instance.open());
    BC_REQUIRE(!instance.load());

    const auto original = stage_vector(stage_repattern, initial, one);
    auto memory = instance.get(instance.allocate(initial));
    BC_REQUIRE(memory);
    std::copy_n(original.begin(), original.size(), memory.begin());

    memory.reset();
    BC_REQUIRE(instance.truncate(retained));
    BC_REQUIRE_EQUAL(instance.allocate(appended), retained);

    memory = instance.get(retained);
    BC_REQUIRE(memory);

    const system::data_chunk zeros(appended);
    const auto end = std::next(memory.begin(), appended);
    BC_REQUIRE_EQUAL_COLLECTIONS(memory.begin(), end, zeros.begin(), zeros.end());

    memory.reset();
    BC_REQUIRE(!instance.unload());
    BC_REQUIRE(!instance.close());
    BC_REQUIRE(!instance.get_fault());
}

BC_AUTO_TEST_CASE(mmap__truncate__unzeroed_reallocate__retained)
{
    constexpr size_t initial = 12'000;
    constexpr size_t retained = 5'000;
    constexpr size_t appended = 4'000;
    const std::string file = TEST_PATH;
    BC_REQUIRE(test::create(file));

    map instance(file, { .size = 1, .rate = 50, .zeroed = false });
    BC_REQUIRE(!instance.open());
    BC_REQUIRE(!instance.load());

    const auto original = stage_vector(stage_repattern, initial, one);
    auto memory = instance.get(instance.allocate(initial));
    BC_REQUIRE(memory);
    std::copy_n(original.begin(), original.size(), memory.begin());

    memory.reset();
    BC_REQUIRE(instance.truncate(retained));
    memory = instance.get_capacity(retained);
    BC_REQUIRE(memory);

    const auto released = std::next(original.begin(), retained);
    const auto released_end = std::next(released, appended);
    BC_REQUIRE_EQUAL_COLLECTIONS(memory.begin(), std::next(memory.begin(), appended), released, released_end);

    memory.reset();
    BC_REQUIRE_EQUAL(instance.allocate(appended), retained);
    memory = instance.get(retained);
    BC_REQUIRE(memory);
    BC_REQUIRE_EQUAL_COLLECTIONS(memory.begin(), std::next(memory.begin(), appended), released, released_end);

    memory.reset();
    BC_REQUIRE(!instance.unload());
    BC_REQUIRE(!instance.close());
    BC_REQUIRE(!instance.get_fault());
}

BC_AUTO_TEST_CASE(mmap__staged__truncate_below_flush_reallocate__zeroed)
{
    constexpr size_t initial = 12'000;
    constexpr size_t retained = 5'000;
    constexpr size_t appended = 6'000;
    const std::string file = TEST_PATH;
    BC_REQUIRE(test::create(file));

    map instance(file, { 1, 50 }, true, true);
    BC_REQUIRE(!instance.open());
    BC_REQUIRE(!instance.load());

    const auto original = stage_vector(stage_repattern, initial, one);
    auto memory = instance.get(instance.allocate(initial));
    BC_REQUIRE(memory);
    std::copy_n(original.begin(), original.size(), memory.begin());

    memory.reset();
    BC_REQUIRE(!instance.flush());
    BC_REQUIRE(instance.truncate(retained));
    BC_REQUIRE_EQUAL(instance.allocate(appended), retained);

    memory = instance.get(retained);
    BC_REQUIRE(memory);

    const system::data_chunk zeros(appended);
    const auto end = std::next(memory.begin(), appended);
    BC_REQUIRE_EQUAL_COLLECTIONS(memory.begin(), end, zeros.begin(), zeros.end());

    memory.reset();
    BC_REQUIRE(!instance.unload());
    BC_REQUIRE(!instance.close());
    BC_REQUIRE(!instance.get_fault());
}

#if defined(MANAGE_STAGING)

BC_AUTO_TEST_CASE(mmap__frontier__staged_out_of_order_completion__expected)
{
    const std::string file = TEST_PATH;
    BC_REQUIRE(test::create(file));

    map instance(file, { 1, 50 }, true, true);
    BC_REQUIRE(!instance.open());
    BC_REQUIRE(!instance.load());

    // Frontier holds at each extent start until its writes complete.
    const auto first = instance.allocate(100);
    const auto second = instance.allocate(50);
    BC_REQUIRE_EQUAL(instance.frontier(), first);

    // Completing the second extent does not advance past the first.
    instance.complete(second, 50);
    BC_REQUIRE_EQUAL(instance.frontier(), first);

    // Completing the first pops both, frontier reaches logical.
    instance.complete(first, 100);
    BC_REQUIRE_EQUAL(instance.frontier(), instance.size());

    BC_REQUIRE(!instance.unload());
    BC_REQUIRE(!instance.close());
    BC_REQUIRE(!instance.get_fault());
}

BC_AUTO_TEST_CASE(mmap__frontier__staged_truncate__discards_extents)
{
    const std::string file = TEST_PATH;
    BC_REQUIRE(test::create(file));

    map instance(file, { 1, 50 }, true, true);
    BC_REQUIRE(!instance.open());
    BC_REQUIRE(!instance.load());

    const auto first = instance.allocate(100);
    instance.complete(first, 100);
    const auto second = instance.allocate(50);
    BC_REQUIRE_EQUAL(instance.frontier(), second);

    // Truncation below the open extent discards it, frontier follows logical.
    BC_REQUIRE(instance.truncate(first + 100));
    BC_REQUIRE_EQUAL(instance.frontier(), instance.size());

    BC_REQUIRE(!instance.unload());
    BC_REQUIRE(!instance.close());
    BC_REQUIRE(!instance.get_fault());
}

#endif // MANAGE_STAGING

BC_AUTO_TEST_CASE(mmap__unstaged__rewrite_below_flush__expected)
{
    constexpr size_t size = 10'000;
    constexpr size_t rewrite = 100;
    const std::string file = TEST_PATH;
    BC_REQUIRE(test::create(file));

    // Unstaged (default) retains in-place rewrite of flushed content (heads).
    map instance(file, { 1, 50 }, true);
    BC_REQUIRE(!instance.open());
    BC_REQUIRE(!instance.load());

    const auto original = stage_vector(stage_pattern, size);
    const auto replaced = stage_vector(stage_repattern, rewrite);

    // Replacement content precedes retained content.
    auto expected = stage_vector(stage_repattern, rewrite);
    const auto retained = stage_vector(stage_pattern, size - rewrite, rewrite);
    expected.insert(expected.end(), retained.begin(), retained.end());

    auto memory = instance.get(instance.allocate(size));
    BC_REQUIRE(memory);
    std::copy_n(original.begin(), original.size(), memory.begin());

    // Raw unstaged mutation is marked (heads mark via their primitives).
    memory.reset();
    instance.mark(zero, size);
    BC_REQUIRE(!instance.flush());

    memory = instance.get();
    BC_REQUIRE(memory);

    std::copy_n(replaced.begin(), replaced.size(), memory.begin());
    memory.reset();
    instance.mark(zero, rewrite);
    BC_REQUIRE(!instance.flush());
    BC_REQUIRE(!instance.unload());
    BC_REQUIRE(!instance.close());
    BC_REQUIRE(!instance.get_fault());

    map reopened(file, { 1, 50 }, true);
    BC_REQUIRE(!reopened.open());
    BC_REQUIRE(!reopened.load());
    BC_REQUIRE_EQUAL(reopened.size(), size);

    memory = reopened.get();
    BC_REQUIRE(memory);

    const auto end = std::next(memory.begin(), expected.size());
    BC_REQUIRE_EQUAL_COLLECTIONS(memory.begin(), end, expected.begin(), expected.end());

    memory.reset();
    BC_REQUIRE(!reopened.unload());
    BC_REQUIRE(!reopened.close());
    BC_REQUIRE(!reopened.get_fault());
}

BC_AUTO_TEST_CASE(mmap__shared__rewrite_below_flush__expected)
{
    constexpr size_t size = 10'000;
    constexpr size_t rewrite = 100;
    const std::string file = TEST_PATH;
    BC_REQUIRE(test::create(file));

    // Shared head writes through its file mapping (no mark or transfer).
    map instance(file, { 1, 50 }, true, false, true);
    BC_REQUIRE(!instance.open());
    BC_REQUIRE(!instance.load());

    const auto original = stage_vector(stage_pattern, size);
    const auto replaced = stage_vector(stage_repattern, rewrite);

    auto expected = stage_vector(stage_repattern, rewrite);
    const auto retained = stage_vector(stage_pattern, size - rewrite, rewrite);
    expected.insert(expected.end(), retained.begin(), retained.end());

    auto memory = instance.get(instance.allocate(size));
    BC_REQUIRE(memory);
    std::copy_n(original.begin(), original.size(), memory.begin());
    memory.reset();
    BC_REQUIRE(!instance.flush());

    memory = instance.get();
    BC_REQUIRE(memory);
    std::copy_n(replaced.begin(), replaced.size(), memory.begin());
    memory.reset();
    BC_REQUIRE(!instance.flush());
    BC_REQUIRE(!instance.unload());
    BC_REQUIRE(!instance.close());
    BC_REQUIRE(!instance.get_fault());

    map reopened(file, { 1, 50 }, true, false, true);
    BC_REQUIRE(!reopened.open());
    BC_REQUIRE(!reopened.load());
    BC_REQUIRE_EQUAL(reopened.size(), size);

    memory = reopened.get();
    BC_REQUIRE(memory);

    const auto end = std::next(memory.begin(), expected.size());
    BC_REQUIRE_EQUAL_COLLECTIONS(memory.begin(), end, expected.begin(), expected.end());

    memory.reset();
    BC_REQUIRE(!reopened.unload());
    BC_REQUIRE(!reopened.close());
    BC_REQUIRE(!reopened.get_fault());
}

BC_AUTO_TEST_CASE(mmap__restage__unloaded__restage_unloaded)
{
    const std::string file = TEST_PATH;
    BC_REQUIRE(test::create(file));

    map instance(file, {}, true, true);
    BC_REQUIRE(!instance.open());
    BC_REQUIRE_EQUAL(instance.restage(false), error::restage_unloaded);
    BC_REQUIRE(!instance.close());
    BC_REQUIRE(!instance.get_fault());
}

BC_AUTO_TEST_CASE(mmap__restage__shared__restage_locked)
{
    const std::string file = TEST_PATH;
    BC_REQUIRE(test::create(file));

    map instance(file, {}, true, true);
    BC_REQUIRE(!instance.open());
    BC_REQUIRE(!instance.load());

    auto memory = instance.get(instance.allocate(1));
    BC_REQUIRE(memory);
    BC_REQUIRE_EQUAL(instance.restage(false), error::restage_locked);
    BC_REQUIRE(instance.is_loaded());

    memory.reset();
    BC_REQUIRE(!instance.restage(false));
    BC_REQUIRE(!instance.restage(true));
    BC_REQUIRE(!instance.unload());
    BC_REQUIRE(!instance.close());
    BC_REQUIRE(!instance.get_fault());
}

BC_AUTO_TEST_CASE(mmap__restage__unstaged_rewrite_below_flush__expected)
{
    constexpr size_t size = 40'000;
    constexpr size_t rewrite = 100;
    const std::string file = TEST_PATH;
    BC_REQUIRE(test::create(file));

    map instance(file, { 1, 50 }, true, true);
    BC_REQUIRE(!instance.open());
    BC_REQUIRE(!instance.load());

    const auto original = stage_vector(stage_pattern, size);
    const auto replaced = stage_vector(stage_repattern, rewrite);

    // Replacement content precedes retained content.
    auto expected = stage_vector(stage_repattern, rewrite);
    const auto retained = stage_vector(stage_pattern, size - rewrite, rewrite);
    expected.insert(expected.end(), retained.begin(), retained.end());

    auto memory = instance.get(instance.allocate(size));
    BC_REQUIRE(memory);
    std::copy_n(original.begin(), original.size(), memory.begin());

    memory.reset();
    BC_REQUIRE(!instance.flush());

    // Flushed content of a staged body is rewritable while it is unstaged.
    BC_REQUIRE(!instance.restage(false));
    BC_REQUIRE_EQUAL(instance.size(), size);

    memory = instance.get();
    BC_REQUIRE(memory);
    std::copy_n(replaced.begin(), replaced.size(), memory.begin());

    memory.reset();
    BC_REQUIRE(!instance.restage(true));
    BC_REQUIRE_EQUAL(instance.size(), size);

    memory = instance.get();
    BC_REQUIRE(memory);

    const auto end1 = std::next(memory.begin(), expected.size());
    BC_REQUIRE_EQUAL_COLLECTIONS(memory.begin(), end1, expected.begin(), expected.end());

    memory.reset();
    BC_REQUIRE(!instance.unload());
    BC_REQUIRE(!instance.close());
    BC_REQUIRE(!instance.get_fault());

    map reopened(file, { 1, 50 }, true, true);
    BC_REQUIRE(!reopened.open());
    BC_REQUIRE(!reopened.load());
    BC_REQUIRE_EQUAL(reopened.size(), size);

    memory = reopened.get();
    BC_REQUIRE(memory);

    const auto end2 = std::next(memory.begin(), expected.size());
    BC_REQUIRE_EQUAL_COLLECTIONS(memory.begin(), end2, expected.begin(), expected.end());

    memory.reset();
    BC_REQUIRE(!reopened.unload());
    BC_REQUIRE(!reopened.close());
    BC_REQUIRE(!reopened.get_fault());
}

BC_AUTO_TEST_CASE(mmap__restage__unstaged_move_down_truncate__expected)
{
    constexpr size_t size = 40'000;
    constexpr size_t kept = 15'000;
    constexpr size_t appended = 5'000;
    const std::string file = TEST_PATH;
    BC_REQUIRE(test::create(file));

    map instance(file, { 1, 50 }, true, true);
    BC_REQUIRE(!instance.open());
    BC_REQUIRE(!instance.load());

    const auto original = stage_vector(stage_pattern, size);
    const auto extended = stage_vector(stage_repattern, appended);

    // The kept tail precedes content appended once restaged.
    auto expected = stage_vector(stage_pattern, kept, size - kept);
    expected.insert(expected.end(), extended.begin(), extended.end());

    auto memory = instance.get(instance.allocate(size));
    BC_REQUIRE(memory);
    std::copy_n(original.begin(), original.size(), memory.begin());

    memory.reset();
    BC_REQUIRE(!instance.flush());
    BC_REQUIRE(!instance.restage(false));

    memory = instance.get();
    BC_REQUIRE(memory);
    std::copy_n(std::next(memory.begin(), size - kept), kept, memory.begin());

    memory.reset();
    BC_REQUIRE(instance.truncate(kept));
    BC_REQUIRE(!instance.restage(true));
    BC_REQUIRE_EQUAL(instance.size(), kept);
    BC_REQUIRE_EQUAL(instance.allocate(appended), kept);

    memory = instance.get(kept);
    BC_REQUIRE(memory);
    std::copy_n(extended.begin(), extended.size(), memory.begin());

    memory.reset();
    BC_REQUIRE(!instance.flush());
    BC_REQUIRE(!instance.unload());
    BC_REQUIRE(!instance.close());
    BC_REQUIRE(!instance.get_fault());

    map reopened(file, { 1, 50 }, true, true);
    BC_REQUIRE(!reopened.open());
    BC_REQUIRE(!reopened.load());
    BC_REQUIRE_EQUAL(reopened.size(), kept + appended);

    memory = reopened.get();
    BC_REQUIRE(memory);

    const auto end = std::next(memory.begin(), expected.size());
    BC_REQUIRE_EQUAL_COLLECTIONS(memory.begin(), end, expected.begin(), expected.end());

    memory.reset();
    BC_REQUIRE(!reopened.unload());
    BC_REQUIRE(!reopened.close());
    BC_REQUIRE(!reopened.get_fault());
}

BC_AUTO_TEST_CASE(mmap__restage__unstaged_aggregate_rewrite_below_flush__expected)
{
    // Two columns, widths 1 and 3 (stride 4), each flushed across pages.
    using storage_t = database::mmap<1, 3>;
    constexpr size_t rows = 40'000;
    constexpr size_t rewrite = 100;
    const std::string file = TEST_PATH;
    const storage_t::paths files{ file + "_0", file + "_1" };

    storage_t instance(files, { 1, 50 }, true, true);
    BC_REQUIRE(!instance.create());
    BC_REQUIRE(!instance.open());
    BC_REQUIRE(!instance.load());

    const auto original0 = stage_vector(stage_pattern, rows);
    const auto original1 = stage_vector(stage_pattern, rows * 3u);
    const auto replaced0 = stage_vector(stage_repattern, rewrite);
    const auto replaced1 = stage_vector(stage_repattern, rewrite * 3u);

    // Replacement content precedes retained content, in each column.
    auto expected0 = stage_vector(stage_repattern, rewrite);
    auto expected1 = stage_vector(stage_repattern, rewrite * 3u);
    const auto retained0 = stage_vector(stage_pattern, rows - rewrite, rewrite);
    const auto retained1 = stage_vector(stage_pattern, (rows - rewrite) * 3u, rewrite * 3u);
    expected0.insert(expected0.end(), retained0.begin(), retained0.end());
    expected1.insert(expected1.end(), retained1.begin(), retained1.end());

    BC_REQUIRE_EQUAL(instance.allocate(rows), zero);

    auto column0 = instance.get_at(0);
    auto column1 = instance.get_at(1);
    BC_REQUIRE(column0);
    BC_REQUIRE(column1);
    std::copy_n(original0.begin(), original0.size(), column0.begin());
    std::copy_n(original1.begin(), original1.size(), column1.begin());

    column0.reset();
    column1.reset();
    BC_REQUIRE(!instance.flush());
    BC_REQUIRE(!instance.restage(false));
    BC_REQUIRE_EQUAL(instance.size(), rows);

    column0 = instance.get_at(0);
    column1 = instance.get_at(1);
    BC_REQUIRE(column0);
    BC_REQUIRE(column1);
    std::copy_n(replaced0.begin(), replaced0.size(), column0.begin());
    std::copy_n(replaced1.begin(), replaced1.size(), column1.begin());

    column0.reset();
    column1.reset();
    BC_REQUIRE(!instance.restage(true));
    BC_REQUIRE(!instance.unload());
    BC_REQUIRE(!instance.close());
    BC_REQUIRE(!instance.get_fault());

    storage_t reopened(files, { 1, 50 }, true, true);
    BC_REQUIRE(!reopened.open());
    BC_REQUIRE(!reopened.load());
    BC_REQUIRE_EQUAL(reopened.size(), rows);

    column0 = reopened.get_at(0);
    column1 = reopened.get_at(1);
    BC_REQUIRE(column0);
    BC_REQUIRE(column1);

    const auto end0 = std::next(column0.begin(), expected0.size());
    const auto end1 = std::next(column1.begin(), expected1.size());
    BC_REQUIRE_EQUAL_COLLECTIONS(column0.begin(), end0, expected0.begin(), expected0.end());
    BC_REQUIRE_EQUAL_COLLECTIONS(column1.begin(), end1, expected1.begin(), expected1.end());

    column0.reset();
    column1.reset();
    BC_REQUIRE(!reopened.unload());
    BC_REQUIRE(!reopened.close());
    BC_REQUIRE(!reopened.get_fault());
}

BC_AUTO_TEST_CASE(mmap__load__shared_aggregate__columns_persist)
{
    // Two columns, widths 1 and 3 (stride 4), each mapped across pages.
    using storage_t = database::mmap<1, 3>;
    constexpr size_t rows = 40'000;
    const std::string file = TEST_PATH;
    const storage_t::paths files{ file + "_0", file + "_1" };
    const auto expected0 = stage_vector(stage_pattern, rows);
    const auto expected1 = stage_vector(stage_pattern, rows * 3u);

    storage_t instance(files, { 1, 50 }, false, false, true);
    BC_REQUIRE(!instance.create());
    BC_REQUIRE(!instance.open());
    BC_REQUIRE(!instance.load());
    BC_REQUIRE(instance.expand(rows));
    BC_REQUIRE_EQUAL(instance.size(), rows);

    auto column0 = instance.get_at(0);
    auto column1 = instance.get_at(1);
    BC_REQUIRE(column0);
    BC_REQUIRE(column1);
    std::copy_n(expected0.begin(), expected0.size(), column0.begin());
    std::copy_n(expected1.begin(), expected1.size(), column1.begin());

    column0.reset();
    column1.reset();
    BC_REQUIRE(!instance.unload());
    BC_REQUIRE(!instance.close());
    BC_REQUIRE(!instance.get_fault());
    BC_REQUIRE_EQUAL(test::size(files.at(0)), rows);
    BC_REQUIRE_EQUAL(test::size(files.at(1)), rows * 3u);

    storage_t reopened(files, { 1, 50 }, false, false, true);
    BC_REQUIRE(!reopened.open());
    BC_REQUIRE(!reopened.load());
    BC_REQUIRE_EQUAL(reopened.size(), rows);

    column0 = reopened.get_at(0);
    column1 = reopened.get_at(1);
    BC_REQUIRE(column0);
    BC_REQUIRE(column1);

    const auto end0 = std::next(column0.begin(), expected0.size());
    const auto end1 = std::next(column1.begin(), expected1.size());
    BC_REQUIRE_EQUAL_COLLECTIONS(column0.begin(), end0, expected0.begin(), expected0.end());
    BC_REQUIRE_EQUAL_COLLECTIONS(column1.begin(), end1, expected1.begin(), expected1.end());

    column0.reset();
    column1.reset();
    BC_REQUIRE(!reopened.unload());
    BC_REQUIRE(!reopened.close());
    BC_REQUIRE(!reopened.get_fault());
}

struct dump_narrow
{
    static constexpr size_t width = 1;
    static constexpr std::string_view suffix{ "narrow" };
};

struct dump_wide
{
    static constexpr size_t width = 3;
    static constexpr std::string_view suffix{ "wide" };
};

BC_AUTO_TEST_CASE(mmaps__dump__shared_aggregate__column_files)
{
    using storage_t = database::mmaps<database::mmap, dump_narrow, dump_wide>;
    constexpr size_t rows = 100;
    const std::string file = TEST_PATH;
    const std::string copy = file + "_copy";

    storage_t instance(file, { 1, 50 }, false, false, true);
    BC_REQUIRE(!instance.create());
    BC_REQUIRE(!instance.open());
    BC_REQUIRE(!instance.load());
    BC_REQUIRE(instance.expand(rows));
    BC_REQUIRE(!instance.dump(copy));
    BC_REQUIRE_EQUAL(test::size(copy + "_narrow"), rows);
    BC_REQUIRE_EQUAL(test::size(copy + "_wide"), rows * 3u);
    BC_REQUIRE(!instance.unload());
    BC_REQUIRE(!instance.close());
    BC_REQUIRE(!instance.get_fault());
}

BC_AUTO_TEST_CASE(mmap__restage__head__rewrite_expected)
{
    constexpr size_t size = 10'000;
    constexpr size_t rewrite = 100;
    const std::string file = TEST_PATH;
    BC_REQUIRE(test::create(file));

    // A head has no staging, so restages as constructed.
    map instance(file, { 1, 50 }, true);
    BC_REQUIRE(!instance.open());
    BC_REQUIRE(!instance.load());

    const auto original = stage_vector(stage_pattern, size);
    const auto replaced = stage_vector(stage_repattern, rewrite);

    auto expected = stage_vector(stage_repattern, rewrite);
    const auto retained = stage_vector(stage_pattern, size - rewrite, rewrite);
    expected.insert(expected.end(), retained.begin(), retained.end());

    auto memory = instance.get(instance.allocate(size));
    BC_REQUIRE(memory);
    std::copy_n(original.begin(), original.size(), memory.begin());

    memory.reset();
    instance.mark(zero, size);
    BC_REQUIRE(!instance.restage(false));

    memory = instance.get();
    BC_REQUIRE(memory);
    instance.prepare(zero, rewrite);
    std::copy_n(replaced.begin(), replaced.size(), memory.begin());

    memory.reset();
    instance.mark(zero, rewrite);
    BC_REQUIRE(!instance.restage(true));

    memory = instance.get();
    BC_REQUIRE(memory);

    const auto end = std::next(memory.begin(), expected.size());
    BC_REQUIRE_EQUAL_COLLECTIONS(memory.begin(), end, expected.begin(), expected.end());

    memory.reset();
    BC_REQUIRE(!instance.unload());
    BC_REQUIRE(!instance.close());
    BC_REQUIRE(!instance.get_fault());
}

#if defined(MANAGE_STAGING)

BC_AUTO_TEST_CASE(mmap__restage__unstaged__frontier_follows_size)
{
    const std::string file = TEST_PATH;
    BC_REQUIRE(test::create(file));

    map instance(file, { 1, 50 }, true, true);
    BC_REQUIRE(!instance.open());
    BC_REQUIRE(!instance.load());
    BC_REQUIRE(!instance.restage(false));

    // An unstaged claim is not held by the frontier.
    BC_REQUIRE_EQUAL(instance.allocate(100), zero);
    BC_REQUIRE_EQUAL(instance.frontier(), instance.size());
    BC_REQUIRE(!instance.unload());
    BC_REQUIRE(!instance.close());
    BC_REQUIRE(!instance.get_fault());
}

BC_AUTO_TEST_CASE(mmap__restage__staged__frontier_holds_claim)
{
    const std::string file = TEST_PATH;
    BC_REQUIRE(test::create(file));

    map instance(file, { 1, 50 }, true, true);
    BC_REQUIRE(!instance.open());
    BC_REQUIRE(!instance.load());
    BC_REQUIRE(!instance.restage(false));
    BC_REQUIRE(!instance.restage(true));

    const auto first = instance.allocate(100);
    BC_REQUIRE_EQUAL(instance.frontier(), first);

    instance.complete(first, 100);
    BC_REQUIRE_EQUAL(instance.frontier(), instance.size());
    BC_REQUIRE(!instance.unload());
    BC_REQUIRE(!instance.close());
    BC_REQUIRE(!instance.get_fault());
}

BC_AUTO_TEST_CASE(mmap__restage__unstaged_reloaded__staged)
{
    const std::string file = TEST_PATH;
    BC_REQUIRE(test::create(file));

    map instance(file, { 1, 50 }, true, true);
    BC_REQUIRE(!instance.open());
    BC_REQUIRE(!instance.load());
    BC_REQUIRE(!instance.restage(false));
    BC_REQUIRE(!instance.unload());
    BC_REQUIRE(!instance.load());

    // Unstaging lasts for the one load.
    const auto first = instance.allocate(100);
    BC_REQUIRE_EQUAL(instance.frontier(), first);

    instance.complete(first, 100);
    BC_REQUIRE(!instance.unload());
    BC_REQUIRE(!instance.close());
    BC_REQUIRE(!instance.get_fault());
}

#endif // MANAGE_STAGING

BC_AUTO_TEST_CASE(mmap__unload__shared__unload_locked)
{
    const std::string file = TEST_PATH;
    BC_REQUIRE(test::create(file));

    map instance(file);
    BC_REQUIRE(!instance.open());
    BC_REQUIRE(!instance.load());

    auto memory = instance.get(instance.allocate(1));
    BC_REQUIRE(memory);
    BC_REQUIRE_EQUAL(instance.unload(), error::unload_locked);

    memory.reset();
    BC_REQUIRE(!instance.unload());
    BC_REQUIRE(!instance.close());
    BC_REQUIRE(!instance.get_fault());
}

BC_AUTO_TEST_CASE(mmap__allocate__aggregate_remap__expected_geometry)
{
    // Two columns, widths 1 and 3 (stride 4): denomination errors that are
    // identity at width 1 are visible on the width 3 column.
    using storage_t = database::mmap<1, 3>;
    constexpr auto rows = 42_size;
    constexpr auto last = sub1(rows);
    const std::string file = TEST_PATH;
    const storage_t::paths files{ file + "_0", file + "_1" };

    // minimum 1 byte -> 1 row; zero expansion makes capacity == rows exactly.
    storage_t instance(files, { 1, 0 });
    BC_REQUIRE(!instance.create());
    BC_REQUIRE(!instance.open());
    BC_REQUIRE(!instance.load());
    BC_REQUIRE_EQUAL(instance.capacity(), 1u);

    // Row zero, written before remap (verifies preservation across remap).
    BC_REQUIRE_EQUAL(instance.allocate(1), zero);

    auto write0_column0 = instance.get_at(0, 0);
    auto write0_column1 = instance.get_at(1, 0);
    BC_REQUIRE(write0_column0);
    BC_REQUIRE(write0_column1);

    write0_column0.begin()[0] = 'a';
    write0_column1.begin()[0] = 'b';
    write0_column1.begin()[1] = 'c';
    write0_column1.begin()[2] = 'd';
    write0_column0.reset();
    write0_column1.reset();

    // Forces remap_all_ (1 -> 42 rows); macOS takes the no-mremap branch.
    BC_REQUIRE_EQUAL(instance.allocate(sub1(rows)), 1u);
    BC_REQUIRE_EQUAL(instance.size(), rows);
    BC_REQUIRE_EQUAL(instance.capacity(), rows);

    // Loaded geometry: resize_ extends each file to capacity x width. Remap
    // denomination check (unload's trim would mask an oversize).
    BC_REQUIRE_EQUAL(std::filesystem::file_size(files.front()), rows * 1u);
    BC_REQUIRE_EQUAL(std::filesystem::file_size(files.back()), rows * 3u);

    // Last row, through the remapped pointers.
    auto write1_column0 = instance.get_at(0, last * 1);
    auto write1_column1 = instance.get_at(1, last * 3);
    BC_REQUIRE(write1_column0);
    BC_REQUIRE(write1_column1);

    write1_column0.begin()[0] = 'w';
    write1_column1.begin()[0] = 'x';
    write1_column1.begin()[1] = 'y';
    write1_column1.begin()[2] = 'z';
    write1_column0.reset();
    write1_column1.reset();
    BC_REQUIRE(!instance.unload());

    // Unloaded geometry: trimmed to logical x width (trim denomination).
    BC_REQUIRE_EQUAL(std::filesystem::file_size(files.front()), rows * 1u);
    BC_REQUIRE_EQUAL(std::filesystem::file_size(files.back()), rows * 3u);

    // Round trip.
    BC_REQUIRE(!instance.load());
    BC_REQUIRE_EQUAL(instance.size(), rows);

    auto read_column0 = instance.get_at(0, 0);
    auto read_column1 = instance.get_at(1, 0);
    BC_REQUIRE(read_column0);
    BC_REQUIRE(read_column1);
    BC_REQUIRE_EQUAL(read_column0.begin()[0], 'a');
    BC_REQUIRE_EQUAL(read_column0.begin()[last], 'w');
    BC_REQUIRE_EQUAL(read_column1.begin()[0], 'b');
    BC_REQUIRE_EQUAL(read_column1.begin()[1], 'c');
    BC_REQUIRE_EQUAL(read_column1.begin()[2], 'd');
    BC_REQUIRE_EQUAL(read_column1.begin()[last * 3 + 0], 'x');
    BC_REQUIRE_EQUAL(read_column1.begin()[last * 3 + 1], 'y');
    BC_REQUIRE_EQUAL(read_column1.begin()[last * 3 + 2], 'z');

    read_column0.reset();
    read_column1.reset();
    BC_REQUIRE(!instance.unload());
    BC_REQUIRE(!instance.close());
    BC_REQUIRE(!instance.get_fault());
}

// Concurrency stress: allocations must be unique, dense claims under
// contention, with growth forced through the remap slow path.
BC_AUTO_TEST_CASE(mmap__allocate__concurrent__unique_dense_claims)
{
    constexpr size_t threads = 8;
    constexpr size_t rounds = 10'000;
    using claim = std::pair<size_t, size_t>;

    const std::string file = TEST_PATH;
    BC_REQUIRE(test::create(file));

    // Tiny minimum and modest expansion force frequent slow-path remaps.
    map instance(file, { 1, 25 });
    BC_REQUIRE(!instance.open());
    BC_REQUIRE(!instance.load());

    std::vector<std::thread> pool{};
    std::vector<size_t> identifiers(threads);
    std::array<std::vector<claim>, threads> claims{};
    std::iota(identifiers.begin(), identifiers.end(), zero);

    // Workers record claims only (varying size by thread, 1..3 bytes);
    // failures manifest in the declarative assertions below.
    std::for_each(identifiers.begin(), identifiers.end(), [&](size_t id)
    {
        pool.emplace_back([&, id]() NOEXCEPT
        {
            const auto count = add1(id % 3);
            auto& mine = claims.at(id);
            mine.reserve(rounds);
            std::generate_n(std::back_inserter(mine), rounds, [&]() NOEXCEPT
            {
                return claim{ instance.allocate(count), count };
            });
        });
    });

    std::for_each(pool.begin(), pool.end(), [](auto& thread)
    {
        thread.join();
    });

    // Aggregate and sort all claims by start.
    std::vector<claim> all{};
    all.reserve(threads * rounds);
    std::for_each(claims.begin(), claims.end(), [&](const auto& mine)
    {
        all.insert(all.end(), mine.begin(), mine.end());
    });

    std::sort(all.begin(), all.end());

    // No allocation failed.
    const auto failed = std::any_of(all.begin(), all.end(),
        [](const auto& entry) NOEXCEPT
        {
            return entry.first == storage::eof;
        });

    // Claims are unique, non-overlapping and dense from zero to size().
    const auto gapped = std::adjacent_find(all.begin(), all.end(),
        [](const auto& left, const auto& right) NOEXCEPT
        {
            return (left.first + left.second) != right.first;
        });

    BC_REQUIRE(!failed);
    BC_REQUIRE(gapped == all.end());
    BC_REQUIRE_EQUAL(all.front().first, zero);
    BC_REQUIRE_EQUAL(all.back().first + all.back().second, instance.size());
    BC_REQUIRE_GE(instance.capacity(), instance.size());

    BC_REQUIRE(!instance.unload());
    BC_REQUIRE(!instance.close());
    BC_REQUIRE(!instance.get_fault());
}

#if defined(MANAGE_STAGING)

// Share transitions are asynchronous (worker tick), so waits are bounded.
constexpr size_t drain_wait = 70;
constexpr size_t share_wait = 120;
constexpr size_t unshare_wait = 60;
constexpr size_t cell_width = sizeof(uint64_t);

static bool shared_within(const map& instance, bool state,
    size_t seconds) NOEXCEPT
{
    const std::vector<size_t> ticks(seconds);
    return std::any_of(ticks.begin(), ticks.end(), [&](size_t) NOEXCEPT
    {
        std::this_thread::sleep_for(std::chrono::seconds(one));
        return instance.shared() == state;
    });
}

// Unguarded write (hashmap head cell path).
static void stamp(map& instance, size_t cell, uint64_t generation) NOEXCEPT
{
    const auto offset = cell * cell_width;
    const auto raw = instance.get_raw(offset);
    instance.prepare(offset, cell_width);
    system::unsafe_to_little_endian<uint64_t>(raw, generation);
    instance.mark(offset, cell_width);
}

// Accessor-guarded write (array/head map paths), held across the mark.
static void stamp_held(map& instance, size_t cell, uint64_t generation) NOEXCEPT
{
    const auto offset = cell * cell_width;
    const auto memory = instance.get(offset);
    instance.prepare(offset, cell_width);
    system::unsafe_to_little_endian<uint64_t>(memory.begin(), generation);
    instance.mark(offset, cell_width);
}

static std::vector<uint64_t> read_cells(const map& instance,
    size_t cells) NOEXCEPT
{
    std::vector<size_t> positions(cells);
    std::iota(positions.begin(), positions.end(), zero);

    const auto read = [&](size_t cell) NOEXCEPT
    {
        const auto raw = instance.get_raw(cell * cell_width);
        return system::unsafe_from_little_endian<uint64_t>(raw);
    };

    std::vector<uint64_t> values(cells);
    std::transform(positions.begin(), positions.end(), values.begin(), read);
    return values;
}

// Sustained volume is repetition by definition, so writer drive iterates.
// Stamps rising generations across cells until stopped, returning last
// generation stamped into each.
static std::vector<uint64_t> drive(map& instance, size_t cells,
    const std::atomic_bool& stop, bool held=false) NOEXCEPT
{
    const auto write = held ? stamp_held : stamp;
    std::vector<uint64_t> last(cells);
    for (uint64_t generation = one; !stop.load(); ++generation)
        for (size_t cell = zero; cell < cells; ++cell)
            write(instance, cell, last.at(cell) = generation);

    return last;
}

BC_AUTO_TEST_CASE(mmap__share__not_current__unshared)
{
    const std::string file = TEST_PATH;
    BC_REQUIRE(test::create(file));

    map instance(file, { 1, 50 });
    BC_REQUIRE(!instance.open());
    BC_REQUIRE(!instance.load());
    BC_REQUIRE_NE(instance.allocate(cell_width), storage::eof);

    BC_REQUIRE(!shared_within(instance, true, drain_wait));
    BC_REQUIRE(!instance.unload());
    BC_REQUIRE(!instance.close());
    BC_REQUIRE(!instance.get_fault());
}

BC_AUTO_TEST_CASE(mmap__share__current_quiescent__shared_until_unload)
{
    const std::string file = TEST_PATH;
    BC_REQUIRE(test::create(file));

    map instance(file, { 1, 50 });
    BC_REQUIRE(!instance.open());
    BC_REQUIRE(!instance.load());
    BC_REQUIRE_NE(instance.allocate(cell_width), storage::eof);

    instance.current(true);
    BC_REQUIRE(shared_within(instance, true, share_wait));
    BC_REQUIRE(!instance.unload());
    BC_REQUIRE(!instance.shared());
    BC_REQUIRE(!instance.close());
    BC_REQUIRE(!instance.get_fault());
}

BC_AUTO_TEST_CASE(mmap__share__write_while_shared__persists)
{
    const std::string file = TEST_PATH;
    BC_REQUIRE(test::create(file));

    map instance(file, { 1, 50 });
    BC_REQUIRE(!instance.open());
    BC_REQUIRE(!instance.load());
    BC_REQUIRE_NE(instance.allocate(cell_width), storage::eof);

    instance.current(true);
    BC_REQUIRE(shared_within(instance, true, share_wait));
    stamp(instance, zero, 42);

    BC_REQUIRE(!instance.unload());
    BC_REQUIRE(!instance.close());
    BC_REQUIRE(!instance.open());
    BC_REQUIRE(!instance.load());
    BC_REQUIRE_EQUAL(read_cells(instance, one).front(), 42u);
    BC_REQUIRE(!instance.unload());
    BC_REQUIRE(!instance.close());
    BC_REQUIRE(!instance.get_fault());
}

BC_AUTO_TEST_CASE(mmap__share__sustained_writes__unshared_without_loss)
{
    constexpr size_t cells = 512;

    const std::string file = TEST_PATH;
    BC_REQUIRE(test::create(file));

    map instance(file, { 1, 50 });
    BC_REQUIRE(!instance.open());
    BC_REQUIRE(!instance.load());
    BC_REQUIRE_NE(instance.allocate(cells * cell_width), storage::eof);

    instance.current(true);
    BC_REQUIRE(shared_within(instance, true, share_wait));

    std::atomic_bool stop{};
    std::vector<uint64_t> expected{};
    std::thread writer([&]() NOEXCEPT { expected = drive(instance, cells, stop); });
    const auto unshared = shared_within(instance, false, unshare_wait);
    stop.store(true);
    writer.join();

    BC_REQUIRE(!instance.unload());
    BC_REQUIRE(!instance.close());
    BC_REQUIRE(!instance.open());
    BC_REQUIRE(!instance.load());
    BC_REQUIRE(unshared);
    BC_REQUIRE(!instance.shared());
    BC_REQUIRE(read_cells(instance, cells) == expected);
    BC_REQUIRE(!instance.unload());
    BC_REQUIRE(!instance.close());
    BC_REQUIRE(!instance.get_fault());
}

BC_AUTO_TEST_CASE(mmap__share__sustained_held_writes__unshared_without_loss)
{
    constexpr size_t cells = 512;

    const std::string file = TEST_PATH;
    BC_REQUIRE(test::create(file));

    map instance(file, { 1, 50 });
    BC_REQUIRE(!instance.open());
    BC_REQUIRE(!instance.load());
    BC_REQUIRE_NE(instance.allocate(cells * cell_width), storage::eof);

    instance.current(true);
    BC_REQUIRE(shared_within(instance, true, share_wait));

    std::atomic_bool stop{};
    std::vector<uint64_t> expected{};
    std::thread writer([&]() NOEXCEPT { expected = drive(instance, cells, stop, true); });
    const auto unshared = shared_within(instance, false, unshare_wait);
    stop.store(true);
    writer.join();

    BC_REQUIRE(!instance.unload());
    BC_REQUIRE(!instance.close());
    BC_REQUIRE(!instance.open());
    BC_REQUIRE(!instance.load());
    BC_REQUIRE(unshared);
    BC_REQUIRE(!instance.shared());
    BC_REQUIRE(read_cells(instance, cells) == expected);
    BC_REQUIRE(!instance.unload());
    BC_REQUIRE(!instance.close());
    BC_REQUIRE(!instance.get_fault());
}

BC_AUTO_TEST_CASE(mmap__share__filled_tail_above_logical__survives_share)
{
    constexpr size_t cells = 512;
    constexpr size_t tail = add1(cells);
    constexpr auto fill = system::bit_all<uint64_t>;

    const std::string file = TEST_PATH;
    BC_REQUIRE(test::create(file));

    map instance(file, { .size = 1, .rate = 50, .fill = system::bit_all<uint8_t> });
    BC_REQUIRE(!instance.open());
    BC_REQUIRE(!instance.load());
    BC_REQUIRE_NE(instance.allocate(cell_width), storage::eof);
    BC_REQUIRE(instance.get_filled(cells * cell_width, cell_width));
    BC_REQUIRE_GT(instance.capacity(), add1(tail) * cell_width);

    instance.current(true);
    BC_REQUIRE(shared_within(instance, true, share_wait));
    BC_REQUIRE(instance.get_filled(tail * cell_width, cell_width));
    BC_REQUIRE_EQUAL(read_cells(instance, add1(tail)).back(), fill);

    BC_REQUIRE(!instance.unload());
    BC_REQUIRE(!instance.close());
    BC_REQUIRE(!instance.get_fault());
}

#endif // MANAGE_STAGING

BC_POP_WARNING()

BC_AUTO_TEST_SUITE_END()
