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
#include "../../test.hpp"
#include "../../mocks/chunk_storage.hpp"

BOOST_AUTO_TEST_SUITE(prevout_tests)

// Setting block metadata on a shared instance creates test side effects.
// Chain objects such as blocks cannot be copied for side-effect-free metadata tests, since
// block copy takes shared pointer references. So create new test blocks for each metadata test.
#define DECLARE_BOGUS_BLOCK \
    const block bogus_block \
    { \
        header \
        { \
            0x31323334, \
            system::null_hash, \
            system::one_hash, \
            0x41424344, \
            0x51525354, \
            0x61626364 \
        }, \
        transactions \
        { \
            transaction \
            { \
                0x01, \
                inputs \
                { \
                    input \
                    { \
                        point{}, \
                        script{}, \
                        witness{}, \
                        0x02 \
                    }, \
                    input \
                    { \
                        point{}, \
                        script{}, \
                        witness{}, \
                        0x03 \
                    } \
                }, \
                outputs \
                { \
                    output \
                    { \
                        0x04, \
                        script{} \
                    } \
                }, \
                0x05 \
            }, \
            transaction \
            { \
                0x06, \
                inputs \
                { \
                    input \
                    { \
                        point{}, \
                        script{}, \
                        witness{}, \
                        0x07 \
                    }, \
                    input \
                    { \
                        point{}, \
                        script{}, \
                        witness{}, \
                        0x08 \
                    } \
                }, \
                outputs \
                { \
                    output \
                    { \
                        0x09, \
                        script{} \
                    } \
                }, \
                0x0a \
            }, \
            transaction \
            { \
                0x0b, \
                inputs \
                { \
                    input \
                    { \
                        point{}, \
                        script{}, \
                        witness{}, \
                        0x0c \
                    }, \
                    input \
                    { \
                        point{}, \
                        script{}, \
                        witness{}, \
                        0x0d \
                    } \
                }, \
                outputs \
                { \
                    output \
                    { \
                        0x0e, \
                        script{} \
                    } \
                }, \
                0x0f \
            } \
        } \
    }

using namespace system;
using namespace system::chain;
using tx = schema::transaction::link;
constexpr auto terminal = schema::transaction::link::terminal;
const std::vector<tx::integer> conflicts1{ 0x01020304 };
const std::vector<tx::integer> conflicts2{ 0xbaadf00d, 0x01020304 };

BOOST_AUTO_TEST_CASE(prevout__put__at1__expected)
{
    DECLARE_BOGUS_BLOCK;
    const table::prevout::slab_put_ref slab1{ {}, conflicts1, bogus_block };
    const table::prevout::slab_put_ref slab2{ {}, conflicts2, bogus_block };
    test::chunk_storage head_store{};
    test::chunk_storage body_store{};
    table::prevout instance{ head_store, body_store, 5 };
    BOOST_REQUIRE(instance.create());

    // Put at key.
    BOOST_REQUIRE(instance.put(3, slab1));
    BOOST_REQUIRE(instance.put(42, slab2));

    // Dereference at key to get link.
    BOOST_REQUIRE_EQUAL(instance.at(3), 0u);
    BOOST_REQUIRE_EQUAL(instance.at(42), 37u);
    BOOST_REQUIRE(instance.at(0).is_terminal());
    BOOST_REQUIRE(instance.at(1).is_terminal());
    BOOST_REQUIRE(instance.at(2).is_terminal());
    BOOST_REQUIRE(instance.at(4).is_terminal());
}

