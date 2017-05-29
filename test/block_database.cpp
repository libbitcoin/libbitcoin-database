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
#include <bitcoin/database.hpp>

using namespace boost::system;
using namespace boost::filesystem;
using namespace bc;
using namespace bc::chain;
using namespace bc::database;

transaction random_tx(size_t fudge)
{
    static const auto genesis = block::genesis_mainnet();
    auto result = genesis.transactions()[0];
    result.inputs()[0].previous_output().set_index(fudge);
    return result;
}

#define DIRECTORY "block_database"

class block_database_directory_setup_fixture
{
public:
    block_database_directory_setup_fixture()
    {
        error_code ec;
        remove_all(DIRECTORY, ec);
        BOOST_REQUIRE(create_directories(DIRECTORY, ec));
        log::initialize();
    }
};

BOOST_FIXTURE_TEST_SUITE(database_tests, block_database_directory_setup_fixture)

BOOST_AUTO_TEST_CASE(block_database__test)
{
    auto block0 = block::genesis_mainnet();
    block0.set_transactions(
    {
        random_tx(0),
        random_tx(1)
    });
    //const auto h0 = block0.header.hash();

    block block1;
    block1.set_header(block0.header());
    block1.header().set_nonce(4);
    block1.set_transactions(
    {
        random_tx(2),
        random_tx(3),
        random_tx(4),
        random_tx(5)
    });
    //const auto h1 = block1.header.hash();

    block block2;
    block2.set_header(block0.header());
    block2.header().set_nonce(110);
    block2.set_transactions(
    {
        random_tx(6),
        random_tx(7),
        random_tx(8),
        random_tx(9),
        random_tx(10)
    });
    const auto h2 = block2.hash();

    block block3;
    block3.set_header(block0.header());
    block3.header().set_nonce(88);
    block3.set_transactions(
    {
        random_tx(11),
        random_tx(12),
        random_tx(13)
    });
    //const auto h3 = block3.hash();

    block block4a;
    block4a.set_header(block0.header());
    block4a.header().set_nonce(63);
    block4a.set_transactions(
    {
        random_tx(14),
        random_tx(15),
        random_tx(16)
    });
    const auto h4a = block4a.hash();

    block block5a;
    block5a.set_header(header(block0.header()));
    block5a.header().set_nonce(99);
    block2.set_transactions(
    {
        random_tx(17),
        random_tx(18),
        random_tx(19),
        random_tx(20),
        random_tx(21)
    });
    const auto h5a = block5a.hash();

    block block4b;
    block4b.set_header(block0.header());
    block4b.header().set_nonce(633);
    block2.set_transactions(
    {
        random_tx(22),
        random_tx(23),
        random_tx(24)
    });
    const auto h4b = block4b.hash();

    block block5b;
    block5b.set_header(block0.header());
    block5b.header().set_nonce(222);
    block2.set_transactions(
    {
        random_tx(25),
        random_tx(26),
        random_tx(27),
        random_tx(28),
        random_tx(29)
    });
    const auto h5b = block5b.hash();

    const auto block_table = DIRECTORY "/block_table";
    const auto header_index = DIRECTORY "/header_index";
    const auto block_index = DIRECTORY "/block_index";
    const auto tx_index = DIRECTORY "/tx_index";

    store::create(block_table);
    store::create(header_index);
    store::create(block_index);
    store::create(tx_index);
    block_database db(block_table, header_index, block_index, tx_index, 1000, 50);
    BOOST_REQUIRE(db.create());

    size_t height;
    BOOST_REQUIRE(!db.top(height));

    db.store(block0, 0, true);
    db.store(block1, 1, true);
    db.store(block2, 2, true);
    db.store(block3, 3, true);
    BOOST_REQUIRE(db.top(height));
    BOOST_REQUIRE_EQUAL(height, 3u);

    // Fetch block 2 by hash.
    {
        auto res_h2 = db.get(h2, true);
        BOOST_REQUIRE(res_h2);
        BOOST_REQUIRE(res_h2.hash() == h2);

        // TODO: set tx association into metadata and validate here.
        ////BOOST_REQUIRE(res_h2.transaction_hash(0) == block2.transactions()[0].hash());
        ////BOOST_REQUIRE(res_h2.transaction_hash(1) == block2.transactions()[1].hash());
        ////BOOST_REQUIRE(res_h2.transaction_hash(2) == block2.transactions()[2].hash());
        ////BOOST_REQUIRE(res_h2.transaction_hash(3) == block2.transactions()[3].hash());
        ////BOOST_REQUIRE(res_h2.transaction_hash(4) == block2.transactions()[4].hash());
    }

    // Try a fork event.
    db.store(block4a, 4, true);
    db.store(block5a, 5, true);

    // Fetch blocks.
    {
        auto result4a = db.get(4);
        BOOST_REQUIRE(result4a);
        BOOST_REQUIRE(result4a.hash() == h4a);
        auto res5a = db.get(5);
        BOOST_REQUIRE(res5a);
        BOOST_REQUIRE(res5a.hash() == h5a);
    }

    // Unlink old chain.
    BOOST_REQUIRE(db.top(height));
    BOOST_REQUIRE_EQUAL(height, 5u);
    db.unconfirm(4);
    BOOST_REQUIRE(db.top(height));
    BOOST_REQUIRE_EQUAL(height, 3u);

    // Block 3 exists.
    {
        auto result3 = db.get(3);
        BOOST_REQUIRE(result3);
    }

    // No blocks exist now.
    {
        auto result4_none = db.get(4);
        BOOST_REQUIRE(!result4_none);
        auto res5_none = db.get(5);
        BOOST_REQUIRE(!res5_none);
    }

    // Add new blocks.
    db.store(block4b, 4, true);
    db.store(block5b, 5, true);
    BOOST_REQUIRE(db.top(height));
    BOOST_REQUIRE_EQUAL(height, 5u);

    // Fetch blocks.
    {
        auto result4b = db.get(4);
        BOOST_REQUIRE(result4b);
        BOOST_REQUIRE(result4b.hash() == h4b);
        auto res5b = db.get(5);
        BOOST_REQUIRE(res5b);
        BOOST_REQUIRE(res5b.hash() == h5b);

        // TODO: set tx association into metadata and validate here.
        ////BOOST_REQUIRE(res5b.transaction_hash(0) == block5b.transactions()[0].hash());
        ////BOOST_REQUIRE(res5b.transaction_hash(1) == block5b.transactions()[1].hash());
        ////BOOST_REQUIRE(res5b.transaction_hash(2) == block5b.transactions()[2].hash());
        ////BOOST_REQUIRE(res5b.transaction_hash(3) == block5b.transactions()[3].hash());
        ////BOOST_REQUIRE(res5b.transaction_hash(4) == block5b.transactions()[4].hash());
    }

    // Test also fetch by hash.
    {
        auto res_h5b = db.get(h5b, true);
        BOOST_REQUIRE(res_h5b);
        BOOST_REQUIRE(res_h5b.hash() == h5b);
        db.commit();
    }
}

BOOST_AUTO_TEST_SUITE_END()
