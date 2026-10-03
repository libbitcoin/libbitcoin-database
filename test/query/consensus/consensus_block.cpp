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
#include "../../test.hpp"
#include "../../mocks/blocks.hpp"
#include "../../mocks/chunk_store.hpp"

BOOST_FIXTURE_TEST_SUITE(query_consensus_tests, test::directory_setup_fixture)

// validate_pooled
// ----------------------------------------------------------------------------

using namespace system::chain;
constexpr auto pooled_flags = flags::bip113_rule;
constexpr uint64_t pooled_interval = 210'000;
constexpr uint64_t pooled_subsidy = 5'000'000'000;
const database::context pooled_context{ pooled_flags, 8, 9 };
const system::chain::context block_context{ pooled_flags, 0, 9, 8, 0, 0, 0 };
const auto genesis_hash = test::genesis.transactions_ptr()->front()->hash(false);

static transaction make_coinbase(uint64_t value) NOEXCEPT
{
    const script nops{ { { opcode::nop }, { opcode::nop } } };
    return
    {
        1,
        inputs{ input{ point{}, nops, witness{}, max_uint32 } },
        outputs{ output{ value, script{} } },
        0
    };
}

static transaction make_spend(const hash_digest& hash, uint32_t index,
    uint64_t value) NOEXCEPT
{
    return
    {
        1,
        inputs{ input{ point{ hash, index }, script{}, witness{}, max_uint32 } },
        outputs{ output{ value, script{} } },
        0
    };
}

static block make_block(const transactions& txs) NOEXCEPT
{
    const header head{ 1, test::block0_hash, hash_digest{ 0x42 }, 0, 0, 0 };
    return { head, transactions{ txs } };
}

static bool store_block(test::query_accessor& query, const block& block) NOEXCEPT
{
    return query.set(block, pooled_context, {}, false, false);
}

static bool pool_tx(test::query_accessor& query, const transaction& tx,
    uint64_t prevout, const database::context& ctx) NOEXCEPT
{
    const auto link = query.to_tx(tx.hash(false));
    for (const auto& in: *tx.inputs_ptr())
        in->prevout = system::to_shared<output>(prevout, script{});

    return query.set_pooled(link, tx, ctx);
}

