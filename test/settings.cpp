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
#include "test.hpp"

BOOST_AUTO_TEST_SUITE(settings_tests)

BOOST_AUTO_TEST_CASE(settings__construct__default__expected)
{
    database::settings configuration;
    BOOST_REQUIRE_EQUAL(configuration.path, "bitcoin");

    // Archives.
    BOOST_REQUIRE_EQUAL(configuration.header_buckets, 128u);
    BOOST_REQUIRE_EQUAL(configuration.header_size, 1u);
    BOOST_REQUIRE_EQUAL(configuration.header_rate, 50u);
    BOOST_REQUIRE_EQUAL(configuration.point_buckets, 128u);
    BOOST_REQUIRE_EQUAL(configuration.point_size, 1u);
    BOOST_REQUIRE_EQUAL(configuration.point_rate, 50u);
    BOOST_REQUIRE_EQUAL(configuration.input_size, 1u);
    BOOST_REQUIRE_EQUAL(configuration.input_rate, 50u);
    BOOST_REQUIRE_EQUAL(configuration.output_size, 1u);
    BOOST_REQUIRE_EQUAL(configuration.output_rate, 50u);
    BOOST_REQUIRE_EQUAL(configuration.ins_size, 1u);
    BOOST_REQUIRE_EQUAL(configuration.ins_rate, 50u);
    BOOST_REQUIRE_EQUAL(configuration.outs_size, 1u);
    BOOST_REQUIRE_EQUAL(configuration.outs_rate, 50u);
    BOOST_REQUIRE_EQUAL(configuration.tx_buckets, 128u);
    BOOST_REQUIRE_EQUAL(configuration.tx_size, 1u);
    BOOST_REQUIRE_EQUAL(configuration.tx_rate, 50u);
    BOOST_REQUIRE_EQUAL(configuration.txs_buckets, 128u);
    BOOST_REQUIRE_EQUAL(configuration.txs_size, 1u);
    BOOST_REQUIRE_EQUAL(configuration.txs_rate, 50u);

    // Indexes.
    BOOST_REQUIRE_EQUAL(configuration.candidate_size, 1u);
    BOOST_REQUIRE_EQUAL(configuration.candidate_rate, 50u);
    BOOST_REQUIRE_EQUAL(configuration.confirmed_size, 1u);
    BOOST_REQUIRE_EQUAL(configuration.confirmed_rate, 50u);
    BOOST_REQUIRE_EQUAL(configuration.strong_tx_buckets, 128u);
    BOOST_REQUIRE_EQUAL(configuration.strong_tx_size, 1u);
    BOOST_REQUIRE_EQUAL(configuration.strong_tx_rate, 50u);

    // Caches.
    BOOST_REQUIRE_EQUAL(configuration.prevout_buckets, 100u);
    BOOST_REQUIRE_EQUAL(configuration.prevout_size, 1u);
    BOOST_REQUIRE_EQUAL(configuration.prevout_rate, 50u);
    BOOST_REQUIRE_EQUAL(configuration.doubles_buckets, 128u);
    BOOST_REQUIRE_EQUAL(configuration.doubles_size, 1u);
    BOOST_REQUIRE_EQUAL(configuration.doubles_rate, 50u);
    BOOST_REQUIRE_EQUAL(configuration.validated_bk_buckets, 128u);
    BOOST_REQUIRE_EQUAL(configuration.validated_bk_size, 1u);
    BOOST_REQUIRE_EQUAL(configuration.validated_bk_rate, 50u);
    BOOST_REQUIRE_EQUAL(configuration.validated_tx_buckets, 128u);
    BOOST_REQUIRE_EQUAL(configuration.validated_tx_size, 1u);
    BOOST_REQUIRE_EQUAL(configuration.validated_tx_rate, 50u);

    // Optionals.
    BOOST_REQUIRE_EQUAL(configuration.address_buckets, 128u);
    BOOST_REQUIRE_EQUAL(configuration.address_size, 1u);
    BOOST_REQUIRE_EQUAL(configuration.address_rate, 50u);
    BOOST_REQUIRE_EQUAL(configuration.neutrino_buckets, 128u);
    BOOST_REQUIRE_EQUAL(configuration.neutrino_size, 1u);
    BOOST_REQUIRE_EQUAL(configuration.neutrino_rate, 50u);
}

BOOST_AUTO_TEST_SUITE_END()
