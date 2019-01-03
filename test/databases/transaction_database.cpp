/**
 * Copyright (c) 2011-2018 libbitcoin developers (see AUTHORS)
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
using namespace bc::database;
using namespace bc::system;
using namespace bc::system::chain;

#define DIRECTORY "transaction_database"
#define TRANSACTION1 "0100000001537c9d05b5f7d67b09e5108e3bd5e466909cc9403ddd98bc42973f366fe729410600000000ffffffff0163000000000000001976a914fe06e7b4c88a719e92373de489c08244aee4520b88ac00000000"
#define TRANSACTION2 "010000000147811c3fc0c0e750af5d0ea7343b16ea2d0c291c002e3db778669216eb689de80000000000ffffffff0118ddf505000000001976a914575c2f0ea88fcbad2389a372d942dea95addc25b88ac00000000"

static BC_CONSTEXPR auto file_path = DIRECTORY "/tx_table";

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

BOOST_AUTO_TEST_CASE(transaction_database__store1__single_transactions__success)
{
    transaction tx1;
    data_chunk wire_tx1;
    BOOST_REQUIRE(decode_base16(wire_tx1, TRANSACTION1));
    BOOST_REQUIRE(tx1.from_data(wire_tx1));

    transaction tx2;
    data_chunk wire_tx2;
    BOOST_REQUIRE(decode_base16(wire_tx2, TRANSACTION2));
    BOOST_REQUIRE(tx2.from_data(wire_tx2));

    test::create(file_path);
    transaction_database instance(file_path, 1, 1000, 50, 0);
    BOOST_REQUIRE(instance.create());

    const auto hash1 = tx1.hash();
    BOOST_REQUIRE(!instance.get(hash1));

    const auto hash2 = tx2.hash();
    BOOST_REQUIRE(!instance.get(hash2));

    // Setup end

    instance.store(tx1, 100);

    const auto result1 = instance.get(hash1);
    BOOST_REQUIRE(result1);
    BOOST_REQUIRE(result1.transaction().hash() == hash1);

    instance.store(tx2, 200);

    const auto result2 = instance.get(hash2);
    BOOST_REQUIRE(result2);
    BOOST_REQUIRE(result2.transaction().hash() == hash2);

    // Verify computed hash and get via link
    const auto result3 = instance.get(result2.link());
    BOOST_REQUIRE(result3);
    BOOST_REQUIRE(result3.transaction().hash() == hash2);
}

BOOST_AUTO_TEST_CASE(transaction_database__store2__list_of_transactions__success)
{
    transaction tx1;
    data_chunk wire_tx1;
    BOOST_REQUIRE(decode_base16(wire_tx1, TRANSACTION1));
    BOOST_REQUIRE(tx1.from_data(wire_tx1));

    transaction tx2;
    data_chunk wire_tx2;
    BOOST_REQUIRE(decode_base16(wire_tx2, TRANSACTION2));
    BOOST_REQUIRE(tx2.from_data(wire_tx2));

    test::create(file_path);
    transaction_database instance(file_path, 1, 1000, 50, 0);
    BOOST_REQUIRE(instance.create());

    const auto hash1 = tx1.hash();
    BOOST_REQUIRE(!instance.get(hash1));

    const auto hash2 = tx2.hash();
    BOOST_REQUIRE(!instance.get(hash2));

    // Setup end

    instance.store({tx1, tx2});

    const auto result1 = instance.get(hash1);
    BOOST_REQUIRE(result1);
    BOOST_REQUIRE(result1.transaction().hash() == hash1);

    const auto result2 = instance.get(hash2);
    BOOST_REQUIRE(result2);
    BOOST_REQUIRE(result2.transaction().hash() == hash2);

    // Verify computed hash and get via link
    const auto result3 = instance.get(result2.link());
    BOOST_REQUIRE(result3);
    BOOST_REQUIRE(result3.transaction().hash() == hash2);
}

BOOST_AUTO_TEST_CASE(transaction_database__store1__single_unconfirmed__success)
{
    transaction tx1;
    data_chunk wire_tx1;
    BOOST_REQUIRE(decode_base16(wire_tx1, TRANSACTION1));
    BOOST_REQUIRE(tx1.from_data(wire_tx1));

    test::create(file_path);
    transaction_database instance(file_path, 1, 1000, 50, 0);
    BOOST_REQUIRE(instance.create());

    const auto hash1 = tx1.hash();
    BOOST_REQUIRE(!instance.get(hash1));

    // Setup end

    instance.store(tx1, 1);

    const auto result1 = instance.get(hash1);
    BOOST_REQUIRE(result1);
    BOOST_REQUIRE(result1.transaction().hash() == hash1);

    // Verify computed hash and get via link
    const auto fetched = instance.get(result1.link());
    BOOST_REQUIRE(fetched);
    BOOST_REQUIRE(fetched.transaction().hash() == hash1);
}

BOOST_AUTO_TEST_CASE(transaction_database__store2__list_of_unconfirmed__success)
{
    transaction tx1;
    data_chunk wire_tx1;
    BOOST_REQUIRE(decode_base16(wire_tx1, TRANSACTION1));
    BOOST_REQUIRE(tx1.from_data(wire_tx1));

    transaction tx2;
    data_chunk wire_tx2;
    BOOST_REQUIRE(decode_base16(wire_tx2, TRANSACTION2));
    BOOST_REQUIRE(tx2.from_data(wire_tx2));

    test::create(file_path);
    transaction_database instance(file_path, 1, 1000, 50, 0);
    BOOST_REQUIRE(instance.create());

    const auto hash1 = tx1.hash();
    BOOST_REQUIRE(!instance.get(hash1));

    const auto hash2 = tx2.hash();
    BOOST_REQUIRE(!instance.get(hash2));

    // Setup end

    instance.store({tx1, tx2});

    const auto result1 = instance.get(hash1);
    BOOST_REQUIRE(result1);
    BOOST_REQUIRE(result1.transaction().hash() == hash1);

    const auto result2 = instance.get(hash2);
    BOOST_REQUIRE(result2);
    BOOST_REQUIRE(result2.transaction().hash() == hash2);

    // Verify computed hash and get via link
    const auto result3 = instance.get(result2.link());
    BOOST_REQUIRE(result3);
    BOOST_REQUIRE(result3.transaction().hash() == hash2);
}


// Candidate/Uncandidate
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(transaction_database__candidate__with_no_input_in_db__candidate_spend_false)
{
    transaction tx1;
    data_chunk wire_tx1;
    BOOST_REQUIRE(decode_base16(wire_tx1, TRANSACTION1));
    BOOST_REQUIRE(tx1.from_data(wire_tx1));

    test::create(file_path);
    transaction_database instance(file_path, 1, 1000, 50, 0);
    BOOST_REQUIRE(instance.create());

    const auto hash1 = tx1.hash();
    instance.store(tx1, 1);

    const auto result1 = instance.get(hash1);
    BOOST_REQUIRE(result1);
    BOOST_REQUIRE(result1.transaction().hash() == hash1);

    // Setup end

    const bool result = instance.candidate(instance.get(hash1).link());
    BOOST_REQUIRE(!result);
}

BOOST_AUTO_TEST_CASE(transaction_database__candidate__with_input_in_db__candidate_spend_true)
{
    uint32_t version = 2345u;
    uint32_t locktime = 0xffffffff;

    test::create(file_path);
    transaction_database instance(file_path, 1, 1000, 50, 0);
    BOOST_REQUIRE(instance.create());

    // tx1: coinbase transaction
    const chain::input::list tx1_inputs
    {
        { chain::point{ null_hash, chain::point::null_index }, {}, 0 }
    };

    const chain::output::list tx1_outputs
    {
        { 1200, {} }
    };

    const chain::transaction tx1(version, locktime, tx1_inputs, tx1_outputs);
    BOOST_REQUIRE(tx1.is_coinbase());
    const auto hash1 = tx1.hash();
    instance.store(tx1, 1);

    const auto result1 = instance.get(hash1);
    BOOST_REQUIRE(result1);
    BOOST_REQUIRE(result1.transaction().hash() == hash1);

    // tx2: spends coinbase, tx1 as input, dummy output
    const chain::input::list tx2_inputs
    {
        { { hash1, 0 }, {}, 0 }
    };

    const chain::output::list tx2_outputs
    {
        { 1200, {} }
    };

    const chain::transaction tx2(version, locktime, tx2_inputs, tx2_outputs);
    const auto hash2 = tx2.hash();
    instance.store(tx2, 1);

    BOOST_REQUIRE(!result1.candidate());
    BOOST_REQUIRE(!result1.transaction().outputs().front().metadata.candidate_spend);

    // Setup end

    const bool result = instance.candidate(instance.get(hash2).link());
    BOOST_REQUIRE(result);

    const auto tx2_reloaded = instance.get(hash2);
    BOOST_REQUIRE(tx2_reloaded.candidate());

    const auto tx1_reloaded = instance.get(hash1);
    BOOST_REQUIRE(!tx1_reloaded.candidate());
    BOOST_REQUIRE(tx1_reloaded.transaction().outputs().front().metadata.candidate_spend);
}

BOOST_AUTO_TEST_CASE(transaction_database__uncandidate__with_no_input_in_db__false)
{
    transaction tx1;
    data_chunk wire_tx1;
    BOOST_REQUIRE(decode_base16(wire_tx1, TRANSACTION1));
    BOOST_REQUIRE(tx1.from_data(wire_tx1));

    test::create(file_path);
    transaction_database instance(file_path, 1, 1000, 50, 0);
    BOOST_REQUIRE(instance.create());

    const auto hash1 = tx1.hash();
    instance.store(tx1, 1);

    const auto result1 = instance.get(hash1);
    BOOST_REQUIRE(result1);
    BOOST_REQUIRE(result1.transaction().hash() == hash1);

    // Setup end

    const bool result = instance.uncandidate(instance.get(hash1).link());
    BOOST_REQUIRE(!result);
}

BOOST_AUTO_TEST_CASE(transaction_database__uncandidate__with_input_in_db__candidate_spend_true)
{
    uint32_t version = 2345u;
    uint32_t locktime = 0xffffffff;

    test::create(file_path);
    transaction_database instance(file_path, 1, 1000, 50, 0);
    BOOST_REQUIRE(instance.create());

    // tx1: coinbase transaction
    const chain::input::list tx1_inputs
    {
        { chain::point{ null_hash, chain::point::null_index }, {}, 0 }
    };

    const chain::output::list tx1_outputs
    {
        { 1200, {} }
    };

    const chain::transaction tx1(version, locktime, tx1_inputs, tx1_outputs);
    BOOST_REQUIRE(tx1.is_coinbase());
    const auto hash1 = tx1.hash();
    instance.store(tx1, 1);

    const auto result1 = instance.get(hash1);
    BOOST_REQUIRE(result1);
    BOOST_REQUIRE(result1.transaction().hash() == hash1);

    // tx2: spends coinbase, tx1 as input, dummy output
    const chain::input::list tx2_inputs
    {
        { { hash1, 0 }, {}, 0 }
    };

    const chain::output::list tx2_outputs
    {
        { 1200, {} }
    };

    const chain::transaction tx2(version, locktime, tx2_inputs, tx2_outputs);
    const auto hash2 = tx2.hash();
    instance.store(tx2, 1);

    BOOST_REQUIRE(!result1.candidate());
    BOOST_REQUIRE(!result1.transaction().outputs().front().metadata.candidate_spend);

    // Setup end

    const bool result = instance.uncandidate(instance.get(hash2).link());
    BOOST_REQUIRE(result);

    BOOST_REQUIRE(!instance.get(hash2).candidate());

    const auto tx1_reloaded = instance.get(hash1);
    BOOST_REQUIRE(!tx1_reloaded.candidate());
    BOOST_REQUIRE(!tx1_reloaded.transaction().outputs().front().metadata.candidate_spend);
}

// Confirm
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(transaction_database__confirm2__single_transaction_not_in_db__failure)
{
    transaction tx1;
    data_chunk wire_tx1;
    BOOST_REQUIRE(decode_base16(wire_tx1, TRANSACTION1));
    BOOST_REQUIRE(tx1.from_data(wire_tx1));

    test::create(file_path);
    transaction_database instance(file_path, 1, 1000, 50, 0);
    BOOST_REQUIRE(instance.create());

    const auto hash1 = tx1.hash();
    BOOST_REQUIRE(!instance.get(hash1));
    instance.store({tx1});

    // Setup end

    const bool result = instance.confirm(instance.get(hash1).link(), 123, 456, 789);
    BOOST_REQUIRE(!result);

    const auto tx1_reloaded = instance.get(hash1);

    BOOST_REQUIRE_EQUAL(tx1_reloaded.height(), machine::rule_fork::unverified);
    BOOST_REQUIRE_EQUAL(tx1_reloaded.median_time_past(), 0u);
    BOOST_REQUIRE_EQUAL(tx1_reloaded.position(), transaction_result::unconfirmed);
    BOOST_REQUIRE(!tx1_reloaded.candidate());
}

BOOST_AUTO_TEST_CASE(transaction_database__confirm2__single_transaction_with_inputs_in_db__success)
{
    uint32_t version = 2345u;
    uint32_t locktime = 0xffffffff;

    test::create(file_path);
    transaction_database instance(file_path, 1, 1000, 50, 0);
    BOOST_REQUIRE(instance.create());

    // tx1: coinbase transaction
    const chain::input::list tx1_inputs
    {
        { chain::point{ null_hash, chain::point::null_index }, {}, 0 }
    };

    const chain::output::list tx1_outputs
    {
        { 1200, {} }
    };

    const chain::transaction tx1(version, locktime, tx1_inputs, tx1_outputs);
    BOOST_REQUIRE(tx1.is_coinbase());
    const auto hash1 = tx1.hash();
    instance.store({tx1});

    // tx2: spends coinbase, tx1 as input, dummy output
    const chain::input::list tx2_inputs
    {
        { { hash1, 0 }, {}, 0 }
    };

    const chain::output::list tx2_outputs
    {
        { 1200, {} }
    };

    const chain::transaction tx2(version, locktime, tx2_inputs, tx2_outputs);
    const auto hash2 = tx2.hash();
    instance.store(tx2, 1);

    BOOST_REQUIRE(!instance.get(hash2).transaction().outputs().front().metadata.candidate_spend);

    BOOST_REQUIRE_EQUAL(instance.get(hash1).height(), machine::rule_fork::unverified);
    BOOST_REQUIRE(!instance.get(hash1).transaction().outputs().front().metadata.spent(123, false));

    // Setup end

    const bool confirm1 = instance.confirm(instance.get(hash1).link(), 23, 56, 89);
    BOOST_REQUIRE(confirm1);
    const bool confirm2 = instance.confirm(instance.get(hash2).link(), 123, 456, 789);
    BOOST_REQUIRE(confirm2);

    const auto tx2_reloaded = instance.get(hash2);

    BOOST_REQUIRE_EQUAL(tx2_reloaded.height(), 123);
    BOOST_REQUIRE_EQUAL(tx2_reloaded.median_time_past(), 456);
    BOOST_REQUIRE_EQUAL(tx2_reloaded.position(), 789);
    BOOST_REQUIRE(!tx2_reloaded.candidate());
    BOOST_REQUIRE(!tx2_reloaded.transaction().outputs().front().metadata.candidate_spend);

    const auto tx1_reloaded = instance.get(hash1);

    BOOST_REQUIRE_EQUAL(tx1_reloaded.height(), 23);
    BOOST_REQUIRE_EQUAL(tx1_reloaded.median_time_past(), 56);
    BOOST_REQUIRE_EQUAL(tx1_reloaded.position(), 89);
    BOOST_REQUIRE(!tx1_reloaded.candidate());

    BOOST_REQUIRE_EQUAL(tx1_reloaded.transaction().outputs().front().metadata.confirmed_spend_height, 123);
    BOOST_REQUIRE(tx1_reloaded.transaction().outputs().front().metadata.spent(123, false));
}

BOOST_AUTO_TEST_CASE(transaction_database_with_cache__confirm2__single_transaction_with_inputs_in_db__success)
{
    uint32_t version = 2345u;
    uint32_t locktime = 0xffffffff;

    test::create(file_path);
    transaction_database instance(file_path, 1, 1000, 50, 100);
    BOOST_REQUIRE(instance.create());

    // tx1: coinbase transaction
    const chain::input::list tx1_inputs
    {
        { chain::point{ null_hash, chain::point::null_index }, {}, 0 }
    };

    const chain::output::list tx1_outputs
    {
        { 1200, {} }
    };

    const chain::transaction tx1(version, locktime, tx1_inputs, tx1_outputs);
    BOOST_REQUIRE(tx1.is_coinbase());
    const auto hash1 = tx1.hash();
    instance.store({tx1});

    // tx2: spends coinbase, tx1 as input, dummy output
    chain::input::list tx2_inputs
    {
        { { hash1, 0 }, {}, 0 }
    };

    const chain::output::list tx2_outputs
    {
        { 1200, {} }
    };

    const chain::transaction tx2(version, locktime, tx2_inputs, tx2_outputs);
    const auto hash2 = tx2.hash();
    instance.store(tx2, 1);

    BOOST_REQUIRE(!instance.get(hash2).transaction().outputs().front().metadata.candidate_spend);

    BOOST_REQUIRE_EQUAL(instance.get(hash1).height(), machine::rule_fork::unverified);
    BOOST_REQUIRE(!instance.get(hash1).transaction().outputs().front().metadata.spent(123, false));

    // Setup end

    const bool confirm1 = instance.confirm(instance.get(hash1).link(), 23, 56, 89);
    BOOST_REQUIRE(confirm1);
    const bool confirm2 = instance.confirm(instance.get(hash2).link(), 123, 456, 789);
    BOOST_REQUIRE(confirm2);

    const auto tx2_reloaded = instance.get(hash2);

    BOOST_REQUIRE_EQUAL(tx2_reloaded.height(), 123);
    BOOST_REQUIRE_EQUAL(tx2_reloaded.median_time_past(), 456);
    BOOST_REQUIRE_EQUAL(tx2_reloaded.position(), 789);
    BOOST_REQUIRE(!tx2_reloaded.candidate());
    BOOST_REQUIRE(!tx2_reloaded.transaction().outputs().front().metadata.candidate_spend);

    const auto tx1_reloaded = instance.get(hash1);

    BOOST_REQUIRE_EQUAL(tx1_reloaded.height(), 23);
    BOOST_REQUIRE_EQUAL(tx1_reloaded.median_time_past(), 56);
    BOOST_REQUIRE_EQUAL(tx1_reloaded.position(), 89);
    BOOST_REQUIRE(!tx1_reloaded.candidate());

    BOOST_REQUIRE_EQUAL(tx1_reloaded.transaction().outputs().front().metadata.confirmed_spend_height, 123);
    BOOST_REQUIRE(tx1_reloaded.transaction().outputs().front().metadata.spent(123, false));
}

BOOST_AUTO_TEST_CASE(transaction_database_with_cache__confirm1__block_transactions_with_inputs_in_db__success)
{
   uint32_t version = 2345u;
   uint32_t locktime = 0xffffffff;

   test::create(file_path);
   transaction_database instance(file_path, 1, 1000, 50, 100);
   BOOST_REQUIRE(instance.create());

   // tx1: coinbase transaction
   const chain::input::list tx1_inputs
   {
       { chain::point{ null_hash, chain::point::null_index }, {}, 0 }
   };

   const chain::output::list tx1_outputs
   {
       { 1200, {} }
   };

   const chain::transaction tx1(version, locktime, tx1_inputs, tx1_outputs);
   BOOST_REQUIRE(tx1.is_coinbase());
   const auto hash1 = tx1.hash();
   instance.store({tx1});

   // tx2: coinbase for block0
   const chain::input::list tx2_inputs
   {
       { chain::point{ null_hash, chain::point::null_index }, {}, 0 }
   };

   const chain::output::list tx2_outputs
   {
       { 1201, {} }
   };

   // tx3: spends coinbase, tx1/1 as input, dummy output
   const chain::input::list tx3_inputs
   {
       { { hash1, 0 }, {}, 0 }
   };

   chain::output::list tx3_outputs
   {
       { 1200, {} }
   };

   const chain::transaction tx2(version, locktime, tx2_inputs, tx2_outputs);
   const auto hash2 = tx2.hash();
   instance.store(tx2, 1);

   const chain::transaction tx3(version, locktime, tx3_inputs, tx3_outputs);
   const auto hash3 = tx3.hash();
   instance.store(tx3, 1);

   BOOST_REQUIRE(!instance.get(hash2).transaction().outputs().front().metadata.candidate_spend);

   BOOST_REQUIRE_EQUAL(instance.get(hash1).height(), machine::rule_fork::unverified);
   BOOST_REQUIRE(!instance.get(hash1).transaction().outputs().front().metadata.spent(123, false));

    const auto settings = system::settings(system::config::settings::mainnet);
    chain::block block0 = settings.genesis_block;
    block0.set_transactions({ tx2, tx3 });

    // Setup end

   const bool confirm1 = instance.confirm(instance.get(hash1).link(), 23, 56, 89);
   BOOST_REQUIRE(confirm1);

   const auto tx1_reloaded = instance.get(hash1);

   BOOST_REQUIRE_EQUAL(tx1_reloaded.height(), 23);
   BOOST_REQUIRE_EQUAL(tx1_reloaded.median_time_past(), 56);
   BOOST_REQUIRE_EQUAL(tx1_reloaded.position(), 89);
   BOOST_REQUIRE(!tx1_reloaded.candidate());

   const bool confirm2_3 = instance.confirm(block0, 123, 456);
   BOOST_REQUIRE(confirm2_3);

   const auto tx2_reloaded = instance.get(hash2);

   BOOST_REQUIRE_EQUAL(tx2_reloaded.height(), 123);
   BOOST_REQUIRE_EQUAL(tx2_reloaded.median_time_past(), 456);
   BOOST_REQUIRE_EQUAL(tx2_reloaded.position(), 0);
   BOOST_REQUIRE(!tx2_reloaded.candidate());
   BOOST_REQUIRE(!tx2_reloaded.transaction().outputs().front().metadata.candidate_spend);

   const auto tx3_reloaded = instance.get(hash3);

   BOOST_REQUIRE_EQUAL(tx3_reloaded.height(), 123);
   BOOST_REQUIRE_EQUAL(tx3_reloaded.median_time_past(), 456);
   BOOST_REQUIRE_EQUAL(tx3_reloaded.position(), 1);
   BOOST_REQUIRE(!tx3_reloaded.candidate());
   BOOST_REQUIRE(!tx3_reloaded.transaction().outputs().front().metadata.candidate_spend);

   const auto tx1_reloaded2 = instance.get(hash1);

   BOOST_REQUIRE_EQUAL(tx1_reloaded2.height(), 23);
   BOOST_REQUIRE_EQUAL(tx1_reloaded2.median_time_past(), 56);
   BOOST_REQUIRE_EQUAL(tx1_reloaded2.position(), 89);
   BOOST_REQUIRE(!tx1_reloaded2.candidate());

   BOOST_REQUIRE_EQUAL(tx1_reloaded2.output(0).metadata.confirmed_spend_height, 123);
   
   BOOST_REQUIRE(tx1_reloaded2.transaction().outputs().front().metadata.spent(123, false));
   BOOST_REQUIRE(tx1_reloaded2.is_spent(123, false));
}

// Unconfirm
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(transaction_database__unconfirm__block_with_unconfirmed_txs__success)
{
   uint32_t version = 2345u;
   uint32_t locktime = 0xffffffff;

   test::create(file_path);
   transaction_database instance(file_path, 1, 1000, 50, 0);
   BOOST_REQUIRE(instance.create());

   // tx1: coinbase transaction
   const chain::input::list tx1_inputs
   {
       { chain::point{ null_hash, chain::point::null_index }, {}, 0 }
   };

   const chain::output::list tx1_outputs
   {
       { 1200, {} }
   };

   const chain::transaction tx1(version, locktime, tx1_inputs, tx1_outputs);
   BOOST_REQUIRE(tx1.is_coinbase());
   const auto hash1 = tx1.hash();
   instance.store(tx1, 1);

   // tx2: spends coinbase, tx1 as input, dummy output
   const chain::input::list tx2_inputs
   {
       { { hash1, 0 }, {}, 0 }
   };

   const chain::output::list tx2_outputs
   {
       { 1200, {} }
   };

   const chain::transaction tx2(version, locktime, tx2_inputs, tx2_outputs);
   instance.store(tx2, 1);

   const auto settings = system::settings(system::config::settings::mainnet);
   chain::block block0 = settings.genesis_block;
   block0.set_transactions({ tx1, tx2 });
   
   // Setup end

   const bool result = instance.unconfirm(block0);
   BOOST_REQUIRE(!result);
}

BOOST_AUTO_TEST_CASE(transaction_database__unconfirm__single_confirmed__success)
{
   uint32_t version = 2345u;
   uint32_t locktime = 0xffffffff;

   test::create(file_path);
   transaction_database instance(file_path, 1, 1000, 50, 0);
   BOOST_REQUIRE(instance.create());

   // tx1: coinbase transaction
   const chain::input::list tx1_inputs
   {
       { chain::point{ null_hash, chain::point::null_index }, {}, 0 }
   };

   const chain::output::list tx1_outputs
   {
       { 1200, {} }
   };

   chain::transaction tx1(version, locktime, tx1_inputs, tx1_outputs);
   BOOST_REQUIRE(tx1.is_coinbase());
   const auto hash1 = tx1.hash();
   instance.store(tx1, 1);

   // tx2: spends coinbase, tx1 as input, dummy output
   const chain::input::list tx2_inputs
   {
       { { hash1, 0 }, {}, 0 }
   };

   const chain::output::list tx2_outputs
   {
       { 1200, {} }
   };

   const chain::transaction tx2(version, locktime, tx2_inputs, tx2_outputs);
   const auto hash2 = tx2.hash();
   instance.store(tx2, 1);

   instance.confirm(instance.get(tx1.hash()).link(), 23, 56, 1);
   instance.confirm(instance.get(tx2.hash()).link(), 123, 156, 1);

   BOOST_REQUIRE_EQUAL(instance.get(hash1).transaction().outputs().front().metadata.confirmed_spend_height, 123);
   BOOST_REQUIRE(instance.get(hash1).transaction().outputs().front().metadata.spent(123, false));

   const auto settings = system::settings(system::config::settings::mainnet);
   chain::block block0 = settings.genesis_block;
   block0.set_transactions({tx2});
   
   // Setup end

   const bool result = instance.unconfirm(block0);
   BOOST_REQUIRE(result);

   const auto tx2_reloaded = instance.get(hash2);

   BOOST_REQUIRE_EQUAL(tx2_reloaded.height(), machine::rule_fork::unverified);
   BOOST_REQUIRE_EQUAL(tx2_reloaded.median_time_past(), 0u);
   BOOST_REQUIRE_EQUAL(tx2_reloaded.position(), transaction_result::unconfirmed);
   BOOST_REQUIRE(!tx2_reloaded.candidate());
   BOOST_REQUIRE(!tx2_reloaded.transaction().outputs().front().metadata.candidate_spend);

   const auto tx1_reloaded = instance.get(hash1);

   BOOST_REQUIRE_EQUAL(tx1_reloaded.height(), 23);
   BOOST_REQUIRE_EQUAL(tx1_reloaded.median_time_past(), 56);
   BOOST_REQUIRE_EQUAL(tx1_reloaded.position(), 1);
   BOOST_REQUIRE(!tx1_reloaded.candidate());
   BOOST_REQUIRE(!tx1_reloaded.transaction().outputs().front().metadata.candidate_spend);


   BOOST_REQUIRE_EQUAL(tx1_reloaded.transaction().outputs().front().metadata.confirmed_spend_height, machine::rule_fork::unverified);
   BOOST_REQUIRE(!tx1_reloaded.transaction().outputs().front().metadata.spent(123, false));
}

BOOST_AUTO_TEST_CASE(transaction_database_with_cache__unconfirm__single_confirmed__success)
{
   uint32_t version = 2345u;
   uint32_t locktime = 0xffffffff;

   test::create(file_path);
   transaction_database instance(file_path, 1, 1000, 50, 100);
   BOOST_REQUIRE(instance.create());

   // tx1: coinbase transaction
   const chain::input::list tx1_inputs
   {
       { chain::point{ null_hash, chain::point::null_index }, {}, 0 }
   };

   const chain::output::list tx1_outputs
   {
       { 1200, {} }
   };

   const chain::transaction tx1(version, locktime, tx1_inputs, tx1_outputs);
   BOOST_REQUIRE(tx1.is_coinbase());
   const auto hash1 = tx1.hash();
   instance.store(tx1, 1);

   // tx2: spends coinbase, tx1 as input, dummy output
   const chain::input::list tx2_inputs
   {
       { { hash1, 0 }, {}, 0 }
   };

   const chain::output::list tx2_outputs
   {
       { 1200, {} }
   };

   const chain::transaction tx2(version, locktime, tx2_inputs, tx2_outputs);
   const auto hash2 = tx2.hash();
   instance.store(tx2, 1);

   instance.confirm(instance.get(tx1.hash()).link(), 23, 56, 1);
   instance.confirm(instance.get(tx2.hash()).link(), 123, 156, 1);

   BOOST_REQUIRE_EQUAL(instance.get(hash1).transaction().outputs().front().metadata.confirmed_spend_height, 123);
   BOOST_REQUIRE(instance.get(hash1).transaction().outputs().front().metadata.spent(123, false));

   const auto settings = system::settings(system::config::settings::mainnet);
   chain::block block0 = settings.genesis_block;
   block0.set_transactions({tx2});

   // Setup end

   const bool result = instance.unconfirm(block0);
   BOOST_REQUIRE(result);

   const auto tx2_reloaded = instance.get(hash2);

   BOOST_REQUIRE_EQUAL(tx2_reloaded.height(), machine::rule_fork::unverified);
   BOOST_REQUIRE_EQUAL(tx2_reloaded.median_time_past(), 0u);
   BOOST_REQUIRE_EQUAL(tx2_reloaded.position(), transaction_result::unconfirmed);
   BOOST_REQUIRE(!tx2_reloaded.candidate());
   BOOST_REQUIRE(!tx2_reloaded.transaction().outputs().front().metadata.candidate_spend);

   const auto tx1_reloaded = instance.get(hash1);

   BOOST_REQUIRE_EQUAL(tx1_reloaded.height(), 23);
   BOOST_REQUIRE_EQUAL(tx1_reloaded.median_time_past(), 56);
   BOOST_REQUIRE_EQUAL(tx1_reloaded.position(), 1);
   BOOST_REQUIRE(!tx1_reloaded.candidate());
   BOOST_REQUIRE(!tx1_reloaded.transaction().outputs().front().metadata.candidate_spend);


   BOOST_REQUIRE_EQUAL(tx1_reloaded.transaction().outputs().front().metadata.confirmed_spend_height, machine::rule_fork::unverified);
   BOOST_REQUIRE(!tx1_reloaded.transaction().outputs().front().metadata.spent(123, false));
}


// Queries - get_block_metadata
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(transaction_database__get_block_metadata__nonexisting__not_found)
{
    uint32_t version = 2345u;
    uint32_t locktime = 0xffffffff;

    test::create(file_path);
    transaction_database instance(file_path, 1, 1000, 50, 0);
    BOOST_REQUIRE(instance.create());

    const transaction tx(version, locktime, {}, {});

    // setup end

    instance.get_block_metadata(tx, 1, 1);

    BOOST_REQUIRE(!tx.metadata.existed);
    BOOST_REQUIRE(!tx.metadata.candidate);
    BOOST_REQUIRE_EQUAL(tx.metadata.link, transaction::validation::unlinked);
    BOOST_REQUIRE(!tx.metadata.verified);
}

BOOST_AUTO_TEST_CASE(transaction_database__get_block_metadata__no_bip34_and_spent__not_found)
{
    uint32_t version = 2345u;
    uint32_t locktime = 0xffffffff;

    test::create(file_path);
    transaction_database instance(file_path, 1, 1000, 50, 0);
    BOOST_REQUIRE(instance.create());

    const transaction tx1{ locktime, version, {}, { { 1200, {} } } };
    const transaction tx2{ locktime, version, { { { tx1.hash(), 0 }, {}, 0 } }, { { 1100, {} } } };

    instance.store({ tx1, tx2 });
    instance.confirm(instance.get(tx1.hash()).link(), 123, 156, 178);
    instance.confirm(instance.get(tx2.hash()).link(), 1230, 1560, 1780);

    // setup end

    instance.get_block_metadata(tx1, 1, 1230);

    BOOST_REQUIRE(!tx1.metadata.existed);
    BOOST_REQUIRE(!tx1.metadata.candidate);
    BOOST_REQUIRE_EQUAL(tx1.metadata.link, transaction::validation::unlinked);
    BOOST_REQUIRE(!tx1.metadata.verified);
}

BOOST_AUTO_TEST_CASE(transaction_database__get_block_metadata__no_bip34_and_not_spent__found)
{
    uint32_t version = 2345u;
    uint32_t locktime = 0xffffffff;

    test::create(file_path);
    transaction_database instance(file_path, 1, 1000, 50, 0);
    BOOST_REQUIRE(instance.create());

    const transaction tx1{ locktime, version, {}, { { 1200, {} } } };

    instance.store(tx1, 100);

    // setup end

    instance.get_block_metadata(tx1, 1, 100);

    BOOST_REQUIRE(tx1.metadata.existed);
    BOOST_REQUIRE(!tx1.metadata.candidate);
    BOOST_REQUIRE_EQUAL(tx1.metadata.link, instance.get(tx1.hash()).link());
    BOOST_REQUIRE(!tx1.metadata.verified);
}

BOOST_AUTO_TEST_CASE(transaction_database__get_block_metadata__confirmed_and_fork_height_lt_height__confirmed)
{
    uint32_t version = 2345u;
    uint32_t locktime = 0xffffffff;

    test::create(file_path);
    transaction_database instance(file_path, 1, 1000, 50, 0);
    BOOST_REQUIRE(instance.create());

    const transaction tx1{ locktime, version, {}, { { 1200, {} } } };

    instance.store(tx1, 100);

    // setup end

    instance.get_block_metadata(tx1, 1, 99);

    BOOST_REQUIRE(tx1.metadata.existed);
    BOOST_REQUIRE(!tx1.metadata.candidate);
    BOOST_REQUIRE(!tx1.metadata.confirmed);
    BOOST_REQUIRE_EQUAL(tx1.metadata.link, instance.get(tx1.hash()).link());
    BOOST_REQUIRE(!tx1.metadata.verified);
}

BOOST_AUTO_TEST_CASE(transaction_database__get_block_metadata__confirmed__unverified)
{
    uint32_t version = 2345u;
    uint32_t locktime = 0xffffffff;

    test::create(file_path);
    transaction_database instance(file_path, 1, 1000, 50, 0);
    BOOST_REQUIRE(instance.create());

    const transaction tx1{ locktime, version, {}, { { 1200, {} } } };

    instance.store(tx1, 100);
    instance.confirm(instance.get(tx1.hash()).link(), 123, 156, 178);

    // setup end

    instance.get_block_metadata(tx1, 1, 123);

    BOOST_REQUIRE(tx1.metadata.existed);
    BOOST_REQUIRE(!tx1.metadata.candidate);
    BOOST_REQUIRE_EQUAL(tx1.metadata.link, instance.get(tx1.hash()).link());
    BOOST_REQUIRE(!tx1.metadata.verified);
}

// Queries - get_pool_metadata
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(transaction_database__get_pool_metadata__nonexisting__not_found)
{
    uint32_t version = 2345u;
    uint32_t locktime = 0xffffffff;

    test::create(file_path);
    transaction_database instance(file_path, 1, 1000, 50, 0);
    BOOST_REQUIRE(instance.create());

    const transaction tx(version, locktime, {}, {});

    // setup end

    instance.get_pool_metadata(tx, 1);

    BOOST_REQUIRE(!tx.metadata.existed);
    BOOST_REQUIRE(!tx.metadata.candidate);
    BOOST_REQUIRE_EQUAL(tx.metadata.link, transaction::validation::unlinked);
    BOOST_REQUIRE(!tx.metadata.verified);}

BOOST_AUTO_TEST_CASE(transaction_database__get_pool_metadata__confirmed__unverified)
{
    uint32_t version = 2345u;
    uint32_t locktime = 0xffffffff;

    test::create(file_path);
    transaction_database instance(file_path, 1, 1000, 50, 0);
    BOOST_REQUIRE(instance.create());

    const transaction tx1{ locktime, version, {}, { { 1200, {} } } };

    instance.store(tx1, 100);
    instance.confirm(instance.get(tx1.hash()).link(), 123, 156, 178);

    // setup end

    instance.get_pool_metadata(tx1, 1);

    BOOST_REQUIRE(tx1.metadata.existed);
    BOOST_REQUIRE(!tx1.metadata.candidate);
    BOOST_REQUIRE_EQUAL(tx1.metadata.link, instance.get(tx1.hash()).link());
    BOOST_REQUIRE(!tx1.metadata.verified);
}

// Queries - get_output
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(transaction_database__get_output__null_point__false)
{
    test::create(file_path);
    transaction_database instance(file_path, 1, 1000, 50, 0);
    BOOST_REQUIRE(instance.create());

    // setup end

    BOOST_REQUIRE(!instance.get_output({ null_hash, point::null_index }, 1, false));
}

BOOST_AUTO_TEST_CASE(transaction_database__get_output__not_found__false)
{
    uint32_t version = 2345u;
    uint32_t locktime = 0xffffffff;

    test::create(file_path);
    transaction_database instance(file_path, 1, 1000, 50, 0);
    BOOST_REQUIRE(instance.create());

    const transaction tx1(version, locktime, {}, {});

    // setup end

    BOOST_REQUIRE(!instance.get_output({ tx1.hash(), 0 }, 1, false));
}


BOOST_AUTO_TEST_CASE(transaction_database__get_output__genesis__false)
{
    uint32_t version = 2345u;
    uint32_t locktime = 0xffffffff;

    test::create(file_path);
    transaction_database instance(file_path, 1, 1000, 50, 0);
    BOOST_REQUIRE(instance.create());

    const transaction tx1(version, locktime, {}, {});
    instance.store(tx1, 0);

    // setup end

    BOOST_REQUIRE(!instance.get_output({ tx1.hash(), 0 }, 1, false));
}

BOOST_AUTO_TEST_CASE(transaction_database__get_output__no_output_at_index__false)
{
    uint32_t version = 2345u;
    uint32_t locktime = 0xffffffff;

    test::create(file_path);
    transaction_database instance(file_path, 1, 1000, 50, 0);
    BOOST_REQUIRE(instance.create());

    const transaction tx1{ locktime, version, {}, { { 1200, {} } } };

    instance.store(tx1, 100);

    BOOST_REQUIRE(!instance.get_output({ tx1.hash(), 1 }, 1, false));
}

BOOST_AUTO_TEST_CASE(transaction_database__get_output__unconfirmed_at_fork_height__unconfirmed)
{
    uint32_t version = 2345u;
    uint32_t locktime = 0xffffffff;

    test::create(file_path);
    transaction_database instance(file_path, 1, 1000, 50, 0);
    BOOST_REQUIRE(instance.create());

    const transaction tx1{ locktime, version, {}, { { 1200, {} } } };

    instance.store(tx1, 100);

    output_point point{ tx1.hash(), 0 };

    BOOST_REQUIRE(instance.get_output(point, 101, false));
    BOOST_REQUIRE(!point.metadata.coinbase);
    BOOST_REQUIRE(!point.metadata.candidate);
    BOOST_REQUIRE(!point.metadata.confirmed);
    BOOST_REQUIRE_EQUAL(point.metadata.height, 100);
    BOOST_REQUIRE_EQUAL(point.metadata.median_time_past, 0);
    BOOST_REQUIRE(!point.metadata.spent);
}

BOOST_AUTO_TEST_CASE(transaction_database__get_output__confirmed_at_height__confirmed)
{
    uint32_t version = 2345u;
    uint32_t locktime = 0xffffffff;

    test::create(file_path);
    transaction_database instance(file_path, 1, 1000, 50, 0);
    BOOST_REQUIRE(instance.create());

    const transaction tx1{ locktime, version, {}, { { 1201, {} } } };

    instance.store({ tx1 });
    const auto settings = system::settings(system::config::settings::mainnet);
    chain::block block0 = settings.genesis_block;

    // get transaction so confirm argument has the link
    block0.set_transactions({ instance.get(tx1.hash()).transaction() });
    instance.confirm(block0, 123, 456);

    output_point point{ tx1.hash(), 0 };

    BOOST_REQUIRE(instance.get_output(point, 123, false));
    BOOST_REQUIRE(point.metadata.coinbase);
    BOOST_REQUIRE(!point.metadata.candidate);
    BOOST_REQUIRE(point.metadata.confirmed);
    BOOST_REQUIRE_EQUAL(point.metadata.height, 123);
    BOOST_REQUIRE_EQUAL(point.metadata.median_time_past, 456);
    BOOST_REQUIRE(!point.metadata.spent);
}

BOOST_AUTO_TEST_CASE(transaction_database_with_cache__get_output__confirmed_at_height__confirmed)
{
   uint32_t version = 2345u;
   uint32_t locktime = 0xffffffff;

   test::create(file_path);
   transaction_database instance(file_path, 1, 1000, 50, 100);
   BOOST_REQUIRE(instance.create());

   const transaction tx1{ locktime, version, {}, { { 1201, {} } } };

   instance.store({tx1});
   const auto settings = system::settings(system::config::settings::mainnet);
   chain::block block0 = settings.genesis_block;

   // get transaction so confirm argument has the link
   block0.set_transactions({ instance.get(tx1.hash()).transaction() });
   instance.confirm(block0, 123, 456);

   output_point point{ tx1.hash(), 0 };

   BOOST_REQUIRE(instance.get_output(point, 123, false));
   BOOST_REQUIRE(!point.metadata.coinbase);
   BOOST_REQUIRE(!point.metadata.candidate);
   BOOST_REQUIRE(point.metadata.confirmed);
   BOOST_REQUIRE_EQUAL(point.metadata.height, 123);
   BOOST_REQUIRE_EQUAL(point.metadata.median_time_past, 456);
   BOOST_REQUIRE(!point.metadata.spent);
}

BOOST_AUTO_TEST_CASE(transaction_database__get_output__unconfirmed_at_height__unconfirmed)
{
    uint32_t version = 2345u;
    uint32_t locktime = 0xffffffff;

    test::create(file_path);
    transaction_database instance(file_path, 1, 1000, 50, 0);
    BOOST_REQUIRE(instance.create());

    // tx1 is not confirmed as it is at coinbase position, so we test
    // with tx2
    const transaction tx1{ locktime, version, {}, { { 1201, {} } } };
    const transaction tx2{ locktime, version, {}, { { 1202, {} } } };

    instance.store({tx1, tx2});
    const auto settings = system::settings(system::config::settings::mainnet);
    chain::block block0 = settings.genesis_block;

    // get transaction so confirm argument has the link
    block0.set_transactions({ instance.get(tx1.hash()).transaction(), instance.get(tx2.hash()).transaction() });
    instance.confirm(block0, 123, 456);

    output_point point{ tx2.hash(), 0 };

    BOOST_REQUIRE(instance.get_output(point, 100, false));
    BOOST_REQUIRE(!point.metadata.coinbase);
    BOOST_REQUIRE(!point.metadata.candidate);
    BOOST_REQUIRE(!point.metadata.confirmed);
    BOOST_REQUIRE_EQUAL(point.metadata.height, 123);
    BOOST_REQUIRE_EQUAL(point.metadata.median_time_past, 456);
    BOOST_REQUIRE(!point.metadata.spent);
}

BOOST_AUTO_TEST_CASE(transaction_database__get_output__prevout_spent__spent)
{
    uint32_t version = 2345u;
    uint32_t locktime = 0xffffffff;

    test::create(file_path);
    transaction_database instance(file_path, 1, 1000, 50, 0);
    BOOST_REQUIRE(instance.create());

    const transaction tx1{ locktime, version, {}, { { 1201, {} } } };
    const transaction tx2{ locktime, version, {}, { { 1202, {} } } };
    const transaction tx3{ locktime, version, { { { tx1.hash(), 0 }, {}, 0 } }, { { 1100, {} } } };

    instance.store({ tx1, tx2, tx3 });
    const auto settings = system::settings(system::config::settings::mainnet);
    chain::block block0 = settings.genesis_block;

    // get transaction so confirm argument has the link
    block0.set_transactions({ instance.get(tx1.hash()).transaction() });
    instance.confirm(block0, 123, 456);

    auto header1 = block0.header();
    header1.set_nonce(4);
    block block1(header1, { instance.get(tx2.hash()).transaction(), instance.get(tx3.hash()).transaction() });
    instance.confirm(block1, 1230, 4560);

    output_point point{ tx1.hash(), 0 };

    BOOST_REQUIRE(instance.get_output(point, 1229, false));
    BOOST_REQUIRE(point.metadata.coinbase);
    BOOST_REQUIRE(!point.metadata.candidate);
    BOOST_REQUIRE(point.metadata.confirmed);
    BOOST_REQUIRE_EQUAL(point.metadata.height, 123);
    BOOST_REQUIRE_EQUAL(point.metadata.median_time_past, 456);
    BOOST_REQUIRE(!point.metadata.spent);

    BOOST_REQUIRE(instance.get_output(point, 1230, false));
    BOOST_REQUIRE(point.metadata.coinbase);
    BOOST_REQUIRE(!point.metadata.candidate);
    BOOST_REQUIRE(point.metadata.confirmed);
    BOOST_REQUIRE_EQUAL(point.metadata.height, 123);
    BOOST_REQUIRE_EQUAL(point.metadata.median_time_past, 456);
    BOOST_REQUIRE(point.metadata.spent);
}

BOOST_AUTO_TEST_SUITE_END()
