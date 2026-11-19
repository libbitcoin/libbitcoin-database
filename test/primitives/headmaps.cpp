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
#include "../mocks/chunk_storage.hpp"

BC_AUTO_TEST_SUITE(headmaps_tests)

using namespace system;

// Column zero is a 4 byte cell, column one a 2 byte cell.
using link3 = linkage<3>;
using table = headmaps<link3, sizeof(uint32_t), sizeof(uint16_t)>;
using head_storages = test::chunk_storages<sizeof(uint32_t), sizeof(uint16_t)>;
static const head_storages::paths head_paths{ "wide", "narrow" };
static const database::storage_settings filled{ .fill = bit_all<uint8_t> };

struct link_schema
{
    using link = link3;
};

struct wide_column
{
    static constexpr size_t width = sizeof(uint32_t);
};

struct narrow_column
{
    static constexpr size_t width = sizeof(uint16_t);
};

static_assert(table::columns == 2u);
static_assert(table::widths.at(0) == sizeof(uint32_t));
static_assert(table::widths.at(1) == sizeof(uint16_t));
static_assert(is_same_type<table::value<0>, uint32_t>);
static_assert(is_same_type<table::value<1>, uint16_t>);
static_assert(is_same_type<head_maps<link_schema, wide_column, narrow_column>, table>);

// Replaces a column zero cell.
struct wide
{
    static constexpr size_t size = sizeof(uint32_t);

    bool from_data(database::reader& source) NOEXCEPT
    {
        source.skip_bytes(size);
        return source;
    }

    bool to_data(database::flipper& sink) const NOEXCEPT
    {
        sink.write_little_endian<uint32_t>(value);
        return sink;
    }

    uint32_t value{};
};

// Replaces a column one cell.
struct narrow
{
    static constexpr size_t size = sizeof(uint16_t);

    bool from_data(database::reader& source) NOEXCEPT
    {
        source.skip_bytes(size);
        return source;
    }

    bool to_data(database::flipper& sink) const NOEXCEPT
    {
        sink.write_little_endian<uint16_t>(value);
        return sink;
    }

    uint16_t value{};
};

// Increments a column zero cell.
struct increment
{
    static constexpr size_t size = sizeof(uint32_t);

    bool from_data(database::reader& source) NOEXCEPT
    {
        value = source.read_little_endian<uint32_t>();
        return source;
    }

    bool to_data(database::flipper& sink) const NOEXCEPT
    {
        sink.write_little_endian<uint32_t>(add1(value));
        return sink;
    }

    uint32_t value{};
};

// Fails to read the current cell.
struct unreadable
{
    static constexpr size_t size = sizeof(uint32_t);

    bool from_data(database::reader&) NOEXCEPT
    {
        return false;
    }

    bool to_data(database::flipper&) const NOEXCEPT
    {
        return true;
    }
};

BC_AUTO_TEST_CASE(headmaps__construct__empty__zero_sizes)
{
    head_storages head_store{ head_paths, filled };
    const table instance{ head_store };
    BC_REQUIRE_EQUAL(instance.count(), 0u);
    BC_REQUIRE(is_zero(instance.head_size()));
    BC_REQUIRE(is_zero(instance.body_size()));
    BC_REQUIRE(instance.verify());
    BC_REQUIRE(!instance.get_fault());
    BC_REQUIRE(is_zero(instance.get_space()));
}

BC_AUTO_TEST_CASE(headmaps__create__empty__true)
{
    head_storages head_store{ head_paths, filled };
    table instance{ head_store };
    BC_REQUIRE(instance.create());
    BC_REQUIRE_EQUAL(instance.count(), 0u);
}

BC_AUTO_TEST_CASE(headmaps__create__nonempty__false)
{
    head_storages head_store{ head_paths, filled };
    table instance{ head_store };
    BC_REQUIRE(instance.expand(1u));
    BC_REQUIRE(!instance.create());
}

