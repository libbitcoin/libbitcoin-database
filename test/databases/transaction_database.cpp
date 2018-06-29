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

#define DIRECTORY "transaction_database"
#define TRANSACTION1 "0100000001537c9d05b5f7d67b09e5108e3bd5e466909cc9403ddd98bc42973f366fe729410600000000ffffffff0163000000000000001976a914fe06e7b4c88a719e92373de489c08244aee4520b88ac00000000"
#define TRANSACTION2 "010000000147811c3fc0c0e750af5d0ea7343b16ea2d0c291c002e3db778669216eb689de80000000000ffffffff0118ddf505000000001976a914575c2f0ea88fcbad2389a372d942dea95addc25b88ac00000000"

struct transaction_database_directory_setup_fixture
{
    transaction_database_directory_setup_fixture()
    {
        test::clear_path(DIRECTORY);
    }
};

BOOST_FIXTURE_TEST_SUITE(database_tests, transaction_database_directory_setup_fixture)

// TODO: reimplement.
BOOST_AUTO_TEST_CASE(transaction_database__test)
{
    ////transaction tx1;
    ////data_chunk wire_tx1;
    ////BOOST_REQUIRE(decode_base16(wire_tx1, TRANSACTION1));
    ////BOOST_REQUIRE(tx1.from_data(wire_tx1));

    ////transaction tx2;
    ////data_chunk wire_tx2;
    ////BOOST_REQUIRE(decode_base16(wire_tx2, TRANSACTION2));
    ////BOOST_REQUIRE(tx2.from_data(wire_tx2));

    ////const auto path = DIRECTORY "/tx_table";
    ////test::create(path);
    ////transaction_database db(path, 1000, 50, 0);
    ////BOOST_REQUIRE(db.create());

    ////const auto hash1 = tx1.hash();
    ////BOOST_REQUIRE(!db.get(hash1));

    ////const auto hash2 = tx2.hash();
    ////BOOST_REQUIRE(!db.get(hash2));

    ////db.store(tx1, 110, 0, 88, false);
    ////db.store(tx2, 4, 0, 6, false);

    ////const auto result1 = db.get(hash1);
    ////BOOST_REQUIRE(result1);
    ////BOOST_REQUIRE(result1.transaction().hash() == hash1);

    ////const auto result2 = db.get(hash2);
    ////BOOST_REQUIRE(result2);
    ////BOOST_REQUIRE(result2.transaction().hash() == hash2);

    ////// Verify computed hash.
    ////const auto result3 = db.get(result2.link());
    ////BOOST_REQUIRE(result3);
    ////BOOST_REQUIRE(result3.transaction().hash() == hash2);

    ////db.commit();
}

BOOST_AUTO_TEST_SUITE_END()
