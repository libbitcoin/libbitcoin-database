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

BC_FIXTURE_TEST_SUITE(query_navigate_tests, test::directory_setup_fixture)

// to_point
// to_output
// to_previous_output

// to_points1
// to_points2
// to_outputs1
// to_outputs2
// to_prevouts1
// to_prevouts2

// to_block_points
// to_block_outputs
// to_block_prevouts

// TODO: break this up.
BC_AUTO_TEST_CASE(query_navigate__to_output__various__expected)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));
    BC_REQUIRE(query.set(test::block1, test::context, {}, false, false));
    BC_REQUIRE(query.set(test::block2, test::context, {}, false, false));
    BC_REQUIRE(query.set(test::block3, test::context, {}, false, false));
    BC_REQUIRE(query.set(test::block1a, test::context, {}, false, false));

    // All 5 blocks have one transaction with 1 output.
    BC_REQUIRE_EQUAL(query.to_output_tx(0 * 0x51), 0u);
    BC_REQUIRE_EQUAL(query.to_output_tx(1 * 0x51), 1u);
    BC_REQUIRE_EQUAL(query.to_output_tx(2 * 0x51), 2u);
    BC_REQUIRE_EQUAL(query.to_output_tx(3 * 0x51), 3u);
    BC_REQUIRE_EQUAL(query.to_output_tx(4 * 0x51), 4u);
    BC_REQUIRE_EQUAL(query.to_output_tx(4 * 0x51 + 7u), 4u);

    BC_REQUIRE_EQUAL(query.to_output(0, 0), 0u * 0x51u);
    BC_REQUIRE_EQUAL(query.to_output(1, 0), 1u * 0x51u);
    BC_REQUIRE_EQUAL(query.to_output(2, 0), 2u * 0x51u);
    BC_REQUIRE_EQUAL(query.to_output(3, 0), 3u * 0x51u);
    BC_REQUIRE_EQUAL(query.to_output(4, 0), 4u * 0x51u);
    BC_REQUIRE_EQUAL(query.to_output(4, 1), 4u * 0x51u + 7u);

    const output_links expected_outputs4{ 4 * 0x51, 4 * 0x51 + 7 };
    BC_REQUIRE_EQUAL(query.to_outputs(0), output_links{ 0 * 0x51 });
    BC_REQUIRE_EQUAL(query.to_outputs(1), output_links{ 1 * 0x51 });
    BC_REQUIRE_EQUAL(query.to_outputs(2), output_links{ 2 * 0x51 });
    BC_REQUIRE_EQUAL(query.to_outputs(3), output_links{ 3 * 0x51 });
    BC_REQUIRE_EQUAL(query.to_outputs(4), expected_outputs4);

    // All blocks have one transaction.
    BC_REQUIRE_EQUAL(query.to_block_outputs(0), output_links{ 0 * 0x51 });
    BC_REQUIRE_EQUAL(query.to_block_outputs(1), output_links{ 1 * 0x51 });
    BC_REQUIRE_EQUAL(query.to_block_outputs(2), output_links{ 2 * 0x51 });
    BC_REQUIRE_EQUAL(query.to_block_outputs(3), output_links{ 3 * 0x51 });
    BC_REQUIRE_EQUAL(query.to_block_outputs(4), expected_outputs4);

    // No prevouts that exist.
    const output_links expected_prevouts4{ output_link::terminal, output_link::terminal, output_link::terminal };
    BC_REQUIRE_EQUAL(query.to_prevouts(0), output_links{ output_link::terminal });
    BC_REQUIRE_EQUAL(query.to_prevouts(1), output_links{ output_link::terminal });
    BC_REQUIRE_EQUAL(query.to_prevouts(2), output_links{ output_link::terminal });
    BC_REQUIRE_EQUAL(query.to_prevouts(3), output_links{ output_link::terminal });
    BC_REQUIRE_EQUAL(query.to_prevouts(4), expected_prevouts4);

    // All blocks have one transaction.
    BC_REQUIRE_EQUAL(query.to_block_prevouts(0), output_links{});
    BC_REQUIRE_EQUAL(query.to_block_prevouts(1), output_links{});
    BC_REQUIRE_EQUAL(query.to_block_prevouts(2), output_links{});
    BC_REQUIRE_EQUAL(query.to_block_prevouts(3), output_links{});
    BC_REQUIRE_EQUAL(query.to_block_prevouts(4), output_links{});

    // Past end.
    BC_REQUIRE_EQUAL(query.to_output_tx(4 * 0x51 + 14), tx_link::terminal);
    BC_REQUIRE_EQUAL(query.to_output(5, 0), output_link::terminal);
    BC_REQUIRE(query.to_outputs(5).empty());
    BC_REQUIRE(query.to_block_outputs(5).empty());
}

// to_transactions
// to_spending_txs

// to_coinbase

BC_AUTO_TEST_CASE(query_navigate__to_coinbase__always__expected)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE_EQUAL(query.to_header(test::genesis.hash()), header_link::terminal);
    BC_REQUIRE(query.initialize(test::genesis));
    BC_REQUIRE_EQUAL(query.to_coinbase(0), 0u);
    BC_REQUIRE(query.to_coinbase(1).is_terminal());
    BC_REQUIRE(query.set(test::block1, test::context, {}, false, false));
    BC_REQUIRE_EQUAL(query.to_coinbase(1), 1u);
}

// to_transaction

BC_AUTO_TEST_SUITE_END()
