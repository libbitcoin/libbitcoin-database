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

constexpr auto default_size = 618_size;

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
    BC_REQUIRE_EQUAL(instance.schema, system::config::version(4, 0, 34, 0));
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

BC_AUTO_TEST_CASE(envelope__construct__default__activations_empty)
{
    const envelope instance{};
    BC_REQUIRE_EQUAL(instance.activations.front().flags, 0u);
    BC_REQUIRE_EQUAL(instance.activations.front().link, schema::header::link::terminal);
    BC_REQUIRE_EQUAL(instance.activations.back().flags, 0u);
    BC_REQUIRE_EQUAL(instance.activations.back().link, schema::header::link::terminal);
}

BC_AUTO_TEST_CASE(envelope__from_data__activations__round_trip)
{
    envelope instance{};
    instance.activations.front() = { 0x00000700, 419328 };
    instance.activations.back() = { 0x00018000, 709632 };
    auto data = to_chunk(instance);
    BC_REQUIRE_EQUAL(data.size(), default_size);

    envelope out{};
    BC_REQUIRE(from_chunk(out, data));
    BC_REQUIRE(out == instance);
}

BC_AUTO_TEST_CASE(envelope__pooling_offset__serialized__expected)
{
    envelope instance{};
    instance.pooling = true;
    instance.scan_frontier = 0x01020304;
    instance.latch_tx = 0x05060708;
    const auto data = to_chunk(instance);
    const auto offset = instance.pooling_offset();
    BC_REQUIRE_EQUAL(data.at(offset), 0x01u);
    BC_REQUIRE_EQUAL(system::unsafe_from_little_endian<uint32_t>(&data.at(add1(offset))), 0x01020304u);
    BC_REQUIRE_EQUAL(system::unsafe_from_little_endian<uint32_t>(&data.at(offset + 5u)), 0x05060708u);
}

BC_AUTO_TEST_CASE(envelope__construct__default__pool_top_terminal)
{
    const envelope instance{};
    BC_REQUIRE_EQUAL(instance.pool_top, schema::header::link::terminal);
}

BC_AUTO_TEST_CASE(envelope__from_data__pool_top__round_trip)
{
    envelope instance{};
    instance.pool_top = 709632;
    auto data = to_chunk(instance);
    BC_REQUIRE_EQUAL(data.size(), default_size);

    envelope out{};
    BC_REQUIRE(from_chunk(out, data));
    BC_REQUIRE_EQUAL(out.pool_top, 709632u);
    BC_REQUIRE(out == instance);
}

BC_AUTO_TEST_CASE(envelope__pool_top_offset__serialized__expected)
{
    envelope instance{};
    instance.pool_top = 0x030201;
    const auto data = to_chunk(instance);
    const auto offset = instance.pool_top_offset();
    BC_REQUIRE_EQUAL(data.at(offset), 0x01u);
    BC_REQUIRE_EQUAL(data.at(offset + 1u), 0x02u);
    BC_REQUIRE_EQUAL(data.at(offset + 2u), 0x03u);
    BC_REQUIRE_EQUAL(instance.pooling_offset() + envelope::pooling_size, offset);
    BC_REQUIRE_EQUAL(offset + envelope::pool_top_size, instance.activation_offset(0));
}

BC_AUTO_TEST_CASE(envelope__activation_offset__serialized__expected)
{
    envelope instance{};
    instance.activations.front() = { 0x01020304, 42 };
    instance.activations.back() = { 0x05060708, 42 };
    const auto data = to_chunk(instance);
    const auto first = instance.activation_offset(0);
    const auto last = instance.activation_offset(sub1(envelope::activation_count));
    BC_REQUIRE_EQUAL(system::unsafe_from_little_endian<uint32_t>(&data.at(first)), 0x01020304u);
    BC_REQUIRE_EQUAL(system::unsafe_from_little_endian<uint32_t>(&data.at(last)), 0x05060708u);
    BC_REQUIRE_EQUAL(last + envelope::activation_size, data.size());
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
}

