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

BOOST_FIXTURE_TEST_SUITE(query_unspent_tests, test::directory_setup_fixture)

BOOST_AUTO_TEST_CASE(query_unspent__get_unspent_totals__empty_branch__success_zeros)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));

    const stopper cancel{};
    unspent_totals totals{};
    BOOST_REQUIRE(!query.get_unspent_totals(cancel, totals, {}));
    BOOST_REQUIRE_EQUAL(totals.outputs, 0u);
    BOOST_REQUIRE_EQUAL(totals.transactions, 0u);
    BOOST_REQUIRE_EQUAL(totals.script_bytes, 0u);
    BOOST_REQUIRE_EQUAL(totals.value, 0u);
}

BOOST_AUTO_TEST_CASE(query_unspent__get_unspent_totals__two_block_branch__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, context{ 0, 1, 0 }, {}, false, false));
    BOOST_REQUIRE(query.set_strong(1));
    BOOST_REQUIRE(query.push_confirmed(1, false));
    BOOST_REQUIRE(query.set(test::block2, context{ 0, 2, 0 }, {}, false, false));
    BOOST_REQUIRE(query.set_strong(2));
    BOOST_REQUIRE(query.push_confirmed(2, false));

    // Each block is one coinbase with one unspent p2pk output (67 bytes).
    const stopper cancel{};
    unspent_totals totals{};
    const header_links branch{ header_link{ 2 }, header_link{ 1 } };
    BOOST_REQUIRE(!query.get_unspent_totals(cancel, totals, branch));
    BOOST_REQUIRE_EQUAL(totals.outputs, 2u);
    BOOST_REQUIRE_EQUAL(totals.transactions, 2u);
    BOOST_REQUIRE_EQUAL(totals.script_bytes, 134u);
    BOOST_REQUIRE_EQUAL(totals.value, 10'000'000'000u);
}

// The commitment element (bitcoind coin serialization) of a confirmed output.
static system::data_chunk output_element(const system::chain::transaction& tx, uint32_t index, size_t height)
{
    const auto& output = *tx.outputs_ptr()->at(index);
    const auto script = output.script().to_data(false);
    const auto txid = tx.hash(false);
    const auto coded = system::possible_narrow_cast<uint32_t>((height << 1) | 1u);
    const auto code = system::to_little_endian(coded);
    const auto value = system::to_little_endian(output.value());
    const auto size = system::to_array(system::possible_narrow_cast<uint8_t>(script.size()));
    return system::build_chunk({ txid, system::to_little_endian(index), code, value, size, script });
}

static system::data_chunk coin_element(const system::chain::block& block, size_t height)
{
    return output_element(*block.transactions_ptr()->front(), 0, height);
}

BOOST_AUTO_TEST_CASE(query_unspent__get_unspent_muhash__two_block_branch__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, context{ 0, 1, 0 }, {}, false, false));
    BOOST_REQUIRE(query.set_strong(1));
    BOOST_REQUIRE(query.push_confirmed(1, false));
    BOOST_REQUIRE(query.set(test::block2, context{ 0, 2, 0 }, {}, false, false));
    BOOST_REQUIRE(query.set_strong(2));
    BOOST_REQUIRE(query.push_confirmed(2, false));

    system::muhash3072 expected{};
    expected.insert(coin_element(test::block1, 1));
    expected.insert(coin_element(test::block2, 2));

    const stopper cancel{};
    hash_digest digest{};
    unspent_totals totals{};
    const header_links branch{ header_link{ 2 }, header_link{ 1 } };
    BOOST_REQUIRE(!query.get_unspent_muhash(cancel, totals, digest, branch));
    BOOST_REQUIRE_EQUAL(digest, expected.flush());
    BOOST_REQUIRE_EQUAL(totals.outputs, 2u);
    BOOST_REQUIRE_EQUAL(totals.transactions, 2u);
    BOOST_REQUIRE_EQUAL(totals.script_bytes, 134u);
    BOOST_REQUIRE_EQUAL(totals.coin_bytes, 2u * (48u + 1u + 67u));
    BOOST_REQUIRE_EQUAL(totals.value, 10'000'000'000u);
}

BOOST_AUTO_TEST_CASE(query_unspent__get_unspent_serialized__two_block_branch__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, context{ 0, 1, 0 }, {}, false, false));
    BOOST_REQUIRE(query.set_strong(1));
    BOOST_REQUIRE(query.push_confirmed(1, false));
    BOOST_REQUIRE(query.set(test::block2, context{ 0, 2, 0 }, {}, false, false));
    BOOST_REQUIRE(query.set_strong(2));
    BOOST_REQUIRE(query.push_confirmed(2, false));

    // Canonical order is by txid, the stream is double sha256 hashed.
    const auto first = coin_element(test::block1, 1);
    const auto second = coin_element(test::block2, 2);
    const auto& tx1 = *test::block1.transactions_ptr()->front();
    const auto& tx2 = *test::block2.transactions_ptr()->front();
    const auto ordered = tx1.hash(false) < tx2.hash(false);
    const auto forward = system::build_chunk({ first, second });
    const auto reverse = system::build_chunk({ second, first });
    const auto expected = system::bitcoin_hash(ordered ? forward : reverse);

    const stopper cancel{};
    hash_digest digest{};
    unspent_totals totals{};
    const header_links branch{ header_link{ 2 }, header_link{ 1 } };
    BOOST_REQUIRE(!query.get_unspent_serialized(cancel, totals, digest, branch));
    BOOST_REQUIRE_EQUAL(digest, expected);
    BOOST_REQUIRE_EQUAL(totals.outputs, 2u);
    BOOST_REQUIRE_EQUAL(totals.transactions, 2u);
    BOOST_REQUIRE_EQUAL(totals.script_bytes, 134u);
    BOOST_REQUIRE_EQUAL(totals.coin_bytes, 2u * (48u + 1u + 67u));
    BOOST_REQUIRE_EQUAL(totals.value, 10'000'000'000u);
}