BOOST_AUTO_TEST_CASE(prevout__put__at2__expected)
{
    DECLARE_BOGUS_BLOCK;
    const table::prevout::slab_put_ref slab1{ {}, conflicts1, bogus_block };
    const table::prevout::slab_put_ref slab2{ {}, conflicts2, bogus_block };
    table::prevout::slab_get element{};
    test::chunk_storage head_store{};
    test::chunk_storage body_store{};
    table::prevout instance{ head_store, body_store, 5 };
    BOOST_REQUIRE(instance.create());

    // Put at key.
    BOOST_REQUIRE(instance.put(3, slab1));
    BOOST_REQUIRE(instance.put(42, slab2));

    // Dereference at key to get element.
    BOOST_REQUIRE(instance.at(3, element));
    BOOST_REQUIRE(element.conflicts == slab1.conflicts);
    ////BOOST_REQUIRE(element.spends == spends1);
    BOOST_REQUIRE(instance.at(42, element));
    BOOST_REQUIRE(element.conflicts == slab2.conflicts);
    ////BOOST_REQUIRE(element.spends == spends2);
    BOOST_REQUIRE(!instance.at(0, element));
    BOOST_REQUIRE(!instance.at(1, element));
    BOOST_REQUIRE(!instance.at(2, element));
    BOOST_REQUIRE(!instance.at(4, element));
}

BOOST_AUTO_TEST_CASE(prevout__put__exists__expected)
{
    DECLARE_BOGUS_BLOCK;
    const table::prevout::slab_put_ref slab1{ {}, conflicts1, bogus_block };
    const table::prevout::slab_put_ref slab2{ {}, conflicts2, bogus_block };
    test::chunk_storage head_store{};
    test::chunk_storage body_store{};
    table::prevout instance{ head_store, body_store, 5 };
    BOOST_REQUIRE(instance.create());

    // Put at key.
    BOOST_REQUIRE(instance.put(3, slab1));
    BOOST_REQUIRE(instance.put(42, slab2));

    // Exists at key.
    BOOST_REQUIRE(instance.exists(3));
    BOOST_REQUIRE(instance.exists(42));
    BOOST_REQUIRE(!instance.exists(0));
    BOOST_REQUIRE(!instance.exists(1));
    BOOST_REQUIRE(!instance.exists(2));
    BOOST_REQUIRE(!instance.exists(4));
}

BOOST_AUTO_TEST_CASE(prevout__put__get__expected)
{
    DECLARE_BOGUS_BLOCK;
    const table::prevout::slab_put_ref slab1{ {}, conflicts1, bogus_block };
    const table::prevout::slab_put_ref slab2{ {}, conflicts2, bogus_block };
    table::prevout::slab_get element{};
    test::chunk_storage head_store{};
    test::chunk_storage body_store{};
    table::prevout instance{ head_store, body_store, 5 };
    BOOST_REQUIRE(instance.create());

    // Put at key.
    BOOST_REQUIRE(instance.put(3, slab1));
    BOOST_REQUIRE_EQUAL(instance.at(3), 0u);
    BOOST_REQUIRE(instance.put(42, slab2));
    BOOST_REQUIRE_EQUAL(instance.at(42), 37u);

    // Get at link.
    BOOST_REQUIRE(instance.get(0, element));
    BOOST_REQUIRE(element.conflicts == slab1.conflicts);
    BOOST_REQUIRE(instance.get(37u, element));
    BOOST_REQUIRE(element.conflicts == slab2.conflicts);
}

