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

    ~transaction_database_directory_setup_fixture()
    {
        test::clear_path(DIRECTORY);
    }
};

BOOST_FIXTURE_TEST_SUITE(transaction_database_tests, transaction_database_directory_setup_fixture)

BOOST_AUTO_TEST_CASE(transaction_database__store__single_transactions__success)
{
    transaction tx1;
    data_chunk wire_tx1;
    BOOST_REQUIRE(decode_base16(wire_tx1, TRANSACTION1));
    BOOST_REQUIRE(tx1.from_data(wire_tx1));

    transaction tx2;
    data_chunk wire_tx2;
    BOOST_REQUIRE(decode_base16(wire_tx2, TRANSACTION2));
    BOOST_REQUIRE(tx2.from_data(wire_tx2));

    const auto path = DIRECTORY "/tx_table";
    test::create(path);
    transaction_database db(path, 1000, 50, 0);
    BOOST_REQUIRE(db.create());

    const auto hash1 = tx1.hash();
    BOOST_REQUIRE(!db.get(hash1));

    const auto hash2 = tx2.hash();
    BOOST_REQUIRE(!db.get(hash2));

    // Setup end
    
    db.store({tx1}, 100, 110);

    const auto result1 = db.get(hash1);
    BOOST_REQUIRE(result1);
    BOOST_REQUIRE(result1.transaction().hash() == hash1);

    db.store({tx2}, 200, 220);

    const auto result2 = db.get(hash2);
    BOOST_REQUIRE(result2);
    BOOST_REQUIRE(result2.transaction().hash() == hash2);

    // Verify computed hash and get via link
    const auto result3 = db.get(result2.link());
    BOOST_REQUIRE(result3);
    BOOST_REQUIRE(result3.transaction().hash() == hash2);

}

BOOST_AUTO_TEST_CASE(transaction_database__store__list_of_transactions__success)
{
    transaction tx1;
    data_chunk wire_tx1;
    BOOST_REQUIRE(decode_base16(wire_tx1, TRANSACTION1));
    BOOST_REQUIRE(tx1.from_data(wire_tx1));

    transaction tx2;
    data_chunk wire_tx2;
    BOOST_REQUIRE(decode_base16(wire_tx2, TRANSACTION2));
    BOOST_REQUIRE(tx2.from_data(wire_tx2));

    const auto path = DIRECTORY "/tx_table";
    test::create(path);
    transaction_database db(path, 1000, 50, 0);
    BOOST_REQUIRE(db.create());

    const auto hash1 = tx1.hash();
    BOOST_REQUIRE(!db.get(hash1));

    const auto hash2 = tx2.hash();
    BOOST_REQUIRE(!db.get(hash2));

    // Setup end
    
    db.store({tx1, tx2}, 100, 110);

    const auto result1 = db.get(hash1);
    BOOST_REQUIRE(result1);
    BOOST_REQUIRE(result1.transaction().hash() == hash1);

    const auto result2 = db.get(hash2);
    BOOST_REQUIRE(result2);
    BOOST_REQUIRE(result2.transaction().hash() == hash2);

    // Verify computed hash and get via link
    const auto result3 = db.get(result2.link());
    BOOST_REQUIRE(result3);
    BOOST_REQUIRE(result3.transaction().hash() == hash2);

}

BOOST_AUTO_TEST_CASE(transaction_database__store__single_unconfirmed_without_a_block__success)
{
    transaction tx1;
    data_chunk wire_tx1;
    BOOST_REQUIRE(decode_base16(wire_tx1, TRANSACTION1));
    BOOST_REQUIRE(tx1.from_data(wire_tx1));
    
    const auto path = DIRECTORY "/tx_table";
    test::create(path);
    transaction_database db(path, 1000, 50, 0);
    BOOST_REQUIRE(db.create());

    const auto hash1 = tx1.hash();
    BOOST_REQUIRE(!db.get(hash1));
    
    // Setup end
    
    db.store(tx1, 1);

    const auto result1 = db.get(hash1);
    BOOST_REQUIRE(result1);
    BOOST_REQUIRE(result1.transaction().hash() == hash1);
    
    // Verify computed hash and get via link
    const auto fetched = db.get(result1.link());
    BOOST_REQUIRE(fetched);
    BOOST_REQUIRE(fetched.transaction().hash() == hash1);

}

