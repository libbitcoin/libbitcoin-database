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

BC_FIXTURE_TEST_SUITE(query_consensus_tests, test::directory_setup_fixture)

// block_confirmable

constexpr auto bip68 = system::chain::flags::bip68_rule;

BC_AUTO_TEST_CASE(query_consensus__block_confirmable__bad_link__integrity_block_confirmable1)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));
    BC_REQUIRE(query.set(test::block1, context{ bip68, 1, 0 }, {}, false, false));
    BC_REQUIRE_EQUAL(query.block_confirmable(2), error::integrity_block_confirmable1);
}

// These pass but are limited by vectors with mostly coinbases.
// ----------------------------------------------------------------------------

BC_AUTO_TEST_CASE(query_consensus__block_confirmable__null_points__success)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));
    BC_REQUIRE(query.set(test::block1, context{ bip68 }, {}, false, false));
    BC_REQUIRE(query.set(test::block2, context{ bip68 }, {}, false, false));
    BC_REQUIRE(query.set(test::block3, context{ bip68 }, {}, false, false));

    // ALL COINBASE TXS
    // block1/2/3 at links 1/2/3 confirming at heights 1/2/3.
    // blocks have only coinbase txs, all txs should be set strong before calling
    // confirmable, but these are bip30 default configuration.
    BC_REQUIRE_EQUAL(query.block_confirmable(1), error::success);
    BC_REQUIRE_EQUAL(query.block_confirmable(2), error::success);
    BC_REQUIRE_EQUAL(query.block_confirmable(3), error::success);
}

BC_AUTO_TEST_CASE(query_consensus__block_confirmable__missing_prevouts__integrity)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));
    BC_REQUIRE(query.set(test::block1a, context{ bip68, 1, 0 }, {}, false, false));

    // ONLY COINBASE TXS
    // block1a is missing all three input prevouts.
    BC_REQUIRE(query.set_strong(1));
    BC_REQUIRE_EQUAL(query.block_confirmable(1), error::success);
}

BC_AUTO_TEST_CASE(query_consensus__block_confirmable__spend_gensis__coinbase_maturity)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    // block_spend_genesis spends the genesis output.
    BC_REQUIRE(query.set(test::block_spend_genesis, context{ 0, 101, 0 }, {}, false, false));
    BC_REQUIRE(query.set_strong(1));

    // COINBASE TX
    // 1 + 100 = 101 (maturity, except genesis)
    BC_REQUIRE_EQUAL(query.block_confirmable(1), error::success);
}

BC_AUTO_TEST_CASE(query_consensus__block_confirmable__immature_prevouts__coinbase_maturity)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    // block1b has only a coinbase tx.
    BC_REQUIRE(query.set(test::block1b, context{ bip68, 1, 0 }, {}, false, false));
    BC_REQUIRE(query.set_strong(1));
    BC_REQUIRE_EQUAL(query.block_confirmable(1), error::success);

    // COINBASE TX
    // block2b prematurely spends block1b's coinbase outputs.
    BC_REQUIRE(query.set(test::block2b, context{ 0, 100, 0 }, {}, false, false));
    BC_REQUIRE(query.set_strong(2));
    BC_REQUIRE_EQUAL(query.block_confirmable(2), error::success);
}

BC_AUTO_TEST_CASE(query_consensus__block_confirmable__mature_prevouts__success)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    // block1b has only a coinbase tx.
    BC_REQUIRE(query.set(test::block1b, context{ bip68, 1, 0 }, {}, false, false));
    BC_REQUIRE(query.set_strong(1));
    BC_REQUIRE_EQUAL(query.block_confirmable(1), error::success);

    // COINBASE TX
    // block2b spends block1b's coinbase outputs.
    BC_REQUIRE(query.set(test::block2b, context{ 0, 101, 0 }, {}, false, false));
    BC_REQUIRE(query.set_strong(2));
    BC_REQUIRE_EQUAL(query.block_confirmable(2), error::success);
}

BC_AUTO_TEST_CASE(query_consensus__block_confirmable__spend_non_coinbase__success)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    // block1a has non-coinbase tx/outputs.
    BC_REQUIRE(query.set(test::block1a, context{ 0, 1, 0 }, {}, false, false));
    BC_REQUIRE(query.set_strong(1));

    // block_spend_1a spends both block1a outputs.
    BC_REQUIRE(query.set(test::block_spend_1a, context{ 0, 2, 0 }, {}, false, false));
    BC_REQUIRE(query.set_strong(2));

    // COINBASE TX
    // Maturity applies only to coinbase prevouts.
    BC_REQUIRE_EQUAL(query.block_confirmable(2), error::success);
}

