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

BOOST_AUTO_TEST_SUITE(filter_tx_tests)

using namespace system;

BOOST_AUTO_TEST_CASE(filter_tx__put__ordered__expected)
{
    const data_chunk expected_head = base16_chunk
    (
        "0000000000"
        "0000000000"
        "2100000000"
        "4200000000"
        "6300000000"
        "8400000000"
        "ffffffffff"
        "ffffffffff"
        "ffffffffff"
    );
    const data_chunk closed_head = base16_chunk
    (
        "a500000000"
        "0000000000"
        "2100000000"
        "4200000000"
        "6300000000"
        "8400000000"
        "ffffffffff"
        "ffffffffff"
        "ffffffffff"
    );
    const data_chunk expected_body = base16_chunk
    (
        "20""0000000000000000000000000000000000000000000000000000000000000000"
        "20""0100000000000000000000000000000000000000000000000000000000000000"
        "20""0200000000000000000000000000000000000000000000000000000000000000"
        "20""0300000000000000000000000000000000000000000000000000000000000000"
        "20""0400000000000000000000000000000000000000000000000000000000000000"
    );

    // These are just blob values, size doesn't have to be the same.
    const auto zero_data = base16_chunk("0000000000000000000000000000000000000000000000000000000000000000");
    const auto one_data = base16_chunk("0100000000000000000000000000000000000000000000000000000000000000");
    const auto two_data = base16_chunk("0200000000000000000000000000000000000000000000000000000000000000");
    const auto three_data = base16_chunk("0300000000000000000000000000000000000000000000000000000000000000");
    const auto four_data = base16_chunk("0400000000000000000000000000000000000000000000000000000000000000");

    const table::filter_tx::put_ref put0{ {}, zero_data };
    const table::filter_tx::put_ref put1{ {}, one_data };
    const table::filter_tx::put_ref put2{ {}, two_data };
    const table::filter_tx::put_ref put3{ {}, three_data };
    const table::filter_tx::put_ref put4{ {}, four_data };

    test::chunk_storage head_store{};
    test::chunk_storage body_store{};
    table::filter_tx instance{ head_store, body_store, 8 };
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
    table::filter_tx::get_filter get0{};
    table::filter_tx::get_filter get1{};
    table::filter_tx::get_filter get2{};
    table::filter_tx::get_filter get3{};
    table::filter_tx::get_filter get4{};
    BOOST_REQUIRE(instance.at(0, get0));
    BOOST_REQUIRE(instance.at(1, get1));
    BOOST_REQUIRE(instance.at(2, get2));
    BOOST_REQUIRE(instance.at(3, get3));
    BOOST_REQUIRE(instance.at(4, get4));
    BOOST_REQUIRE_EQUAL(get0.filter, zero_data);
    BOOST_REQUIRE_EQUAL(get1.filter, one_data);
    BOOST_REQUIRE_EQUAL(get2.filter, two_data);
    BOOST_REQUIRE_EQUAL(get3.filter, three_data);
    BOOST_REQUIRE_EQUAL(get4.filter, four_data);
}

BOOST_AUTO_TEST_CASE(filter_tx__put__disordered__expected)
{
    const data_chunk expected_head = base16_chunk
    (
        "0000000000"
        "2100000000" // ->zero_data
        "0000000000" // ->one_data
        "6300000000" // ->two_data
        "8400000000" // ->three_data
        "4200000000" // ->four_data
        "ffffffffff"
        "ffffffffff"
        "ffffffffff"
    );
    const data_chunk closed_head = base16_chunk
    (
        "a500000000"
        "2100000000" // ->zero_data
        "0000000000" // ->one_data
        "6300000000" // ->two_data
        "8400000000" // ->three_data
        "4200000000" // ->four_data
        "ffffffffff"
        "ffffffffff"
        "ffffffffff"
    );
    const data_chunk  expected_body = base16_chunk
    (
        "20""0100000000000000000000000000000000000000000000000000000000000000"
        "20""0000000000000000000000000000000000000000000000000000000000000000"
        "20""0400000000000000000000000000000000000000000000000000000000000000"
        "20""0200000000000000000000000000000000000000000000000000000000000000"
        "20""0300000000000000000000000000000000000000000000000000000000000000"
    );

    // These are just blob values, size doesn't have to be the same.
    const auto zero_data = base16_chunk("0000000000000000000000000000000000000000000000000000000000000000");
    const auto one_data = base16_chunk("0100000000000000000000000000000000000000000000000000000000000000");
    const auto two_data = base16_chunk("0200000000000000000000000000000000000000000000000000000000000000");
    const auto three_data = base16_chunk("0300000000000000000000000000000000000000000000000000000000000000");
    const auto four_data = base16_chunk("0400000000000000000000000000000000000000000000000000000000000000");

    const table::filter_tx::put_ref put0{ {}, zero_data };
    const table::filter_tx::put_ref put1{ {}, one_data };
    const table::filter_tx::put_ref put2{ {}, two_data };
    const table::filter_tx::put_ref put3{ {}, three_data };
    const table::filter_tx::put_ref put4{ {}, four_data };

    test::chunk_storage head_store{};
    test::chunk_storage body_store{};
    table::filter_tx instance{ head_store, body_store, 8 };
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
    table::filter_tx::get_filter get0{};
    table::filter_tx::get_filter get1{};
    table::filter_tx::get_filter get2{};
    table::filter_tx::get_filter get3{};
    table::filter_tx::get_filter get4{};
    BOOST_REQUIRE(instance.at(0, get0));
    BOOST_REQUIRE(instance.at(1, get1));
    BOOST_REQUIRE(instance.at(2, get2));
    BOOST_REQUIRE(instance.at(3, get3));
    BOOST_REQUIRE(instance.at(4, get4));
    BOOST_REQUIRE_EQUAL(get0.filter, zero_data);
    BOOST_REQUIRE_EQUAL(get1.filter, one_data);
    BOOST_REQUIRE_EQUAL(get2.filter, two_data);
    BOOST_REQUIRE_EQUAL(get3.filter, three_data);
    BOOST_REQUIRE_EQUAL(get4.filter, four_data);
}

BOOST_AUTO_TEST_SUITE_END()
