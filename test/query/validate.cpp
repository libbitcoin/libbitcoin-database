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
#include "../test.hpp"
#include "../mocks/blocks.hpp"
#include "../mocks/chunk_store.hpp"

BOOST_FIXTURE_TEST_SUITE(query_validate_tests, test::directory_setup_fixture)

// nop event handler.
const auto events_handler = [](auto, auto) {};

BOOST_AUTO_TEST_CASE(query_validate__get_top_timestamp__always__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(events_handler), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, context{}, false, false));
    BOOST_REQUIRE(query.set(test::block2, context{}, false, false));
    BOOST_REQUIRE(query.set(test::block3, context{}, false, false));
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

BOOST_AUTO_TEST_CASE(query_validate__get_timestamp__genesis__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(events_handler), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));

    uint32_t timestamp{};
    BOOST_REQUIRE(!query.get_timestamp(timestamp, 1));
    BOOST_REQUIRE(query.get_timestamp(timestamp, 0));
    BOOST_REQUIRE_EQUAL(timestamp, 0x495fab29_u32);
}

BOOST_AUTO_TEST_CASE(query_validate__get_version__genesis__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(events_handler), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));

    uint32_t version{};
    BOOST_REQUIRE(!query.get_version(version, 1));
    BOOST_REQUIRE(query.get_version(version, 0));
    BOOST_REQUIRE_EQUAL(version, 0x00000001_u32);
}

BOOST_AUTO_TEST_CASE(query_validate__get_bits__genesis__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(events_handler), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));

    uint32_t bits{};
    BOOST_REQUIRE(!query.get_bits(bits, 1));
    BOOST_REQUIRE(query.get_bits(bits, 0));
    BOOST_REQUIRE_EQUAL(bits, 0x1d00ffff_u32);
}

BOOST_AUTO_TEST_CASE(query_validate__get_context__genesis__default)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(events_handler), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));

    context ctx{};
    BOOST_REQUIRE(query.get_context(ctx, 0));
    BOOST_REQUIRE(ctx == context{});

    system::chain::context chain_ctx{};
    BOOST_REQUIRE(query.get_context(chain_ctx, 0));
    BOOST_REQUIRE(chain_ctx == system::chain::context{});
}

BOOST_AUTO_TEST_CASE(query_validate__get_context__invalid__default)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(events_handler), error::success);
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

BOOST_AUTO_TEST_CASE(query_validate__get_context__block1__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(events_handler), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));

    const context expected{ 12, 34, 56 };
    BOOST_REQUIRE(query.set(test::block1, expected, false, false));

    context ctx{};
    BOOST_REQUIRE(query.get_context(ctx, 1));
    BOOST_REQUIRE(ctx == expected);

    system::chain::context chain_ctx{};
    const system::chain::context chain_expected{ expected.flags, 0, expected.mtp, expected.height, 0, 0 };
    BOOST_REQUIRE(query.get_context(chain_ctx, 1));
    BOOST_REQUIRE(chain_ctx == chain_expected);
}

BOOST_AUTO_TEST_CASE(query_validate__get_block_state__invalid_link__unassociated)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(events_handler), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));

    uint64_t fees{};
    BOOST_REQUIRE_EQUAL(query.get_header_state(1), error::unvalidated);
    BOOST_REQUIRE_EQUAL(query.get_block_state(1), error::unassociated);
    BOOST_REQUIRE_EQUAL(fees, 0u);
}

BOOST_AUTO_TEST_CASE(query_validate__get_block_state__unassociated_link__unassociated)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(events_handler), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1.header(), context{}, false));
    BOOST_REQUIRE(query.set(*test::block1.transactions_ptr()->front()));

    uint64_t fees{};
    BOOST_REQUIRE_EQUAL(query.get_header_state(1), error::unvalidated);
    BOOST_REQUIRE_EQUAL(query.get_block_state(1), error::unassociated);
    BOOST_REQUIRE_EQUAL(fees, 0u);
}

BOOST_AUTO_TEST_CASE(query_validate__get_block_state__unvalidated_link__unvalidated)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(events_handler), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, context{}, false, false));

    uint64_t fees{};
    BOOST_REQUIRE_EQUAL(query.get_header_state(1), error::unvalidated);
    BOOST_REQUIRE_EQUAL(query.get_block_state(1), error::unvalidated);
    BOOST_REQUIRE_EQUAL(fees, 0u);
}

BOOST_AUTO_TEST_CASE(query_validate__get_block_state__confirmable__block_confirmable)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(events_handler), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, context{}, false, false));

    uint64_t fees{};
    BOOST_REQUIRE_EQUAL(query.get_header_state(0), error::unvalidated);
    BOOST_REQUIRE_EQUAL(query.get_block_state(0), error::unvalidated);
    BOOST_REQUIRE_EQUAL(fees, 0u);

    BOOST_REQUIRE(query.set_block_confirmable(1));
    BOOST_REQUIRE_EQUAL(query.get_header_state(1), error::block_confirmable);
    BOOST_REQUIRE_EQUAL(query.get_block_state(1), error::block_confirmable);
    BOOST_REQUIRE_EQUAL(fees, 0u);
}

BOOST_AUTO_TEST_CASE(query_validate__get_block_state__valid__block_valid)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(events_handler), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, context{}, false, false));

    BOOST_REQUIRE(query.set_block_valid(1, 42));
    BOOST_REQUIRE_EQUAL(query.get_header_state(1), error::block_valid);
    BOOST_REQUIRE_EQUAL(query.get_block_state(1), error::block_valid);
}

