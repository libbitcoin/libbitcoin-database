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
#include "../mocks/blocks.hpp"
#include "../mocks/chunk_store.hpp"

BOOST_FIXTURE_TEST_SUITE(query_properties_block_tests, test::directory_setup_fixture)

BOOST_AUTO_TEST_CASE(query_properties_block__get_top_timestamp__always__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, context{}, {}, false, false));
    BOOST_REQUIRE(query.set(test::block2, context{}, {}, false, false));
    BOOST_REQUIRE(query.set(test::block3, context{}, {}, false, false));
    BOOST_REQUIRE(query.push_candidate(1));
    BOOST_REQUIRE_EQUAL(query.get_top_timestamp(true), 0x495fab29_u32);
    BOOST_REQUIRE_EQUAL(query.get_top_timestamp(false), 0x4966bc61_u32);
    BOOST_REQUIRE(query.push_candidate(2));
    BOOST_REQUIRE_EQUAL(query.get_top_timestamp(true), 0x495fab29_u32);
    BOOST_REQUIRE_EQUAL(query.get_top_timestamp(false), 0x4966bcb0_u32);
    BOOST_REQUIRE(query.push_confirmed(1, false));
    BOOST_REQUIRE_EQUAL(query.get_top_timestamp(true), 0x4966bc61_u32);
    BOOST_REQUIRE_EQUAL(query.get_top_timestamp(false), 0x4966bcb0_u32);
    BOOST_REQUIRE(query.push_confirmed(2, false));
    BOOST_REQUIRE_EQUAL(query.get_top_timestamp(true), 0x4966bcb0_u32);
    BOOST_REQUIRE_EQUAL(query.get_top_timestamp(false), 0x4966bcb0_u32);
}

BOOST_AUTO_TEST_CASE(query_properties_block__get_timestamp__genesis__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));

    uint32_t timestamp{};
    BOOST_REQUIRE(!query.get_timestamp(timestamp, 1));
    BOOST_REQUIRE(query.get_timestamp(timestamp, 0));
    BOOST_REQUIRE_EQUAL(timestamp, 0x495fab29_u32);
}

BOOST_AUTO_TEST_CASE(query_properties_block__get_version__genesis__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));

    uint32_t version{};
    BOOST_REQUIRE(!query.get_version(version, 1));
    BOOST_REQUIRE(query.get_version(version, 0));
    BOOST_REQUIRE_EQUAL(version, 0x00000001_u32);
}

BOOST_AUTO_TEST_CASE(query_properties_block__get_bits__genesis__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));

    uint32_t bits{};
    BOOST_REQUIRE(!query.get_bits(bits, 1));
    BOOST_REQUIRE(query.get_bits(bits, 0));
    BOOST_REQUIRE_EQUAL(bits, 0x1d00ffff_u32);
}

BOOST_AUTO_TEST_CASE(query_properties_block__get_context__genesis__default)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));

    context ctx{};
    BOOST_REQUIRE(query.get_context(ctx, 0));
    BOOST_REQUIRE(ctx == context{});

    system::chain::context chain_ctx{};
    BOOST_REQUIRE(query.get_context(chain_ctx, 0));
    BOOST_REQUIRE_EQUAL(chain_ctx.timestamp, 0x495fab29_u32);

    chain_ctx.timestamp = {};
    BOOST_REQUIRE(chain_ctx == system::chain::context{});
}

BOOST_AUTO_TEST_CASE(query_properties_block__get_context__invalid__default)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));

    context ctx{};
    BOOST_REQUIRE(!query.get_context(ctx, header_link::terminal));
    BOOST_REQUIRE(!query.get_context(ctx, 1));
    BOOST_REQUIRE(ctx == context{});

    system::chain::context chain_ctx{};
    BOOST_REQUIRE(!query.get_context(chain_ctx, header_link::terminal));
    BOOST_REQUIRE(!query.get_context(chain_ctx, 1));
    BOOST_REQUIRE(chain_ctx == system::chain::context{});
}

BOOST_AUTO_TEST_CASE(query_properties_block__get_context__block1__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));

    const context expected{ 12, 34, 56 };
    BOOST_REQUIRE(query.set(test::block1, expected, {}, false, false));

    context ctx{};
    BOOST_REQUIRE(query.get_context(ctx, 1));
    BOOST_REQUIRE(ctx == expected);

    system::chain::context chain_ctx{};
    constexpr auto expected_timestamp = 0x4966bc61_u32;
    const system::chain::context chain_expected{ expected.flags, expected_timestamp, expected.mtp, expected.height, 0, 0 };
    BOOST_REQUIRE(query.get_context(chain_ctx, 1));
    BOOST_REQUIRE(chain_ctx == chain_expected);
}

