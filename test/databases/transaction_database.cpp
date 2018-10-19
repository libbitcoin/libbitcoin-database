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
#include "../utility/utility.hpp"

using namespace boost::system;
using namespace boost::filesystem;
using namespace bc;
using namespace bc::chain;
using namespace bc::database;

struct transaction_database_fixture
{
    const std::string directory = "transaction_database";
    const std::string path = "/tx_table";
    const std::string transaction1 = "0100000001537c9d05b5f7d67b09e5108e3bd5e466909cc9403ddd98bc42973f366fe729410600000000ffffffff0163000000000000001976a914fe06e7b4c88a719e92373de489c08244aee4520b88ac00000000";
    const std::string transaction2 = "010000000147811c3fc0c0e750af5d0ea7343b16ea2d0c291c002e3db778669216eb689de80000000000ffffffff0118ddf505000000001976a914575c2f0ea88fcbad2389a372d942dea95addc25b88ac00000000";

    std::unique_ptr<transaction_database> db;
    
    transaction tx1;
    data_chunk wire_tx1;
    hash_digest hash1;

    transaction tx2;
    data_chunk wire_tx2;    
    hash_digest hash2;

    transaction::list all_transactions;
    
    transaction_database_fixture()
    {
        test::clear_path(directory);
        test::create(directory + path);

        db = std::unique_ptr<transaction_database>{new transaction_database(directory + path, 1000, 50, 0)};
        BOOST_REQUIRE(db->create());

        BOOST_REQUIRE(decode_base16(wire_tx1, transaction1));
        BOOST_REQUIRE(tx1.from_data(wire_tx1));
        all_transactions.push_back(tx1);
        
        BOOST_REQUIRE(decode_base16(wire_tx2, transaction2));
        BOOST_REQUIRE(tx2.from_data(wire_tx2));
        all_transactions.push_back(tx2);
        
        hash1 = tx1.hash();        
        hash2 = tx2.hash();
        
        BOOST_REQUIRE(!db->get(hash1));
        BOOST_REQUIRE(!db->get(hash2));
    }

    ~transaction_database_fixture()
    {
        test::clear_path(directory);
    }
    
};

BOOST_FIXTURE_TEST_SUITE(transaction_database_tests, transaction_database_fixture)

BOOST_AUTO_TEST_CASE(transaction_database__store_and_get__single_transactions__succeeded)
{
    db->store({tx1}, 110, 100);

    const auto result1 = db->get(hash1);
    BOOST_REQUIRE(result1);
    BOOST_REQUIRE(result1.transaction().hash() == hash1);

    db->store({tx2}, 220, 200);

    const auto result2 = db->get(hash2);
    BOOST_REQUIRE(result2);
    BOOST_REQUIRE(result2.transaction().hash() == hash2);

    // Verify get by link in tx metadata
    const auto result3 = db->get(result2.link());
    BOOST_REQUIRE(result3);
    BOOST_REQUIRE(result3.transaction().hash() == hash2);

    db->commit();
}

BOOST_AUTO_TEST_CASE(transaction_database__store_and_get__transaction_list__succeeded)
{
    db->store({tx1, tx2}, 110, 100);

    const auto result1 = db->get(hash1);
    BOOST_REQUIRE(result1);
    BOOST_REQUIRE(result1.transaction().hash() == hash1);

    const auto result2 = db->get(hash2);
    BOOST_REQUIRE(result2);
    BOOST_REQUIRE(result2.transaction().hash() == hash2);

    // Verify get by link in tx metadata
    const auto result3 = db->get(result2.link());
    BOOST_REQUIRE(result3);
    BOOST_REQUIRE(result3.transaction().hash() == hash2);

    db->commit();
}

BOOST_AUTO_TEST_SUITE_END()
