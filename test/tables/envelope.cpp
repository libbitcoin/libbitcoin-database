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

BOOST_AUTO_TEST_SUITE(envelope_tests)

using namespace system;

constexpr auto default_size = 270_size;

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

BOOST_AUTO_TEST_CASE(envelope__serialized_size__default__expected)
{
    const envelope instance{};
    BOOST_REQUIRE_EQUAL(instance.serialized_size(), default_size);
    BOOST_REQUIRE_EQUAL(to_chunk(instance).size(), default_size);
}

BOOST_AUTO_TEST_CASE(envelope__to_data__default__expected_version)
{
    const envelope instance{};
    BOOST_REQUIRE_EQUAL(to_chunk(instance).front(), envelope::current);
}

BOOST_AUTO_TEST_CASE(envelope__from_data__default__round_trip)
{
    const envelope instance{};
    auto data = to_chunk(instance);
    BOOST_REQUIRE_EQUAL(data.size(), default_size);

    envelope out{};
    BOOST_REQUIRE(from_chunk(out, data));
    BOOST_REQUIRE(out == instance);
}

BOOST_AUTO_TEST_CASE(envelope__from_data__prior_version__false)
{
    const envelope instance{};
    auto data = to_chunk(instance);
    data.front() = sub1(envelope::current);

    envelope out{};
    BOOST_REQUIRE(!from_chunk(out, data));
}

BOOST_AUTO_TEST_CASE(envelope__from_data__next_version__false)
{
    const envelope instance{};
    auto data = to_chunk(instance);
    data.front() = add1(envelope::current);

    envelope out{};
    BOOST_REQUIRE(!from_chunk(out, data));
}

BOOST_AUTO_TEST_CASE(envelope__from_data__truncated__false)
{
    const envelope instance{};
    auto data = to_chunk(instance);
    data.resize(sub1(data.size()));

    envelope out{};
    BOOST_REQUIRE(!from_chunk(out, data));
}

BOOST_AUTO_TEST_CASE(envelope__set__default_settings__expected)
{
    const database::settings configuration{};
    envelope instance{};
    instance.set(configuration);

    BOOST_REQUIRE_EQUAL(instance.interval_depth, configuration.interval_depth);
    BOOST_REQUIRE_EQUAL(instance.header_buckets, configuration.header.buckets);
    BOOST_REQUIRE_EQUAL(instance.ins_buckets, configuration.ins.buckets);
    BOOST_REQUIRE_EQUAL(instance.outs_buckets, configuration.outs.buckets);
    BOOST_REQUIRE_EQUAL(instance.tx_buckets, configuration.tx.buckets);
    BOOST_REQUIRE_EQUAL(instance.strong_tx_buckets, configuration.strong_tx.buckets);
    BOOST_REQUIRE_EQUAL(instance.duplicate_buckets, configuration.duplicate.buckets);
    BOOST_REQUIRE_EQUAL(instance.validated_tx_buckets, configuration.validated_tx.buckets);
    BOOST_REQUIRE(instance.filter);
}

BOOST_AUTO_TEST_CASE(envelope__set__unbucketed_filter_bk__false)
{
    database::settings configuration{};
    configuration.filter_bk.buckets = 0;
    envelope instance{};
    instance.set(configuration);
    BOOST_REQUIRE(!instance.filter);
}

BOOST_AUTO_TEST_CASE(envelope__set__unbucketed_filter_tx__false)
{
    database::settings configuration{};
    configuration.filter_tx.buckets = 0;
    envelope instance{};
    instance.set(configuration);
    BOOST_REQUIRE(!instance.filter);
}

BOOST_AUTO_TEST_CASE(envelope__construct__mainnet__expected)
{
    const system::settings bitcoin{ chain::selection::mainnet };
    const database::settings configuration{ chain::selection::mainnet };
    const envelope instance{ bitcoin, configuration };

    BOOST_REQUIRE_EQUAL(instance.initial_subsidy_bitcoin, 50u);
    BOOST_REQUIRE_EQUAL(instance.subsidy_interval_blocks, 210000u);
    BOOST_REQUIRE_EQUAL(instance.bip16_activation_time, 0x4f779a80_u32);
    BOOST_REQUIRE_EQUAL(instance.bip34_activation_threshold, 750u);
    BOOST_REQUIRE_EQUAL(instance.bip34_enforcement_threshold, 950u);
    BOOST_REQUIRE_EQUAL(instance.bip34_activation_sample, 1000u);
    BOOST_REQUIRE_EQUAL(instance.bip9_bit1_active_checkpoint.height(), 481824u);
    BOOST_REQUIRE_EQUAL(instance.bip9_bit1_active_checkpoint.hash(), bitcoin.bip9_bit1_active_checkpoint.hash());
    BOOST_REQUIRE(instance.forks.bip341);
    BOOST_REQUIRE(instance.filter);
    BOOST_REQUIRE_EQUAL(instance.interval_depth, configuration.interval_depth);
    BOOST_REQUIRE_EQUAL(instance.header_buckets, configuration.header.buckets);
}

BOOST_AUTO_TEST_CASE(envelope__construct__mainnet__unset_filter_k)
{
    const system::settings bitcoin{ chain::selection::mainnet };
    const database::settings configuration{ chain::selection::mainnet };
    const envelope instance{ bitcoin, configuration };

    BOOST_REQUIRE_EQUAL(instance.header_k, 0u);
    BOOST_REQUIRE_EQUAL(instance.ins_k, 0u);
    BOOST_REQUIRE_EQUAL(instance.outs_k, 0u);
    BOOST_REQUIRE_EQUAL(instance.tx_k, 0u);
    BOOST_REQUIRE_EQUAL(instance.strong_tx_k, 0u);
    BOOST_REQUIRE_EQUAL(instance.duplicate_k, 0u);
    BOOST_REQUIRE_EQUAL(instance.validated_tx_k, 0u);
}

BOOST_AUTO_TEST_CASE(envelope__from_data__mainnet__round_trip)
{
    const system::settings bitcoin{ chain::selection::mainnet };
    const database::settings configuration{ chain::selection::mainnet };
    const envelope instance{ bitcoin, configuration };
    auto data = to_chunk(instance);
    BOOST_REQUIRE_EQUAL(data.size(), instance.serialized_size());

    envelope out{};
    BOOST_REQUIRE(from_chunk(out, data));
    BOOST_REQUIRE(out == instance);
}

BOOST_AUTO_TEST_CASE(envelope__from_data__regtest__round_trip)
{
    const system::settings bitcoin{ chain::selection::regtest };
    const database::settings configuration{ chain::selection::regtest };
    const envelope instance{ bitcoin, configuration };
    auto data = to_chunk(instance);
    BOOST_REQUIRE_EQUAL(data.size(), instance.serialized_size());

    envelope out{};
    BOOST_REQUIRE(from_chunk(out, data));
    BOOST_REQUIRE(out == instance);
}

BOOST_AUTO_TEST_CASE(envelope__initialize__mainnet__expected)
{
    const system::settings bitcoin{ chain::selection::mainnet };
    database::settings configuration{ chain::selection::mainnet };
    configuration.initialize(bitcoin);

    BOOST_REQUIRE(configuration.envelope == envelope(bitcoin, configuration));
    BOOST_REQUIRE_EQUAL(configuration.envelope.proof_of_work_limit, bitcoin.proof_of_work_limit);
    BOOST_REQUIRE_EQUAL(configuration.envelope.bip9_bit1_active_checkpoint.height(), 481824u);
}

BOOST_AUTO_TEST_SUITE_END()