BC_AUTO_TEST_CASE(headmaps__lifecycle__always__true)
{
    head_storages head_store{ head_paths, filled };
    table instance{ head_store };
    BC_REQUIRE(instance.close());
    BC_REQUIRE(instance.backup());
    BC_REQUIRE(instance.backup(true));
    BC_REQUIRE(instance.restore());
    BC_REQUIRE(!instance.reload());
}

BC_AUTO_TEST_CASE(headmaps__expand__count__filled_rows)
{
    head_storages head_store{ head_paths, filled };
    table instance{ head_store };
    BC_REQUIRE(instance.expand(2u));
    BC_REQUIRE_EQUAL(instance.count(), 2u);
    BC_REQUIRE_EQUAL(instance.head_size(), 2u * (sizeof(uint32_t) + sizeof(uint16_t)));
    BC_REQUIRE_EQUAL(head_store.buffers_.at(0), base16_chunk("ffffffff" "ffffffff"));
    BC_REQUIRE_EQUAL(head_store.buffers_.at(1), base16_chunk("ffff" "ffff"));
}

BC_AUTO_TEST_CASE(headmaps__expand__lesser__unchanged)
{
    head_storages head_store{ head_paths, filled };
    table instance{ head_store };
    BC_REQUIRE(instance.expand(3u));
    BC_REQUIRE(instance.expand(1u));
    BC_REQUIRE_EQUAL(instance.count(), 3u);
}

BC_AUTO_TEST_CASE(headmaps__expand__terminal__false)
{
    head_storages head_store{ head_paths, filled };
    table instance{ head_store };
    BC_REQUIRE(!instance.expand(link3::terminal));
}

BC_AUTO_TEST_CASE(headmaps__truncate__lesser__reduced)
{
    head_storages head_store{ head_paths, filled };
    table instance{ head_store };
    BC_REQUIRE(instance.expand(3u));
    BC_REQUIRE(instance.truncate(1u));
    BC_REQUIRE_EQUAL(instance.count(), 1u);
}

BC_AUTO_TEST_CASE(headmaps__truncate__greater__false)
{
    head_storages head_store{ head_paths, filled };
    table instance{ head_store };
    BC_REQUIRE(instance.expand(1u));
    BC_REQUIRE(!instance.truncate(2u));
    BC_REQUIRE_EQUAL(instance.count(), 1u);
}

BC_AUTO_TEST_CASE(headmaps__truncate__terminal__false)
{
    head_storages head_store{ head_paths, filled };
    table instance{ head_store };
    BC_REQUIRE(!instance.truncate(link3::terminal));
}

BC_AUTO_TEST_CASE(headmaps__drop__nonempty__zero_count)
{
    head_storages head_store{ head_paths, filled };
    table instance{ head_store };
    BC_REQUIRE(instance.expand(3u));
    BC_REQUIRE(instance.drop());
    BC_REQUIRE_EQUAL(instance.count(), 0u);
}

BC_AUTO_TEST_CASE(headmaps__at__unallocated__default)
{
    head_storages head_store{ head_paths, filled };
    table instance{ head_store };
    BC_REQUIRE_EQUAL(instance.at<0>(0u), 0u);
    BC_REQUIRE_EQUAL(instance.at<1>(0u), 0u);
    BC_REQUIRE(instance.expand(1u));
    BC_REQUIRE_EQUAL(instance.at<0>(1u), 0u);
    BC_REQUIRE_EQUAL(instance.at<1>(1u), 0u);
}

BC_AUTO_TEST_CASE(headmaps__at__allocated_unset__fill)
{
    head_storages head_store{ head_paths, filled };
    table instance{ head_store };
    BC_REQUIRE(instance.expand(1u));
    BC_REQUIRE_EQUAL(instance.at<0>(0u), 0xffffffffu);
    BC_REQUIRE_EQUAL(instance.at<1>(0u), 0xffffu);
}

