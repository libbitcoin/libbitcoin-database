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

BC_AUTO_TEST_SUITE(envelope_tests)

using namespace system;

constexpr auto default_size = 368_size;

static data_chunk to_chunk(const envelope& instance)
{
    data_chunk data(instance.serialized_size(), 0x00);
    system::stream::flip::fast stream{ data };
    database::finalizer sink{ stream };
    if (!instance.to_data(sink) || sink.get_write_position() != data.size())
        return {};

    return data;
}

static bool from_chunk(envelope& out, data_chunk& data)
{
    system::stream::flip::fast stream{ data };
    database::reader source{ stream };
    return out.from_data(source) && source.get_read_position() == data.size();
}

BC_AUTO_TEST_CASE(envelope__serialized_size__default__expected)
{
    const envelope instance{};
    BC_REQUIRE_EQUAL(instance.serialized_size(), default_size);
    BC_REQUIRE_EQUAL(to_chunk(instance).size(), default_size);
}

BC_AUTO_TEST_CASE(envelope__to_data__default__expected_version)
{
    const envelope instance{};
    BC_REQUIRE_EQUAL(instance.schema, system::config::version(4, 0, 18, 0));
    BC_REQUIRE_EQUAL(to_chunk(instance).front(), schema::version.front());
}

BC_AUTO_TEST_CASE(envelope__from_data__default__round_trip)
{
    const envelope instance{};
    auto data = to_chunk(instance);
    BC_REQUIRE_EQUAL(data.size(), default_size);

    envelope out{};
    BC_REQUIRE(from_chunk(out, data));
    BC_REQUIRE(out == instance);
}

BC_AUTO_TEST_CASE(envelope__from_data__prior_version__false)
{
    const envelope instance{};
    auto data = to_chunk(instance);
    data.front() = sub1(data.front());

    envelope out{};
    BC_REQUIRE(!from_chunk(out, data));
}

BC_AUTO_TEST_CASE(envelope__from_data__next_version__false)
{
    const envelope instance{};
    auto data = to_chunk(instance);
    data.front() = add1(data.front());

    envelope out{};
    BC_REQUIRE(!from_chunk(out, data));
}

BC_AUTO_TEST_CASE(envelope__from_data__truncated__false)
{
    const envelope instance{};
    auto data = to_chunk(instance);
    data.resize(sub1(data.size()));

    envelope out{};
    BC_REQUIRE(!from_chunk(out, data));
}

BC_AUTO_TEST_CASE(envelope__set__default_settings__expected)
{
    const database::settings configuration{ test::database_settings() };
    envelope instance{};
    instance.set(configuration);

    BC_REQUIRE_EQUAL(instance.interval_depth, configuration.interval_depth);
    BC_REQUIRE_EQUAL(instance.header_buckets, configuration.header.buckets);
    BC_REQUIRE_EQUAL(instance.ins_buckets, configuration.ins.buckets);
    BC_REQUIRE_EQUAL(instance.outs_buckets, configuration.outs.buckets);
    BC_REQUIRE_EQUAL(instance.tx_buckets, configuration.tx.buckets);
    BC_REQUIRE_EQUAL(instance.duplicate_buckets, configuration.duplicate.buckets);
    BC_REQUIRE_EQUAL(instance.pool_buckets, configuration.pool.buckets);
    BC_REQUIRE_EQUAL(instance.wtxid_buckets, configuration.wtxid.buckets);
    BC_REQUIRE(instance.provide_filters);
}

BC_AUTO_TEST_CASE(envelope__set__unbucketed_filter_bk__false)
{
    database::settings configuration{ test::database_settings() };
    configuration.filter_bk.buckets = 0;
    envelope instance{};
    instance.set(configuration);
    BC_REQUIRE(!instance.provide_filters);
}

BC_AUTO_TEST_CASE(envelope__set__unbucketed_filter_tx__false)
{
    database::settings configuration{ test::database_settings() };
    configuration.filter_tx.buckets = 0;
    envelope instance{};
    instance.set(configuration);
    BC_REQUIRE(!instance.provide_filters);
}

