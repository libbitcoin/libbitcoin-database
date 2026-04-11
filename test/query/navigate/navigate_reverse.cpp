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

BOOST_FIXTURE_TEST_SUITE(query_navigate_tests, test::directory_setup_fixture)

// to_parent

BOOST_AUTO_TEST_CASE(query_navigate__to_parent__always__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(test::events_handler), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, test::context, false, false));
    BOOST_REQUIRE(query.set(test::block2, test::context, false, false));
    BOOST_REQUIRE(query.set(test::block1a, test::context, false, false));
    BOOST_REQUIRE(query.set(test::block2a, test::context, false, false));
    BOOST_REQUIRE_EQUAL(query.to_parent(0), header_link::terminal);
    BOOST_REQUIRE_EQUAL(query.to_parent(1), 0u);
    BOOST_REQUIRE_EQUAL(query.to_parent(2), 1u);
    BOOST_REQUIRE_EQUAL(query.to_parent(3), 0u);
    BOOST_REQUIRE_EQUAL(query.to_parent(4), 3u);
    BOOST_REQUIRE_EQUAL(query.to_parent(5), header_link::terminal);
}

// to_address_outputs1
// to_address_outputs2

BOOST_AUTO_TEST_CASE(query_address__to_address_outputs__genesis__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(test::events_handler), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));

    output_links out{};
    std::atomic_bool cancel{};
    BOOST_REQUIRE(!query.to_address_outputs(cancel, out, test::genesis_address));
    BOOST_REQUIRE_EQUAL(out.size(), 1u);
    BOOST_REQUIRE_EQUAL(out.front(), query.to_output(0, 0));
}

// to_spending_tx
// to_output_tx

BOOST_AUTO_TEST_CASE(query_navigate__to_output_tx__to_output__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(test::events_handler), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, test::context, false, false));
    BOOST_REQUIRE(query.set(test::block2, test::context, false, false));
    BOOST_REQUIRE(query.set(test::block3, test::context, false, false));
    BOOST_REQUIRE(query.set(test::block1a, test::context, false, false));

    // All 5 blocks have one transaction with 1 output.
    BOOST_REQUIRE_EQUAL(query.to_output_tx(0 * 0x51), 0u);
    BOOST_REQUIRE_EQUAL(query.to_output_tx(1 * 0x51), 1u);
    BOOST_REQUIRE_EQUAL(query.to_output_tx(2 * 0x51), 2u);
    BOOST_REQUIRE_EQUAL(query.to_output_tx(3 * 0x51), 3u);
    BOOST_REQUIRE_EQUAL(query.to_output_tx(4 * 0x51), 4u);
    BOOST_REQUIRE_EQUAL(query.to_output_tx(4 * 0x51 + 7u), 4u);

    BOOST_REQUIRE_EQUAL(query.to_output(0, 0), 0u * 0x51u);
    BOOST_REQUIRE_EQUAL(query.to_output(1, 0), 1u * 0x51u);
    BOOST_REQUIRE_EQUAL(query.to_output(2, 0), 2u * 0x51u);
    BOOST_REQUIRE_EQUAL(query.to_output(3, 0), 3u * 0x51u);
    BOOST_REQUIRE_EQUAL(query.to_output(4, 0), 4u * 0x51u);
    BOOST_REQUIRE_EQUAL(query.to_output(4, 1), 4u * 0x51u + 7u);

    const output_links expected_outputs4{ 4 * 0x51, 4 * 0x51 + 7 };
    BOOST_REQUIRE_EQUAL(query.to_outputs(0), output_links{ 0 * 0x51 });
    BOOST_REQUIRE_EQUAL(query.to_outputs(1), output_links{ 1 * 0x51 });
    BOOST_REQUIRE_EQUAL(query.to_outputs(2), output_links{ 2 * 0x51 });
    BOOST_REQUIRE_EQUAL(query.to_outputs(3), output_links{ 3 * 0x51 });
    BOOST_REQUIRE_EQUAL(query.to_outputs(4), expected_outputs4);

    // All blocks have one transaction.
    BOOST_REQUIRE_EQUAL(query.to_block_outputs(0), output_links{ 0 * 0x51 });
    BOOST_REQUIRE_EQUAL(query.to_block_outputs(1), output_links{ 1 * 0x51 });
    BOOST_REQUIRE_EQUAL(query.to_block_outputs(2), output_links{ 2 * 0x51 });
    BOOST_REQUIRE_EQUAL(query.to_block_outputs(3), output_links{ 3 * 0x51 });
    BOOST_REQUIRE_EQUAL(query.to_block_outputs(4), expected_outputs4);

    // No prevouts that exist.
    const output_links expected_prevouts4{ output_link::terminal, output_link::terminal, output_link::terminal };
    BOOST_REQUIRE_EQUAL(query.to_prevouts(0), output_links{ output_link::terminal });
    BOOST_REQUIRE_EQUAL(query.to_prevouts(1), output_links{ output_link::terminal });
    BOOST_REQUIRE_EQUAL(query.to_prevouts(2), output_links{ output_link::terminal });
    BOOST_REQUIRE_EQUAL(query.to_prevouts(3), output_links{ output_link::terminal });
    BOOST_REQUIRE_EQUAL(query.to_prevouts(4), expected_prevouts4);

    // All blocks have one transaction.
    BOOST_REQUIRE_EQUAL(query.to_block_prevouts(0), output_links{});
    BOOST_REQUIRE_EQUAL(query.to_block_prevouts(1), output_links{});
    BOOST_REQUIRE_EQUAL(query.to_block_prevouts(2), output_links{});
    BOOST_REQUIRE_EQUAL(query.to_block_prevouts(3), output_links{});
    BOOST_REQUIRE_EQUAL(query.to_block_prevouts(4), output_links{});

    // Past end.
    BOOST_REQUIRE_EQUAL(query.to_output_tx(4 * 0x51 + 14), tx_link::terminal);
    BOOST_REQUIRE_EQUAL(query.to_output(5, 0), output_link::terminal);
    BOOST_REQUIRE(query.to_outputs(5).empty());
    BOOST_REQUIRE(query.to_block_outputs(5).empty());
}