BOOST_AUTO_TEST_CASE(query_unspent__get_unspent_serialized__two_output_coinbase__one_transaction)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1c, context{ 0, 1, 0 }, {}, false, false));
    BOOST_REQUIRE(query.set_strong(1));
    BOOST_REQUIRE(query.push_confirmed(1, false));

    // Both outputs of the one coinbase are unspent, ordered by index.
    const auto& tx = *test::block1c.transactions_ptr()->front();
    const auto first = output_element(tx, 0, 1);
    const auto second = output_element(tx, 1, 1);
    const auto expected = system::bitcoin_hash(system::build_chunk({ first, second }));
    system::muhash3072 muhash{};
    muhash.insert(first);
    muhash.insert(second);

    const stopper cancel{};
    hash_digest digest{};
    unspent_totals totals{};
    const header_links branch{ header_link{ 1 } };
    BOOST_REQUIRE(!query.get_unspent_serialized(cancel, totals, digest, branch));
    BOOST_REQUIRE_EQUAL(digest, expected);
    BOOST_REQUIRE_EQUAL(totals.outputs, 2u);
    BOOST_REQUIRE_EQUAL(totals.transactions, 1u);
    BOOST_REQUIRE_EQUAL(totals.value, 66u);

    totals = {};
    BOOST_REQUIRE(!query.get_unspent_muhash(cancel, totals, digest, branch));
    BOOST_REQUIRE_EQUAL(digest, muhash.flush());
    BOOST_REQUIRE_EQUAL(totals.outputs, 2u);
    BOOST_REQUIRE_EQUAL(totals.transactions, 1u);
}

BOOST_AUTO_TEST_CASE(query_unspent__get_unspent_matches__two_block_branch__one_match)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, context{ 0, 1, 0 }, {}, false, false));
    BOOST_REQUIRE(query.set_strong(1));
    BOOST_REQUIRE(query.push_confirmed(1, false));
    BOOST_REQUIRE(query.set(test::block2, context{ 0, 2, 0 }, {}, false, false));
    BOOST_REQUIRE(query.set_strong(2));
    BOOST_REQUIRE(query.push_confirmed(2, false));

    const auto& tx = *test::block1.transactions_ptr()->front();
    const auto script = tx.outputs_ptr()->front()->script().to_data(false);
    const std::unordered_set<hash_digest> keys{ system::sha256_hash(script) };

    const stopper cancel{};
    size_t txouts{};
    unspent_coins coins{};
    const header_links branch{ header_link{ 2 }, header_link{ 1 } };
    BOOST_REQUIRE(!query.get_unspent_matches(cancel, coins, txouts, keys, branch));
    BOOST_REQUIRE_EQUAL(txouts, 2u);
    BOOST_REQUIRE_EQUAL(coins.size(), 1u);
    BOOST_REQUIRE_EQUAL(coins.front().out.point().hash(), tx.hash(false));
    BOOST_REQUIRE_EQUAL(coins.front().out.point().index(), 0u);
    BOOST_REQUIRE_EQUAL(coins.front().height, 1u);
    BOOST_REQUIRE(coins.front().coinbase);
    BOOST_REQUIRE_EQUAL(coins.front().out.value(), 5'000'000'000u);
    BOOST_REQUIRE_EQUAL(coins.front().script, script);
}

BOOST_AUTO_TEST_CASE(query_unspent__get_unspent_matches__no_keys__no_matches)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, context{ 0, 1, 0 }, {}, false, false));
    BOOST_REQUIRE(query.set_strong(1));
    BOOST_REQUIRE(query.push_confirmed(1, false));

    const stopper cancel{};
    size_t txouts{};
    unspent_coins coins{};
    const std::unordered_set<hash_digest> keys{};
    const header_links branch{ header_link{ 1 } };
    BOOST_REQUIRE(!query.get_unspent_matches(cancel, coins, txouts, keys, branch));
    BOOST_REQUIRE_EQUAL(txouts, 1u);
    BOOST_REQUIRE(coins.empty());
}

BOOST_AUTO_TEST_CASE(query_unspent__get_unspent_totals__cancelled__query_canceled)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, context{ 0, 1, 0 }, {}, false, false));
    BOOST_REQUIRE(query.set_strong(1));
    BOOST_REQUIRE(query.push_confirmed(1, false));

    const stopper cancel{ true };
    unspent_totals totals{};
    const header_links branch{ header_link{ 1 } };
    const auto ec = query.get_unspent_totals(cancel, totals, branch);
    BOOST_REQUIRE_EQUAL(ec, error::query_canceled);
}

BOOST_AUTO_TEST_SUITE_END()
