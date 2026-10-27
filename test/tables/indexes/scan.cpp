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
#include "../../test.hpp"
#include "../../mocks/chunk_storage.hpp"

BOOST_AUTO_TEST_SUITE(scan_tests)

using namespace system;
using namespace test;

// scan (aggregate)
// ----------------------------------------------------------------------------

using scan_table = table::scan<chunk_storages>;
using scan_storage = default_storage<table::scan_storage<chunk_storages>>;

BOOST_AUTO_TEST_CASE(scan__create_verify_close__aggregate__expected)
{
    data_chunk head_file{};
    test::chunk_storage head{ head_file, test::terminal_fill };
    scan_storage body{ "body" };
    scan_table instance{ head, body, 1 };

    BOOST_REQUIRE(instance.create());
    BOOST_REQUIRE(instance.verify());
    BOOST_REQUIRE(instance.close());
}

BOOST_AUTO_TEST_CASE(scan__at__unset__terminal)
{
    data_chunk head_file{};
    test::chunk_storage head{ head_file, test::terminal_fill };
    scan_storage body{ "body" };
    scan_table instance{ head, body, 1 };
    BOOST_REQUIRE(instance.create());
    BOOST_REQUIRE(instance.at(0).is_terminal());
    BOOST_REQUIRE(instance.at(1).is_terminal());
    BOOST_REQUIRE(instance.close());
}

BOOST_AUTO_TEST_CASE(scan__push__beyond_buckets__expanded)
{
    data_chunk head_file{};
    test::chunk_storage head{ head_file, test::terminal_fill };
    scan_storage body{ "body" };
    scan_table instance{ head, body, 1 };
    BOOST_REQUIRE(instance.create());
    BOOST_REQUIRE(instance.push(7, 2));
    BOOST_REQUIRE(instance.at(0).is_terminal());
    BOOST_REQUIRE(instance.at(1).is_terminal());
    BOOST_REQUIRE_EQUAL(instance.at(2), 7u);
    BOOST_REQUIRE(instance.push(9, 2));
    BOOST_REQUIRE_EQUAL(instance.at(2), 9u);
    BOOST_REQUIRE(instance.verify());
    BOOST_REQUIRE(instance.close());
}

BOOST_AUTO_TEST_CASE(scan__put_columns__two_rows__expected)
{
    data_chunk head_file{};
    test::chunk_storage head{ head_file, test::terminal_fill };
    scan_storage body{ "body" };
    scan_table instance{ head, body, 1 };
    BOOST_REQUIRE(instance.create());

    constexpr auto compressed = base16_array
    (
        "111111111111111111111111111111111111111111111111111111111111111111"
    );

    const auto fk = instance.allocate(2);
    BOOST_REQUIRE_EQUAL(fk, 0u);

    BOOST_REQUIRE(instance.correlate.put(fk, table::scan_correlate::record{ {}, 0x11223344_u32 }));
    BOOST_REQUIRE(instance.point.put(fk, table::scan_point::put_ref{ {}, compressed }));
    BOOST_REQUIRE(instance.outputs.put(fk, table::scan_outputs::record{ {}, 0x01020304_u32, 0x060708_u32 }));
    BOOST_REQUIRE(instance.correlate.put(fk + 1, table::scan_correlate::record{ {}, 0x55667788_u32 }));
    BOOST_REQUIRE(instance.point.put(fk + 1, table::scan_point::put_ref{ {}, compressed }));
    BOOST_REQUIRE(instance.outputs.put(fk + 1, table::scan_outputs::record{ {}, 0x0a0b0c0d_u32, 0x0f1011_u32 }));

    const auto expected_correlate = base16_chunk
    (
        "44332211"
        "88776655"
    );
    const auto expected_point = base16_chunk
    (
        "111111111111111111111111111111111111111111111111111111111111111111"
        "111111111111111111111111111111111111111111111111111111111111111111"
    );
    const auto expected_outputs = base16_chunk
    (
        "04030201" "080706"
        "0d0c0b0a" "11100f"
    );

    BOOST_REQUIRE_EQUAL(body.buffers_.at(0), expected_correlate);
    BOOST_REQUIRE_EQUAL(body.buffers_.at(1), expected_point);
    BOOST_REQUIRE_EQUAL(body.buffers_.at(2), expected_outputs);

    table::scan_outputs::record outputs{};
    BOOST_REQUIRE(instance.outputs.get(fk + 1, outputs));
    BOOST_REQUIRE_EQUAL(outputs.first, 0x0a0b0c0d_u32);
    BOOST_REQUIRE_EQUAL(outputs.outputs, 0x0f1011_u32);
    BOOST_REQUIRE(instance.close());
}

// scan_prefix
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(scan_prefix__put_refs__three__expected)
{
    test::chunk_storage head{ test::terminal_fill };
    test::chunk_storage body{};
    table::scan_prefix instance{ head, body };
    BOOST_REQUIRE(instance.create());

    const std::vector<uint64_t> prefixes
    {
        0x1111111111111111_u64,
        0x2222222222222222_u64,
        0x3333333333333333_u64
    };

    const auto fk = instance.allocate(3);
    BOOST_REQUIRE_EQUAL(fk, 0u);
    BOOST_REQUIRE(instance.put(fk, table::scan_prefix::put_refs{ {}, prefixes }));

    const auto expected = base16_chunk
    (
        "1111111111111111"
        "2222222222222222"
        "3333333333333333"
    );

    BOOST_REQUIRE_EQUAL(body.buffer(), expected);
    BOOST_REQUIRE(instance.close());
}

BOOST_AUTO_TEST_CASE(scan_prefix__put_span__two__expected)
{
    test::chunk_storage head{ test::terminal_fill };
    test::chunk_storage body{};
    table::scan_prefix instance{ head, body };
    BOOST_REQUIRE(instance.create());

    const std::vector<table::scan_prefix::bytes> prefixes
    {
        base16_array("0102030405060708"),
        base16_array("1112131415161718")
    };

    const auto fk = instance.allocate(2);
    BOOST_REQUIRE_EQUAL(fk, 0u);
    BOOST_REQUIRE(instance.put(fk, table::scan_prefix::put_span{ {}, prefixes }));

    const auto expected = base16_chunk
    (
        "0102030405060708"
        "1112131415161718"
    );

    BOOST_REQUIRE_EQUAL(body.buffer(), expected);
    BOOST_REQUIRE(instance.close());
}

BOOST_AUTO_TEST_SUITE_END()
