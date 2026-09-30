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

using namespace system::chain;
using context = database::context;

static transaction make_coinbase() NOEXCEPT
{
    const script nops{ { { opcode::nop }, { opcode::nop } } };
    const inputs ins{ input{ point{}, nops, witness{}, max_uint32 } };
    return { 1, ins, outputs{ output{ 1, script{} } }, 0 };
}

// set_prevouts
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(query_consensus__set_prevouts__coinbase_only__true)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, context{ 0, 1, 0 }, {}, false, false));
    BOOST_REQUIRE(query.set_prevouts(1, test::block1));
}

BOOST_AUTO_TEST_CASE(query_consensus__set_prevouts__spending_block__true)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1a, context{ 0, 1, 0 }, {}, false, false));
    BOOST_REQUIRE(query.set_strong(1));
    BOOST_REQUIRE(query.set(test::block_spend_1a, context{ 0, 2, 0 }, {}, false, false));
    BOOST_REQUIRE(query.set_strong(2));
    BOOST_REQUIRE(query.set_prevouts(2, test::block_spend_1a));
}

BOOST_AUTO_TEST_CASE(query_consensus__set_prevouts__coinbase_and_spend__true)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1a, context{ 0, 1, 0 }, {}, false, false));
    BOOST_REQUIRE(query.set_strong(1));
    BOOST_REQUIRE(query.set(test::block_coinbase_spend_1a, context{ 0, 2, 0 }, {}, false, false));
    BOOST_REQUIRE(query.set_strong(2));
    BOOST_REQUIRE(!query.to_spending_txs(2).empty());
    BOOST_REQUIRE(query.set_prevouts(2, test::block_coinbase_spend_1a));
}

// get_block_prevouts
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(query_consensus__get_block_prevouts__coinbase_only__empty)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, context{ 0, 1, 0 }, {}, false, false));

    system::data_chunk prevouts{};
    table::prevout::spends spends{};
    tx_links conflicts{};
    BOOST_REQUIRE(query.get_block_prevouts(prevouts, spends, conflicts, 1));
    BOOST_REQUIRE(prevouts.empty());
    BOOST_REQUIRE(spends.empty());
    BOOST_REQUIRE(conflicts.empty());
    BOOST_REQUIRE(query.set_prevouts(1, spends, conflicts));
}

BOOST_AUTO_TEST_CASE(query_consensus__get_block_prevouts__spending_block__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1a, context{ 0, 1, 0 }, {}, false, false));
    BOOST_REQUIRE(query.set_strong(1));
    const block spending{ header{ 1, test::block1a.hash(), hash_digest{ 0x43 }, 0, 0, 0 }, transactions{ make_coinbase(), test::tx4 } };
    BOOST_REQUIRE(query.set(spending, context{ 0, 2, 0 }, {}, false, false));
    BOOST_REQUIRE(query.set_strong(2));

    // tx4 spends outputs 0 and 1 of the block1a transaction.
    const auto& parent = *test::block1a.transactions_ptr()->front();
    const auto& outs = *parent.outputs_ptr();
    const auto parent_fk = query.to_tx(parent.hash(false));
    BOOST_REQUIRE(!parent_fk.is_terminal());

    system::data_chunk prevouts{};
    table::prevout::spends spends{};
    tx_links conflicts{};
    BOOST_REQUIRE(query.get_block_prevouts(prevouts, spends, conflicts, 2));
    BOOST_REQUIRE_EQUAL(prevouts, system::splice(outs.at(0)->to_data(), outs.at(1)->to_data()));
    BOOST_REQUIRE_EQUAL(spends.size(), 2u);
    BOOST_REQUIRE(conflicts.empty());
    BOOST_REQUIRE_EQUAL(table::prevout::slab_get::output_tx_fk(spends.at(0).first), parent_fk.value);
    BOOST_REQUIRE_EQUAL(table::prevout::slab_get::output_tx_fk(spends.at(1).first), parent_fk.value);
    BOOST_REQUIRE(!table::prevout::slab_get::coinbase(spends.at(0).first));
    BOOST_REQUIRE_EQUAL(spends.at(0).second, 0xa5u);
    BOOST_REQUIRE_EQUAL(spends.at(1).second, 0x85u);
    BOOST_REQUIRE(query.set_prevouts(2, spends, conflicts));

    table::prevout::slab_get cache{};
    cache.spends.resize(2);
    BOOST_REQUIRE(store.prevout.at(2, cache));
    BOOST_REQUIRE(cache.conflicts.empty());
    BOOST_REQUIRE_EQUAL(cache.spends.at(0).first, spends.at(0).first);
    BOOST_REQUIRE_EQUAL(cache.spends.at(0).second, spends.at(0).second);
    BOOST_REQUIRE_EQUAL(cache.spends.at(1).first, spends.at(1).first);
    BOOST_REQUIRE_EQUAL(cache.spends.at(1).second, spends.at(1).second);
}