BOOST_AUTO_TEST_CASE(transaction_database__store__list_unconfirmed_without_a_block__success)
{
    transaction tx1;
    data_chunk wire_tx1;
    BOOST_REQUIRE(decode_base16(wire_tx1, TRANSACTION1));
    BOOST_REQUIRE(tx1.from_data(wire_tx1));

    transaction tx2;
    data_chunk wire_tx2;
    BOOST_REQUIRE(decode_base16(wire_tx2, TRANSACTION2));
    BOOST_REQUIRE(tx2.from_data(wire_tx2));

    const auto path = DIRECTORY "/tx_table";
    test::create(path);
    transaction_database db(path, 1000, 50, 0);
    BOOST_REQUIRE(db.create());

    const auto hash1 = tx1.hash();
    BOOST_REQUIRE(!db.get(hash1));

    const auto hash2 = tx2.hash();
    BOOST_REQUIRE(!db.get(hash2));

    // Setup end
    
    db.store({tx1, tx2});

    const auto result1 = db.get(hash1);
    BOOST_REQUIRE(result1);
    BOOST_REQUIRE(result1.transaction().hash() == hash1);

    const auto result2 = db.get(hash2);
    BOOST_REQUIRE(result2);
    BOOST_REQUIRE(result2.transaction().hash() == hash2);

    // Verify computed hash and get via link
    const auto result3 = db.get(result2.link());
    BOOST_REQUIRE(result3);
    BOOST_REQUIRE(result3.transaction().hash() == hash2);

}


// Candidate/Uncandidate
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(transaction_database__candidate__with_no_input_in_db__returns_false)
{
    transaction tx1;
    data_chunk wire_tx1;
    BOOST_REQUIRE(decode_base16(wire_tx1, TRANSACTION1));
    BOOST_REQUIRE(tx1.from_data(wire_tx1));

    const auto path = DIRECTORY "/tx_table";
    test::create(path);
    transaction_database db(path, 1000, 50, 0);
    BOOST_REQUIRE(db.create());

    const auto hash1 = tx1.hash();
    db.store(tx1, 1);

    const auto result1 = db.get(hash1);
    BOOST_REQUIRE(result1);
    BOOST_REQUIRE(result1.transaction().hash() == hash1);
    
    // Setup end    

    const bool result = db.candidate(db.get(hash1).link());
    BOOST_REQUIRE(!result);

}

BOOST_AUTO_TEST_CASE(transaction_database__candidate__with_input_in_db__returns_true)
{
    uint32_t version = 2345u;
    uint32_t locktime = 0xffffffff;
    
    const auto path = DIRECTORY "/tx_table";
    test::create(path);
    transaction_database db(path, 1000, 50, 0);
    BOOST_REQUIRE(db.create());

    // tx1: coinbase transaction
    const chain::input::list tx1_inputs
    {
        { chain::point{ null_hash, chain::point::null_index }, {}, 0 }
    };
    
    chain::output::list tx1_outputs;
    tx1_outputs.emplace_back();
    tx1_outputs.back().set_value(1200);

    chain::transaction tx1(version, locktime, tx1_inputs, tx1_outputs);
    BOOST_REQUIRE(tx1.is_coinbase());    
    const auto hash1 = tx1.hash();
    db.store(tx1, 1);
    
    const auto result1 = db.get(hash1);
    BOOST_REQUIRE(result1);
    BOOST_REQUIRE(result1.transaction().hash() == hash1);

    // tx2: spends coinbase, tx1 as input, dummy output
    chain::input::list tx2_inputs;
    chain::output_point previous_output{hash1, 0};
    chain::input instance(previous_output, {}, 0);
    tx2_inputs.emplace_back(instance);

    chain::output::list tx2_outputs;
    tx2_outputs.emplace_back();
    tx2_outputs.back().set_value(1200);
    
    chain::transaction tx2(version, locktime, tx2_inputs, tx2_outputs);
    const auto hash2 = tx2.hash();
    db.store(tx2, 1);

    BOOST_REQUIRE(!result1.candidate());
    BOOST_REQUIRE(!result1.transaction().outputs().front().metadata.candidate_spend);
    
    // Setup end    

    const bool result = db.candidate(db.get(hash2).link());
    BOOST_REQUIRE(result);
    
    const auto tx2_reloaded = db.get(hash2);
    BOOST_REQUIRE(tx2_reloaded.candidate());

    const auto tx1_reloaded = db.get(hash1);
    BOOST_REQUIRE(!tx1_reloaded.candidate());
    BOOST_REQUIRE(tx1_reloaded.transaction().outputs().front().metadata.candidate_spend);
    
}