BC_AUTO_TEST_CASE(envelope__set__unbucketed_filters__provide_filters_unchanged)
{
    database::settings configuration{ test::database_settings() };
    configuration.filter_bk.buckets = 0;
    configuration.filter_tx.buckets = 0;
    envelope instance{};
    instance.provide_filters = true;
    instance.set(configuration);
    BC_REQUIRE(instance.provide_filters);
}

BC_AUTO_TEST_CASE(envelope__construct__mainnet__expected)
{
    const system::settings bitcoin{ chain::selection::mainnet };
    const database::settings configuration{ test::database_settings() };
    const envelope instance{ bitcoin, configuration, false, true, true, 42 };

    BC_REQUIRE(instance.network == chain::selection::mainnet);
    BC_REQUIRE_EQUAL(instance.initial_subsidy_bitcoin, 50u);
    BC_REQUIRE_EQUAL(instance.subsidy_interval_blocks, 210000u);
    BC_REQUIRE_EQUAL(instance.bip16_activation_time, 0x4f779a80_u32);
    BC_REQUIRE_EQUAL(instance.bip34_activation_threshold, 750u);
    BC_REQUIRE_EQUAL(instance.bip34_enforcement_threshold, 950u);
    BC_REQUIRE_EQUAL(instance.bip34_activation_sample, 1000u);
    BC_REQUIRE_EQUAL(instance.bip9_activation_period, 2016u);
    BC_REQUIRE_EQUAL(instance.bip9_bit2_deployment.activation_height, 709632u);
    BC_REQUIRE(instance.bip9_bit2_deployment.speedy);
    BC_REQUIRE_EQUAL(instance.bip9_bit1_deployment.threshold, 1916u);
    BC_REQUIRE(instance.forks.bip341);
    BC_REQUIRE(instance.forks.bip54_timewarp);
    BC_REQUIRE_EQUAL(instance.bip54_timewarp_deployment.threshold, 0u);
    BC_REQUIRE(instance.forks.bip54_timespan);
    BC_REQUIRE_EQUAL(instance.bip54_timespan_deployment.threshold, 0u);
    BC_REQUIRE(instance.forks.bip54_sigops);
    BC_REQUIRE_EQUAL(instance.bip54_sigops_deployment.threshold, 0u);
    BC_REQUIRE(instance.forks.bip54_txsize);
    BC_REQUIRE_EQUAL(instance.bip54_txsize_deployment.threshold, 0u);
    BC_REQUIRE(instance.forks.bip54_coinbase);
    BC_REQUIRE_EQUAL(instance.bip54_coinbase_deployment.threshold, 0u);
    BC_REQUIRE(instance.provide_filters);
    BC_REQUIRE_EQUAL(instance.interval_depth, configuration.interval_depth);
    BC_REQUIRE_EQUAL(instance.header_buckets, configuration.header.buckets);
}

BC_AUTO_TEST_CASE(envelope__construct__mainnet__unset_filter_k)
{
    const system::settings bitcoin{ chain::selection::mainnet };
    const database::settings configuration{ test::database_settings() };
    const envelope instance{ bitcoin, configuration, false, true, true, 42 };

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
    const envelope instance{ bitcoin, configuration, false, true, true, 42 };
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
    const envelope instance{ bitcoin, configuration, false, true, true, 42 };
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
    const envelope instance{ bitcoin, configuration, false, true, true, 42 };
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
    configuration.initialize(bitcoin, false, true, true, 42);

    BC_REQUIRE(configuration.envelope == envelope(bitcoin, configuration, false, true, true, 42));
    BC_REQUIRE(configuration.envelope.provide_filters);
    BC_REQUIRE(configuration.envelope.require_witness);
    BC_REQUIRE_EQUAL(configuration.envelope.silent_start_height, 42u);
    BC_REQUIRE_EQUAL(configuration.envelope.proof_of_work_limit, bitcoin.proof_of_work_limit);
    BC_REQUIRE_EQUAL(configuration.envelope.bip9_bit1_deployment.threshold, 1916u);
}