BC_AUTO_TEST_CASE(headmaps__set__column_zero__extends_all_columns)
{
    head_storages head_store{ head_paths, filled };
    table instance{ head_store };
    BC_REQUIRE(instance.set<0>(2u, wide{ 0x04030201 }));
    BC_REQUIRE_EQUAL(instance.count(), 3u);
    BC_REQUIRE_EQUAL(instance.at<0>(0u), 0xffffffffu);
    BC_REQUIRE_EQUAL(instance.at<0>(2u), 0x04030201u);
    BC_REQUIRE_EQUAL(instance.at<1>(2u), 0xffffu);
    BC_REQUIRE_EQUAL(head_store.buffers_.at(0), base16_chunk("ffffffff" "ffffffff" "01020304"));
    BC_REQUIRE_EQUAL(head_store.buffers_.at(1), base16_chunk("ffff" "ffff" "ffff"));
}

BC_AUTO_TEST_CASE(headmaps__set__column_one__extends_all_columns)
{
    head_storages head_store{ head_paths, filled };
    table instance{ head_store };
    BC_REQUIRE(instance.set<1>(1u, narrow{ 0x0201 }));
    BC_REQUIRE_EQUAL(instance.count(), 2u);
    BC_REQUIRE_EQUAL(instance.at<1>(1u), 0x0201u);
    BC_REQUIRE_EQUAL(instance.at<0>(1u), 0xffffffffu);
    BC_REQUIRE_EQUAL(head_store.buffers_.at(0), base16_chunk("ffffffff" "ffffffff"));
    BC_REQUIRE_EQUAL(head_store.buffers_.at(1), base16_chunk("ffff" "0102"));
}

BC_AUTO_TEST_CASE(headmaps__set__both_columns__independent)
{
    head_storages head_store{ head_paths, filled };
    table instance{ head_store };
    BC_REQUIRE(instance.set<0>(0u, wide{ 0x0000002a }));
    BC_REQUIRE(instance.set<1>(0u, narrow{ 0x0007 }));
    BC_REQUIRE_EQUAL(instance.count(), 1u);
    BC_REQUIRE_EQUAL(instance.at<0>(0u), 0x0000002au);
    BC_REQUIRE_EQUAL(instance.at<1>(0u), 0x0007u);
}

BC_AUTO_TEST_CASE(headmaps__set__replace__last_value)
{
    head_storages head_store{ head_paths, filled };
    table instance{ head_store };
    BC_REQUIRE(instance.set<0>(0u, wide{ 0xffffffff }));
    BC_REQUIRE(instance.set<0>(0u, wide{ 0x00000007 }));
    BC_REQUIRE_EQUAL(instance.count(), 1u);
    BC_REQUIRE_EQUAL(instance.at<0>(0u), 7u);
}

BC_AUTO_TEST_CASE(headmaps__set__reads_current__incremented)
{
    head_storages head_store{ head_paths, filled };
    table instance{ head_store };
    BC_REQUIRE(instance.set<0>(0u, wide{ 41u }));
    BC_REQUIRE(instance.set<0>(0u, increment{}));
    BC_REQUIRE_EQUAL(instance.at<0>(0u), 42u);
}

BC_AUTO_TEST_CASE(headmaps__set__unreadable__false_unchanged)
{
    head_storages head_store{ head_paths, filled };
    table instance{ head_store };
    BC_REQUIRE(instance.set<0>(0u, wide{ 41u }));
    BC_REQUIRE(!instance.set<0>(0u, unreadable{}));
    BC_REQUIRE_EQUAL(instance.at<0>(0u), 41u);
}

BC_AUTO_TEST_CASE(headmaps__set__terminal__false)
{
    head_storages head_store{ head_paths, filled };
    table instance{ head_store };
    BC_REQUIRE(!instance.set<0>(link3::terminal, wide{ 1u }));
    BC_REQUIRE_EQUAL(instance.count(), 0u);
}

