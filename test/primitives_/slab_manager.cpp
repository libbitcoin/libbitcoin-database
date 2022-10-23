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
#include "../utility/storage.hpp"

BOOST_AUTO_TEST_SUITE(slab_manager_tests)

BOOST_AUTO_TEST_CASE(slab_manager__construct__one_slab__expected)
{
    typedef uint64_t link_type;

    test::storage file;
    BOOST_REQUIRE(file.load());

    const auto link_size = sizeof(link_type);
    slab_manager<link_type> manager(file, 0);
    BOOST_REQUIRE(manager.create());
    BOOST_REQUIRE(manager.start());

    const auto slab1_size = 100;
    const auto link1 = manager.allocate(slab1_size);
    BOOST_REQUIRE_EQUAL(link1, link_size);

    BOOST_REQUIRE_GE(file.capacity(), link_size + slab1_size);

    manager.commit();
}

BOOST_AUTO_TEST_CASE(slab_manager__construct__one_slab_offset__expected)
{
    typedef uint64_t link_type;

    test::storage file;
    BOOST_REQUIRE(file.load());

    const auto offset = 42;
    const auto link_size = sizeof(link_type);
    slab_manager<link_type> manager(file, offset);
    BOOST_REQUIRE(manager.create());

    const auto slab1_size = 100;
    const auto link1 = manager.allocate(slab1_size);
    BOOST_REQUIRE_EQUAL(link1, link_size);

    BOOST_REQUIRE_GE(file.capacity(), offset + link_size + slab1_size);
}

BOOST_AUTO_TEST_CASE(slab_manager__allocate__two_slabs__expected)
{
    typedef uint64_t link_type;

    test::storage file;
    BOOST_REQUIRE(file.load());

    const auto link_size = sizeof(link_type);
    slab_manager<link_type> manager(file, 0);
    BOOST_REQUIRE(manager.create());

    const auto slab1_size = 100;
    const auto link1 = manager.allocate(slab1_size);
    BOOST_REQUIRE_EQUAL(link1, link_size);

    const auto slab2_size = 42;
    const auto link2 = manager.allocate(slab2_size);
    BOOST_REQUIRE_EQUAL(link2, link_size + slab1_size);

    BOOST_REQUIRE_GE(file.capacity(), link_size + slab1_size + slab2_size);
}

BOOST_AUTO_TEST_CASE(slab_manager__payload_size__one_slab_with_offset__expected)
{
    typedef uint32_t link_type;

    test::storage file;
    BOOST_REQUIRE(file.load());

    const auto link_size = sizeof(link_type);
    slab_manager<link_type> manager(file, 42);
    BOOST_REQUIRE(manager.create());

    const auto slab1_size = 100;
    manager.allocate(slab1_size);
    BOOST_REQUIRE_EQUAL(manager.payload_size(), link_size + slab1_size);
}

BOOST_AUTO_TEST_CASE(slab_manager__get__read_write__expected)
{
    test::storage file;
    BOOST_REQUIRE(file.load());

    uint64_t value = 0x0102030405060708;
    slab_manager<uint32_t> manager(file, 0);
    BOOST_REQUIRE(manager.create());

    const auto link1 = manager.allocate(sizeof(value));
    auto memory = manager.get(link1);
    system::unsafe_to_little_endian(memory->data(), value + 0);
    memory.reset();

    const auto link2 = manager.allocate(sizeof(value));
    memory = manager.get(link2);
    system::unsafe_to_little_endian(memory->data(), value + 1);
    memory.reset();

    const auto link3 = manager.allocate(sizeof(value));
    memory = manager.get(link3);
    system::unsafe_to_little_endian(memory->data(), value + 2);
    memory.reset();

    memory = manager.get(link1);
    BOOST_REQUIRE_EQUAL(system::unsafe_from_little_endian<uint64_t>(memory->data()), value + 0);
    memory.reset();

    memory = manager.get(link2);
    BOOST_REQUIRE_EQUAL(system::unsafe_from_little_endian<uint64_t>(memory->data()), value + 1);
    memory.reset();

    memory = manager.get(link3);
    BOOST_REQUIRE_EQUAL(system::unsafe_from_little_endian<uint64_t>(memory->data()), value + 2);
    memory.reset();
}

BOOST_AUTO_TEST_SUITE_END()