BC_AUTO_TEST_CASE(envelope__apply__matching__unchanged)
{
    system::settings bitcoin{ chain::selection::mainnet };
    database::settings configuration{ test::database_settings() };
    const envelope instance{ bitcoin, configuration, true, true, true, 42 };

    auto limited = true;
    auto witness = true;
    auto silent = 42_u32;
    BC_REQUIRE(instance.apply(bitcoin, configuration, limited, witness, silent).empty());
    BC_REQUIRE(limited);
    BC_REQUIRE(witness);
    BC_REQUIRE_EQUAL(silent, 42u);
}

BC_AUTO_TEST_CASE(envelope__apply__differing__assigned)
{
    system::settings bitcoin{ chain::selection::mainnet };
    database::settings configuration{ test::database_settings() };
    const envelope instance{ bitcoin, configuration, true, true, true, 42 };

    bitcoin.subsidy_interval_blocks = 1;
    configuration.tx.buckets = 1;
    auto limited = false;
    auto witness = false;
    auto silent = 7_u32;
    const string_list expected{ "subsidy_interval_blocks", "tx_buckets", "limited_blocks", "require_witness", "silent_start_height" };
    BC_REQUIRE(instance.apply(bitcoin, configuration, limited, witness, silent) == expected);
    BC_REQUIRE_EQUAL(bitcoin.subsidy_interval_blocks, instance.subsidy_interval_blocks);
    BC_REQUIRE_EQUAL(configuration.tx.buckets, instance.tx_buckets);
    BC_REQUIRE(limited);
    BC_REQUIRE(witness);
    BC_REQUIRE_EQUAL(silent, 42u);
}

BC_AUTO_TEST_CASE(envelope__apply__differing_network__assigned)
{
    system::settings bitcoin{ chain::selection::testnet4 };
    database::settings configuration{ test::database_settings() };
    const envelope instance{ bitcoin, configuration, true, true, true, 42 };

    bitcoin.network = chain::selection::mainnet;
    auto limited = true;
    auto witness = true;
    auto silent = 42_u32;
    const string_list expected{ "network" };
    BC_REQUIRE(instance.apply(bitcoin, configuration, limited, witness, silent) == expected);
    BC_REQUIRE(bitcoin.network == chain::selection::testnet4);
}

BC_AUTO_TEST_CASE(envelope__from_data__testnet4__round_trip_network)
{
    const system::settings bitcoin{ chain::selection::testnet4 };
    const database::settings configuration{ test::database_settings() };
    const envelope instance{ bitcoin, configuration, false, true, true, 42 };
    auto data = to_chunk(instance);
    BC_REQUIRE_EQUAL(data.size(), instance.serialized_size());

    envelope out{};
    BC_REQUIRE(from_chunk(out, data));
    BC_REQUIRE(out.network == chain::selection::testnet4);
}

BC_AUTO_TEST_CASE(envelope__from_data__testnet5__round_trip_network)
{
    const system::settings bitcoin{ chain::selection::testnet5 };
    const database::settings configuration{ test::database_settings() };
    const envelope instance{ bitcoin, configuration, false, true, true, 42 };
    auto data = to_chunk(instance);
    BC_REQUIRE_EQUAL(data.size(), instance.serialized_size());

    envelope out{};
    BC_REQUIRE(from_chunk(out, data));
    BC_REQUIRE(out.network == chain::selection::testnet5);
}

BC_AUTO_TEST_CASE(envelope__apply__unprovided_filters__unbucketed)
{
    system::settings bitcoin{ chain::selection::mainnet };
    database::settings configuration{ test::database_settings() };
    const envelope instance{ bitcoin, configuration, false, false, true, 42 };

    auto limited = false;
    auto witness = true;
    auto silent = 42_u32;
    BC_REQUIRE(instance.apply(bitcoin, configuration, limited, witness, silent).empty());
    BC_REQUIRE_EQUAL(configuration.filter_bk.buckets, 0u);
    BC_REQUIRE_EQUAL(configuration.filter_tx.buckets, 0u);
}