static bool set_internal_spend(test::query_accessor& query,
    const system::chain::block& block) NOEXCEPT
{
    const auto& parent = *test::block1a.transactions_ptr()->front();
    const auto& txs = *block.transactions_ptr();
    auto& metadata = txs.at(1)->inputs_ptr()->front()->metadata;
    metadata.parent_tx = query.to_tx(parent.hash(false)).value;
    metadata.coinbase = false;

    return query.set(test::block1a, context{ 0, 1, 0 }, {}, false, false)
        && query.set_strong(1)
        && query.set(block, context{ bip68, 2, 0 }, {}, false, false)
        && query.set_prevouts(2, block)
        && query.set_strong(2);
}

BC_AUTO_TEST_CASE(query_consensus__block_confirmable__internal_relative_locked__relative_time_locked)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));
    BC_REQUIRE(set_internal_spend(query, test::block_spend_internal_locked_1a));
    BC_REQUIRE(query.block_confirmable(2) == system::error::relative_time_locked);
}

BC_AUTO_TEST_CASE(query_consensus__block_confirmable__internal_relative_disabled__success)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));
    BC_REQUIRE(set_internal_spend(query, test::block_spend_internal_disabled_1a));
    BC_REQUIRE_EQUAL(query.block_confirmable(2), error::success);
}

BC_AUTO_TEST_CASE(query_consensus__block_confirmable__internal_relative_zero__success)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));
    BC_REQUIRE(set_internal_spend(query, test::block_spend_internal_unlocked_1a));
    BC_REQUIRE_EQUAL(query.block_confirmable(2), error::success);
}

// These pass but vectors need to be updated to create clear test conditions.
// ----------------------------------------------------------------------------

BC_AUTO_TEST_CASE(query_consensus__block_confirmable__spend_coinbase_and_internal_immature__coinbase_maturity)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    // block1b has coinbase tx/outputs.
    BC_REQUIRE(query.set(test::block1b, context{ 0, 1, 0 }, {}, false, false));
    BC_REQUIRE(query.set_strong(1));

    // block_spend_internal_2b spends first block1a output and first own output.
    BC_REQUIRE(query.set(test::block_spend_internal_2b, context{ 0, 100, 0 }, {}, false, false));
    ////BC_REQUIRE(query.set_strong(2));

    // Not confirmable because tx not strong.
    BC_REQUIRE_EQUAL(query.block_confirmable(2), error::integrity_get_prevouts);
    BC_REQUIRE(query.set_strong(2));

    // Not confirmable because lack of maturity.
    BC_REQUIRE_EQUAL(query.block_confirmable(2), error::integrity_get_prevouts);
}

BC_AUTO_TEST_CASE(query_consensus__block_confirmable__spend_coinbase_and_internal_mature__success)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    // block1b has coinbase tx/outputs.
    BC_REQUIRE(query.set(test::block1b, context{ 0, 1, 0 }, {}, false, false));
    BC_REQUIRE(query.set_strong(1));

    BC_REQUIRE(query.set(test::block_spend_internal_2b, context{ 0, 101, 0 }, {}, false, false));
    ////BC_REQUIRE(query.set_strong(2));

    // Not confirmable because tx not strong.
    BC_REQUIRE_EQUAL(query.block_confirmable(2), error::integrity_get_prevouts);

    // block_spend_internal_2b spends first block1b output and first own output.
    // But first block1b is first tx in block_spend_internal_2b so excluded as coinbase.
    // It spends only its first own output (coinbase) and that can never be mature.
    // But spend target is not stored as coinbase because it's not a null point.
    BC_REQUIRE(query.set_strong(2));
    BC_REQUIRE_EQUAL(query.block_confirmable(2), error::integrity_get_prevouts);
}

BC_AUTO_TEST_CASE(query_consensus__block_confirmable__confirmed_double_spend__confirmed_double_spend)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    // block1a has non-coinbase tx/outputs.
    BC_REQUIRE(query.set(test::block1a, context{ 0, 1, 0 }, {}, false, false));
    BC_REQUIRE(query.set_strong(1));

    // block2a spends both block1a outputs (though not itself confirmable is set strong).
    BC_REQUIRE(query.set(test::block2a, context{ 0, 2, 0 }, {}, false, false));
    BC_REQUIRE(query.set_strong(2));

    // block_spend_1a (also) spends both block1a outputs (and is otherwise confirmable).
    BC_REQUIRE(query.set(test::block_spend_1a, context{ bip68, 3, 0 }, {}, false, false));
    BC_REQUIRE(query.set_strong(3));

    // Not confirmable because of intervening block2a implies double spend.
    BC_REQUIRE_EQUAL(query.block_confirmable(3), error::success);
}