BOOST_AUTO_TEST_CASE(query_validate__get_block_state__unconfirmable__block_unconfirmable)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(events_handler), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, context{}, false, false));

    BOOST_REQUIRE(query.set_block_unconfirmable(1));
    BOOST_REQUIRE_EQUAL(query.get_header_state(1), error::block_unconfirmable);
    BOOST_REQUIRE_EQUAL(query.get_block_state(1), error::block_unconfirmable);
}

BOOST_AUTO_TEST_CASE(query_validate__get_tx_state__invalid_link__unvalidated)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(events_handler), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));

    uint64_t fee{};
    size_t sigops{};
    BOOST_REQUIRE_EQUAL(query.get_tx_state(1, {}), error::unvalidated);
    BOOST_REQUIRE_EQUAL(query.get_tx_state(fee, sigops, 1, {}), error::unvalidated);
    BOOST_REQUIRE_EQUAL(fee, 0u);
}

BOOST_AUTO_TEST_CASE(query_validate__get_tx_state__unvalidated__unvalidated)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(events_handler), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, context{}, false, false));

    uint64_t fee{};
    size_t sigops{};
    BOOST_REQUIRE_EQUAL(query.get_tx_state(1, {}), error::unvalidated);
    BOOST_REQUIRE_EQUAL(query.get_tx_state(fee, sigops, 1, {}), error::unvalidated);
    BOOST_REQUIRE_EQUAL(fee, 0u);
}

BOOST_AUTO_TEST_CASE(query_validate__get_tx_state__connected_out_of_context__unvalidated)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    settings.validated_tx_buckets = 1;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(events_handler), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, context{}, false, false));
    BOOST_REQUIRE(query.set(test::block2, context{}, false, false));
    BOOST_REQUIRE(query.set(test::block3, context{}, false, false));

    uint64_t fee{};
    size_t sigops{};
    constexpr context ctx{ 7, 8, 9 };

    // Set a context which does not match ctx.
    BOOST_REQUIRE(query.set_tx_connected(0, { 1, 5, 9 }, 0, 0));
    BOOST_REQUIRE(query.set_tx_connected(1, { 2, 6, 0 }, 0, 0));
    BOOST_REQUIRE(query.set_tx_connected(2, { 3, 7, 1 }, 0, 0));
    BOOST_REQUIRE(query.set_tx_connected(3, { 4, 8, 2 }, 0, 0));
    BOOST_REQUIRE_EQUAL(query.get_tx_state(1, ctx), error::unvalidated);
    BOOST_REQUIRE_EQUAL(query.get_tx_state(fee, sigops, 1, ctx), error::unvalidated);
    BOOST_REQUIRE_EQUAL(fee, 0u);
    BOOST_REQUIRE_EQUAL(sigops, 0u);
}

BOOST_AUTO_TEST_CASE(query_validate__get_tx_state__connected_in_context__tx_connected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    settings.validated_tx_buckets = 1;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(events_handler), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, context{}, false, false));
    BOOST_REQUIRE(query.set(test::block2, context{}, false, false));
    BOOST_REQUIRE(query.set(test::block3, context{}, false, false));

    uint64_t fee{};
    size_t sigops{};
    constexpr uint64_t expected_fee = 42;
    constexpr size_t expected_sigops = 24;
    constexpr context ctx{ 7, 8, 9 };
    BOOST_REQUIRE(query.set_tx_connected(0, ctx, 11, 12));
    BOOST_REQUIRE(query.set_tx_connected(1, ctx, 13, 14));
    BOOST_REQUIRE(query.set_tx_connected(2, ctx, expected_fee, expected_sigops));
    BOOST_REQUIRE(query.set_tx_connected(2, { 1, 5, 9 }, 15, 16));
    BOOST_REQUIRE(query.set_tx_connected(2, { 2, 6, 0 }, 17, 18));
    BOOST_REQUIRE(query.set_tx_connected(3, ctx, 19, 20));
    BOOST_REQUIRE_EQUAL(query.get_tx_state(2, ctx), error::tx_connected);
    BOOST_REQUIRE_EQUAL(query.get_tx_state(fee, sigops, 2, ctx), error::tx_connected);
    BOOST_REQUIRE_EQUAL(fee, expected_fee);
    BOOST_REQUIRE_EQUAL(sigops, expected_sigops);
}

BOOST_AUTO_TEST_CASE(query_validate__get_tx_state__connected_in_context__tx_disconnected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(events_handler), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, context{}, false, false));
    BOOST_REQUIRE(query.set(test::block2, context{}, false, false));
    BOOST_REQUIRE(query.set(test::block3, context{}, false, false));

    uint64_t fee{};
    size_t sigops{};
    constexpr context ctx{ 7, 8, 9 };
    BOOST_REQUIRE(query.set_tx_disconnected(4, ctx));
    BOOST_REQUIRE_EQUAL(query.get_tx_state(4, ctx), error::tx_disconnected);
    BOOST_REQUIRE_EQUAL(query.get_tx_state(fee, sigops, 4, ctx), error::tx_disconnected);
    BOOST_REQUIRE_EQUAL(fee, 0u);
    BOOST_REQUIRE_EQUAL(sigops, 0u);
}

BOOST_AUTO_TEST_SUITE_END()