BC_AUTO_TEST_CASE(envelope__apply__provided_filters__buckets_unchanged)
{
    system::settings bitcoin{ chain::selection::mainnet };
    database::settings configuration{ test::database_settings() };
    const envelope instance{ bitcoin, configuration, false, true, true, 42 };

    auto limited = false;
    auto witness = true;
    auto silent = 42_u32;
    BC_REQUIRE(instance.apply(bitcoin, configuration, limited, witness, silent).empty());
    BC_REQUIRE_EQUAL(configuration.filter_bk.buckets, 128u);
    BC_REQUIRE_EQUAL(configuration.filter_tx.buckets, 128u);
}

BC_AUTO_TEST_CASE(envelope__from_data__4_0_31__false)
{
    auto data = test::envelope_4_0_31;
    envelope out{};
    BC_REQUIRE(!from_chunk(out, data));
    BC_REQUIRE(out.schema == system::config::version(4, 0, 31, 0));
}

BC_AUTO_TEST_CASE(envelope__get_schema__short__default)
{
    BC_REQUIRE(envelope::get_schema(data_chunk(15)).is_default());
}

BC_AUTO_TEST_CASE(envelope__migrate_step__4_0_31__stepwise_expected)
{
    envelope configured{};
    configured.require_witness = true;
    auto data = test::envelope_4_0_31;
    BC_REQUIRE(envelope::get_schema(data) == system::config::version(4, 0, 31, 0));

    // 4.0.31 -> 4.0.32
    BC_REQUIRE(envelope::migrate_step(data, configured));
    BC_REQUIRE(envelope::get_schema(data) == system::config::version(4, 0, 32, 0));
    BC_REQUIRE_EQUAL(data.size(), add1(test::envelope_4_0_31.size()));

    // 4.0.32 -> 4.0.33
    BC_REQUIRE(envelope::migrate_step(data, configured));
    BC_REQUIRE(envelope::get_schema(data) == system::config::version(4, 0, 33, 0));
    BC_REQUIRE_EQUAL(data.size(), add1(test::envelope_4_0_31.size()));

    // 4.0.33 -> 4.0.34
    BC_REQUIRE(envelope::migrate_step(data, configured));
    BC_REQUIRE(envelope::get_schema(data) == system::config::version(4, 0, 34, 0));
    BC_REQUIRE_EQUAL(data.size(), test::envelope_4_0_31.size() + 4u);

    // compiled
    BC_REQUIRE(envelope::get_schema(data) == envelope::compiled);
    BC_REQUIRE(!envelope::migrate_step(data, configured));

    envelope out{};
    BC_REQUIRE(from_chunk(out, data));
    BC_REQUIRE(out.network == chain::selection::mainnet);
    BC_REQUIRE(out.require_witness);
    BC_REQUIRE(out.provide_filters);
    BC_REQUIRE(!out.limited_blocks);
    BC_REQUIRE_EQUAL(out.silent_start_height, 42u);
    BC_REQUIRE_EQUAL(out.pool_top, schema::header::link::terminal);
}

BC_AUTO_TEST_CASE(envelope__migrate_4_0_30_to_4_0_31__schema_only)
{
    const auto expected = test::envelope_4_0_31;
    auto data = expected;
    data.at(8) = 30;
    BC_REQUIRE(envelope::migrate_4_0_30_to_4_0_31(data));
    BC_REQUIRE(data == expected);
}

BC_AUTO_TEST_CASE(envelope__migrate_4_0_32_to_4_0_33__schema_only)
{
    auto data = test::envelope_4_0_31;
    data.at(8) = 32;
    auto expected = data;
    expected.at(8) = 33;
    BC_REQUIRE(envelope::migrate_4_0_32_to_4_0_33(data));
    BC_REQUIRE(data == expected);
}