BOOST_AUTO_TEST_CASE(query_properties_block__is_witness_committed__genesis__header_merkle_root)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));

    BOOST_REQUIRE(query.is_witness_committed(test::genesis.header().merkle_root(), 0));
    BOOST_REQUIRE(!query.is_witness_committed(system::one_hash, 0));
}

BOOST_AUTO_TEST_CASE(query_properties_block__is_witness_committed__committed__expected)
{
    using namespace system::chain;
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));

    const system::hash_digest reserved_hash{ 0x02 };
    const auto commitment = system::bitcoin_hash(system::one_hash, reserved_hash);
    const script commitment_script(system::splice(system::base16_chunk("6a24aa21a9ed"), commitment), false);
    const witness reserved{ system::chunk_cptrs{ system::to_shared(system::to_chunk(reserved_hash)) } };
    const inputs ins{ input{ point{}, script{ operations{ operation{ system::data_chunk{ 0x01, 0x02 }, false } } }, reserved, 0xffffffff } };
    const transaction coinbase{ 1, ins, outputs{ output{ 0, commitment_script } }, 0 };
    const block block1{ header{ 1, test::genesis.hash(), system::null_hash, 0, 0, 0 }, transactions{ coinbase } };
    BOOST_REQUIRE(query.set(block1, database::context{}, {}, false, false));

    const auto link = query.to_header(block1.hash());
    BOOST_REQUIRE(query.is_witness_committed(system::one_hash, link));
    BOOST_REQUIRE(!query.is_witness_committed(reserved_hash, link));
}

// get_wtxids

BOOST_AUTO_TEST_CASE(query_properties_block__get_wtxids__genesis__coinbase_witness_hash)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));

    const auto& coinbase = *test::genesis.transactions_ptr()->front();
    BOOST_REQUIRE_EQUAL(query.get_wtxids(0), system::hashes{ coinbase.hash(true) });
    BOOST_REQUIRE(query.get_wtxids(1).empty());
}

// is_block_segregated

BOOST_AUTO_TEST_CASE(query_properties_block__is_block_segregated__always__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_CHECK_EQUAL(store.create(test::events_handler), error::success);
    BOOST_CHECK(query.initialize(test::genesis));
    BOOST_CHECK(query.set(test::block1a, context{}, {}, false, false));
    BOOST_CHECK(!query.is_block_segregated(0));
    BOOST_CHECK( query.is_block_segregated(1));
    BOOST_CHECK(!query.is_block_segregated(2));
}

// is_milestone

BOOST_AUTO_TEST_CASE(query_properties_block__is_milestone__genesis__false)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_CHECK_EQUAL(store.create(test::events_handler), error::success);
    BOOST_CHECK(query.initialize(test::genesis));
    BOOST_CHECK(!query.is_milestone(0));
    BOOST_CHECK(!query.is_milestone(1));
}

BOOST_AUTO_TEST_CASE(query_properties_block__is_milestone__set__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_CHECK_EQUAL(store.create(test::events_handler), error::success);
    BOOST_CHECK(query.initialize(test::genesis));
    BOOST_CHECK(query.set(test::block1, context{}, {}, true, false));
    BOOST_CHECK(query.set(test::block2, context{}, {}, false, false));;
    BOOST_CHECK(!query.is_milestone(0));
    BOOST_CHECK(query.is_milestone(1));
    BOOST_CHECK(!query.is_milestone(2));
}

// is_compact

BOOST_AUTO_TEST_CASE(query_properties_block__is_compact__set__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_CHECK_EQUAL(store.create(test::events_handler), error::success);
    BOOST_CHECK(query.initialize(test::genesis));
    header_link link1{};
    header_link link2{};
    BOOST_CHECK(!query.set_code(link1, test::block1.header(), context{}, {}, false, true));
    BOOST_CHECK(!query.set_code(link2, test::block2.header(), context{}, {}, true, false));
    BOOST_CHECK(!query.is_compact(0));
    BOOST_CHECK(query.is_compact(link1));
    BOOST_CHECK(!query.is_milestone(link1));
    BOOST_CHECK(!query.is_compact(link2));
    BOOST_CHECK(query.is_milestone(link2));
}

// get_tx_keys

BOOST_AUTO_TEST_CASE(query_properties_block__get_tx_keys__not_found__empty)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_CHECK(!store.create(test::events_handler));
    BOOST_CHECK(query.get_tx_keys(query.to_header(system::null_hash)).empty());
    BOOST_CHECK(!store.close(test::events_handler));
}

// get_height1

