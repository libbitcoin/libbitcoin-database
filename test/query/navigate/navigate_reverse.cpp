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
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, context{ 0, 1, 0 }, false, false));
    BOOST_REQUIRE(query.set(test::block2, context{ 0, 2, 0 }, false, false));
    BOOST_REQUIRE(query.set(test::block1a, context{ 0, 1, 0 }, false, false));
    BOOST_REQUIRE(query.set(test::block2a, context{ 0, 2, 0 }, false, false));
    BOOST_REQUIRE_EQUAL(query.to_parent(0), header_link::terminal);
    BOOST_REQUIRE_EQUAL(query.to_parent(1), 0u);
    BOOST_REQUIRE_EQUAL(query.to_parent(2), 1u);
    BOOST_REQUIRE_EQUAL(query.to_parent(3), 0u);
    BOOST_REQUIRE_EQUAL(query.to_parent(4), 3u);
    BOOST_REQUIRE_EQUAL(query.to_parent(5), header_link::terminal);
}

// to_touched_txs1

BOOST_AUTO_TEST_CASE(query_navigate__to_touched_txs1__always__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(setup_three_block_unconfirmed_address_store(query));

    output_links links{};
    BOOST_REQUIRE(!query.to_address_outputs(links, test::block1a_address1));

    tx_links out{};
    BOOST_REQUIRE(!query.to_touched_txs(out, links));
    BOOST_REQUIRE_EQUAL(out.size(), 4u);

    // owners
    BOOST_REQUIRE_EQUAL(out.at(0), 1u); // block1a (tx1)

    // spenders of (0)
    BOOST_REQUIRE_EQUAL(out.at(1), 2u); // block2a (tx2/3)
    BOOST_REQUIRE_EQUAL(out.at(2), 4u); // tx4
    BOOST_REQUIRE_EQUAL(out.at(3), 6u); // block3a (tx6)
}

// to_touched_txs2

BOOST_AUTO_TEST_CASE(query_navigate__to_touched_txs2__always__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(setup_three_block_unconfirmed_address_store(query));

    output_links links{};
    const std::atomic_bool cancel{};
    BOOST_REQUIRE(!query.to_address_outputs(cancel, links, test::block1a_address0));

    tx_links out{};
    BOOST_REQUIRE(!query.to_touched_txs(cancel, out, links));
    BOOST_REQUIRE_EQUAL(out.size(), 10u);

    // owners
    BOOST_REQUIRE_EQUAL(out.at(0), 1u); // block1a (tx1)

    // spenders of (0)
    BOOST_REQUIRE_EQUAL(out.at(1), 2u); // block2a (tx2/3)
    BOOST_REQUIRE_EQUAL(out.at(2), 4u); // tx4
    BOOST_REQUIRE_EQUAL(out.at(3), 5u); // tx5
    BOOST_REQUIRE_EQUAL(out.at(4), 6u); // block3a (tx6)

    // owners (unspent)
    BOOST_REQUIRE_EQUAL(out.at(5), 2u); // block2a (tx2)    [duplicate]
    BOOST_REQUIRE_EQUAL(out.at(6), 3u); // block2a (tx3)
    BOOST_REQUIRE_EQUAL(out.at(7), 4u); // tx4              [duplicate]
    BOOST_REQUIRE_EQUAL(out.at(8), 5u); // tx5              [duplicate]
    BOOST_REQUIRE_EQUAL(out.at(9), 6u); // block3a (tx6)    [duplicate]
}

// to_address_outputs1

BOOST_AUTO_TEST_CASE(query_navigate__to_address_outputs1__always__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(setup_three_block_unconfirmed_address_store(query));

    output_links out{};
    BOOST_REQUIRE(!query.to_address_outputs(out, test::block1a_address1));

    // There is 1 instance of the `script{ { { opcode::roll } } }` output.
    BOOST_REQUIRE_EQUAL(out.size(), 1u);
    BOOST_REQUIRE_EQUAL(out.front(), 88u);
}

// to_address_outputs2

BOOST_AUTO_TEST_CASE(query_navigate__to_address_outputs2__always__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(setup_three_block_unconfirmed_address_store(query));

    output_links out{};
    const std::atomic_bool cancel{};
    BOOST_REQUIRE(!query.to_address_outputs(cancel, out, test::block1a_address0));

    // There are 6 instances of the `script{ { { opcode::pick } } }` output.
    BOOST_REQUIRE_EQUAL(out.size(), 6u);
    BOOST_REQUIRE_EQUAL(out.at(0), 123u);
    BOOST_REQUIRE_EQUAL(out.at(1), 116u);
    BOOST_REQUIRE_EQUAL(out.at(2), 109u);
    BOOST_REQUIRE_EQUAL(out.at(3), 102u);
    BOOST_REQUIRE_EQUAL(out.at(4), 95u);
    BOOST_REQUIRE_EQUAL(out.at(5), 81u);
}

// to_address_outputs3

BOOST_AUTO_TEST_CASE(query_navigate__to_address_outputs3__terminal__not_reduced)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(setup_three_block_unconfirmed_address_store(query));

    output_links out{};
    address_link end{};
    const std::atomic_bool cancel{};
    BOOST_REQUIRE(!query.to_address_outputs(cancel, end, out, test::block1a_address0));

    // There are 6 instances of the `script{ { { opcode::pick } } }` output.
    BOOST_REQUIRE_EQUAL(out.size(), 6u);
    BOOST_REQUIRE_EQUAL(out.at(0), 123u);
    BOOST_REQUIRE_EQUAL(out.at(1), 116u);
    BOOST_REQUIRE_EQUAL(out.at(2), 109u);
    BOOST_REQUIRE_EQUAL(out.at(3), 102u);
    BOOST_REQUIRE_EQUAL(out.at(4), 95u);
    BOOST_REQUIRE_EQUAL(out.at(5), 81u);
}