BC_AUTO_TEST_CASE(query_consensus__block_confirmable__unconfirmed_double_spend__success)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    // block1a has non-coinbase tx/outputs.
    BC_REQUIRE(query.set(test::block1a, context{ 0, 1, 0 }, {}, false, false));
    BC_REQUIRE(query.set_strong(1));

    // tx5 spends first block1a output.
    BC_REQUIRE(query.set(test::tx5));

    // block_spend_1a (also) spends both block1a outputs.
    BC_REQUIRE(query.set(test::block_spend_1a, context{ 0, 2, 0 }, {}, false, false));
    BC_REQUIRE(query.set_strong(2));

    // Confirmable because of intervening tx5 is unconfirmed double spend.
    BC_REQUIRE_EQUAL(query.block_confirmable(2), error::success);
}

// validate_pooled
// ----------------------------------------------------------------------------

using namespace system::chain;
constexpr auto pooled_flags = flags::bip113_rule;
constexpr size_t pooled_interval = 210'000;
constexpr uint64_t pooled_subsidy = 5'000'000'000;
const database::context pooled_context{ pooled_flags, 8, 9 };
const system::chain::context block_context{ pooled_flags, 0, 9, 8, 0, 0, 0 };
static hash_digest genesis_hash() NOEXCEPT
{
    return test::genesis.transactions_ptr()->front()->hash(false);
}

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

    return query.set_pooled(link, tx, ctx, 0);
}