////// values
////
////BOOST_AUTO_TEST_CASE(prevout__put__isolated_values__expected)
////{
////    constexpr auto bits = sub1(to_bits(schema::transaction::link::size));
////
////    test::chunk_storage head_store{};
////    test::chunk_storage body_store{};
////    table::prevout instance{ head_store, body_store, 5 };
////    BOOST_REQUIRE(instance.create());
////
////    constexpr auto cb_only = table::prevout::record{ {}, 0b10000000'00000000'00000000'00000000_u32 };
////    BOOST_REQUIRE(instance.put(3, cb_only));
////
////    constexpr auto tx_only = table::prevout::record{ {}, 0b01010101'01010101'01010101'01010101_u32 };
////    BOOST_REQUIRE(instance.put(42, tx_only));
////
////    table::prevout::record element1{};
////    BOOST_REQUIRE(instance.at(3, element1));
////    BOOST_REQUIRE(element1.coinbase());
////    BOOST_REQUIRE_EQUAL(element1.coinbase(), cb_only.coinbase());
////    BOOST_REQUIRE_EQUAL(element1.output_tx_fk(), cb_only.output_tx_fk());
////    BOOST_REQUIRE_EQUAL(element1.output_tx_fk(), set_right(cb_only.prevout_tx, bits, false));
////
////    table::prevout::record element2{};
////    BOOST_REQUIRE(instance.at(42, element2));
////    BOOST_REQUIRE(!element2.coinbase());
////    BOOST_REQUIRE_EQUAL(element2.coinbase(), tx_only.coinbase());
////    BOOST_REQUIRE_EQUAL(element2.output_tx_fk(), tx_only.output_tx_fk());
////    BOOST_REQUIRE_EQUAL(element2.output_tx_fk(), set_right(tx_only.prevout_tx, bits, false));
////}
////
////BOOST_AUTO_TEST_CASE(prevout__put__merged_values__expected)
////{
////    test::chunk_storage head_store{};
////    test::chunk_storage body_store{};
////    table::prevout instance{ head_store, body_store, 5 };
////    BOOST_REQUIRE(instance.create());
////
////    constexpr auto expected_cb = true;
////    constexpr auto expected_tx = 0b01010101'01010101'01010101'01010101_u32;
////    auto record = table::prevout::record{ {}, 0_u32 };
////    record.set(expected_cb, expected_tx);
////    BOOST_REQUIRE(instance.put(3, record));
////
////    table::prevout::record element{};
////    BOOST_REQUIRE(instance.at(3, element));
////    BOOST_REQUIRE_EQUAL(element.coinbase(), expected_cb);
////    BOOST_REQUIRE_EQUAL(element.output_tx_fk(), expected_tx);
////}
////
////// put_ref
////
////BOOST_AUTO_TEST_CASE(prevout__put_ref__get_non_empty_block_with_default_metadata__inside_spend_terminals)
////{
////    DECLARE_BOGUS_BLOCK;
////
////    test::chunk_storage head_store{};
////    test::chunk_storage body_store{};
////    table::prevout instance{ head_store, body_store, 5 };
////    BOOST_REQUIRE(instance.create());
////
////    const auto record = table::prevout::put_ref{ {}, bogus_block };
////    BOOST_REQUIRE(instance.put(2, record));
////
////    table::prevout::record_get element{};
////    const auto spends = bogus_block.spends();
////    BOOST_REQUIRE_EQUAL(spends, 4u);
////
////    element.values.resize(spends);
////    BOOST_REQUIRE(instance.at(2, element));
////    BOOST_REQUIRE_EQUAL(element.count(), 4u);
////
////    // First block.tx is coinbase, no spends, so only 4, all terminal (defaults).
////    BOOST_REQUIRE_EQUAL(element.values.at(0).prevout_tx, terminal);
////    BOOST_REQUIRE_EQUAL(element.values.at(1).prevout_tx, terminal);
////    BOOST_REQUIRE_EQUAL(element.values.at(2).prevout_tx, terminal);
////    BOOST_REQUIRE_EQUAL(element.values.at(3).prevout_tx, terminal);
////
////    // Block-internal spend.
////    // Positionally identifies spends not requiring a double spend check.
////    // Blocks are guarded agianst internal double spends, and tx previously
////    // confirmed would imply a double spend of it or one of its ancestors.
////    BOOST_REQUIRE(element.inside(0));
////    BOOST_REQUIRE(element.inside(1));
////    BOOST_REQUIRE(element.inside(2));
////    BOOST_REQUIRE(element.inside(3));
////
////    // Inside are always reflected as coinbase.
////    BOOST_REQUIRE(element.coinbase(0));
////    BOOST_REQUIRE(element.coinbase(1));
////    BOOST_REQUIRE(element.coinbase(2));
////    BOOST_REQUIRE(element.coinbase(3));
////
////    // Inside are always mapped to terminal.
////    BOOST_REQUIRE_EQUAL(element.output_tx_fk(0), terminal);
////    BOOST_REQUIRE_EQUAL(element.output_tx_fk(1), terminal);
////    BOOST_REQUIRE_EQUAL(element.output_tx_fk(2), terminal);
////    BOOST_REQUIRE_EQUAL(element.output_tx_fk(3), terminal);
////}
////
////BOOST_AUTO_TEST_CASE(prevout__put_ref__get_non_empty_block_with_metadata__expected)
////{
////    DECLARE_BOGUS_BLOCK;
////
////    const auto tx0 = bogus_block.transactions_ptr()->at(0);
////    const auto tx1 = bogus_block.transactions_ptr()->at(1);
////    const auto tx2 = bogus_block.transactions_ptr()->at(2);
////
////    const auto& in0_0 = *tx0->inputs_ptr()->at(0);
////    const auto& in0_1 = *tx0->inputs_ptr()->at(1);
////    const auto& in1_0 = *tx1->inputs_ptr()->at(0);
////    const auto& in1_1 = *tx1->inputs_ptr()->at(1);
////    const auto& in2_0 = *tx2->inputs_ptr()->at(0);
////    const auto& in2_1 = *tx2->inputs_ptr()->at(1);
////
////    // Coinbase identifies parent of prevout (spent), not input (spender).
////    in0_0.metadata.parent = 0x01234560_u32;
////    in0_1.metadata.parent = 0x01234561_u32;
////    in1_0.metadata.parent = 0x01234562_u32;
////    in1_1.metadata.parent = 0x01234563_u32;
////    in2_0.metadata.parent = 0x01234564_u32;
////    in2_1.metadata.parent = 0x01234565_u32;
////
////    // Coinbase identifies prevout (spent), not input (spender).
////    in0_0.metadata.coinbase = false;
////    in0_1.metadata.coinbase = true;
////    in1_0.metadata.coinbase = false;
////    in1_1.metadata.coinbase = false;
////    in2_0.metadata.coinbase = false;
////    in2_1.metadata.coinbase = true;
////
////    // Inside implies prevout spent in block (terminal).
////    in0_0.metadata.inside = false;
////    in0_1.metadata.inside = true;
////    in1_0.metadata.inside = false;
////    in1_1.metadata.inside = false;
////    in2_0.metadata.inside = true;
////    in2_1.metadata.inside = false;
////
////    test::chunk_storage head_store{};
////    test::chunk_storage body_store{};
////    table::prevout instance{ head_store, body_store, 5 };
////    BOOST_REQUIRE(instance.create());
////
////    const auto record = table::prevout::put_ref{ {}, bogus_block };
////    BOOST_REQUIRE(instance.put(2, record));
////
////    table::prevout::record_get element{};
////    const auto spends = bogus_block.spends();
////    BOOST_REQUIRE_EQUAL(spends, 4u);
////
////    element.values.resize(spends);
////    BOOST_REQUIRE(instance.at(2, element));
////    BOOST_REQUIRE_EQUAL(element.count(), spends);
////
////    // First block.tx is coinbase, no spends, so only 4, none terminal.
////    BOOST_REQUIRE_NE(element.values.at(0).prevout_tx, terminal);
////    BOOST_REQUIRE_NE(element.values.at(1).prevout_tx, terminal);
////    BOOST_REQUIRE_EQUAL(element.values.at(2).prevout_tx, terminal);
////    BOOST_REQUIRE_NE(element.values.at(3).prevout_tx, terminal);
////
////    // Inside spends are used as positional placeholders for double spend check bypass.
////    BOOST_REQUIRE(!element.inside(0));
////    BOOST_REQUIRE(!element.inside(1));
////    BOOST_REQUIRE(element.inside(2));
////    BOOST_REQUIRE(!element.inside(3));
////
////    BOOST_REQUIRE(!element.coinbase(0));
////    BOOST_REQUIRE(!element.coinbase(1));
////    BOOST_REQUIRE(element.coinbase(2));
////    BOOST_REQUIRE(element.coinbase(3));
////
////    // Spend ordinal position relative to the block must be preserved (coinbase excluded).
////    BOOST_REQUIRE_EQUAL(element.output_tx_fk(0), in1_0.metadata.parent);
////    BOOST_REQUIRE_EQUAL(element.output_tx_fk(1), in1_1.metadata.parent);
////    BOOST_REQUIRE_EQUAL(element.output_tx_fk(2), terminal);
////    BOOST_REQUIRE_EQUAL(element.output_tx_fk(3), in2_1.metadata.parent);
////}

BOOST_AUTO_TEST_SUITE_END()