BC_AUTO_TEST_CASE(envelope__construct__mainnet__expected)
{
    const system::settings bitcoin{ chain::selection::mainnet };
    const database::settings configuration{ test::database_settings() };
    const envelope instance{ bitcoin, configuration, false, 42 };

    BC_REQUIRE(instance.network == chain::selection::mainnet);
    BC_REQUIRE_EQUAL(instance.initial_subsidy_bitcoin, 50u);
    BC_REQUIRE_EQUAL(instance.subsidy_interval_blocks, 210000u);
    BC_REQUIRE_EQUAL(instance.bip16_activation_time, 0x4f779a80_u32);
    BC_REQUIRE_EQUAL(instance.bip34_activation_threshold, 750u);
    BC_REQUIRE_EQUAL(instance.bip34_enforcement_threshold, 950u);
    BC_REQUIRE_EQUAL(instance.bip34_activation_sample, 1000u);
    BC_REQUIRE_EQUAL(instance.bip9_bit1_active_checkpoint.height(), 481824u);
    BC_REQUIRE_EQUAL(instance.bip9_bit1_active_checkpoint.hash(), bitcoin.bip9_bit1_active_checkpoint.hash());
    BC_REQUIRE(instance.forks.bip341);
    BC_REQUIRE(instance.provide_filters);
    BC_REQUIRE_EQUAL(instance.interval_depth, configuration.interval_depth);
    BC_REQUIRE_EQUAL(instance.header_buckets, configuration.header.buckets);
}

BC_AUTO_TEST_CASE(envelope__construct__mainnet__unset_filter_k)
{
    const system::settings bitcoin{ chain::selection::mainnet };
    const database::settings configuration{ test::database_settings() };
    const envelope instance{ bitcoin, configuration, false, 42 };

    BC_REQUIRE_EQUAL(instance.header_k, 0u);
    BC_REQUIRE_EQUAL(instance.ins_k, 0u);
    BC_REQUIRE_EQUAL(instance.outs_k, 0u);
    BC_REQUIRE_EQUAL(instance.tx_k, 0u);
    BC_REQUIRE_EQUAL(instance.duplicate_k, 0u);
    BC_REQUIRE_EQUAL(instance.pool_k, 0u);
    BC_REQUIRE_EQUAL(instance.wtxid_k, 0u);
}

BC_AUTO_TEST_CASE(envelope__from_data__mainnet__round_trip)
{
    const system::settings bitcoin{ chain::selection::mainnet };
    const database::settings configuration{ test::database_settings() };
    const envelope instance{ bitcoin, configuration, false, 42 };
    auto data = to_chunk(instance);
    BC_REQUIRE_EQUAL(data.size(), instance.serialized_size());

    envelope out{};
    BC_REQUIRE(from_chunk(out, data));
    BC_REQUIRE(out == instance);
}

BC_AUTO_TEST_CASE(envelope__from_data__regtest__round_trip)
{
    const system::settings bitcoin{ chain::selection::regtest };
    const database::settings configuration{ test::database_settings() };
    const envelope instance{ bitcoin, configuration, false, 42 };
    auto data = to_chunk(instance);
    BC_REQUIRE_EQUAL(data.size(), instance.serialized_size());

    envelope out{};
    BC_REQUIRE(from_chunk(out, data));
    BC_REQUIRE(out == instance);
}

BC_AUTO_TEST_CASE(envelope__from_data__signet__round_trip)
{
    const system::settings bitcoin{ chain::selection::signet };
    const database::settings configuration{ test::database_settings() };
    const envelope instance{ bitcoin, configuration, false, 42 };
    BC_REQUIRE(instance.forks.signet);
    BC_REQUIRE(instance.signet_challenge == bitcoin.signet_challenge);

    auto data = to_chunk(instance);
    BC_REQUIRE_EQUAL(data.size(), instance.serialized_size());

    envelope out{};
    BC_REQUIRE(from_chunk(out, data));
    BC_REQUIRE(out == instance);
}