BOOST_AUTO_TEST_CASE(transaction_database__uncandidate__with_no_input_in_db__returns_false)
{
    transaction tx1;
    data_chunk wire_tx1;
    BOOST_REQUIRE(decode_base16(wire_tx1, TRANSACTION1));
    BOOST_REQUIRE(tx1.from_data(wire_tx1));

    const auto path = DIRECTORY "/tx_table";
    test::create(path);
    transaction_database db(path, 1000, 50, 0);
    BOOST_REQUIRE(db.create());

    const auto hash1 = tx1.hash();
    db.store(tx1, 1);

    const auto result1 = db.get(hash1);
    BOOST_REQUIRE(result1);
    BOOST_REQUIRE(result1.transaction().hash() == hash1);
    
    // Setup end    

    const bool result = db.uncandidate(db.get(hash1).link());
    BOOST_REQUIRE(!result);

}

BOOST_AUTO_TEST_CASE(transaction_database__uncandidate__with_input_in_db__returns_true)
{
    uint32_t version = 2345u;
    uint32_t locktime = 0xffffffff;
    
    const auto path = DIRECTORY "/tx_table";
    test::create(path);
    transaction_database db(path, 1000, 50, 0);
    BOOST_REQUIRE(db.create());

    // tx1: coinbase transaction
    const chain::input::list tx1_inputs
    {
        { chain::point{ null_hash, chain::point::null_index }, {}, 0 }
    };
    
    chain::output::list tx1_outputs;
    tx1_outputs.emplace_back();
    tx1_outputs.back().set_value(1200);

    chain::transaction tx1(version, locktime, tx1_inputs, tx1_outputs);
    BOOST_REQUIRE(tx1.is_coinbase());    
    const auto hash1 = tx1.hash();
    db.store(tx1, 1);
    
    const auto result1 = db.get(hash1);
    BOOST_REQUIRE(result1);
    BOOST_REQUIRE(result1.transaction().hash() == hash1);

    // tx2: spends coinbase, tx1 as input, dummy output
    chain::input::list tx2_inputs;
    chain::output_point previous_output{hash1, 0};
    chain::input instance(previous_output, {}, 0);
    tx2_inputs.emplace_back(instance);

    chain::output::list tx2_outputs;
    tx2_outputs.emplace_back();
    tx2_outputs.back().set_value(1200);
    
    chain::transaction tx2(version, locktime, tx2_inputs, tx2_outputs);
    const auto hash2 = tx2.hash();
    db.store(tx2, 1);

    BOOST_REQUIRE(!result1.candidate());
    BOOST_REQUIRE(!result1.transaction().outputs().front().metadata.candidate_spend);
    
    // Setup end
    
    const bool result = db.uncandidate(db.get(hash2).link());
    BOOST_REQUIRE(result);
    
    BOOST_REQUIRE(!db.get(hash2).candidate());

    const auto tx1_reloaded = db.get(hash1);
    BOOST_REQUIRE(!tx1_reloaded.candidate());
    BOOST_REQUIRE(!tx1_reloaded.transaction().outputs().front().metadata.candidate_spend);
}

// Confirm
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(transaction_database__confirm__single_transaction__success)
{
}

BOOST_AUTO_TEST_CASE(transaction_database__confirm__single_transaction_not_in_db__success)
{
}

BOOST_AUTO_TEST_CASE(transaction_database_with_cache__confirm__single_transaction__success)
{
}

// Unconfirm
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(transaction_database__unconfirm__single_unconfirmed__success)
{
}

BOOST_AUTO_TEST_CASE(transaction_database_with_cache__unconfirm__single_confirmed__success)
{
}


// Queries
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(transaction_database__get_block_metadata__nonexisting__not_found)
{
}

BOOST_AUTO_TEST_CASE(transaction_database__get_block_metadata__confirmed_and_fork_height_lt_height__confirmed)
{
}

BOOST_AUTO_TEST_CASE(transaction_database__get_block_metadata__confirmed_and_fork_height_gt_height__unconfirmed)
{
}

// From bitcoin/chain/transaction.hpp
//         /// There is no distiction between a tx that can be valid under some
//         /// forks and one that cannot be valid under any forks. The only
//         /// criteria for storage is deserialization and DoS protection. The
//         /// latter is provided by pool validation or containing block PoW.
//         /// A transaction that is deconfirmed is set to unverified, which is
//         /// simply a storage space optimization. This results in revalidation
//         /// in the case where the transaction may be confirmed again.
//         /// If verified the tx has been validated relative to given forks.
//         bool verified = false;


BOOST_AUTO_TEST_CASE(transaction_database__get_block_metadata__unconfirmed__unverified)
{
}

BOOST_AUTO_TEST_CASE(transaction_database__get_block_metadata__confirmed_forked__unverified)
{
}

BOOST_AUTO_TEST_CASE(transaction_database__get_block_metadata__unverified2__confirmed)
{
}

BOOST_AUTO_TEST_SUITE_END()
