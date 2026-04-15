/**
 * Copyright (c) 2011-2026 libbitcoin developers (see AUTHORS)
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

BOOST_FIXTURE_TEST_SUITE(query_address_tests, test::directory_setup_fixture)

// get_unconfirmed_history
// get_confirmed_history
// get_history

BOOST_AUTO_TEST_CASE(query_address__get_history__genesis__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(test::setup_three_block_confirmed_address_store(query));

    histories out{};
    const std::atomic_bool cancel{};
    BOOST_REQUIRE(!query.get_unconfirmed_history(cancel, out, test::genesis_address0));
    BOOST_REQUIRE_EQUAL(out.size(), 0u);

    out.clear();
    BOOST_REQUIRE(!query.get_confirmed_history(cancel, out, test::genesis_address0));
    BOOST_REQUIRE_EQUAL(out.at(0).fee, 0u);
    BOOST_REQUIRE_EQUAL(out.at(0).position, 0u);
    BOOST_REQUIRE_EQUAL(out.at(0).tx.height(), 0u);
    BOOST_REQUIRE_EQUAL(out.at(0).tx.hash(), test::genesis.transactions_ptr()->at(0)->hash(false));

    out.clear();
    BOOST_REQUIRE(!query.get_history(cancel, out, test::genesis_address0));
    BOOST_REQUIRE_EQUAL(out.at(0).fee, 0u);
    BOOST_REQUIRE_EQUAL(out.at(0).position, 0u);
    BOOST_REQUIRE_EQUAL(out.at(0).tx.height(), 0u);
    BOOST_REQUIRE_EQUAL(out.at(0).tx.hash(), test::genesis.transactions_ptr()->at(0)->hash(false));
}

// block1a_address0 has 9 instances `script{ { { opcode::pick } } }`.
// 7 outputs are in individual txs, 2 are in tx bk1b0.
// 4 owning txs are confirmed (bk1a/bk2a/bk3a).
// 4 owning txs are unconfirmed (bk1b/bk2b/tx4/tx5).
// 6 outputs are spent, (bk2a/bk3a/tx4/tx5/bk2b00/bk2b01).
// 2 spend txs are confirmed, (bk2a/bk3a).
// 3 spend txs are unconfirmed, (tx4/tx5/bk2b0(2)).
//
// One owner owns two (one owner removed by deduplication).
// All spenders are also owners (all spends removed by deduplication).
// 4 owns + 2 spend txs are confirmed.
// 4 owns + 2 spend txs are confirmed.
//
// 4 unconfirmed:
// tx4     spends 1u + owns 1u (+1u) [unconfirmed rooted].
// tx5     spends 1u + owns 1u (+1u) [unconfirmed rooted].
// bk1btx0             owns 2u (+1u) [unconfirmed unrooted].
// bk2btx0 spends 2u + owns 1u (+1u) [unconfirmed unrooted].
//
// 4 confirmed:
// bk1atx0             owns 1c (+1c) [confirmed].
// bk2atx0 spends 1c + owns 1c (+1c) [confirmed].
// bk2atx1             owns 1c (+1c) [confirmed].
// bk3atx0 spends 1c + owns 1c (+1c) [confirmed].

BOOST_AUTO_TEST_CASE(query_address__get_unconfirmed_history__turbo_block1a_address0__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(test::setup_three_block_confirmed_address_store(query));

    using namespace system;
    histories out{};
    const std::atomic_bool cancel{};
    BOOST_REQUIRE(!query.get_unconfirmed_history(cancel, out, test::block1a_address0, true));
    BOOST_REQUIRE_EQUAL(out.size(), 4u);

    // Identities (not part of sort).
    BOOST_REQUIRE_EQUAL(out.at(0).tx.hash(), test::tx4.hash(false));                                // tx4
    BOOST_REQUIRE_EQUAL(out.at(1).tx.hash(), test::block2b.transactions_ptr()->at(0)->hash(false)); // tx8
    BOOST_REQUIRE_EQUAL(out.at(2).tx.hash(), test::tx5.hash(false));                                // tx5
    BOOST_REQUIRE_EQUAL(out.at(3).tx.hash(), test::block1b.transactions_ptr()->at(0)->hash(false)); // tx7

    // Confirmed by height ascending (not part of sort).

    // Unconfirmed rooted before unrooted.
    BOOST_REQUIRE_EQUAL(out.at(0).tx.height(), history::rooted_height);     // spends block1a (tx0 both outputs).
    BOOST_REQUIRE_EQUAL(out.at(1).tx.height(), history::unrooted_height);   // bk2btx0 unrooted (spends bk1).
    BOOST_REQUIRE_EQUAL(out.at(2).tx.height(), history::unrooted_height);   // spend exceeds value (treated as missing prevout).
    BOOST_REQUIRE_EQUAL(out.at(3).tx.height(), history::unrooted_height);   // bk1btx0 unrooted (missing prevouts).

    // Confirmed height by block position (not part of sort).
    BOOST_REQUIRE_EQUAL(out.at(0).position, history::unconfirmed_position);
    BOOST_REQUIRE_EQUAL(out.at(1).position, history::unconfirmed_position);
    BOOST_REQUIRE_EQUAL(out.at(2).position, history::unconfirmed_position);
    BOOST_REQUIRE_EQUAL(out.at(3).position, history::unconfirmed_position);

    // Unconfirmed system::encode_hash(hash) lexically sorted.
    BOOST_REQUIRE(encode_hash(out.at(1).tx.hash()) < encode_hash(out.at(2).tx.hash()));
    BOOST_REQUIRE(encode_hash(out.at(2).tx.hash()) < encode_hash(out.at(3).tx.hash()));

    // Fee (not part of sort).
    BOOST_REQUIRE_EQUAL(out.at(0).fee, floored_subtract(0x18u + 0x2au, 0x08u));
    BOOST_REQUIRE_EQUAL(out.at(1).fee, floored_subtract(0xb1u + 0xb1u, 0xb2u));
    BOOST_REQUIRE_EQUAL(out.at(2).fee, history::missing_prevout);           // spend exceeds value (treated as missing prevout).
    BOOST_REQUIRE_EQUAL(out.at(3).fee, 0u);                                 // coinbase (archived with null single point).
}

BOOST_AUTO_TEST_CASE(query_address__get_confirmed_history__turbo_block1a_address0__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(test::setup_three_block_confirmed_address_store(query));

    histories out{};
    const std::atomic_bool cancel{};
    BOOST_REQUIRE(!query.get_confirmed_history(cancel, out, test::block1a_address0, true));
    BOOST_REQUIRE_EQUAL(out.size(), 4u);

    // Identities (not part of sort).
    BOOST_REQUIRE_EQUAL(out.at(0).tx.hash(), test::block1a.transactions_ptr()->at(0)->hash(false)); // tx1
    BOOST_REQUIRE_EQUAL(out.at(1).tx.hash(), test::block2a.transactions_ptr()->at(0)->hash(false)); // tx2
    BOOST_REQUIRE_EQUAL(out.at(2).tx.hash(), test::block2a.transactions_ptr()->at(1)->hash(false)); // tx3
    BOOST_REQUIRE_EQUAL(out.at(3).tx.hash(), test::block3a.transactions_ptr()->at(0)->hash(false)); // tx6

    // Confirmed by height ascending.
    BOOST_REQUIRE_EQUAL(out.at(0).tx.height(), 1u);
    BOOST_REQUIRE_EQUAL(out.at(1).tx.height(), 2u);
    BOOST_REQUIRE_EQUAL(out.at(2).tx.height(), 2u);
    BOOST_REQUIRE_EQUAL(out.at(3).tx.height(), 3u);

    // Unconfirmed rooted before unrooted (not part of sort).

    // Confirmed height by block position.
    BOOST_REQUIRE_EQUAL(out.at(0).position, 0u);
    BOOST_REQUIRE_EQUAL(out.at(1).position, 0u);
    BOOST_REQUIRE_EQUAL(out.at(2).position, 1u);
    BOOST_REQUIRE_EQUAL(out.at(3).position, 0u);

    // Unconfirmed system::encode_hash(hash) lexically sorted (not part of sort).

    // Fee (not part of sort).
    BOOST_REQUIRE_EQUAL(out.at(0).fee, history::missing_prevout); // spend exceeds value (treated as missing prevout).
    BOOST_REQUIRE_EQUAL(out.at(1).fee, history::missing_prevout); // spend exceeds value (treated as missing prevout).
    BOOST_REQUIRE_EQUAL(out.at(2).fee, history::missing_prevout); // missing prevout.
    BOOST_REQUIRE_EQUAL(out.at(3).fee, history::missing_prevout); // spend exceeds value (treated as missing prevout).
}

BOOST_AUTO_TEST_CASE(query_address__get_history__turbo_block1a_address0__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(test::setup_three_block_confirmed_address_store(query));

    using namespace system;
    histories out{};
    const std::atomic_bool cancel{};
    BOOST_REQUIRE(!query.get_history(cancel, out, test::block1a_address0, true));
    BOOST_REQUIRE_EQUAL(out.size(), 8u);

    // Identities (not part of sort).
    BOOST_REQUIRE_EQUAL(out.at(0).tx.hash(), test::block1a.transactions_ptr()->at(0)->hash(false)); // tx1/tx1 (same)
    BOOST_REQUIRE_EQUAL(out.at(1).tx.hash(), test::block2a.transactions_ptr()->at(0)->hash(false)); // tx2/tx2 (same)
    BOOST_REQUIRE_EQUAL(out.at(2).tx.hash(), test::block2a.transactions_ptr()->at(1)->hash(false)); // tx3/tx3 (same)
    BOOST_REQUIRE_EQUAL(out.at(3).tx.hash(), test::block3a.transactions_ptr()->at(0)->hash(false)); // tx6/tx6 (same)
    BOOST_REQUIRE_EQUAL(out.at(4).tx.hash(), test::tx5.hash(false));                                // tx5/tx4
    BOOST_REQUIRE_EQUAL(out.at(5).tx.hash(), test::tx4.hash(false));                                // tx4/tx8
    BOOST_REQUIRE_EQUAL(out.at(6).tx.hash(), test::block2b.transactions_ptr()->at(0)->hash(false)); // tx8/tx5
    BOOST_REQUIRE_EQUAL(out.at(7).tx.hash(), test::block1b.transactions_ptr()->at(0)->hash(false)); // tx7/tx7 (same)

    // Confirmed by height ascending.
    // Unconfirmed rooted before unrooted.
    // NOTE: at(5) is spend exceeds value, which is returned as unrooted_height in get_unconfirmed_history()
    // NOTE: but as rooted_height in get_history(). This is a consequence of the optimized processing order
    // NOTE: and does not affect valid confirmed/unconfirmed txs.
    BOOST_REQUIRE_EQUAL(out.at(0).tx.height(), 1u);                             // tx1
    BOOST_REQUIRE_EQUAL(out.at(1).tx.height(), 2u);                             // tx2
    BOOST_REQUIRE_EQUAL(out.at(2).tx.height(), 2u);                             // tx3
    BOOST_REQUIRE_EQUAL(out.at(3).tx.height(), 3u);                             // tx6
    BOOST_REQUIRE_EQUAL(out.at(4).tx.height(), history::rooted_height);         // tx5/tx4
    BOOST_REQUIRE_EQUAL(out.at(5).tx.height(), history::rooted_height);         // tx4/tx8
    BOOST_REQUIRE_EQUAL(out.at(6).tx.height(), history::unrooted_height);       // tx8/tx5
    BOOST_REQUIRE_EQUAL(out.at(7).tx.height(), history::unrooted_height);       // tx7

    // Confirmed height by block position.
    BOOST_REQUIRE_EQUAL(out.at(0).position, 0u);                                // tx1
    BOOST_REQUIRE_EQUAL(out.at(1).position, 0u);                                // tx2
    BOOST_REQUIRE_EQUAL(out.at(2).position, 1u);                                // tx3
    BOOST_REQUIRE_EQUAL(out.at(3).position, 0u);                                // tx6
    BOOST_REQUIRE_EQUAL(out.at(4).position, history::unconfirmed_position);     // tx5/tx4
    BOOST_REQUIRE_EQUAL(out.at(5).position, history::unconfirmed_position);     // tx4/tx8
    BOOST_REQUIRE_EQUAL(out.at(6).position, history::unconfirmed_position);     // tx8/tx5
    BOOST_REQUIRE_EQUAL(out.at(7).position, history::unconfirmed_position);     // tx7

    // Unconfirmed system::encode_hash(hash) lexically sorted.
    BOOST_REQUIRE(encode_hash(out.at(6).tx.hash()) < encode_hash(out.at(7).tx.hash()));

    // Fee (not part of sort).
    BOOST_REQUIRE_EQUAL(out.at(0).fee, history::missing_prevout);               // tx1
    BOOST_REQUIRE_EQUAL(out.at(1).fee, history::missing_prevout);               // tx2
    BOOST_REQUIRE_EQUAL(out.at(2).fee, history::missing_prevout);               // tx3
    BOOST_REQUIRE_EQUAL(out.at(3).fee, history::missing_prevout);               // tx6
    BOOST_REQUIRE_EQUAL(out.at(4).fee, history::missing_prevout);               // tx5/tx4
    BOOST_REQUIRE_EQUAL(out.at(5).fee, floored_subtract(0x18u + 0x2au, 0x08u)); // tx4/tx8
    BOOST_REQUIRE_EQUAL(out.at(6).fee, floored_subtract(0xb1u + 0xb1u, 0xb2u)); // tx8/tx5
    BOOST_REQUIRE_EQUAL(out.at(7).fee, 0u);                                     // tx7
}

// get_tx_history1
// get_tx_history2

BOOST_AUTO_TEST_CASE(query_address__get_tx_history__bogus__invalid)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(test::setup_three_block_confirmed_address_store(query));

    const auto history = query.get_tx_history(hash_digest{ 0x42 });
    BOOST_REQUIRE(!history.valid());
}

BOOST_AUTO_TEST_CASE(query_address__get_tx_history__genesis__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(test::setup_three_block_confirmed_address_store(query));

    const auto hash = test::genesis.transactions_ptr()->at(0)->hash(false);
    auto history = query.get_tx_history(0);
    BOOST_REQUIRE(history.valid());
    BOOST_REQUIRE_EQUAL(history.fee, 0u);
    BOOST_REQUIRE_EQUAL(history.position, 0u);
    BOOST_REQUIRE_EQUAL(history.tx.height(), 0u);
    BOOST_REQUIRE_EQUAL(history.tx.hash(), hash);

    history = query.get_tx_history(hash);
    BOOST_REQUIRE(history.valid());
    BOOST_REQUIRE_EQUAL(history.fee, 0u);
    BOOST_REQUIRE_EQUAL(history.position, 0u);
    BOOST_REQUIRE_EQUAL(history.tx.height(), 0u);
    BOOST_REQUIRE_EQUAL(history.tx.hash(), hash);
}

BOOST_AUTO_TEST_CASE(query_address__get_tx_history__confirmed__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(test::setup_three_block_confirmed_address_store(query));

    const auto hash = test::block2a.transactions_ptr()->at(0)->hash(false);
    const auto history = query.get_tx_history(hash);
    BOOST_REQUIRE(history.valid());
    BOOST_REQUIRE_EQUAL(history.fee, history::missing_prevout); // spend > value
    BOOST_REQUIRE_EQUAL(history.position, 0u);
    BOOST_REQUIRE_EQUAL(history.tx.height(), 2u);
    BOOST_REQUIRE_EQUAL(history.tx.hash(), hash);
}

BOOST_AUTO_TEST_CASE(query_address__get_tx_history__confirmed_missing_prevout__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(test::setup_three_block_confirmed_address_store(query));

    const auto hash = test::block2a.transactions_ptr()->at(1)->hash(false);
    const auto history = query.get_tx_history(hash);
    BOOST_REQUIRE(history.valid());
    BOOST_REQUIRE_EQUAL(history.fee, history::missing_prevout); // missing prevout
    BOOST_REQUIRE_EQUAL(history.position, 1u);
    BOOST_REQUIRE_EQUAL(history.tx.height(), 2u);
    BOOST_REQUIRE_EQUAL(history.tx.hash(), hash);
}

BOOST_AUTO_TEST_CASE(query_address__get_tx_history__rooted__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(test::setup_three_block_confirmed_address_store(query));

    using namespace system;
    const auto hash = test::tx4.hash(false);
    const auto history = query.get_tx_history(hash);
    BOOST_REQUIRE(history.valid());
    BOOST_REQUIRE_EQUAL(history.fee, floored_subtract(0x18u + 0x2au, 0x08u));
    BOOST_REQUIRE_EQUAL(history.position, history::unconfirmed_position);
    BOOST_REQUIRE_EQUAL(history.tx.height(), history::rooted_height);
    BOOST_REQUIRE_EQUAL(history.tx.hash(), hash);
}

BOOST_AUTO_TEST_CASE(query_address__get_tx_history__unrooted__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(test::setup_three_block_confirmed_address_store(query));

    using namespace system;
    const auto hash = test::block2b.transactions_ptr()->at(0)->hash(false);
    const auto history = query.get_tx_history(hash);
    BOOST_REQUIRE(history.valid());
    BOOST_REQUIRE_EQUAL(history.fee, floored_subtract(0xb1u + 0xb1u, 0xb2u));
    BOOST_REQUIRE_EQUAL(history.position, history::unconfirmed_position);
    BOOST_REQUIRE_EQUAL(history.tx.height(), history::unrooted_height);
    BOOST_REQUIRE_EQUAL(history.tx.hash(), hash);
}

// get_spenders_history

BOOST_AUTO_TEST_CASE(query_address__get_spenders_history__bogus__empty)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(test::setup_three_block_confirmed_address_store(query));

    const auto histories = query.get_spenders_history({ 0x42 }, 0);
    BOOST_REQUIRE(histories.empty());
}

BOOST_AUTO_TEST_CASE(query_address__get_spenders_history__genesis__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(test::setup_three_block_confirmed_address_store(query));

    const auto hash = test::genesis.transactions_ptr()->at(0)->hash(false);
    const auto histories = query.get_spenders_history(hash, 0);
    BOOST_REQUIRE_EQUAL(histories.size(), 0u);
}

BOOST_AUTO_TEST_CASE(query_address__get_spenders_history__confirmed__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(test::setup_three_block_confirmed_address_store(query));

    using namespace system;
    const auto hash = test::block1a.transactions_ptr()->at(0)->hash(false);
    const auto histories = query.get_spenders_history(hash, 0);
    BOOST_REQUIRE_EQUAL(histories.size(), 4u);

    BOOST_REQUIRE(histories.at(0).valid());
    BOOST_REQUIRE_EQUAL(histories.at(0).fee, history::missing_prevout); // spend > value
    BOOST_REQUIRE_EQUAL(histories.at(0).position, 0u);
    BOOST_REQUIRE_EQUAL(histories.at(0).tx.height(), 2u);
    BOOST_REQUIRE_EQUAL(histories.at(0).tx.hash(), test::block2a.transactions_ptr()->at(0)->hash(false));

    BOOST_REQUIRE(histories.at(1).valid());
    BOOST_REQUIRE_EQUAL(histories.at(1).fee, history::missing_prevout); // spend > value
    BOOST_REQUIRE_EQUAL(histories.at(1).position, 0u);
    BOOST_REQUIRE_EQUAL(histories.at(1).tx.height(), 3u);
    BOOST_REQUIRE_EQUAL(histories.at(1).tx.hash(), test::block3a.transactions_ptr()->at(0)->hash(false));

    BOOST_REQUIRE(histories.at(2).valid());
    BOOST_REQUIRE_EQUAL(histories.at(2).fee, history::missing_prevout); // spend > value
    BOOST_REQUIRE_EQUAL(histories.at(2).position, history::unconfirmed_position);
    BOOST_REQUIRE_EQUAL(histories.at(2).tx.height(), history::rooted_height);
    BOOST_REQUIRE_EQUAL(histories.at(2).tx.hash(), test::tx5.hash(false));

    BOOST_REQUIRE(histories.at(3).valid());
    BOOST_REQUIRE_EQUAL(histories.at(3).fee, floored_subtract(0x18u + 0x2au, 0x08u));
    BOOST_REQUIRE_EQUAL(histories.at(3).position, history::unconfirmed_position);
    BOOST_REQUIRE_EQUAL(histories.at(3).tx.height(), history::rooted_height);
    BOOST_REQUIRE_EQUAL(histories.at(3).tx.hash(), test::tx4.hash(false));
}

BOOST_AUTO_TEST_SUITE_END()