BC_AUTO_TEST_CASE(envelope__initialize__mainnet__expected)
{
    const system::settings bitcoin{ chain::selection::mainnet };
    database::settings configuration{ test::database_settings() };
    configuration.initialize(bitcoin, false, 42);

    BC_REQUIRE(configuration.envelope == envelope(bitcoin, configuration, false, 42));
    BC_REQUIRE_EQUAL(configuration.envelope.silent_start_height, 42u);
    BC_REQUIRE_EQUAL(configuration.envelope.proof_of_work_limit, bitcoin.proof_of_work_limit);
    BC_REQUIRE_EQUAL(configuration.envelope.bip9_bit1_active_checkpoint.height(), 481824u);
}

BC_AUTO_TEST_CASE(envelope__apply__matching__unchanged)
{
    system::settings bitcoin{ chain::selection::mainnet };
    database::settings configuration{ test::database_settings() };
    const envelope instance{ bitcoin, configuration, true, 42 };

    auto limited = true;
    auto silent = 42_u32;
    BC_REQUIRE(instance.apply(bitcoin, configuration, limited, silent).empty());
    BC_REQUIRE(limited);
    BC_REQUIRE_EQUAL(silent, 42u);
}

BC_AUTO_TEST_CASE(envelope__apply__differing__assigned)
{
    system::settings bitcoin{ chain::selection::mainnet };
    database::settings configuration{ test::database_settings() };
    const envelope instance{ bitcoin, configuration, true, 42 };

    bitcoin.subsidy_interval_blocks = 1;
    configuration.tx.buckets = 1;
    auto limited = false;
    auto silent = 7_u32;
    const string_list expected{ "subsidy_interval_blocks", "tx_buckets", "limited_blocks", "silent_start_height" };
    BC_REQUIRE(instance.apply(bitcoin, configuration, limited, silent) == expected);
    BC_REQUIRE_EQUAL(bitcoin.subsidy_interval_blocks, instance.subsidy_interval_blocks);
    BC_REQUIRE_EQUAL(configuration.tx.buckets, instance.tx_buckets);
    BC_REQUIRE(limited);
    BC_REQUIRE_EQUAL(silent, 42u);
}

BC_AUTO_TEST_CASE(envelope__apply__differing_network__assigned)
{
    system::settings bitcoin{ chain::selection::testnet4 };
    database::settings configuration{ test::database_settings() };
    const envelope instance{ bitcoin, configuration, true, 42 };

    bitcoin.network = chain::selection::mainnet;
    auto limited = true;
    auto silent = 42_u32;
    const string_list expected{ "network" };
    BC_REQUIRE(instance.apply(bitcoin, configuration, limited, silent) == expected);
    BC_REQUIRE(bitcoin.network == chain::selection::testnet4);
}

BC_AUTO_TEST_CASE(envelope__from_data__testnet4__round_trip_network)
{
    const system::settings bitcoin{ chain::selection::testnet4 };
    const database::settings configuration{ test::database_settings() };
    const envelope instance{ bitcoin, configuration, false, 42 };
    auto data = to_chunk(instance);
    BC_REQUIRE_EQUAL(data.size(), instance.serialized_size());

    envelope out{};
    BC_REQUIRE(from_chunk(out, data));
    BC_REQUIRE(out.network == chain::selection::testnet4);
}

BC_AUTO_TEST_CASE(envelope__apply__unprovided_filters__unbucketed)
{
    system::settings bitcoin{ chain::selection::mainnet };
    database::settings configuration{ test::database_settings() };
    configuration.filter_bk.buckets = 0;
    const envelope instance{ bitcoin, configuration, false, 42 };

    configuration.filter_bk.buckets = 1;
    auto limited = false;
    auto silent = 42_u32;
    const string_list expected{ "provide_filters" };
    BC_REQUIRE(instance.apply(bitcoin, configuration, limited, silent) == expected);
    BC_REQUIRE_EQUAL(configuration.filter_bk.buckets, 0u);
    BC_REQUIRE_EQUAL(configuration.filter_tx.buckets, 0u);
}

BC_AUTO_TEST_SUITE_END()