BC_AUTO_TEST_CASE(query_consensus__validate_pooled__pooled__success_with_prevouts)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    const auto parent = make_spend(genesis_hash(), 0, 4'000'000'000);
    const auto child = make_spend(parent.hash(false), 0, 3'000'000'000);
    const auto block = make_block({ make_coinbase(1), parent, child });
    BC_REQUIRE(store_block(query, block));

    parent.inputs_ptr()->front()->metadata.parent_tx = 0;
    parent.inputs_ptr()->front()->metadata.coinbase = true;
    BC_REQUIRE(pool_tx(query, parent, 5'000'000'000, pooled_context));
    BC_REQUIRE(pool_tx(query, child, 4'000'000'000, pooled_context));

    const auto link = query.to_header(block.hash());
    BC_REQUIRE_EQUAL(query.validate_pooled(link, block_context, pooled_interval, pooled_subsidy), error::success);

    table::prevout::slab_get cache{};
    cache.spends.resize(2);
    BC_REQUIRE(store.prevout.at(link.value, cache));
    BC_REQUIRE(cache.conflicts.empty());
    BC_REQUIRE_EQUAL(table::prevout::slab_get::output_tx_fk(cache.spends.at(0).first), 0u);
    BC_REQUIRE(table::prevout::slab_get::coinbase(cache.spends.at(0).first));
    BC_REQUIRE_EQUAL(cache.spends.at(1).first, table::prevout::tx::terminal);
    BC_REQUIRE_EQUAL(cache.spends.at(1).second, max_uint32);
}

BC_AUTO_TEST_CASE(query_consensus__validate_pooled__insufficient__unvalidated)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    const auto parent = make_spend(genesis_hash(), 0, 4'000'000'000);
    const auto block = make_block({ make_coinbase(1), parent });
    BC_REQUIRE(store_block(query, block));

    parent.inputs_ptr()->front()->metadata.parent_tx = 0;
    BC_REQUIRE(pool_tx(query, parent, 5'000'000'000, database::context{ pooled_flags, 9, 9 }));

    const auto link = query.to_header(block.hash());
    BC_REQUIRE_EQUAL(query.validate_pooled(link, block_context, pooled_interval, pooled_subsidy), error::unvalidated);
}

BC_AUTO_TEST_CASE(query_consensus__validate_pooled__not_pooled__unvalidated)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    const auto block = make_block({ make_coinbase(1), make_spend(genesis_hash(), 0, 1) });
    BC_REQUIRE(store_block(query, block));

    const auto link = query.to_header(block.hash());
    BC_REQUIRE_EQUAL(query.validate_pooled(link, block_context, pooled_interval, pooled_subsidy), error::unvalidated);
}

BC_AUTO_TEST_CASE(query_consensus__validate_pooled__overclaim__coinbase_value_limit)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    // Fee is 1'000'000'000, so claim may not exceed 6'000'000'000.
    const auto parent = make_spend(genesis_hash(), 0, 4'000'000'000);
    const auto block = make_block({ make_coinbase(6'000'000'001), parent });
    BC_REQUIRE(store_block(query, block));

    parent.inputs_ptr()->front()->metadata.parent_tx = 0;
    BC_REQUIRE(pool_tx(query, parent, 5'000'000'000, pooled_context));

    const auto link = query.to_header(block.hash());
    BC_REQUIRE_EQUAL(query.validate_pooled(link, block_context, pooled_interval, pooled_subsidy), system::error::coinbase_value_limit);
}

BC_AUTO_TEST_CASE(query_consensus__validate_pooled__forward_reference__forward_reference)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    const auto parent = make_spend(genesis_hash(), 0, 4'000'000'000);
    const auto child = make_spend(parent.hash(false), 0, 3'000'000'000);
    const auto block = make_block({ make_coinbase(1), child, parent });
    BC_REQUIRE(store_block(query, block));

    parent.inputs_ptr()->front()->metadata.parent_tx = 0;
    BC_REQUIRE(pool_tx(query, parent, 5'000'000'000, pooled_context));
    BC_REQUIRE(pool_tx(query, child, 4'000'000'000, pooled_context));

    const auto link = query.to_header(block.hash());
    BC_REQUIRE_EQUAL(query.validate_pooled(link, block_context, pooled_interval, pooled_subsidy), system::error::forward_reference);
}

BC_AUTO_TEST_CASE(query_consensus__validate_pooled__internal_double_spend__block_internal_double_spend)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    const auto spend1 = make_spend(genesis_hash(), 0, 4'000'000'000);
    const auto spend2 = make_spend(genesis_hash(), 0, 3'000'000'000);
    const auto block = make_block({ make_coinbase(1), spend1, spend2 });
    BC_REQUIRE(store_block(query, block));

    spend1.inputs_ptr()->front()->metadata.parent_tx = 0;
    spend2.inputs_ptr()->front()->metadata.parent_tx = 0;
    BC_REQUIRE(pool_tx(query, spend1, 5'000'000'000, pooled_context));
    BC_REQUIRE(pool_tx(query, spend2, 5'000'000'000, pooled_context));

    const auto link = query.to_header(block.hash());
    BC_REQUIRE_EQUAL(query.validate_pooled(link, block_context, pooled_interval, pooled_subsidy), system::error::block_internal_double_spend);
}

BC_AUTO_TEST_CASE(query_consensus__validate_pooled__internal_coinbase_spend__coinbase_maturity)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    const auto coinbase = make_coinbase(1);
    const auto spend = make_spend(coinbase.hash(false), 0, 1);
    const auto block = make_block({ coinbase, spend });
    BC_REQUIRE(store_block(query, block));

    BC_REQUIRE(pool_tx(query, spend, 1, pooled_context));

    const auto link = query.to_header(block.hash());
    BC_REQUIRE_EQUAL(query.validate_pooled(link, block_context, pooled_interval, pooled_subsidy), system::error::coinbase_maturity);
}

BC_AUTO_TEST_CASE(query_consensus__validate_pooled__mature__block_confirmable)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    // Genesis is unspendable, so spend a strong coinbase at height one.
    const auto source = make_coinbase(5'000'000'000);
    const block first{ header{ 1, test::block0_hash, hash_digest{ 0x01 }, 0, 0, 0 }, transactions{ source } };
    BC_REQUIRE(query.set(first, database::context{ pooled_flags, 1, 0 }, {}, false, false));
    BC_REQUIRE(query.set_strong(query.to_header(first.hash())));

    const database::context mature_context{ pooled_flags, 200, 9 };
    const system::chain::context mature_block{ pooled_flags, 0, 9, 200, 0, 0, 0 };
    const auto parent = make_spend(source.hash(false), 0, 4'000'000'000);
    const auto child = make_spend(parent.hash(false), 0, 3'000'000'000);
    const auto block = make_block({ make_coinbase(1), parent, child });
    BC_REQUIRE(query.set(block, mature_context, {}, false, false));

    parent.inputs_ptr()->front()->metadata.parent_tx = query.to_tx(source.hash(false));
    parent.inputs_ptr()->front()->metadata.coinbase = true;
    BC_REQUIRE(pool_tx(query, parent, 5'000'000'000, mature_context));
    BC_REQUIRE(pool_tx(query, child, 4'000'000'000, mature_context));

    const auto link = query.to_header(block.hash());
    BC_REQUIRE_EQUAL(query.validate_pooled(link, mature_block, pooled_interval, pooled_subsidy), error::success);
    BC_REQUIRE_EQUAL(query.block_confirmable(link), error::success);
}

BC_AUTO_TEST_CASE(query_consensus__validate_pooled__disabled__unvalidated)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    settings.pool.buckets = 0;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));
    BC_REQUIRE_EQUAL(query.validate_pooled(0, block_context, pooled_interval, pooled_subsidy), error::unvalidated);
}

BC_AUTO_TEST_SUITE_END()
