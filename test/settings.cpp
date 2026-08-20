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
#include "test.hpp"

BOOST_AUTO_TEST_SUITE(settings_tests)

BOOST_AUTO_TEST_CASE(settings__construct__default__expected)
{
    database::settings configuration;
    BOOST_REQUIRE_EQUAL(configuration.turbo, false);
    BOOST_REQUIRE_EQUAL(configuration.mark_unconfirmable, true);
    BOOST_REQUIRE_EQUAL(configuration.interval_depth, 255u);
    BOOST_REQUIRE_EQUAL(configuration.fork_flags, 0u);
    BOOST_REQUIRE_EQUAL(configuration.path, "bitcoin");

    // Archives.
    BOOST_REQUIRE_EQUAL(configuration.header.buckets, 128u);
    BOOST_REQUIRE_EQUAL(configuration.header.size, 1u);
    BOOST_REQUIRE_EQUAL(configuration.header.rate, 5u);
    BOOST_REQUIRE_EQUAL(configuration.ins.buckets, 128u);
    BOOST_REQUIRE_EQUAL(configuration.ins.size, 1u);
    BOOST_REQUIRE_EQUAL(configuration.ins.rate, 5u);
    BOOST_REQUIRE_EQUAL(configuration.input.size, 1u);
    BOOST_REQUIRE_EQUAL(configuration.input.rate, 5u);
    BOOST_REQUIRE_EQUAL(configuration.output.size, 1u);
    BOOST_REQUIRE_EQUAL(configuration.output.rate, 5u);
    BOOST_REQUIRE_EQUAL(configuration.outs.buckets, 128u);
    BOOST_REQUIRE_EQUAL(configuration.outs.size, 1u);
    BOOST_REQUIRE_EQUAL(configuration.outs.rate, 5u);
    BOOST_REQUIRE_EQUAL(configuration.tx.buckets, 128u);
    BOOST_REQUIRE_EQUAL(configuration.tx.size, 1u);
    BOOST_REQUIRE_EQUAL(configuration.tx.rate, 5u);
    BOOST_REQUIRE_EQUAL(configuration.txs.buckets, 128u);
    BOOST_REQUIRE_EQUAL(configuration.txs.size, 1u);
    BOOST_REQUIRE_EQUAL(configuration.txs.rate, 5u);

    // Indexes.
    BOOST_REQUIRE_EQUAL(configuration.candidate.buckets, 128u);
    BOOST_REQUIRE_EQUAL(configuration.candidate.rate, 5u);
    BOOST_REQUIRE_EQUAL(configuration.confirmed.buckets, 128u);
    BOOST_REQUIRE_EQUAL(configuration.confirmed.rate, 5u);
    BOOST_REQUIRE_EQUAL(configuration.strong_tx.buckets, 128u);
    BOOST_REQUIRE_EQUAL(configuration.strong_tx.size, 1u);
    BOOST_REQUIRE_EQUAL(configuration.strong_tx.rate, 5u);

    // Caches.
    BOOST_REQUIRE_EQUAL(configuration.ecdsa.size, 1u);
    BOOST_REQUIRE_EQUAL(configuration.ecdsa.rate, 5u);
    BOOST_REQUIRE_EQUAL(configuration.schnorr.size, 1u);
    BOOST_REQUIRE_EQUAL(configuration.schnorr.rate, 5u);
    BOOST_REQUIRE_EQUAL(configuration.silent.size, 1u);
    BOOST_REQUIRE_EQUAL(configuration.silent.rate, 5u);
    BOOST_REQUIRE_EQUAL(configuration.duplicate.buckets, 128u);
    BOOST_REQUIRE_EQUAL(configuration.duplicate.size, 1u);
    BOOST_REQUIRE_EQUAL(configuration.duplicate.rate, 5u);
    BOOST_REQUIRE_EQUAL(configuration.prevalid.size, 1u);
    BOOST_REQUIRE_EQUAL(configuration.prevalid.rate, 5u);
    BOOST_REQUIRE_EQUAL(configuration.prevout.buckets, 128u);
    BOOST_REQUIRE_EQUAL(configuration.prevout.size, 1u);
    BOOST_REQUIRE_EQUAL(configuration.prevout.rate, 5u);
    BOOST_REQUIRE_EQUAL(configuration.validated_bk.buckets, 128u);
    BOOST_REQUIRE_EQUAL(configuration.validated_bk.size, 1u);
    BOOST_REQUIRE_EQUAL(configuration.validated_bk.rate, 5u);
    BOOST_REQUIRE_EQUAL(configuration.validated_tx.buckets, 128u);
    BOOST_REQUIRE_EQUAL(configuration.validated_tx.size, 1u);
    BOOST_REQUIRE_EQUAL(configuration.validated_tx.rate, 5u);

    // Optionals.
    BOOST_REQUIRE_EQUAL(configuration.filter_bk.buckets, 128u);
    BOOST_REQUIRE_EQUAL(configuration.filter_bk.size, 1u);
    BOOST_REQUIRE_EQUAL(configuration.filter_bk.rate, 5u);
    BOOST_REQUIRE_EQUAL(configuration.filter_tx.buckets, 128u);
    BOOST_REQUIRE_EQUAL(configuration.filter_tx.size, 1u);
    BOOST_REQUIRE_EQUAL(configuration.filter_tx.rate, 5u);
}

BOOST_AUTO_TEST_SUITE_END()