BOOST_AUTO_TEST_CASE(query_navigate__to_address_outputs3__stop_mismatch__populated_not_found)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(setup_three_block_unconfirmed_address_store(query));

    output_links out{};
    address_link cursor{ 4242 };
    const std::atomic_bool cancel{};
    const auto ec = query.to_address_outputs(cancel, cursor, out, test::block1a_address0);
    BOOST_REQUIRE_EQUAL(ec, error::not_found);

    // The end was not found but the full list is returned.
    BOOST_REQUIRE_EQUAL(out.size(), 6u);
    BOOST_REQUIRE_EQUAL(out.at(0), 123u);
    BOOST_REQUIRE_EQUAL(out.at(1), 116u);
    BOOST_REQUIRE_EQUAL(out.at(2), 109u);
    BOOST_REQUIRE_EQUAL(out.at(3), 102u);
    BOOST_REQUIRE_EQUAL(out.at(4), 95u);
    BOOST_REQUIRE_EQUAL(out.at(5), 81u);
}

BOOST_AUTO_TEST_CASE(query_navigate__to_address_outputs3__stop_match__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(setup_three_block_unconfirmed_address_store(query));

    output_links out{};
    address_link cursor{ 3 };
    const std::atomic_bool cancel{};
    BOOST_REQUIRE(!query.to_address_outputs(cancel, cursor, out, test::block1a_address0));
    BOOST_REQUIRE_EQUAL(cursor.value, 7u);

    // The stop was found so partial list is returned.
    BOOST_REQUIRE_EQUAL(out.size(), 4u);
    BOOST_REQUIRE_EQUAL(out.at(0), 123u);
    BOOST_REQUIRE_EQUAL(out.at(1), 116u);
    BOOST_REQUIRE_EQUAL(out.at(2), 109u);
    BOOST_REQUIRE_EQUAL(out.at(3), 102u);
}

BOOST_AUTO_TEST_CASE(query_navigate__to_address_outputs3__progression__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));

    output_links out{};
    address_link cursor{};
    const std::atomic_bool cancel{};

    // Add three unconfirmed blocks and two txs, with 7 outputs, 6 matching address.
    BOOST_REQUIRE(setup_three_block_unconfirmed_address_store(query));
    BOOST_REQUIRE(!query.to_address_outputs(cancel, cursor, out, test::block1a_address0));
    BOOST_REQUIRE_EQUAL(cursor.value, 7u);
    BOOST_REQUIRE_EQUAL(out.size(), 6u);
    BOOST_REQUIRE_EQUAL(out.at(0), 123u);
    BOOST_REQUIRE_EQUAL(out.at(1), 116u);
    BOOST_REQUIRE_EQUAL(out.at(2), 109u);
    BOOST_REQUIRE_EQUAL(out.at(3), 102u);
    BOOST_REQUIRE_EQUAL(out.at(4), 95u);
    BOOST_REQUIRE_EQUAL(out.at(5), 81u);

    // Add two unconfirmed blocks with 3 outputs, all matching address.
    BOOST_REQUIRE(query.set(test::block1b, database::context{ 0, 1, 0 }, false, false));
    BOOST_REQUIRE(query.set(test::block2b, database::context{ 0, 2, 0 }, false, false));
    BOOST_REQUIRE(!query.to_address_outputs(cancel, cursor, out, test::block1a_address0));
    BOOST_REQUIRE_EQUAL(cursor.value, 10u);
    BOOST_REQUIRE_EQUAL(out.size(), 3u);
    BOOST_REQUIRE_EQUAL(out.at(0), 144u);
    BOOST_REQUIRE_EQUAL(out.at(1), 137u);
    BOOST_REQUIRE_EQUAL(out.at(2), 130u);

    // Add one tx with one output, matching address.
    BOOST_REQUIRE(query.set(test::tx2b));
    BOOST_REQUIRE(!query.to_address_outputs(cancel, cursor, out, test::block1a_address0));
    BOOST_REQUIRE_EQUAL(cursor.value, 11u);
    BOOST_REQUIRE_EQUAL(out.size(), 1u);
    BOOST_REQUIRE_EQUAL(out.at(0), 151u);

    // No changes to this address since cursor.
    BOOST_REQUIRE(!query.to_address_outputs(cancel, cursor, out, test::block1a_address0));
    BOOST_REQUIRE_EQUAL(cursor.value, 11u);
    BOOST_REQUIRE(out.empty());
}

// to_input_tx
// to_output_tx

BOOST_AUTO_TEST_CASE(query_navigate__to_output_tx__to_output__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, context{ 0, 1, 0 }, false, false));
    BOOST_REQUIRE(query.set(test::block2, context{ 0, 1, 0 }, false, false));
    BOOST_REQUIRE(query.set(test::block3, context{ 0, 1, 0 }, false, false));
    BOOST_REQUIRE(query.set(test::block1a, context{ 0, 1, 0 }, false, false));

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
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1a, context{ 0, 1, 0 }, false, false));
    BOOST_REQUIRE(query.set(test::block2a, context{ 0, 1, 0 }, false, false));

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
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, context{ 0, 1, 0 }, false, false));
    BOOST_REQUIRE(query.set(test::block2, context{ 0, 1, 0 }, false, false));

    // Either not strong or not found, except genesis.
    BOOST_REQUIRE(!query.to_block(0).is_terminal());
    BOOST_REQUIRE(query.to_block(1).is_terminal());
    BOOST_REQUIRE(query.to_block(2).is_terminal());
    BOOST_REQUIRE(query.to_block(3).is_terminal());
}

// to_input_index
// to_output_index

BOOST_AUTO_TEST_SUITE_END()