BOOST_AUTO_TEST_CASE(query_properties_block__get_height1__always__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_CHECK(!store.create(test::events_handler));
    BOOST_CHECK(query.initialize(test::genesis));
    BOOST_CHECK(query.set(test::block1, context{ 0, 1, 0 }, {}, false, false));
    BOOST_CHECK(query.set(test::block2, context{ 0, 2, 0 }, {}, false, false));
    BOOST_CHECK(query.set(test::block3, context{ 0, 3, 0 }, {}, false, false));
    BOOST_CHECK(query.set(test::block1a, context{ 0, 1, 0 }, {}, false, false));
    BOOST_CHECK(query.set(test::block2a, context{ 0, 2, 0 }, {}, false, false));

    size_t out{};
    BOOST_CHECK(query.get_height(out, 0));
    BOOST_CHECK_EQUAL(out, 0u);
    BOOST_CHECK(query.get_height(out, 1));
    BOOST_CHECK_EQUAL(out, 1u);
    BOOST_CHECK(query.get_height(out, 2));
    BOOST_CHECK_EQUAL(out, 2u);
    BOOST_CHECK(query.get_height(out, 3));
    BOOST_CHECK_EQUAL(out, 3u);
    BOOST_CHECK(query.get_height(out, 4));
    BOOST_CHECK_EQUAL(out, 1u);
    BOOST_CHECK(query.get_height(out, 5));
    BOOST_CHECK_EQUAL(out, 2u);
    BOOST_CHECK(!query.get_height(out, 6));
}

// get_height2

BOOST_AUTO_TEST_CASE(query_properties_block__get_height2__always__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_CHECK(!store.create(test::events_handler));
    BOOST_CHECK(query.initialize(test::genesis));
    BOOST_CHECK(query.set(test::block1, context{ 0, 1, 0 }, {}, false, false));
    BOOST_CHECK(query.set(test::block2, context{ 0, 2, 0 }, {}, false, false));
    BOOST_CHECK(query.set(test::block3, context{ 0, 3, 0 }, {}, false, false));
    BOOST_CHECK(query.set(test::block1a, context{ 0, 1, 0 }, {}, false, false));
    BOOST_CHECK(query.set(test::block2a, context{ 0, 2, 0 }, {}, false, false));

    size_t out{};
    BOOST_CHECK(query.get_height(out, test::genesis.hash()));
    BOOST_CHECK_EQUAL(out, 0u);
    BOOST_CHECK(query.get_height(out, test::block1.hash()));
    BOOST_CHECK_EQUAL(out, 1u);
    BOOST_CHECK(query.get_height(out, test::block2.hash()));
    BOOST_CHECK_EQUAL(out, 2u);
    BOOST_CHECK(query.get_height(out, test::block3.hash()));
    BOOST_CHECK_EQUAL(out, 3u);
    BOOST_CHECK(query.get_height(out, test::block1a.hash()));
    BOOST_CHECK_EQUAL(out, 1u);
    BOOST_CHECK(query.get_height(out, test::block2a.hash()));
    BOOST_CHECK_EQUAL(out, 2u);
    BOOST_CHECK(!query.get_height(out, system::one_hash));
}

// get_position_tx

BOOST_AUTO_TEST_CASE(query_properties_block__get_position_tx__always__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_CHECK(!store.create(test::events_handler));
    BOOST_CHECK(query.initialize(test::genesis));
    BOOST_CHECK(query.set(test::block1a, context{ 0, 1, 0 }, {}, false, false));
    BOOST_CHECK(query.set(test::block2a, context{ 0, 2, 0 }, {}, false, false));
    BOOST_CHECK(query.set(test::block3a, context{ 0, 3, 0 }, {}, false, false));

    BOOST_CHECK_EQUAL(query.get_position_tx(0, 0), 0u);
    BOOST_CHECK_EQUAL(query.get_position_tx(1, 0), 1u);
    BOOST_CHECK_EQUAL(query.get_position_tx(2, 0), 2u);
    BOOST_CHECK_EQUAL(query.get_position_tx(2, 1), 3u);
    BOOST_CHECK_EQUAL(query.get_position_tx(3, 0), 4u);
    BOOST_CHECK(query.get_position_tx(3, 1).is_terminal());
    BOOST_CHECK(query.get_position_tx({}, 0).is_terminal());
}

// get_tx_count

BOOST_AUTO_TEST_CASE(query_properties_block__get_tx_count__coinbase__1)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_CHECK(!store.create(test::events_handler));
    BOOST_CHECK(query.initialize(test::genesis));
    BOOST_CHECK_EQUAL(query.get_tx_count(0), 1u);
}

BOOST_AUTO_TEST_SUITE_END()
