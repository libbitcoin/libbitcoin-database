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
#include "../../test.hpp"
#include "../../mocks/chunk_storage.hpp"

BOOST_AUTO_TEST_SUITE(filter_bk_tests)

using namespace system;

BOOST_AUTO_TEST_CASE(filter_bk__put__ordered__expected)
{
    const data_chunk expected_head = base16_chunk
    (
        "000000"
        "000000"
        "010000"
        "020000"
        "030000"
        "040000"
        "ffffff"
        "ffffff"
        "ffffff"
    );
    const data_chunk closed_head = base16_chunk
    (
        "050000"
        "000000"
        "010000"
        "020000"
        "030000"
        "040000"
        "ffffff"
        "ffffff"
        "ffffff"
    );
    const data_chunk expected_body = base16_chunk
    (
        "0000000000000000000000000000000000000000000000000000000000000000"
        "0100000000000000000000000000000000000000000000000000000000000000"
        "0200000000000000000000000000000000000000000000000000000000000000"
        "0300000000000000000000000000000000000000000000000000000000000000"
        "0400000000000000000000000000000000000000000000000000000000000000"
    );

    constexpr hash_digest two_hash = from_uintx(uint256_t(2));
    constexpr hash_digest three_hash = from_uintx(uint256_t(3));
    constexpr hash_digest four_hash = from_uintx(uint256_t(4));

    const table::filter_bk::put_ref put0{ {}, null_hash };
    const table::filter_bk::put_ref put1{ {}, one_hash };
    const table::filter_bk::put_ref put2{ {}, two_hash };
    const table::filter_bk::put_ref put3{ {}, three_hash };
    const table::filter_bk::put_ref put4{ {}, four_hash };

    test::chunk_storage head_store{};
    test::chunk_storage body_store{};
    table::filter_bk instance{ head_store, body_store, 8 };
    BOOST_REQUIRE(instance.create());

    BOOST_REQUIRE(instance.put(0, put0));
    BOOST_REQUIRE(instance.put(1, put1));
    BOOST_REQUIRE(instance.put(2, put2));
    BOOST_REQUIRE(instance.put(3, put3));
    BOOST_REQUIRE(instance.put(4, put4));

    BOOST_REQUIRE_EQUAL(head_store.buffer(), expected_head);
    BOOST_REQUIRE_EQUAL(body_store.buffer(), expected_body);
    BOOST_REQUIRE(instance.close());
    BOOST_REQUIRE_EQUAL(head_store.buffer(), closed_head);

    // Logical order is unaffected by put order.
    table::filter_bk::get_head get0{};
    table::filter_bk::get_head get1{};
    table::filter_bk::get_head get2{};
    table::filter_bk::get_head get3{};
    table::filter_bk::get_head get4{};
    BOOST_REQUIRE(instance.at(0, get0));
    BOOST_REQUIRE(instance.at(1, get1));
    BOOST_REQUIRE(instance.at(2, get2));
    BOOST_REQUIRE(instance.at(3, get3));
    BOOST_REQUIRE(instance.at(4, get4));
    BOOST_REQUIRE_EQUAL(get0.head, null_hash);
    BOOST_REQUIRE_EQUAL(get1.head, one_hash);
    BOOST_REQUIRE_EQUAL(get2.head, two_hash);
    BOOST_REQUIRE_EQUAL(get3.head, three_hash);
    BOOST_REQUIRE_EQUAL(get4.head, four_hash);
}

BOOST_AUTO_TEST_CASE(filter_bk__put__disordered__expected)
{
    const data_chunk expected_head = base16_chunk
    (
        "000000"
        "010000" // ->null_hash
        "000000" // ->one_hash
        "030000" // ->two_hash
        "040000" // ->three_hash
        "020000" // ->four_hash
        "ffffff"
        "ffffff"
        "ffffff"
    );
    const data_chunk closed_head = base16_chunk
    (
        "050000"
        "010000" // ->null_hash
        "000000" // ->one_hash
        "030000" // ->two_hash
        "040000" // ->three_hash
        "020000" // ->four_hash
        "ffffff"
        "ffffff"
        "ffffff"
    );
    const data_chunk  expected_body = base16_chunk
    (
        "0100000000000000000000000000000000000000000000000000000000000000"
        "0000000000000000000000000000000000000000000000000000000000000000"
        "0400000000000000000000000000000000000000000000000000000000000000"
        "0200000000000000000000000000000000000000000000000000000000000000"
        "0300000000000000000000000000000000000000000000000000000000000000"
    );

    constexpr hash_digest two_hash = from_uintx(uint256_t(2));
    constexpr hash_digest three_hash = from_uintx(uint256_t(3));
    constexpr hash_digest four_hash = from_uintx(uint256_t(4));

    const table::filter_bk::put_ref put0{ {}, null_hash };
    const table::filter_bk::put_ref put1{ {}, one_hash };
    const table::filter_bk::put_ref put2{ {}, two_hash };
    const table::filter_bk::put_ref put3{ {}, three_hash };
    const table::filter_bk::put_ref put4{ {}, four_hash };

    test::chunk_storage head_store{};
    test::chunk_storage body_store{};
    table::filter_bk instance{ head_store, body_store, 8 };
    BOOST_REQUIRE(instance.create());

    BOOST_REQUIRE(instance.put(1, put1));
    BOOST_REQUIRE(instance.put(0, put0));
    BOOST_REQUIRE(instance.put(4, put4));
    BOOST_REQUIRE(instance.put(2, put2));
    BOOST_REQUIRE(instance.put(3, put3));

    BOOST_REQUIRE_EQUAL(head_store.buffer(), expected_head);
    BOOST_REQUIRE_EQUAL(body_store.buffer(), expected_body);
    BOOST_REQUIRE(instance.close());
    BOOST_REQUIRE_EQUAL(head_store.buffer(), closed_head);

    // Logical order is unaffected by put order.
    table::filter_bk::get_head get0{};
    table::filter_bk::get_head get1{};
    table::filter_bk::get_head get2{};
    table::filter_bk::get_head get3{};
    table::filter_bk::get_head get4{};
    BOOST_REQUIRE(instance.at(0, get0));
    BOOST_REQUIRE(instance.at(1, get1));
    BOOST_REQUIRE(instance.at(2, get2));
    BOOST_REQUIRE(instance.at(3, get3));
    BOOST_REQUIRE(instance.at(4, get4));
    BOOST_REQUIRE_EQUAL(get0.head, null_hash);
    BOOST_REQUIRE_EQUAL(get1.head, one_hash);
    BOOST_REQUIRE_EQUAL(get2.head, two_hash);
    BOOST_REQUIRE_EQUAL(get3.head, three_hash);
    BOOST_REQUIRE_EQUAL(get4.head, four_hash);
}

BOOST_AUTO_TEST_SUITE_END()