BC_AUTO_TEST_CASE(headmaps__set__accessor_allocated__expected)
{
    head_storages head_store{ head_paths, filled };
    table instance{ head_store };
    BC_REQUIRE(instance.expand(2u));

    const auto guard = instance.get_memory();
    BC_REQUIRE(guard);
    BC_REQUIRE(instance.set<0>(guard, 1u, wide{ 9u }));
    BC_REQUIRE(instance.set<1>(guard, 1u, narrow{ 7u }));
    BC_REQUIRE_EQUAL(instance.at<0>(guard, 1u), 9u);
    BC_REQUIRE_EQUAL(instance.at<1>(guard, 1u), 7u);
    BC_REQUIRE_EQUAL(instance.at<1>(guard, 0u), 0xffffu);
    BC_REQUIRE_EQUAL(instance.count(), 2u);
}

BC_AUTO_TEST_CASE(headmaps__set__accessor_unallocated__false)
{
    head_storages head_store{ head_paths, filled };
    table instance{ head_store };
    BC_REQUIRE(instance.expand(2u));

    const auto guard = instance.get_memory();
    BC_REQUIRE(guard);
    BC_REQUIRE(!instance.set<0>(guard, 2u, wide{ 9u }));
    BC_REQUIRE_EQUAL(instance.at<0>(guard, 2u), 0u);
    BC_REQUIRE_EQUAL(instance.count(), 2u);
}

BC_AUTO_TEST_CASE(headmaps__set__null_accessor__false)
{
    head_storages head_store{ head_paths, filled };
    table instance{ head_store };
    BC_REQUIRE(instance.expand(1u));
    BC_REQUIRE(!instance.set<0>(memory{}, 0u, wide{ 9u }));
    BC_REQUIRE_EQUAL(instance.at<0>(memory{}, 0u), 0u);
}

BC_AUTO_TEST_CASE(headmaps__get_raw__empty__null)
{
    head_storages head_store{ head_paths, filled };
    const table instance{ head_store };
    BC_REQUIRE(is_null(instance.get_raw<0>()));
    BC_REQUIRE(is_null(instance.get_raw<1>()));
}

BC_AUTO_TEST_CASE(headmaps__get_raw__nonempty__column_bases)
{
    head_storages head_store{ head_paths, filled };
    table instance{ head_store };
    BC_REQUIRE(instance.expand(1u));
    BC_REQUIRE(instance.get_raw<0>() == head_store.buffers_.at(0).data());
    BC_REQUIRE(instance.get_raw<1>() == head_store.buffers_.at(1).data());
}

BC_AUTO_TEST_CASE(head_column__at_set__column_one__forwarded)
{
    head_storages head_store{ head_paths, filled };
    table instance{ head_store };
    head_column<table, 1> column{ instance };
    BC_REQUIRE(column.set(1u, narrow{ 5u }));
    BC_REQUIRE_EQUAL(column.at(1u), 5u);
    BC_REQUIRE_EQUAL(column.at(0u), 0xffffu);
    BC_REQUIRE_EQUAL(instance.at<0>(1u), 0xffffffffu);
    BC_REQUIRE(column.get_raw() == head_store.buffers_.at(1).data());
}

BC_AUTO_TEST_CASE(head_column__at_set__accessor__forwarded)
{
    head_storages head_store{ head_paths, filled };
    table instance{ head_store };
    head_column<table, 0> column{ instance };
    BC_REQUIRE(instance.expand(1u));

    const auto guard = instance.get_memory();
    BC_REQUIRE(column.set(guard, 0u, wide{ 3u }));
    BC_REQUIRE_EQUAL(column.at(guard, 0u), 3u);
    BC_REQUIRE(!column.set(guard, 1u, wide{ 3u }));
}

BC_AUTO_TEST_SUITE_END()
