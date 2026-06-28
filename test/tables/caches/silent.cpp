/**
 * Copyright (c) 2011-2026 libbitcoin developers (see AUTHORS)
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

BOOST_AUTO_TEST_SUITE(silent_tests)

using namespace system;
using namespace test;

// silent (aggregate)
// ----------------------------------------------------------------------------

using silent_table = table::silent<chunk_storage>;
using silent_storage = default_storage<table::silent_storage<chunk_storage>>;

BOOST_AUTO_TEST_CASE(silent__create_verify_close__aggregate__expected)
{
    silent_storage head{ "head" };
    silent_storage body{ "body" };
    silent_table instance{ head, body };

    BOOST_REQUIRE(instance.create());
    BOOST_REQUIRE(instance.verify());
    BOOST_REQUIRE(instance.close());
}

BOOST_AUTO_TEST_CASE(silent__put_columns__three_rows__expected)
{
    silent_storage head{ "head" };
    silent_storage body{ "body" };
    silent_table instance{ head, body };
    BOOST_REQUIRE(instance.create());

    constexpr auto rows = 3_size;
    constexpr auto tx_fk = 0x11223344_u32;
    constexpr auto compressed = base16_array
    (
        "111111111111111111111111111111111111111111111111111111111111111111"
    );

    const std::vector<uint64_t> prefixes
    {
        0x1111111111111111_u64,
        0x2222222222222222_u64,
        0x3333333333333333_u64
    };

    // Allocate correlate rows for one tx_fk, returns base fk.
    silent_link fk{};
    const table::silent_correlate::records correlate{ {}, rows, tx_fk };
    BOOST_REQUIRE(instance.correlate.put_link(fk, correlate));
    BOOST_REQUIRE_EQUAL(fk, 0u);

    // Expand subordinate columns to match correlate row count.
    BOOST_REQUIRE(instance.prefix.expand(fk + rows));
    BOOST_REQUIRE(instance.compressed.expand(fk + rows));

    const table::silent_prefix::put_ref prefix{ {}, prefixes };
    const table::silent_compressed::put_ref compress{ {}, rows, compressed };
    BOOST_REQUIRE(instance.prefix.put(fk, prefix));
    BOOST_REQUIRE(instance.compressed.put(fk, compress));

    const auto expected_correlate = base16_chunk
    (
        "44332211"
        "44332211"
        "44332211"
    );
    const auto expected_prefix = base16_chunk
    (
        "1111111111111111"
        "2222222222222222"
        "3333333333333333"
    );
    const auto expected_compressed = base16_chunk
    (
        "111111111111111111111111111111111111111111111111111111111111111111"
        "111111111111111111111111111111111111111111111111111111111111111111"
        "111111111111111111111111111111111111111111111111111111111111111111"
    );

    BOOST_REQUIRE_EQUAL(body.correlate.buffer(), expected_correlate);
    BOOST_REQUIRE_EQUAL(body.prefix.buffer(), expected_prefix);
    BOOST_REQUIRE_EQUAL(body.compressed.buffer(), expected_compressed);
    BOOST_REQUIRE(instance.close());
}

BOOST_AUTO_TEST_SUITE_END()