BOOST_AUTO_TEST_CASE(query_consensus__validate_pooled__pooled__success_with_prevouts)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));

    const auto parent = make_spend(genesis_hash, 0, 4'000'000'000);
    const auto child = make_spend(parent.hash(false), 0, 3'000'000'000);
    const auto block = make_block({ make_coinbase(1), parent, child });
    BOOST_REQUIRE(store_block(query, block));

    parent.inputs_ptr()->front()->metadata.parent_tx = 0;
    parent.inputs_ptr()->front()->metadata.coinbase = true;
    BOOST_REQUIRE(pool_tx(query, parent, 5'000'000'000, pooled_context));
    BOOST_REQUIRE(pool_tx(query, child, 4'000'000'000, pooled_context));

    const auto link = query.to_header(block.hash());
    BOOST_REQUIRE_EQUAL(query.validate_pooled(link, block_context, pooled_interval, pooled_subsidy), error::success);

    table::prevout::slab_get cache{};
    cache.spends.resize(2);
    BOOST_REQUIRE(store.prevout.at(link.value, cache));
    BOOST_REQUIRE(cache.conflicts.empty());
    BOOST_REQUIRE_EQUAL(table::prevout::slab_get::output_tx_fk(cache.spends.at(0).first), 0u);
    BOOST_REQUIRE(table::prevout::slab_get::coinbase(cache.spends.at(0).first));
    BOOST_REQUIRE_EQUAL(cache.spends.at(1).first, table::prevout::tx::terminal);
    BOOST_REQUIRE_EQUAL(cache.spends.at(1).second, max_uint32);
}

BOOST_AUTO_TEST_CASE(query_consensus__validate_pooled__insufficient__unvalidated)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));

    const auto parent = make_spend(genesis_hash, 0, 4'000'000'000);
    const auto block = make_block({ make_coinbase(1), parent });
    BOOST_REQUIRE(store_block(query, block));

    parent.inputs_ptr()->front()->metadata.parent_tx = 0;
    BOOST_REQUIRE(pool_tx(query, parent, 5'000'000'000, database::context{ pooled_flags, 9, 9 }));

    const auto link = query.to_header(block.hash());
    BOOST_REQUIRE_EQUAL(query.validate_pooled(link, block_context, pooled_interval, pooled_subsidy), error::unvalidated);
}

BOOST_AUTO_TEST_CASE(query_consensus__validate_pooled__not_pooled__unvalidated)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));

    const auto block = make_block({ make_coinbase(1), make_spend(genesis_hash, 0, 1) });
    BOOST_REQUIRE(store_block(query, block));

    const auto link = query.to_header(block.hash());
    BOOST_REQUIRE_EQUAL(query.validate_pooled(link, block_context, pooled_interval, pooled_subsidy), error::unvalidated);
}

BOOST_AUTO_TEST_CASE(query_consensus__validate_pooled__overclaim__coinbase_value_limit)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));

    // Fee is 1'000'000'000, so claim may not exceed 6'000'000'000.
    const auto parent = make_spend(genesis_hash, 0, 4'000'000'000);
    const auto block = make_block({ make_coinbase(6'000'000'001), parent });
    BOOST_REQUIRE(store_block(query, block));

    parent.inputs_ptr()->front()->metadata.parent_tx = 0;
    BOOST_REQUIRE(pool_tx(query, parent, 5'000'000'000, pooled_context));

    const auto link = query.to_header(block.hash());
    BOOST_REQUIRE_EQUAL(query.validate_pooled(link, block_context, pooled_interval, pooled_subsidy), system::error::coinbase_value_limit);
}

BOOST_AUTO_TEST_CASE(query_consensus__validate_pooled__forward_reference__forward_reference)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));

    const auto parent = make_spend(genesis_hash, 0, 4'000'000'000);
    const auto child = make_spend(parent.hash(false), 0, 3'000'000'000);
    const auto block = make_block({ make_coinbase(1), child, parent });
    BOOST_REQUIRE(store_block(query, block));

    parent.inputs_ptr()->front()->metadata.parent_tx = 0;
    BOOST_REQUIRE(pool_tx(query, parent, 5'000'000'000, pooled_context));
    BOOST_REQUIRE(pool_tx(query, child, 4'000'000'000, pooled_context));

    const auto link = query.to_header(block.hash());
    BOOST_REQUIRE_EQUAL(query.validate_pooled(link, block_context, pooled_interval, pooled_subsidy), system::error::forward_reference);
}

BOOST_AUTO_TEST_CASE(query_consensus__validate_pooled__internal_double_spend__block_internal_double_spend)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));

    const auto spend1 = make_spend(genesis_hash, 0, 4'000'000'000);
    const auto spend2 = make_spend(genesis_hash, 0, 3'000'000'000);
    const auto block = make_block({ make_coinbase(1), spend1, spend2 });
    BOOST_REQUIRE(store_block(query, block));

    spend1.inputs_ptr()->front()->metadata.parent_tx = 0;
    spend2.inputs_ptr()->front()->metadata.parent_tx = 0;
    BOOST_REQUIRE(pool_tx(query, spend1, 5'000'000'000, pooled_context));
    BOOST_REQUIRE(pool_tx(query, spend2, 5'000'000'000, pooled_context));

    const auto link = query.to_header(block.hash());
    BOOST_REQUIRE_EQUAL(query.validate_pooled(link, block_context, pooled_interval, pooled_subsidy), system::error::block_internal_double_spend);
}

BOOST_AUTO_TEST_CASE(query_consensus__validate_pooled__internal_coinbase_spend__coinbase_maturity)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));

    const auto coinbase = make_coinbase(1);
    const auto spend = make_spend(coinbase.hash(false), 0, 1);
    const auto block = make_block({ coinbase, spend });
    BOOST_REQUIRE(store_block(query, block));

    BOOST_REQUIRE(pool_tx(query, spend, 1, pooled_context));

    const auto link = query.to_header(block.hash());
    BOOST_REQUIRE_EQUAL(query.validate_pooled(link, block_context, pooled_interval, pooled_subsidy), system::error::coinbase_maturity);
}

BOOST_AUTO_TEST_CASE(query_consensus__validate_pooled__mature__block_confirmable)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));

    // Genesis is unspendable, so spend a strong coinbase at height one.
    const auto source = make_coinbase(5'000'000'000);
    const block first{ header{ 1, test::block0_hash, hash_digest{ 0x01 }, 0, 0, 0 }, transactions{ source } };
    BOOST_REQUIRE(query.set(first, database::context{ pooled_flags, 1, 0 }, {}, false, false));
    BOOST_REQUIRE(query.set_strong(query.to_header(first.hash())));

    const database::context mature_context{ pooled_flags, 200, 9 };
    const system::chain::context mature_block{ pooled_flags, 0, 9, 200, 0, 0, 0 };
    const auto parent = make_spend(source.hash(false), 0, 4'000'000'000);
    const auto child = make_spend(parent.hash(false), 0, 3'000'000'000);
    const auto block = make_block({ make_coinbase(1), parent, child });
    BOOST_REQUIRE(query.set(block, mature_context, {}, false, false));

    parent.inputs_ptr()->front()->metadata.parent_tx = query.to_tx(source.hash(false));
    parent.inputs_ptr()->front()->metadata.coinbase = true;
    BOOST_REQUIRE(pool_tx(query, parent, 5'000'000'000, mature_context));
    BOOST_REQUIRE(pool_tx(query, child, 4'000'000'000, mature_context));

    const auto link = query.to_header(block.hash());
    BOOST_REQUIRE_EQUAL(query.validate_pooled(link, mature_block, pooled_interval, pooled_subsidy), error::success);
    BOOST_REQUIRE_EQUAL(query.block_confirmable(link), error::success);
}

BOOST_AUTO_TEST_CASE(query_consensus__validate_pooled__disabled__unvalidated)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    settings.pool.buckets = 0;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE_EQUAL(query.validate_pooled(0, block_context, pooled_interval, pooled_subsidy), error::unvalidated);
}

BOOST_AUTO_TEST_SUITE_END()