// to_prevout_tx

BOOST_AUTO_TEST_CASE(query_navigate__to_prevout_tx__to_prevout__expected)
{
    settings settings{};
    settings.tx_buckets = 3;
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(test::events_handler), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1a, test::context, false, false));
    BOOST_REQUIRE(query.set(test::block2a, test::context, false, false));

    // inputs in link order.
    BOOST_REQUIRE_EQUAL(query.to_prevout_tx(0), tx_link::terminal);
    BOOST_REQUIRE_EQUAL(query.to_prevout_tx(1), tx_link::terminal);
    BOOST_REQUIRE_EQUAL(query.to_prevout_tx(2), tx_link::terminal);
    BOOST_REQUIRE_EQUAL(query.to_prevout_tx(3), tx_link::terminal);
    BOOST_REQUIRE_EQUAL(query.to_prevout_tx(4), 1u); // block1a:0 (second serialized tx)
    BOOST_REQUIRE_EQUAL(query.to_prevout_tx(5), 1u); // block1a:0 (second serialized tx)
    BOOST_REQUIRE_EQUAL(query.to_prevout_tx(6), tx_link::terminal);
    BOOST_REQUIRE_EQUAL(query.to_prevout_tx(7), tx_link::terminal);

    const output_links expected_prevouts1{ output_link::terminal };
    const output_links expected_prevouts2{ output_link::terminal, output_link::terminal };
    const output_links expected_prevouts3{ output_link::terminal, output_link::terminal, output_link::terminal };
    const output_links expected_prevouts{ 0x51u, 0x51u + 7u };
    BOOST_REQUIRE_EQUAL(query.to_prevouts(0), expected_prevouts1);
    BOOST_REQUIRE_EQUAL(query.to_prevouts(1), expected_prevouts3);
    BOOST_REQUIRE_EQUAL(query.to_prevouts(2), expected_prevouts);
    BOOST_REQUIRE_EQUAL(query.to_prevouts(3), expected_prevouts2);

    // First tx is coinbase, or tx has undefined prevouts.
    BOOST_REQUIRE_EQUAL(query.to_block_prevouts(0), output_links{});
    BOOST_REQUIRE_EQUAL(query.to_block_prevouts(1), output_links{});
    BOOST_REQUIRE_EQUAL(query.to_block_prevouts(2), expected_prevouts2);

    // Past end.
    BOOST_REQUIRE_EQUAL(query.to_prevout_tx(8), tx_link::terminal);
}

// to_spenders1
// to_spenders2
// to_duplicates

// to_block

BOOST_AUTO_TEST_CASE(query_navigate__to_block__always__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };

    class accessor
      : public test::query_accessor
    {
    public:
        using test::query_accessor::query_accessor;
        using test::query_accessor::to_block;
    };

    accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(test::events_handler), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, test::context, false, false));
    BOOST_REQUIRE(query.set(test::block2, test::context, false, false));

    // Either not strong or not found, except genesis.
    BOOST_REQUIRE(!query.to_block(0).is_terminal());
    BOOST_REQUIRE(query.to_block(1).is_terminal());
    BOOST_REQUIRE(query.to_block(2).is_terminal());
    BOOST_REQUIRE(query.to_block(3).is_terminal());
}

// to_input_index
// to_output_index

BOOST_AUTO_TEST_SUITE_END()