BC_AUTO_TEST_CASE(envelope__migrate_4_0_33_to_4_0_34__terminal_before_activations)
{
    // Activations are the last 224 bytes.
    auto image = test::envelope_4_0_31;
    image.at(8) = 33;
    image.back() = 0x42;
    auto data = image;
    const data_chunk terminal{ 0xff, 0xff, 0x7f };
    BC_REQUIRE(envelope::migrate_4_0_33_to_4_0_34(data));
    BC_REQUIRE(envelope::get_schema(data) == system::config::version(4, 0, 34, 0));
    BC_REQUIRE_EQUAL(data.size(), image.size() + 3u);
    BC_REQUIRE(data_chunk(std::prev(data.end(), 227), std::prev(data.end(), 224)) == terminal);
    BC_REQUIRE(data_chunk(std::prev(data.end(), 224), data.end()) == data_chunk(std::prev(image.end(), 224), image.end()));
    BC_REQUIRE(data_chunk(std::next(data.begin(), 16), std::prev(data.end(), 227)) == data_chunk(std::next(image.begin(), 16), std::prev(image.end(), 224)));
}

BC_AUTO_TEST_CASE(envelope__migrate_4_0_33_to_4_0_34__short__false)
{
    data_chunk data(223);
    BC_REQUIRE(!envelope::migrate_4_0_33_to_4_0_34(data));
}

BC_AUTO_TEST_CASE(envelope__migrate__4_0_30__compiled)
{
    envelope configured{};
    configured.require_witness = true;
    auto data = test::envelope_4_0_31;
    data.at(8) = 30;
    BC_REQUIRE(envelope::migrate(data, configured));
    BC_REQUIRE(envelope::get_schema(data) == envelope::compiled);

    envelope out{};
    BC_REQUIRE(from_chunk(out, data));
    BC_REQUIRE(out.require_witness);
    BC_REQUIRE_EQUAL(out.silent_start_height, 42u);
}

BC_AUTO_TEST_CASE(envelope__migrate__4_0_31__compiled)
{
    envelope configured{};
    configured.require_witness = false;
    auto data = test::envelope_4_0_31;
    BC_REQUIRE(envelope::migrate(data, configured));
    BC_REQUIRE(envelope::get_schema(data) == envelope::compiled);

    envelope out{};
    BC_REQUIRE(from_chunk(out, data));
    BC_REQUIRE(!out.require_witness);
    BC_REQUIRE_EQUAL(out.silent_start_height, 42u);
}

BC_AUTO_TEST_CASE(envelope__migrate__premigratable__false)
{
    const system::settings bitcoin{ chain::selection::mainnet };
    const database::settings configuration{ test::database_settings() };
    const envelope instance{ bitcoin, configuration, false, true, true, 42 };
    auto data = to_chunk(instance);
    const auto schema = to_little_endian(sub1(envelope::migratable.segments().at(2)));
    std::copy(schema.begin(), schema.end(), std::next(data.begin(), 2 * sizeof(uint32_t)));

    BC_REQUIRE(!envelope::migrate(data, instance));
    envelope out{};
    BC_REQUIRE(!from_chunk(out, data));
}

BC_AUTO_TEST_CASE(envelope__is_readable__range__expected)
{
    BC_REQUIRE(envelope::is_readable(envelope::compiled));
    BC_REQUIRE(envelope::is_readable(envelope::migratable));
    BC_REQUIRE(!envelope::is_readable({ 4, 0, sub1(envelope::migratable.segments().at(2)), 0 }));
    BC_REQUIRE(!envelope::is_readable({ 4, 0, add1(envelope::compiled.segments().at(2)), 0 }));
}

BC_AUTO_TEST_CASE(envelope__from_data__filters_witness__round_trip)
{
    const system::settings bitcoin{ chain::selection::mainnet };
    const database::settings configuration{ test::database_settings() };
    const envelope instance{ bitcoin, configuration, false, false, true, 42 };
    auto data = to_chunk(instance);
    BC_REQUIRE_EQUAL(data.size(), instance.serialized_size());

    envelope out{};
    BC_REQUIRE(from_chunk(out, data));
    BC_REQUIRE(!out.provide_filters);
    BC_REQUIRE(out.require_witness);
}

BC_AUTO_TEST_SUITE_END()
