/**
 * Copyright (c) 2011-2019 libbitcoin developers (see AUTHORS)
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

#define DIRECTORY "payment_database"

#define OUTPUT_SCRIPT0 "dup hash160 [58350574280395ad2c3e2ee20e322073d94e5e40] equalverify checksig"
#define OUTPUT_SCRIPT1 "dup hash160 [68350574280395ad2c3e2ee20e322073d94e5e40] equalverify checksig"
#define OUTPUT_SCRIPT2 "dup hash160 [78350574280395ad2c3e2ee20e322073d94e5e40] equalverify checksig"
#define OUTPUT_SCRIPT3 "dup hash160 [88350574280395ad2c3e2ee20e322073d94e5e40] equalverify checksig"

#define INPUT_SCRIPT1 "ece424a6bb6ddf4db592c0faed60685047a361b1"

static constexpr auto lookup_filename = DIRECTORY "/payment_lookup_file";
static constexpr auto rows_filename = DIRECTORY "/payment_rows_file";

static chain_state::data data_for_chain_state()
{
    chain_state::data value;
    value.height = 1;
    value.bits = { 0, { 0 } };
    value.version = { 1, { 0 } };
    value.timestamp = { 0, 0, { 0 } };
    return value;
}

static void set_state(transaction& tx)
{
    const auto state = std::make_shared<chain_state>(
        chain_state{ data_for_chain_state(), {}, 0, 0, {} });
    tx.metadata.state = state;
}

struct payment_database_directory_setup_fixture
{
    payment_database_directory_setup_fixture()
    {
        test::clear_path(DIRECTORY);
    }

    ~payment_database_directory_setup_fixture()
    {
        test::clear_path(DIRECTORY);
    }
};

BOOST_FIXTURE_TEST_SUITE(payment_database_tests, payment_database_directory_setup_fixture)

class payment_database_accessor
  : public payment_database
{
public:
    payment_database_accessor(const path& lookup_filename,
        const path& rows_filename, size_t table_minimum, size_t index_minimum,
        uint32_t buckets, size_t expansion)
      : payment_database(lookup_filename, rows_filename, table_minimum,
          index_minimum, buckets, expansion)
    {
    }

    void store(const system::hash_digest& hash,
        const system::chain::point& point, size_t height, uint64_t value,
        bool input)
    {
        payment_database::store(hash, point, height, value, input);
    }

    void catalog(const system::chain::transaction& tx)
    {
        payment_database::catalog(tx);        
    }
};

BOOST_AUTO_TEST_CASE(payment_database__store__two_inputs_two_outputs_two_transactions__success)
{
    test::create(lookup_filename);
    test::create(rows_filename);
    payment_database_accessor instance(lookup_filename, rows_filename, 10, 10, 1000, 50);
    BOOST_REQUIRE(instance.create());

    const hash_digest tx0_hash = sha256_hash(to_chunk("tx0_hash"));
    const hash_digest tx1_hash = sha256_hash(to_chunk("tx1_hash"));

    script prevout_script0;
    prevout_script0.from_string(OUTPUT_SCRIPT0);
    const auto script_hash0 = sha256_hash(prevout_script0.to_data(false));

    script prevout_script1;
    prevout_script1.from_string(OUTPUT_SCRIPT1);
    const auto script_hash1 = sha256_hash(prevout_script1.to_data(false));

    const auto input0 = input_point{ tx0_hash, 7890 };
    const auto input1 = input_point{ tx1_hash, 7891 };

    const auto output0 = output_point{ tx1_hash, 456 };
    const auto output1 = output_point{ tx1_hash, 457 };

    // End of setup.

    instance.store(script_hash0, input0, 1234, 4321, false);
    instance.store(script_hash0, input1, 1235, 4321, false);
    instance.store(script_hash0, output0, 1235, 5321, true);
    instance.store(script_hash1, output1, 1235, 5321, true);

    // Test conditions.

    const auto result0 = instance.get(script_hash0);
    auto payments0 = result0.begin();

    const auto payment = *payments0;
    BOOST_REQUIRE(payment.is_output());
    BOOST_REQUIRE_EQUAL(payment.link(), 1235);
    BOOST_REQUIRE_EQUAL(payment.height(), payment_record::unconfirmed);
    BOOST_REQUIRE(payment.hash() == null_hash);
    BOOST_REQUIRE(payment.index() == 456);

    const auto payment1 = *(++payments0);
    BOOST_REQUIRE(!payment1.is_output());
    BOOST_REQUIRE_EQUAL(payment1.link(), 1235);
    BOOST_REQUIRE_EQUAL(payment1.height(), payment_record::unconfirmed);
    BOOST_REQUIRE(payment1.hash() == null_hash);
    BOOST_REQUIRE(payment1.index() == 7891);

    const auto payment2 = *(++payments0);
    BOOST_REQUIRE(!payment2.is_output());
    BOOST_REQUIRE_EQUAL(payment2.link(), 1234);
    BOOST_REQUIRE_EQUAL(payment2.height(), payment_record::unconfirmed);
    BOOST_REQUIRE(payment2.hash() == null_hash);
    BOOST_REQUIRE(payment2.index() == 7890);

    BOOST_REQUIRE(++payments0 == result0.end());

    const auto result1 = instance.get(script_hash1);
    auto payments1 = result1.begin();

    const auto payment3 = *payments1;
    BOOST_REQUIRE(payment3.is_output());
    BOOST_REQUIRE_EQUAL(payment3.link(), 1235);
    BOOST_REQUIRE_EQUAL(payment3.height(), payment_record::unconfirmed);
    BOOST_REQUIRE(payment3.hash() == null_hash);
    BOOST_REQUIRE(payment3.index() == 457);

    BOOST_REQUIRE(++payments0 == result0.end());
}

BOOST_AUTO_TEST_CASE(payment_database__catalog__coinbase_transaction__success)
{
    uint32_t version = 2345u;
    uint32_t locktime = 0xffffffff;

    test::create(lookup_filename);
    test::create(rows_filename);
    payment_database_accessor instance(lookup_filename, rows_filename, 10, 10, 1000, 50);
    BOOST_REQUIRE(instance.create());

    script script0;
    script0.from_string(OUTPUT_SCRIPT0);
    const auto script_hash0 = sha256_hash(script0.to_data(false));

    const chain::input::list inputs
    {
        { chain::point{ null_hash, chain::point::null_index }, {}, 0 }
    };

    const chain::output::list outputs
    {
        { 1200, script0 }
    };

    chain::transaction tx{ version, locktime, inputs, outputs };

    set_state(tx);
    tx.metadata.link = 100;

    // End of setup.

    instance.catalog(tx);

    // Test conditions.

    const auto result0 = instance.get(script_hash0);
    auto payments0 = result0.begin();

    const auto payment = *payments0;
    BOOST_REQUIRE(payment.is_output());
    BOOST_REQUIRE_EQUAL(payment.link(), 100);
    BOOST_REQUIRE_EQUAL(payment.height(), payment_record::unconfirmed);
    BOOST_REQUIRE(payment.hash() == null_hash);
    BOOST_REQUIRE(payment.index() == 0);

    BOOST_REQUIRE(++payments0 == result0.end());
}

BOOST_AUTO_TEST_CASE(payment_database__catalog__tx1_spends_from_tx0__success)
{
    uint32_t version = 2345u;
    uint32_t locktime = 0xffffffff;

    test::create(lookup_filename);
    test::create(rows_filename);
    payment_database_accessor instance(lookup_filename, rows_filename, 10, 10, 1000, 50);
    BOOST_REQUIRE(instance.create());

    script script0;
    script0.from_string(OUTPUT_SCRIPT0);
    BOOST_REQUIRE(script0.is_valid());
    const auto script_hash0 = sha256_hash(script0.to_data(false));

    script script1;
    script1.from_string(OUTPUT_SCRIPT1);
    const auto script_hash1 = sha256_hash(script1.to_data(false));
    BOOST_REQUIRE(script1.is_valid());

    script script2;
    script2.from_string(OUTPUT_SCRIPT2);
    const auto script_hash2 = sha256_hash(script2.to_data(false));
    BOOST_REQUIRE(script2.is_valid());

    script script3;
    script3.from_string(OUTPUT_SCRIPT3);
    const auto script_hash3 = sha256_hash(script3.to_data(false));
    BOOST_REQUIRE(script3.is_valid());

    // Transaction structure. Cryptic but might be helpful in future.
    // tx0{ inputs0[cb], outputs0[script0,script1,script2] }
    // tx1{ inputs1[outputs0.script0, outputs0.script2], outputs1[script1,script2,script3]}

    // Expected payments index is:
    // script0: tx0/outputs:0, tx1/inputs:0
    // script1: tx0/outputs:1, tx1/outputs:0
    // script2: tx0/outputs:2, tx1/inputs:1, tx1/outputs:1
    // script3: tx1/outputs:2

    // Setup first transaction.
    const chain::input::list inputs0
    {
        { chain::point{ null_hash, chain::point::null_index }, {}, 0 },
    };
    const chain::output::list outputs0
    {
        { 100, script0 },
        { 101, script1 },
        { 102, script2 },
    };

    // Setup metadata for first transaction.
    chain::transaction tx0{ version, locktime, inputs0, outputs0 };
    set_state(tx0);

    tx0.metadata.link = 1000;
    tx0.inputs().front().previous_output().metadata.cache.set_script(script0);

    // Setup second transaction.
    script input_script;
    const auto chunk = to_chunk(base16_literal(INPUT_SCRIPT1));
    BOOST_REQUIRE(input_script.from_data(chunk, false));
    BOOST_REQUIRE(input_script.is_valid());

    const chain::input::list inputs1
    {
        { chain::point{ tx0.hash(), 0 }, input_script, 0 },
        { chain::point{ tx0.hash(), 1 }, input_script, 0 },
    };
    const chain::output::list outputs1
    {
        { 200, script1 },
        { 201, script2 },
        { 202, script3 },
    };

    // Setup metadata second transaction.
    chain::transaction tx1{ version, locktime, inputs1, outputs1 };
    set_state(tx1);
    tx1.metadata.link = 2000;

    tx1.inputs()[0].previous_output().metadata.cache.set_script(script0);
    tx1.inputs()[0].previous_output().metadata.cache.set_value(100);
    BOOST_REQUIRE(tx1.inputs()[0].previous_output().metadata.cache.is_valid());

    tx1.inputs()[1].previous_output().metadata.cache.set_script(script2);
    tx1.inputs()[1].previous_output().metadata.cache.set_value(102);
    BOOST_REQUIRE(tx1.inputs()[1].previous_output().metadata.cache.is_valid());

    // End of setup.

    instance.catalog(tx0);
    instance.catalog(tx1);

    // Test conditions.

    // Verify script0: tx0/outputs:0, tx1/inputs:0
    const auto result0 = instance.get(script_hash0);
    auto payments0 = result0.begin();

    auto payment = *payments0;
    BOOST_REQUIRE(!payment.is_output());
    BOOST_REQUIRE_EQUAL(payment.link(), 2000);
    BOOST_REQUIRE_EQUAL(payment.height(), payment_record::unconfirmed);
    BOOST_REQUIRE(payment.hash() == null_hash);
    BOOST_REQUIRE(payment.index() == 0);

    payment = *(++payments0);
    BOOST_REQUIRE(payment.is_output());
    BOOST_REQUIRE_EQUAL(payment.link(), 1000);
    BOOST_REQUIRE_EQUAL(payment.height(), payment_record::unconfirmed);
    BOOST_REQUIRE(payment.hash() == null_hash);
    BOOST_REQUIRE(payment.index() == 0);

    BOOST_REQUIRE(++payments0 == result0.end());

    // Verify script1: tx0/outputs:1, tx1/outputs:0
    const auto result1 = instance.get(script_hash1);
    auto payments1 = result1.begin();

    payment = *payments1;
    BOOST_REQUIRE(payment.is_output());
    BOOST_REQUIRE_EQUAL(payment.link(), 2000);
    BOOST_REQUIRE_EQUAL(payment.height(), payment_record::unconfirmed);
    BOOST_REQUIRE(payment.hash() == null_hash);
    BOOST_REQUIRE(payment.index() == 0);

    payment = *(++payments1);
    BOOST_REQUIRE(payment.is_output());
    BOOST_REQUIRE_EQUAL(payment.link(), 1000);
    BOOST_REQUIRE_EQUAL(payment.height(), payment_record::unconfirmed);
    BOOST_REQUIRE(payment.hash() == null_hash);
    BOOST_REQUIRE(payment.index() == 1);

    BOOST_REQUIRE(++payments1 == result1.end());

    // Verify script2: tx0/outputs:2, tx1/inputs:1, tx1/outputs:1
    const auto result2 = instance.get(script_hash2);
    auto payments2 = result2.begin();

    payment = *payments2;
    BOOST_REQUIRE(payment.is_output());
    BOOST_REQUIRE_EQUAL(payment.link(), 2000);
    BOOST_REQUIRE_EQUAL(payment.height(), payment_record::unconfirmed);
    BOOST_REQUIRE(payment.hash() == null_hash);
    BOOST_REQUIRE(payment.index() == 1);

    payment = *(++payments2);
    BOOST_REQUIRE(!payment.is_output());
    BOOST_REQUIRE_EQUAL(payment.link(), 2000);
    BOOST_REQUIRE_EQUAL(payment.height(), payment_record::unconfirmed);
    BOOST_REQUIRE(payment.hash() == null_hash);
    BOOST_REQUIRE(payment.index() == 1);

    payment = *(++payments2);
    BOOST_REQUIRE(payment.is_output());
    BOOST_REQUIRE_EQUAL(payment.link(), 1000);
    BOOST_REQUIRE_EQUAL(payment.height(), payment_record::unconfirmed);
    BOOST_REQUIRE(payment.hash() == null_hash);
    BOOST_REQUIRE(payment.index() == 2);

    BOOST_REQUIRE(++payments2 == result2.end());

    // Verify script3: tx1/outputs:2
    const auto result3 = instance.get(script_hash3);
    auto payments3 = result3.begin();

    payment = *payments3;
    BOOST_REQUIRE(payment.is_output());
    BOOST_REQUIRE_EQUAL(payment.link(), 2000);
    BOOST_REQUIRE_EQUAL(payment.height(), payment_record::unconfirmed);
    BOOST_REQUIRE(payment.hash() == null_hash);
    BOOST_REQUIRE(payment.index() == 2);

    BOOST_REQUIRE(++payments3 == result3.end());
}

BOOST_AUTO_TEST_SUITE_END()
