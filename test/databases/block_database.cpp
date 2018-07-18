/**
 * Copyright (c) 2011-2017 libbitcoin developers (see AUTHORS)
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
#include <boost/test/unit_test.hpp>

#include <boost/filesystem.hpp>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database.hpp>
#include "../utility/utility.hpp"

using namespace boost::system;
using namespace boost::filesystem;
using namespace bc;
using namespace bc::chain;
using namespace bc::database;

transaction random_tx(size_t fudge)
{
    static const auto genesis = bc::settings(bc::config::settings::mainnet)
        .genesis_block;
    auto tx = genesis.transactions()[0];
    tx.inputs()[0].previous_output().set_index(fudge);
    tx.metadata.link = fudge;
    return tx;
}

#define DIRECTORY "block_database"

struct block_database_directory_setup_fixture
{
    block_database_directory_setup_fixture()
    {
        test::clear_path(DIRECTORY);
    }
};

BOOST_FIXTURE_TEST_SUITE(database_tests, block_database_directory_setup_fixture)

BOOST_AUTO_TEST_CASE(block_database__test)
{
    // TODO: replace.
    ////auto block0 = block::genesis_mainnet(bc::settings());
    ////block0.set_transactions(
    ////{
    ////    random_tx(0),
    ////    random_tx(1)
    ////});
    ////const auto h0 = block0.hash();

    ////block block1(bc::settings{});
    ////block1.set_header(block0.header());
    ////block1.header().set_nonce(4);
    ////block1.set_transactions(
    ////{
    ////    random_tx(2),
    ////    random_tx(3),
    ////    random_tx(4),
    ////    random_tx(5)
    ////});
    ////const auto h1 = block1.hash();

    ////block block2(bc::settings{});
    ////block2.set_header(block0.header());
    ////block2.header().set_nonce(110);
    ////block2.set_transactions(
    ////{
    ////    random_tx(6),
    ////    random_tx(7),
    ////    random_tx(8),
    ////    random_tx(9),
    ////    random_tx(10)
    ////});
    ////const auto h2 = block2.hash();

    ////block block3(bc::settings{});
    ////block3.set_header(block0.header());
    ////block3.header().set_nonce(88);
    ////block3.set_transactions(
    ////{
    ////    random_tx(11),
    ////    random_tx(12),
    ////    random_tx(13)
    ////});
    ////const auto h3 = block3.hash();

    ////block block4a(bc::settings{});
    ////block4a.set_header(block0.header());
    ////block4a.header().set_nonce(63);
    ////block4a.set_transactions(
    ////{
    ////    random_tx(14),
    ////    random_tx(15),
    ////    random_tx(16)
    ////});
    ////const auto h4a = block4a.hash();

    ////block block4b(bc::settings{});
    ////block4b.set_header(block0.header());
    ////block4b.header().set_nonce(633);
    ////block4b.set_transactions(
    ////{
    ////    random_tx(22),
    ////    random_tx(23),
    ////    random_tx(24)
    ////});
    ////const auto h4b = block4b.hash();

    ////block block5a(bc::settings{});
    ////block5a.set_header(header(block0.header()));
    ////block5a.header().set_nonce(99);
    ////block5a.set_transactions(
    ////{
    ////    random_tx(17),
    ////    random_tx(18),
    ////    random_tx(19),
    ////    random_tx(20),
    ////    random_tx(21)
    ////});
    ////const auto h5a = block5a.hash();

    ////block block5b(bc::settings{});
    ////block5b.set_header(block0.header());
    ////block5b.header().set_nonce(222);
    ////block5b.set_transactions(
    ////{
    ////    random_tx(25),
    ////    random_tx(26),
    ////    random_tx(27),
    ////    random_tx(28),
    ////    random_tx(29)
    ////});
    ////const auto h5b = block5b.hash();

    ////const auto block_table = DIRECTORY "/block_table";
    ////const auto candidate_index = DIRECTORY "/candidate_index";
    ////const auto confirmed_index = DIRECTORY "/confirmed_index";
    ////const auto tx_index = DIRECTORY "/tx_index";

    ////test::create(block_table);
    ////test::create(candidate_index);
    ////test::create(confirmed_index);
    ////test::create(tx_index);
    ////block_database db(block_table, candidate_index, confirmed_index, tx_index, 1000, 50, bc::settings());
    ////BOOST_REQUIRE(db.create());

    ////size_t height;
    ////BOOST_REQUIRE(!db.top(height, false));

    ////db.push(block0, 0, 0);
    ////db.push(block1, 1, 0);
    ////db.push(block2, 2, 0);
    ////db.push(block3, 3, 0);
    ////BOOST_REQUIRE(db.top(height, false));
    ////BOOST_REQUIRE_EQUAL(height, 3u);

    ////// Fetch block 0 by hash.
    ////const auto result0 = db.get(h0);
    ////BOOST_REQUIRE(result0);
    ////BOOST_REQUIRE(result0.hash() == h0);

    ////auto it0 = result0.begin();
    ////BOOST_REQUIRE(it0 != result0.end());
    ////BOOST_REQUIRE_EQUAL(*it0++, 0u);
    ////BOOST_REQUIRE_EQUAL(*it0++, 1u);
    ////BOOST_REQUIRE(it0 == result0.end());

    ////// Fetch block 2 by hash.
    ////const auto result2 = db.get(h2);
    ////BOOST_REQUIRE(result2);
    ////BOOST_REQUIRE(result2.hash() == h2);

    ////auto it2 = result2.begin();
    ////BOOST_REQUIRE(it2 != result2.end());
    ////BOOST_REQUIRE_EQUAL(*it2++, 6u);
    ////BOOST_REQUIRE_EQUAL(*it2++, 7u);
    ////BOOST_REQUIRE_EQUAL(*it2++, 8u);
    ////BOOST_REQUIRE_EQUAL(*it2++, 9u);
    ////BOOST_REQUIRE_EQUAL(*it2++, 10u);
    ////BOOST_REQUIRE(it2 == result2.end());

    ////// Try a fork event.
    ////db.push(block4a, 4, 0);
    ////db.push(block5a, 5, 0);

    ////// Fetch blocks 4/5.
    ////const auto result4a = db.get(4);
    ////BOOST_REQUIRE(result4a);
    ////BOOST_REQUIRE(result4a.hash() == h4a);
    ////const auto result5a = db.get(5);
    ////BOOST_REQUIRE(result5a);
    ////BOOST_REQUIRE(result5a.hash() == h5a);

    ////// Unlink blocks 4a/5a.
    ////BOOST_REQUIRE(db.top(height, false));
    ////BOOST_REQUIRE_EQUAL(height, 5u);
    ////db.unconfirm(h5a, 5, true);
    ////db.unconfirm(h4a, 4, true);
    ////BOOST_REQUIRE(db.top(height, false));
    ////BOOST_REQUIRE_EQUAL(height, 3u);

    ////// Block 3 exists.
    ////const auto result3 = db.get(3);
    ////BOOST_REQUIRE(result3);

    ////// Blocks 4a/5a are missing (verify index guard).
    ////const auto result4 = db.get(4);
    ////BOOST_REQUIRE(!result4);
    ////const auto result5 = db.get(5);
    ////BOOST_REQUIRE(!result5);

    ////// Add new blocks 4b/5b.
    ////db.push(block4b, 4, 0);
    ////db.push(block5b, 5, 0);
    ////BOOST_REQUIRE(db.top(height, false));
    ////BOOST_REQUIRE_EQUAL(height, 5u);

    ////// Fetch blocks 4b/5b.
    ////const auto result4b = db.get(4);
    ////BOOST_REQUIRE(result4b);
    ////BOOST_REQUIRE(result4b.hash() == h4b);
    ////const auto result5b = db.get(5);
    ////BOOST_REQUIRE(result5b);
    ////BOOST_REQUIRE(result5b.hash() == h5b);

    ////// Test also fetch by hash.
    ////const auto result_h5b = db.get(h5b);
    ////BOOST_REQUIRE(result_h5b);
    ////BOOST_REQUIRE(result_h5b.hash() == h5b);

    ////db.commit();
}

BOOST_AUTO_TEST_SUITE_END()
