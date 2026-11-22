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
#include "../../mocks/pooling.hpp"

BC_FIXTURE_TEST_SUITE(query_pool_tests, test::directory_setup_fixture)

using namespace system::chain;
using context = database::context;
using test::bip113;
using test::block_limit;
using test::tight_limit;
using test::to_spend;
using test::genesis_coinbase;
using test::archive_confirm;
using test::pool_populated;
using test::pool_candidate;

// get_pool_coverage
// ----------------------------------------------------------------------------


BC_AUTO_TEST_CASE(query_pool__get_pool_coverage__no_conflict__rate_bound)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    store.set_pooling();

    // Genesis coinbase is 5,000,000,000 satoshis, so the fee is 1,000.
    const auto tx = to_spend(genesis_coinbase(), 4'999'999'000);
    const auto rate = 1'000'000_u64 / tx.virtual_size();
    transaction_cptrs txs{};
    const auto populated = pool_candidate(txs, query, tx);
    bool at_rate{}, above_rate{};
    const context ctx{ bip113, 8, 9 };
    const auto at_rate_ec = query.get_pool_coverage(at_rate, txs, { false }, ctx, rate, 20, block_limit, 1'024);
    const auto above_rate_ec = query.get_pool_coverage(above_rate, txs, { false }, ctx, add1(rate), 20, block_limit, 1'024);

    BC_REQUIRE(populated);
    BC_REQUIRE_EQUAL(at_rate_ec, error::success);
    BC_REQUIRE(at_rate);
    BC_REQUIRE_EQUAL(above_rate_ec, error::success);
    BC_REQUIRE(!above_rate);
}

BC_AUTO_TEST_CASE(query_pool__get_pool_coverage__stored__not_counted)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    store.set_pooling();
    transaction_cptrs txs{};
    const auto populated = pool_candidate(txs, query, to_spend(genesis_coinbase(), 4'999'999'000));
    bool covered{};
    const context ctx{ bip113, 8, 9 };
    const auto ec = query.get_pool_coverage(covered, txs, { true }, ctx, max_uint64, 20, block_limit, 1'024);

    BC_REQUIRE(populated);
    BC_REQUIRE_EQUAL(ec, error::success);
    BC_REQUIRE(covered);
}

// Pooled fee of 1,000 covers both rivals, as chargeable size includes conflict.
BC_AUTO_TEST_CASE(query_pool__get_pool_coverage__lower_fee_conflict__covered_by_surplus)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    store.set_pooling();
    const auto tx = to_spend(genesis_coinbase(), 4'999'999'000);
    const auto pooled = pool_populated(query, tx);
    transaction_cptrs txs{};
    const auto rival = to_spend(genesis_coinbase(), 4'999'999'500);
    const auto populated = pool_candidate(txs, query, rival);
    const auto rate = 1'000'000_u64 / (tx.virtual_size() + rival.virtual_size());
    bool at_rate{}, above_rate{};
    const context ctx{ bip113, 8, 9 };
    const auto at_rate_ec = query.get_pool_coverage(at_rate, txs, { false }, ctx, rate, 20, block_limit, 1'024);
    const auto above_rate_ec = query.get_pool_coverage(above_rate, txs, { false }, ctx, add1(rate), 20, block_limit, 1'024);

    BC_REQUIRE(pooled);
    BC_REQUIRE(populated);
    BC_REQUIRE_EQUAL(at_rate_ec, error::success);
    BC_REQUIRE(at_rate);
    BC_REQUIRE_EQUAL(above_rate_ec, error::success);
    BC_REQUIRE(!above_rate);
}

// The higher fee rival is the best outcome, which covers both rivals.
BC_AUTO_TEST_CASE(query_pool__get_pool_coverage__higher_fee_conflict__best_covers_both)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    store.set_pooling();
    const auto tx = to_spend(genesis_coinbase(), 4'999'999'000);
    const auto pooled = pool_populated(query, tx);
    transaction_cptrs txs{};
    const auto rival = to_spend(genesis_coinbase(), 4'999'998'000);
    const auto populated = pool_candidate(txs, query, rival);
    const auto rate = 2'000'000_u64 / (tx.virtual_size() + rival.virtual_size());
    bool at_rate{}, above_rate{};
    const context ctx{ bip113, 8, 9 };
    const auto at_rate_ec = query.get_pool_coverage(at_rate, txs, { false }, ctx, rate, 20, block_limit, 1'024);
    const auto above_rate_ec = query.get_pool_coverage(above_rate, txs, { false }, ctx, add1(rate), 20, block_limit, 1'024);

    BC_REQUIRE(pooled);
    BC_REQUIRE(populated);
    BC_REQUIRE_EQUAL(at_rate_ec, error::success);
    BC_REQUIRE(at_rate);
    BC_REQUIRE_EQUAL(above_rate_ec, error::success);
    BC_REQUIRE(!above_rate);
}

// A pooled parent's surplus covers a child of no fee (parent pays for child).
BC_AUTO_TEST_CASE(query_pool__get_pool_coverage__unpaid_child__covered_by_parent)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    store.set_pooling();
    const auto parent = to_spend(genesis_coinbase(), 4'999'999'000);
    const auto child = to_spend(parent.hash(false), 4'999'999'000);
    const auto pooled = pool_populated(query, parent);
    transaction_cptrs txs{};
    const auto populated = pool_candidate(txs, query, child);
    const auto rate = 1'000'000_u64 / (parent.virtual_size() + child.virtual_size());
    bool at_rate{}, above_rate{};
    const context ctx{ bip113, 8, 9 };
    const auto at_rate_ec = query.get_pool_coverage(at_rate, txs, { false }, ctx, rate, 20, block_limit, 1'024);
    const auto above_rate_ec = query.get_pool_coverage(above_rate, txs, { false }, ctx, add1(rate), 20, block_limit, 1'024);

    BC_REQUIRE(pooled);
    BC_REQUIRE(populated);
    BC_REQUIRE_EQUAL(at_rate_ec, error::success);
    BC_REQUIRE(at_rate);
    BC_REQUIRE_EQUAL(above_rate_ec, error::success);
    BC_REQUIRE(!above_rate);
}

BC_AUTO_TEST_CASE(query_pool__get_pool_coverage__conflicted_child__best_outcome_counted)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    store.set_pooling();

    // Fees: parent 1,000, child 2,000, candidate (conflicts the child) 2,500.
    // Best outcome (parent and candidate, 3,500) covers all three.
    const auto parent = to_spend(genesis_coinbase(), 4'999'999'000);
    const auto child = to_spend(parent.hash(false), 4'999'997'000);
    const auto tx = to_spend(parent.hash(false), 4'999'996'500);
    const auto bytes = parent.virtual_size() + child.virtual_size() + tx.virtual_size();
    const auto rate = 3'500'000_u64 / bytes;
    const auto pooled = pool_populated(query, parent) && pool_populated(query, child);
    transaction_cptrs txs{};
    const auto populated = pool_candidate(txs, query, tx);
    bool at_rate{}, above_rate{};
    const context ctx{ bip113, 8, 9 };
    const auto at_rate_ec = query.get_pool_coverage(at_rate, txs, { false }, ctx, rate, 20, block_limit, 1'024);
    const auto above_rate_ec = query.get_pool_coverage(above_rate, txs, { false }, ctx, add1(rate), 20, block_limit, 1'024);

    BC_REQUIRE(pooled);
    BC_REQUIRE(populated);
    BC_REQUIRE_EQUAL(at_rate_ec, error::success);
    BC_REQUIRE(at_rate);
    BC_REQUIRE_EQUAL(above_rate_ec, error::success);
    BC_REQUIRE(!above_rate);
}

BC_AUTO_TEST_CASE(query_pool__get_pool_coverage__confirmed_spender__not_covered)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    store.set_pooling();
    const auto spender = to_spend(genesis_coinbase(), 4'999'999'000);
    const auto confirmed = !archive_confirm(query, spender).is_terminal();
    transaction_cptrs txs{};
    const auto populated = pool_candidate(txs, query, to_spend(genesis_coinbase(), 4'999'999'500));
    bool covered{};
    const context ctx{ bip113, 8, 9 };
    const auto ec = query.get_pool_coverage(covered, txs, { false }, ctx, 0, 20, block_limit, 1'024);

    BC_REQUIRE(confirmed);
    BC_REQUIRE(populated);
    BC_REQUIRE_EQUAL(ec, error::success);
    BC_REQUIRE(!covered);
}

BC_AUTO_TEST_CASE(query_pool__get_pool_coverage__outcomes_over_limit__pool_limit)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    store.set_pooling();
    const auto pooled = pool_populated(query, to_spend(genesis_coinbase(), 4'999'999'000));
    transaction_cptrs txs{};
    const auto populated = pool_candidate(txs, query, to_spend(genesis_coinbase(), 4'999'999'500));
    bool covered{};
    const context ctx{ bip113, 8, 9 };
    const auto ec = query.get_pool_coverage(covered, txs, { false }, ctx, 0, 20, block_limit, 1);

    BC_REQUIRE(pooled);
    BC_REQUIRE(populated);
    BC_REQUIRE_EQUAL(ec, error::pool_limit);
    BC_REQUIRE(!covered);
}

BC_AUTO_TEST_CASE(query_pool__get_pool_coverage__contested_over_limit__pool_limit)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    store.set_pooling();
    const auto pooled = pool_populated(query, to_spend(genesis_coinbase(), 4'999'999'000));
    transaction_cptrs txs{};
    const auto populated = pool_candidate(txs, query, to_spend(genesis_coinbase(), 4'999'998'500));
    bool covered{};
    const context ctx{ bip113, 8, 9 };
    const auto ec = query.get_pool_coverage(covered, txs, { false }, ctx, 0, 20, tight_limit, 1'024);

    BC_REQUIRE(pooled);
    BC_REQUIRE(populated);
    BC_REQUIRE_EQUAL(ec, error::pool_limit);
    BC_REQUIRE(!covered);
}

BC_AUTO_TEST_CASE(query_pool__get_pool_coverage__uncontested_child__not_limited)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    store.set_pooling();

    // The child fee is 1,000 and its parent is pooled, with no conflict.
    const auto parent = to_spend(genesis_coinbase(), 4'999'999'000);
    const auto tx = to_spend(parent.hash(false), 4'999'998'000);
    const auto rate = 2'000'000_u64 / (parent.virtual_size() + tx.virtual_size());
    const auto pooled = pool_populated(query, parent);
    transaction_cptrs txs{};
    const auto populated = pool_candidate(txs, query, tx);
    bool at_rate{}, above_rate{};
    const context ctx{ bip113, 8, 9 };
    const auto at_rate_ec = query.get_pool_coverage(at_rate, txs, { false }, ctx, rate, 20, tight_limit, 1'024);
    const auto above_rate_ec = query.get_pool_coverage(above_rate, txs, { false }, ctx, add1(rate), 20, tight_limit, 1'024);

    BC_REQUIRE(pooled);
    BC_REQUIRE(populated);
    BC_REQUIRE_EQUAL(at_rate_ec, error::success);
    BC_REQUIRE(at_rate);
    BC_REQUIRE_EQUAL(above_rate_ec, error::success);
    BC_REQUIRE(!above_rate);
}

BC_AUTO_TEST_SUITE_END()