BOOST_AUTO_TEST_CASE(query_consensus__get_block_prevouts__internal_spend__terminal)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));

    const auto& genesis_coinbase = *test::genesis.transactions_ptr()->front();
    const transaction parent{ 1, inputs{ input{ point{ genesis_coinbase.hash(false), 0 }, script{}, witness{}, 42 } }, outputs{ output{ 7, script{} } }, 0 };
    const transaction child{ 1, inputs{ input{ point{ parent.hash(false), 0 }, script{}, witness{}, 24 } }, outputs{ output{ 6, script{} } }, 0 };
    const block spending{ header{ 1, test::block0_hash, hash_digest{ 0x44 }, 0, 0, 0 }, transactions{ make_coinbase(), parent, child } };
    BOOST_REQUIRE(query.set(spending, context{ 0, 1, 0 }, {}, false, false));

    system::data_chunk prevouts{};
    table::prevout::spends spends{};
    tx_links conflicts{};
    BOOST_REQUIRE(query.get_block_prevouts(prevouts, spends, conflicts, 1));
    BOOST_REQUIRE_EQUAL(prevouts, system::splice(genesis_coinbase.outputs_ptr()->front()->to_data(), parent.outputs_ptr()->front()->to_data()));
    BOOST_REQUIRE_EQUAL(spends.size(), 2u);
    BOOST_REQUIRE_EQUAL(table::prevout::slab_get::output_tx_fk(spends.at(0).first), 0u);
    BOOST_REQUIRE(table::prevout::slab_get::coinbase(spends.at(0).first));
    BOOST_REQUIRE_EQUAL(spends.at(0).second, 42u);
    BOOST_REQUIRE_EQUAL(spends.at(1).first, table::prevout::tx::terminal);
    BOOST_REQUIRE_EQUAL(spends.at(1).second, 24u);
}

BOOST_AUTO_TEST_CASE(query_consensus__get_block_prevouts__missing_prevout__false)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));

    const transaction spend{ 1, inputs{ input{ point{ system::one_hash, 0 }, script{}, witness{}, max_uint32 } }, outputs{ output{ 1, script{} } }, 0 };
    const block missing{ header{ 1, test::block0_hash, hash_digest{ 0x42 }, 0, 0, 0 }, transactions{ make_coinbase(), spend } };
    BOOST_REQUIRE(query.set(missing, context{ 0, 1, 0 }, {}, false, false));

    system::data_chunk prevouts{};
    table::prevout::spends spends{};
    tx_links conflicts{};
    BOOST_REQUIRE(!query.get_block_prevouts(prevouts, spends, conflicts, 1));
}

// get_prevouts (via block_confirmable)
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(query_consensus__get_prevouts__cached__success)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1a, context{ 0, 1, 0 }, {}, false, false));
    BOOST_REQUIRE(query.set_strong(1));
    BOOST_REQUIRE(query.set(test::block_coinbase_spend_1a, context{ 0, 2, 0 }, {}, false, false));
    BOOST_REQUIRE(query.set_strong(2));
    BOOST_REQUIRE(query.set_prevouts(2, test::block_coinbase_spend_1a));
    BOOST_REQUIRE_EQUAL(query.block_confirmable(2), error::success);
}

BOOST_AUTO_TEST_CASE(query_consensus__get_prevouts__populated_metadata__success)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1a, context{ 0, 1, 0 }, {}, false, false));
    BOOST_REQUIRE(query.set_strong(1));
    BOOST_REQUIRE(query.set(test::block_coinbase_spend_1a, context{ 0, 2, 0 }, {}, false, false));
    BOOST_REQUIRE(query.set_strong(2));
    BOOST_REQUIRE(query.populate_with_metadata(test::block_coinbase_spend_1a));
    BOOST_REQUIRE(query.set_prevouts(2, test::block_coinbase_spend_1a));
    BOOST_REQUIRE_EQUAL(query.block_confirmable(2), error::success);
}

BOOST_AUTO_TEST_CASE(query_consensus__get_prevouts__not_cached__integrity_get_prevouts)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1a, context{ 0, 1, 0 }, {}, false, false));
    BOOST_REQUIRE(query.set_strong(1));
    BOOST_REQUIRE(query.set(test::block_coinbase_spend_1a, context{ 0, 2, 0 }, {}, false, false));
    BOOST_REQUIRE(query.set_strong(2));
    BOOST_REQUIRE_EQUAL(query.block_confirmable(2), error::integrity_get_prevouts);
}

BOOST_AUTO_TEST_SUITE_END()
