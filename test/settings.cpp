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

BC_AUTO_TEST_SUITE(settings_tests)

BC_AUTO_TEST_CASE(settings__construct__default__expected)
{
    database::settings configuration;
    BC_REQUIRE(configuration.network == system::chain::selection::none);
    BC_REQUIRE_EQUAL(configuration.turbo, true);
    BC_REQUIRE_EQUAL(configuration.gpu_scan_minimum, 65'536u);
    BC_REQUIRE_EQUAL(configuration.mark_unconfirmable, true);
    BC_REQUIRE_EQUAL(configuration.interval_depth, 11u);
    BC_REQUIRE_EQUAL(configuration.envelope.interval_depth, 11u);
    BC_REQUIRE_EQUAL(configuration.envelope.header_buckets, 128u);
    BC_REQUIRE_EQUAL(configuration.envelope.ins_buckets, 128u);
    BC_REQUIRE_EQUAL(configuration.envelope.outs_buckets, 128u);
    BC_REQUIRE_EQUAL(configuration.envelope.tx_buckets, 128u);
    BC_REQUIRE_EQUAL(configuration.envelope.duplicate_buckets, 1024u);
    BC_REQUIRE_EQUAL(configuration.envelope.pool_buckets, 128u);
    BC_REQUIRE_EQUAL(configuration.envelope.wtxid_buckets, 128u);
    BC_REQUIRE_EQUAL(configuration.envelope.provide_filters, false);
    BC_REQUIRE_EQUAL(configuration.path, "bitcoin");

    // Archives.
    BC_REQUIRE_EQUAL(configuration.header.buckets, 128u);
    BC_REQUIRE_EQUAL(configuration.header.expected, 0u);
    BC_REQUIRE_EQUAL(configuration.header.size, 1u);
    BC_REQUIRE_EQUAL(configuration.header.rate, 1u);
    BC_REQUIRE_EQUAL(configuration.ins.buckets, 128u);
    BC_REQUIRE_EQUAL(configuration.ins.expected, 0u);
    BC_REQUIRE_EQUAL(configuration.ins.size, 1u);
    BC_REQUIRE_EQUAL(configuration.ins.rate, 1u);
    BC_REQUIRE_EQUAL(configuration.input.size, 1u);
    BC_REQUIRE_EQUAL(configuration.input.rate, 1u);
    BC_REQUIRE_EQUAL(configuration.output.size, 1u);
    BC_REQUIRE_EQUAL(configuration.output.rate, 1u);
    BC_REQUIRE_EQUAL(configuration.outs.buckets, 128u);
    BC_REQUIRE_EQUAL(configuration.outs.expected, 0u);
    BC_REQUIRE_EQUAL(configuration.outs.size, 1u);
    BC_REQUIRE_EQUAL(configuration.outs.rate, 1u);
    BC_REQUIRE_EQUAL(configuration.tx.buckets, 128u);
    BC_REQUIRE_EQUAL(configuration.tx.expected, 0u);
    BC_REQUIRE_EQUAL(configuration.tx.size, 1u);
    BC_REQUIRE_EQUAL(configuration.tx.rate, 1u);
    BC_REQUIRE_EQUAL(configuration.txs.buckets, 128u);
    BC_REQUIRE_EQUAL(configuration.txs.size, 1u);
    BC_REQUIRE_EQUAL(configuration.txs.rate, 1u);

    // Indexes.
    BC_REQUIRE_EQUAL(configuration.candidate.buckets, 128u);
    BC_REQUIRE_EQUAL(configuration.candidate.rate, 1u);
    BC_REQUIRE_EQUAL(configuration.confirmed.buckets, 128u);
    BC_REQUIRE_EQUAL(configuration.confirmed.rate, 1u);
    BC_REQUIRE_EQUAL(configuration.strong.buckets, 128u);
    BC_REQUIRE_EQUAL(configuration.strong.size, 1u);
    BC_REQUIRE_EQUAL(configuration.strong.rate, 1u);
    BC_REQUIRE_EQUAL(configuration.scan.buckets, 128u);
    BC_REQUIRE_EQUAL(configuration.scan.size, 0u);
    BC_REQUIRE_EQUAL(configuration.scan.rate, 1u);
    BC_REQUIRE_EQUAL(configuration.wtxid.buckets, 128u);
    BC_REQUIRE_EQUAL(configuration.wtxid.expected, 10'000'000u);
    BC_REQUIRE_EQUAL(configuration.wtxid.size, 0u);
    BC_REQUIRE_EQUAL(configuration.wtxid.rate, 1u);

    // Caches.
    BC_REQUIRE_EQUAL(configuration.ecdsa.size, 0u);
    BC_REQUIRE_EQUAL(configuration.ecdsa.rate, 1u);
    BC_REQUIRE_EQUAL(configuration.schnorr.size, 0u);
    BC_REQUIRE_EQUAL(configuration.schnorr.rate, 1u);
    BC_REQUIRE_EQUAL(configuration.silent.size, 0u);
    BC_REQUIRE_EQUAL(configuration.silent.rate, 1u);
    BC_REQUIRE_EQUAL(configuration.duplicate.buckets, 1024u);
    BC_REQUIRE_EQUAL(configuration.duplicate.expected, 0u);
    BC_REQUIRE_EQUAL(configuration.duplicate.size, 0u);
    BC_REQUIRE_EQUAL(configuration.duplicate.rate, 1u);
    BC_REQUIRE_EQUAL(configuration.prevalid.size, 0u);
    BC_REQUIRE_EQUAL(configuration.prevalid.rate, 1u);
    BC_REQUIRE_EQUAL(configuration.presilent.size, 0u);
    BC_REQUIRE_EQUAL(configuration.presilent.rate, 1u);
    BC_REQUIRE_EQUAL(configuration.prevout.buckets, 0u);
    BC_REQUIRE_EQUAL(configuration.prevout.size, 0u);
    BC_REQUIRE_EQUAL(configuration.prevout.rate, 1u);
    BC_REQUIRE_EQUAL(configuration.state.buckets, 128u);
    BC_REQUIRE_EQUAL(configuration.state.size, 1u);
    BC_REQUIRE_EQUAL(configuration.state.rate, 1u);
    BC_REQUIRE_EQUAL(configuration.pool.buckets, 128u);
    BC_REQUIRE_EQUAL(configuration.pool.expected, 10'000'000u);
    BC_REQUIRE_EQUAL(configuration.pool.size, 0u);
    BC_REQUIRE_EQUAL(configuration.pool.rate, 1u);
    BC_REQUIRE_EQUAL(configuration.spends.size, 0u);
    BC_REQUIRE_EQUAL(configuration.spends.rate, 1u);

    // Optionals.
    BC_REQUIRE_EQUAL(configuration.filter_bk.buckets, 0u);
    BC_REQUIRE_EQUAL(configuration.filter_bk.size, 0u);
    BC_REQUIRE_EQUAL(configuration.filter_bk.rate, 1u);
    BC_REQUIRE_EQUAL(configuration.filter_tx.buckets, 0u);
    BC_REQUIRE_EQUAL(configuration.filter_tx.size, 0u);
    BC_REQUIRE_EQUAL(configuration.filter_tx.rate, 1u);
}

BC_AUTO_TEST_CASE(settings__construct__mainnet__expected)
{
    database::settings configuration{ system::chain::selection::mainnet };
    BC_REQUIRE(configuration.network == system::chain::selection::mainnet);
    BC_REQUIRE_EQUAL(configuration.path, "mainnet");
    BC_REQUIRE_EQUAL(configuration.header.expected, 962'953u);
    BC_REQUIRE_EQUAL(configuration.header.size, 124'220'679u);
    BC_REQUIRE_EQUAL(configuration.txs.buckets, 950'001u);
    BC_REQUIRE_EQUAL(configuration.txs.size, 54'471'017u);
    BC_REQUIRE_EQUAL(configuration.tx.expected, 1'359'871'695u);
    BC_REQUIRE_EQUAL(configuration.tx.size, 870'317'885u);
    BC_REQUIRE_EQUAL(configuration.ins.expected, 3'363'467'251u);
    BC_REQUIRE_EQUAL(configuration.ins.size, 1'749'002'971u);
    BC_REQUIRE_EQUAL(configuration.outs.expected, 3'741'929'086u);
    BC_REQUIRE_EQUAL(configuration.outs.size, 336'773'618u);
    BC_REQUIRE_EQUAL(configuration.input.size, 67'269'346u);
    BC_REQUIRE_EQUAL(configuration.output.size, 1'278'023'220u);
    BC_REQUIRE_EQUAL(configuration.candidate.buckets, 950'001u);
    BC_REQUIRE_EQUAL(configuration.confirmed.buckets, 950'001u);
    BC_REQUIRE_EQUAL(configuration.strong.buckets, 1'359'871'695u);
    BC_REQUIRE_EQUAL(configuration.scan.buckets, 950'001u);
    BC_REQUIRE_EQUAL(configuration.state.buckets, 950'001u);
    BC_REQUIRE_EQUAL(configuration.interval_depth, 11u);
    BC_REQUIRE_EQUAL(configuration.turbo, true);
}

BC_AUTO_TEST_CASE(settings__construct__testnet3__expected)
{
    database::settings configuration{ system::chain::selection::testnet3 };
    BC_REQUIRE(configuration.network == system::chain::selection::testnet3);
    BC_REQUIRE_EQUAL(configuration.path, "testnet3");
    BC_REQUIRE_EQUAL(configuration.header.expected, 5'158'746u);
    BC_REQUIRE_EQUAL(configuration.header.size, 588'097'044u);
    BC_REQUIRE_EQUAL(configuration.txs.buckets, 5'100'001u);
    BC_REQUIRE_EQUAL(configuration.txs.size, 21'874'552u);
    BC_REQUIRE_EQUAL(configuration.tx.expected, 536'643'888u);
    BC_REQUIRE_EQUAL(configuration.tx.size, 343'452'088u);
    BC_REQUIRE_EQUAL(configuration.ins.expected, 598'161'444u);
    BC_REQUIRE_EQUAL(configuration.ins.size, 311'043'951u);
    BC_REQUIRE_EQUAL(configuration.outs.expected, 1'166'697'559u);
    BC_REQUIRE_EQUAL(configuration.outs.size, 105'002'780u);
    BC_REQUIRE_EQUAL(configuration.input.size, 981'182'034u);
    BC_REQUIRE_EQUAL(configuration.output.size, 600'719'803u);
    BC_REQUIRE_EQUAL(configuration.candidate.buckets, 5'100'001u);
    BC_REQUIRE_EQUAL(configuration.confirmed.buckets, 5'100'001u);
    BC_REQUIRE_EQUAL(configuration.strong.buckets, 536'643'888u);
    BC_REQUIRE_EQUAL(configuration.scan.buckets, 5'100'001u);
    BC_REQUIRE_EQUAL(configuration.state.buckets, 5'100'001u);
    BC_REQUIRE_EQUAL(configuration.interval_depth, 11u);
    BC_REQUIRE_EQUAL(configuration.turbo, true);
}

BC_AUTO_TEST_CASE(settings__construct__testnet4__expected)
{
    database::settings configuration{ system::chain::selection::testnet4 };
    BC_REQUIRE(configuration.network == system::chain::selection::testnet4);
    BC_REQUIRE_EQUAL(configuration.path, "testnet4");
    BC_REQUIRE_EQUAL(configuration.header.expected, 156'456u);
    BC_REQUIRE_EQUAL(configuration.header.size, 17'835'984u);
    BC_REQUIRE_EQUAL(configuration.txs.buckets, 150'001u);
    BC_REQUIRE_EQUAL(configuration.txs.size, 604'424u);
    BC_REQUIRE_EQUAL(configuration.tx.expected, 14'810'011u);
    BC_REQUIRE_EQUAL(configuration.tx.size, 9'478'407u);
    BC_REQUIRE_EQUAL(configuration.ins.expected, 33'127'580u);
    BC_REQUIRE_EQUAL(configuration.ins.size, 17'226'342u);
    BC_REQUIRE_EQUAL(configuration.outs.expected, 57'089'349u);
    BC_REQUIRE_EQUAL(configuration.outs.size, 5'138'041u);
    BC_REQUIRE_EQUAL(configuration.input.size, 87'852'864u);
    BC_REQUIRE_EQUAL(configuration.output.size, 20'343'818u);
    BC_REQUIRE_EQUAL(configuration.candidate.buckets, 150'001u);
    BC_REQUIRE_EQUAL(configuration.confirmed.buckets, 150'001u);
    BC_REQUIRE_EQUAL(configuration.strong.buckets, 14'810'011u);
    BC_REQUIRE_EQUAL(configuration.scan.buckets, 150'001u);
    BC_REQUIRE_EQUAL(configuration.state.buckets, 150'001u);
    BC_REQUIRE_EQUAL(configuration.interval_depth, 11u);
    BC_REQUIRE_EQUAL(configuration.turbo, true);
}

BC_AUTO_TEST_CASE(settings__construct__regtest__expected)
{
    database::settings configuration{ system::chain::selection::regtest };
    BC_REQUIRE(configuration.network == system::chain::selection::regtest);
    BC_REQUIRE_EQUAL(configuration.path, "regtest");
    BC_REQUIRE_EQUAL(configuration.header.expected, 10'000u);
    BC_REQUIRE_EQUAL(configuration.header.size, 0u);
    BC_REQUIRE_EQUAL(configuration.txs.buckets, 10'001u);
    BC_REQUIRE_EQUAL(configuration.txs.size, 0u);
    BC_REQUIRE_EQUAL(configuration.tx.expected, 100'000u);
    BC_REQUIRE_EQUAL(configuration.tx.size, 0u);
    BC_REQUIRE_EQUAL(configuration.ins.expected, 200'000u);
    BC_REQUIRE_EQUAL(configuration.ins.size, 0u);
    BC_REQUIRE_EQUAL(configuration.outs.expected, 300'000u);
    BC_REQUIRE_EQUAL(configuration.outs.size, 0u);
    BC_REQUIRE_EQUAL(configuration.input.size, 0u);
    BC_REQUIRE_EQUAL(configuration.output.size, 0u);
    BC_REQUIRE_EQUAL(configuration.candidate.buckets, 10'001u);
    BC_REQUIRE_EQUAL(configuration.confirmed.buckets, 10'001u);
    BC_REQUIRE_EQUAL(configuration.strong.buckets, 100'000u);
    BC_REQUIRE_EQUAL(configuration.scan.buckets, 10'001u);
    BC_REQUIRE_EQUAL(configuration.state.buckets, 10'001u);
    BC_REQUIRE_EQUAL(configuration.interval_depth, 11u);
    BC_REQUIRE_EQUAL(configuration.turbo, true);
}

BC_AUTO_TEST_CASE(settings__construct__signet__expected)
{
    database::settings configuration{ system::chain::selection::signet };
    BC_REQUIRE(configuration.network == system::chain::selection::signet);
    BC_REQUIRE_EQUAL(configuration.path, "signet");
    BC_REQUIRE_EQUAL(configuration.header.expected, 330'000u);
    BC_REQUIRE_EQUAL(configuration.header.size, 0u);
    BC_REQUIRE_EQUAL(configuration.txs.buckets, 330'001u);
    BC_REQUIRE_EQUAL(configuration.txs.size, 0u);
    BC_REQUIRE_EQUAL(configuration.tx.expected, 100'000u);
    BC_REQUIRE_EQUAL(configuration.tx.size, 0u);
    BC_REQUIRE_EQUAL(configuration.ins.expected, 200'000u);
    BC_REQUIRE_EQUAL(configuration.ins.size, 0u);
    BC_REQUIRE_EQUAL(configuration.outs.expected, 300'000u);
    BC_REQUIRE_EQUAL(configuration.outs.size, 0u);
    BC_REQUIRE_EQUAL(configuration.input.size, 0u);
    BC_REQUIRE_EQUAL(configuration.output.size, 0u);
    BC_REQUIRE_EQUAL(configuration.candidate.buckets, 330'001u);
    BC_REQUIRE_EQUAL(configuration.confirmed.buckets, 330'001u);
    BC_REQUIRE_EQUAL(configuration.strong.buckets, 100'000u);
    BC_REQUIRE_EQUAL(configuration.scan.buckets, 330'001u);
    BC_REQUIRE_EQUAL(configuration.state.buckets, 330'001u);
    BC_REQUIRE_EQUAL(configuration.interval_depth, 11u);
    BC_REQUIRE_EQUAL(configuration.turbo, true);
}

BC_AUTO_TEST_SUITE_END()
