/**
 * Copyright (c) 2011-2017 libbitcoin developers (see AUTHORS)
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
#include <boost/test/unit_test.hpp>

#include <bitcoin/database.hpp>
#include "../utility/storage.hpp"

using namespace bc;
using namespace bc::database;

BOOST_AUTO_TEST_SUITE(record_manager_tests)

BOOST_AUTO_TEST_CASE(record_manager__construct__one_record__expected)
{
    typedef uint64_t link_type;

    test::storage file;
    BOOST_REQUIRE(file.open());

    const auto record_size = 10u;
    const auto link_size = sizeof(link_type);
    record_manager<link_type> manager(file, 0, record_size);
    BOOST_REQUIRE(manager.create());
    BOOST_REQUIRE(manager.start());

    const auto link1 = manager.allocate(1);
    BOOST_REQUIRE_EQUAL(link1, 0u);

    BOOST_REQUIRE_GE(file.size(), link_size + 1 * record_size);

    manager.commit();
}

BOOST_AUTO_TEST_CASE(record_manager__construct__one_record_offset__expected)
{
    typedef uint64_t link_type;

    test::storage file;
    BOOST_REQUIRE(file.open());

    const auto offset = 42;
    const auto record_size = 10u;
    const auto link_size = sizeof(link_type);
    record_manager<link_type> manager(file, offset, record_size);
    BOOST_REQUIRE(manager.create());

    const auto link1 = manager.allocate(1);
    BOOST_REQUIRE_EQUAL(link1, 0u);

    BOOST_REQUIRE_GE(file.size(), offset + link_size + 1 * record_size);
}

BOOST_AUTO_TEST_CASE(record_manager__allocate__two_records__expected)
{
    typedef uint64_t link_type;

    test::storage file;
    BOOST_REQUIRE(file.open());

    const auto record_size = 10u;
    const auto link_size = sizeof(link_type);
    record_manager<link_type> manager(file, 0, record_size);
    BOOST_REQUIRE(manager.create());

    const auto link1 = manager.allocate(1);
    BOOST_REQUIRE_EQUAL(link1, 0u);

    const auto link2 = manager.allocate(1);
    BOOST_REQUIRE_EQUAL(link2, 1u);

    BOOST_REQUIRE_GE(file.size(), link_size + 2 * record_size);
}

BOOST_AUTO_TEST_CASE(record_manager__count__multiple_records_with_offset__expected)
{
    typedef uint64_t link_type;

    test::storage file;
    BOOST_REQUIRE(file.open());

    const auto record_size = 10u;
    // const auto link_size = sizeof(link_type);
    record_manager<link_type> manager(file, 42, record_size);
    BOOST_REQUIRE(manager.create());

    const auto records1 = 24u;
    const auto records2 = 42u;
    manager.allocate(records1);
    manager.allocate(records2);
    BOOST_REQUIRE_EQUAL(manager.count(), records1 + records2);
}

BOOST_AUTO_TEST_CASE(record_manager__set_count__multiple_records_with_offset__expected)
{
    typedef uint32_t link_type;

    test::storage file;
    BOOST_REQUIRE(file.open());

    const auto record_size = 10u;
    // const auto link_size = sizeof(link_type);
    record_manager<link_type> manager(file, 42, record_size);
    BOOST_REQUIRE(manager.create());

    manager.allocate(24u);
    manager.allocate(42u);
    const auto expected = 9u;
    manager.set_count(expected);
    BOOST_REQUIRE_EQUAL(manager.count(), expected);
}

BOOST_AUTO_TEST_CASE(record_manager__get__read_write__expected)
{
    test::storage file;
    BOOST_REQUIRE(file.open());

    uint64_t value = 0x0102030405060708;
    record_manager<uint32_t> manager(file, 0, sizeof(value));
    BOOST_REQUIRE(manager.create());

    const auto link1 = manager.allocate(1);
    auto memory = manager.get(link1);
    auto serial1 = make_unsafe_serializer(memory->buffer());
    serial1.write_little_endian(value + 0);
    memory.reset();

    const auto link2 = manager.allocate(1);
    memory = manager.get(link2);
    auto serial2 = make_unsafe_serializer(memory->buffer());
    serial2.write_little_endian(value + 1);
    memory.reset();

    const auto link3 = manager.allocate(1);
    memory = manager.get(link3);
    auto serial3 = make_unsafe_serializer(memory->buffer());
    serial3.write_little_endian(value + 2);
    memory.reset();

    memory = manager.get(link1);
    auto deserial1 = make_unsafe_deserializer(memory->buffer());
    BOOST_REQUIRE_EQUAL(deserial1.template read_little_endian<uint64_t>(), value + 0);
    memory.reset();

    memory = manager.get(link2);
    auto deserial2 = make_unsafe_deserializer(memory->buffer());
    BOOST_REQUIRE_EQUAL(deserial2.template read_little_endian<uint64_t>(), value + 1);
    memory.reset();

    memory = manager.get(link3);
    auto deserial3 = make_unsafe_deserializer(memory->buffer());
    BOOST_REQUIRE_EQUAL(deserial3.template read_little_endian<uint64_t>(), value + 2);
    memory.reset();
}

BOOST_AUTO_TEST_SUITE_END()
