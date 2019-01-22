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

#include <future>
#include <memory>
#include <boost/filesystem.hpp>
#include <bitcoin/database.hpp>
#include "utility/utility.hpp"

using namespace bc::system;
using namespace bc::system::chain;
using namespace bc::system::wallet;
using namespace bc::database;
using namespace boost::system;
using namespace boost::filesystem;

static void test_outputs_cataloged(const address_database& payments_store,
    const transaction& tx, bool is_found)
{
    for (size_t j = 0; j < tx.outputs().size(); ++j)
    {
        const auto& output = tx.outputs()[j];
        output_point outpoint{ tx.hash(), static_cast<uint32_t>(j) };
        const auto& script_hash = sha256_hash(output.script().to_data(false));
        auto payments = payments_store.get(script_hash);
        auto found = false;

        for (const payment_record& row: payments)
        {
            BOOST_REQUIRE(row.is_valid());

            if (row.link() == tx.metadata.link && row.index() == j)
            {
                BOOST_REQUIRE_EQUAL(row.data(), outpoint.checksum());
                found = true;
                break;
            }
        }
        if (is_found)
            BOOST_REQUIRE(found);
        else
            BOOST_REQUIRE(!found);
    }
}

static void test_inputs_cataloged(const address_database& payments_store,
    const transaction& tx, bool catalog, bool is_found)
{
    for (auto j = 0u; j < tx.inputs().size(); ++j)
    {
        const auto& input = tx.inputs()[j];
        input_point spend{ tx.hash(), j };
        BOOST_REQUIRE_EQUAL(spend.index(), j);

        if (!catalog)
            continue;

        const auto& prevout = input.previous_output();
        const auto& prevout_script = prevout.metadata.cache.script();
        const auto& script_hash = sha256_hash(prevout_script.to_data(false));

        auto payments = payments_store.get(script_hash);
        auto found = false;
        for (const payment_record& row: payments)
        {
            if (row.link() == tx.metadata.link && row.index() == j)
            {
                found = true;
                break;
            }
        }
        if (is_found)
            BOOST_REQUIRE(found);
        else
            BOOST_REQUIRE(!found);
    }
}

static void test_block_exists(const data_base& interface, size_t height,
    const block& block, bool catalog, bool candidate)
{
    const auto& payments_store = interface.addresses();
    const auto block_hash = block.hash();
    const auto result = interface.blocks().get(height, candidate);
    const auto result_by_hash = interface.blocks().get(block_hash);

    BOOST_REQUIRE(result);
    BOOST_REQUIRE(result_by_hash);
    BOOST_REQUIRE(result.hash() == block_hash);
    BOOST_REQUIRE(result_by_hash.hash() == block_hash);
    BOOST_REQUIRE_EQUAL(result.height(), height);
    BOOST_REQUIRE_EQUAL(result_by_hash.height(), height);
    BOOST_REQUIRE_EQUAL(result.transaction_count(), block.transactions().size());
    BOOST_REQUIRE_EQUAL(result_by_hash.transaction_count(), block.transactions().size());

    // TODO: test tx offsets (vs. tx hashes).

    for (size_t i = 0; i < block.transactions().size(); ++i)
    {
        const auto& tx = block.transactions()[i];
        const auto tx_hash = tx.hash();
        ////BOOST_REQUIRE(result.transaction_hash(i) == tx_hash);
        ////BOOST_REQUIRE(result_by_hash.transaction_hash(i) == tx_hash);

        auto result_tx = interface.transactions().get(tx_hash);
        BOOST_REQUIRE(result_tx);
        BOOST_REQUIRE(result_by_hash);
        BOOST_REQUIRE(result_tx.transaction().hash() == tx_hash);
        BOOST_REQUIRE_EQUAL(result_tx.height(), height);
        BOOST_REQUIRE_EQUAL(result_tx.position(), i);

        if (!catalog)
            return;

        if (!tx.is_coinbase())
        {
            test_inputs_cataloged(payments_store, tx, catalog, true);
        }

        test_outputs_cataloged(payments_store, tx, true);
    }
}

static void test_block_not_exists(const data_base& interface,
    const block& block0, bool catalog)
{
    const auto& payments_store = interface.addresses();

    // Popped blocks still exist in the block hash table, but not confirmed.
    const auto block_hash = block0.hash();
    const auto result = interface.blocks().get(block_hash);
    BOOST_REQUIRE(!is_confirmed(result.state()));

    for (size_t i = 0; i < block0.transactions().size(); ++i)
    {
        const auto& tx = block0.transactions()[i];

        if (!catalog)
            return;

        if (!tx.is_coinbase())
        {
            test_inputs_cataloged(payments_store, tx, catalog, true);
        }

        test_outputs_cataloged(payments_store, tx, true);
    }
}

static chain_state::data data_for_chain_state()
{
    chain_state::data value;
    value.height = 1;
    value.bits = { 0, { 0 } };
    value.version = { 1, { 0 } };
    value.timestamp = { 0, 0, { 0 } };
    return value;
}

static void set_state(block& block)
{
    const auto state = std::make_shared<chain_state>(
        chain_state{ data_for_chain_state(), {}, 0, 0, {} });
    block.header().metadata.state = state;
}

static block read_block(const std::string hex)
{
    data_chunk data;
    BOOST_REQUIRE(decode_base16(data, hex));
    block result;
    BOOST_REQUIRE(result.from_data(data));
    set_state(result);
    return result;
}

static void store_block_transactions(data_base& instance, const block& block,
    size_t forks)
{
    for (const auto& tx: block.transactions())
        instance.store(tx, forks);
}

#define DIRECTORY "data_base"

struct data_base_setup_fixture
{
    data_base_setup_fixture()
    {
        test::clear_path(DIRECTORY);
    }
    
    ~data_base_setup_fixture()
    {
        test::clear_path(DIRECTORY);
    }
};


BOOST_FIXTURE_TEST_SUITE(data_base_tests, data_base_setup_fixture)

#define TRANSACTION1 "0100000001537c9d05b5f7d67b09e5108e3bd5e466909cc9403ddd98bc42973f366fe729410600000000ffffffff0163000000000000001976a914fe06e7b4c88a719e92373de489c08244aee4520b88ac00000000"

#define MAINNET_BLOCK1                                                  \
"010000006fe28c0ab6f1b372c1a6a246ae63f74f931e8365e15a089c68d6190000000000982" \
"051fd1e4ba744bbbe680e1fee14677ba1a3c3540bf7b1cdb606e857233e0e61bc6649ffff00" \
"1d01e3629901010000000100000000000000000000000000000000000000000000000000000" \
"00000000000ffffffff0704ffff001d0104ffffffff0100f2052a0100000043410496b538e8" \
"53519c726a2c91e61ec11600ae1390813a627c66fb8be7947be63c52da7589379515d4e0a60" \
"4f8141781e62294721166bf621e73a82cbf2342c858eeac00000000"

#define MAINNET_BLOCK2                                                  \
"010000004860eb18bf1b1620e37e9490fc8a427514416fd75159ab86688e9a8300000000d5f" \
"dcc541e25de1c7a5addedf24858b8bb665c9f36ef744ee42c316022c90f9bb0bc6649ffff00" \
"1d08d2bd6101010000000100000000000000000000000000000000000000000000000000000" \
"00000000000ffffffff0704ffff001d010bffffffff0100f2052a010000004341047211a824" \
"f55b505228e4c3d5194c1fcfaa15a456abdf37f9b9d97a4040afc073dee6c89064984f03385" \
"237d92167c13e236446b417ab79a0fcae412ae3316b77ac00000000"

#define MAINNET_BLOCK3                                                  \
"01000000bddd99ccfda39da1b108ce1a5d70038d0a967bacb68b6b63065f626a0000000044f" \
"672226090d85db9a9f2fbfe5f0f9609b387af7be5b7fbb7a1767c831c9e995dbe6649ffff00" \
"1d05e0ed6d01010000000100000000000000000000000000000000000000000000000000000" \
"00000000000ffffffff0704ffff001d010effffffff0100f2052a0100000043410494b9d3e7" \
"6c5b1629ecf97fff95d7a4bbdac87cc26099ada28066c6ff1eb9191223cd897194a08d0c272" \
"6c5747f1db49e8cf90e75dc3e3550ae9b30086f3cd5aaac00000000"

class data_base_accessor
  : public data_base
{
public:
    data_base_accessor(const bc::database::settings& settings, bool catalog=false)
      : data_base(settings, catalog)
    {
    }

    bool push_all(block_const_ptr_list_const_ptr blocks,
        const config::checkpoint& fork_point)
    {
        return data_base::push_all(blocks, fork_point);
    }

    bool push_all(header_const_ptr_list_const_ptr headers,
        const config::checkpoint& fork_point)
    {
        return data_base::push_all(headers, fork_point);
    }
    
    code push_header(const header& header, size_t height,
        uint32_t median_time_past)
    {
        return data_base::push_header(header, height, median_time_past);
    }

    code push_block(const block& block, size_t height)
    {
        return data_base::push_block(block, height);
    }
    
    code store(const transaction& tx, uint32_t forks)
    {
        return data_base::store(tx, forks);
    }
    
    code pop_header(chain::header& out_header, size_t height) 
    {
        return data_base::pop_header(out_header, height);
    }

    code pop_block(chain::block& out_block, size_t height)
    {
        return data_base::pop_block(out_block, height);
    }

    bool pop_above(header_const_ptr_list_ptr headers,
        const config::checkpoint& fork_point)
    {
        return data_base::pop_above(headers, fork_point);
    }

    bool pop_above(block_const_ptr_list_ptr blocks,
        const config::checkpoint& fork_point)
    {
        return data_base::pop_above(blocks, fork_point);
    }

    code catalog(const chain::block& block)
    {
        return data_base::catalog(block);
    }

    code catalog(const chain::transaction& tx)
    {
        return data_base::catalog(tx);
    }
};

static void test_heights(const data_base& instance,
    size_t check_candidate_height, size_t check_confirmed_height)
{    
    size_t candidate_height = 0u;
    size_t confirmed_height = 0u;
    BOOST_REQUIRE(instance.blocks().top(candidate_height, true));
    BOOST_REQUIRE(instance.blocks().top(confirmed_height, false));
    
    BOOST_REQUIRE_EQUAL(candidate_height, check_candidate_height);
    BOOST_REQUIRE_EQUAL(confirmed_height, check_confirmed_height);
}

BOOST_AUTO_TEST_CASE(data_base__create__block_transactions_index_interaction__success)
{
    create_directory(DIRECTORY);
    bc::database::settings settings;
    settings.directory = DIRECTORY;
    settings.flush_writes = false;
    settings.file_growth_rate = 42;
    settings.block_table_buckets = 42;
    settings.transaction_table_buckets = 42;
    settings.address_table_buckets = 42;
  
    data_base instance(settings, false); 
   
    const auto bc_settings = bc::system::settings(config::settings::mainnet);
    BOOST_REQUIRE(instance.create(bc_settings.genesis_block));
   
    test_heights(instance, 0u, 0u);

    transaction tx1;
    data_chunk wire_tx1;
    BOOST_REQUIRE(decode_base16(wire_tx1, TRANSACTION1));
    BOOST_REQUIRE(tx1.from_data(wire_tx1));

    const auto hash1 = tx1.hash();
    BOOST_REQUIRE(!instance.transactions().get(hash1));   
}

BOOST_AUTO_TEST_CASE(data_base__create__genesis_block_available__success)
{
    create_directory(DIRECTORY);
    bc::database::settings settings;
    settings.directory = DIRECTORY;
    settings.flush_writes = false;
    settings.file_growth_rate = 42;
    settings.block_table_buckets = 42;
    settings.transaction_table_buckets = 42;
    settings.address_table_buckets = 42;

    data_base instance(settings, true);
   
    const auto bc_settings = bc::system::settings(config::settings::mainnet);
    const chain::block& genesis = bc_settings.genesis_block;    
    BOOST_REQUIRE(instance.create(genesis));

    test_block_exists(instance, 0, genesis, true, false);
}

BOOST_AUTO_TEST_CASE(data_base__push__adds_to_blocks_and_transactions_validates_and_confirms__success)
{
    create_directory(DIRECTORY);
    bc::database::settings settings;
    settings.directory = DIRECTORY;
    settings.flush_writes = false;
    settings.file_growth_rate = 42;
    settings.block_table_buckets = 42;
    settings.transaction_table_buckets = 42;
    settings.address_table_buckets = 42;
  
    data_base instance(settings, true);
   
    const auto bc_settings = bc::system::settings(config::settings::mainnet);
    BOOST_REQUIRE(instance.create(bc_settings.genesis_block));

    const auto block1 = read_block(MAINNET_BLOCK1);   

    // setup ends
   
    BOOST_REQUIRE_EQUAL(instance.push(block1, 1), error::success);

    // test conditions
   
    test_block_exists(instance, 1, block1, true, false);
    test_heights(instance, 1u, 1u);
}

// BLOCK ORGANIZER tests
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(data_base__push_block__not_existing___failure)
{
    create_directory(DIRECTORY);
    bc::database::settings settings;
    settings.directory = DIRECTORY;
    settings.flush_writes = false;
    settings.file_growth_rate = 42;
    settings.block_table_buckets = 42;
    settings.transaction_table_buckets = 42;
    settings.address_table_buckets = 42;
    
    data_base_accessor instance(settings); 
    
    const auto bc_settings = bc::system::settings(config::settings::mainnet);
    BOOST_REQUIRE(instance.create(bc_settings.genesis_block));
    
    const auto block1 = read_block(MAINNET_BLOCK1);
    store_block_transactions(instance, block1, 1);
    
    // setup ends
    
    BOOST_REQUIRE_EQUAL(instance.push_block(block1, 1), error::operation_failed);

    // test conditions

    test_heights(instance, 0u, 0u);
}

BOOST_AUTO_TEST_CASE(data_base__push_block__incorrect_height___failure)
{
    create_directory(DIRECTORY);
    bc::database::settings settings;
    settings.directory = DIRECTORY;
    settings.flush_writes = false;
    settings.file_growth_rate = 42;
    settings.block_table_buckets = 42;
    settings.transaction_table_buckets = 42;
    settings.address_table_buckets = 42;

    data_base_accessor instance(settings);

    const auto bc_settings = bc::system::settings(config::settings::mainnet);
    BOOST_REQUIRE(instance.create(bc_settings.genesis_block));

    const auto block1 = read_block(MAINNET_BLOCK1);
    store_block_transactions(instance, block1, 1);

    BOOST_REQUIRE_EQUAL(instance.push_header(block1.header(), 1, 100), error::success);
    BOOST_REQUIRE_EQUAL(instance.candidate(block1), error::success);
    test_heights(instance, 1u, 0u);

    // setup ends

#ifndef NDEBUG
    BOOST_REQUIRE_EQUAL(instance.push_block(block1, 2), error::store_block_invalid_height);
#else
    BOOST_REQUIRE_EQUAL(instance.push_block(block1, 2), error::operation_failed);
#endif
}

BOOST_AUTO_TEST_CASE(data_base__push_header__missing_parent___failure)
{
    create_directory(DIRECTORY);
    bc::database::settings settings;
    settings.directory = DIRECTORY;
    settings.flush_writes = false;
    settings.file_growth_rate = 42;
    settings.block_table_buckets = 42;
    settings.transaction_table_buckets = 42;
    settings.address_table_buckets = 42;

    data_base_accessor instance(settings);

    const auto bc_settings = bc::system::settings(config::settings::mainnet);
    BOOST_REQUIRE(instance.create(bc_settings.genesis_block));

    auto block1 = read_block(MAINNET_BLOCK1);
    store_block_transactions(instance, block1, 1);
    block1.set_header(chain::header{});

    // setup ends

#ifndef NDEBUG
    BOOST_REQUIRE_EQUAL(instance.push_header(block1.header(), 1, 100), error::store_block_missing_parent);
#else
    BOOST_REQUIRE_EQUAL(instance.push_header(block1.header(), 1, 100), error::success);
#endif
}

BOOST_AUTO_TEST_CASE(data_base__push_block_and_update__already_candidated___success)
{
    create_directory(DIRECTORY);
    bc::database::settings settings;
    settings.directory = DIRECTORY;
    settings.flush_writes = false;
    settings.file_growth_rate = 42;
    settings.block_table_buckets = 42;
    settings.transaction_table_buckets = 42;
    settings.address_table_buckets = 42;
  
    data_base_accessor instance(settings); 
   
    const auto bc_settings = bc::system::settings(config::settings::mainnet);
    BOOST_REQUIRE(instance.create(bc_settings.genesis_block));
   
    const auto block1 = read_block(MAINNET_BLOCK1);
    store_block_transactions(instance, block1, 1);

    BOOST_REQUIRE_EQUAL(instance.push_header(block1.header(), 1, 100), error::success);
    BOOST_REQUIRE_EQUAL(instance.candidate(block1), error::success);
    test_heights(instance, 1u, 0u);

    // setup ends
   
    BOOST_REQUIRE_EQUAL(instance.push_block(block1, 1), error::success);
    BOOST_REQUIRE_EQUAL(instance.update(block1, 1), error::success);

    // test conditions

    test_heights(instance, 1u, 1u);
    test_block_exists(instance, 1, block1, false, false);
}

BOOST_AUTO_TEST_CASE(data_base__pop_header_not_top___failure)
{
    create_directory(DIRECTORY);
    bc::database::settings settings;
    settings.directory = DIRECTORY;
    settings.flush_writes = false;
    settings.file_growth_rate = 42;
    settings.block_table_buckets = 42;
    settings.transaction_table_buckets = 42;
    settings.address_table_buckets = 42;
  
    data_base_accessor instance(settings); 
   
    const auto bc_settings = bc::system::settings(config::settings::mainnet);
    BOOST_REQUIRE(instance.create(bc_settings.genesis_block));
   
    const auto block1 = read_block(MAINNET_BLOCK1);

    // setup ends

    chain::header out_header;
    BOOST_REQUIRE_EQUAL(instance.pop_header(out_header, 1), error::operation_failed);
}

BOOST_AUTO_TEST_CASE(data_base__pop_header__candidate___success)
{
    create_directory(DIRECTORY);
    bc::database::settings settings;
    settings.directory = DIRECTORY;
    settings.flush_writes = false;
    settings.file_growth_rate = 42;
    settings.block_table_buckets = 42;
    settings.transaction_table_buckets = 42;
    settings.address_table_buckets = 42;
  
    data_base_accessor instance(settings); 
   
    const auto bc_settings = bc::system::settings(config::settings::mainnet);
    BOOST_REQUIRE(instance.create(bc_settings.genesis_block));
   
    const auto block1 = read_block(MAINNET_BLOCK1);
    store_block_transactions(instance, block1, 1);

    BOOST_REQUIRE_EQUAL(instance.push_header(block1.header(), 1, 100), error::success);
    BOOST_REQUIRE_EQUAL(instance.candidate(block1), error::success);

    // setup ends

    chain::header out_header;
    BOOST_REQUIRE_EQUAL(instance.pop_header(out_header, 1), error::success);

    // test conditions

    BOOST_REQUIRE(out_header.hash() == block1.header().hash());
    test_heights(instance, 0u, 0u);
}

BOOST_AUTO_TEST_CASE(data_base__pop_block_not_top___failure)
{
    create_directory(DIRECTORY);
    bc::database::settings settings;
    settings.directory = DIRECTORY;
    settings.flush_writes = false;
    settings.file_growth_rate = 42;
    settings.block_table_buckets = 42;
    settings.transaction_table_buckets = 42;
    settings.address_table_buckets = 42;

    data_base_accessor instance(settings);

    const auto bc_settings = bc::system::settings(config::settings::mainnet);
    BOOST_REQUIRE(instance.create(bc_settings.genesis_block));

    const auto block1 = read_block(MAINNET_BLOCK1);

    // setup ends

    chain::block out_block;
    BOOST_REQUIRE_EQUAL(instance.pop_block(out_block, 1), error::operation_failed);
}

BOOST_AUTO_TEST_CASE(data_base__pop_block__confirmed___success)
{
    create_directory(DIRECTORY);
    bc::database::settings settings;
    settings.directory = DIRECTORY;
    settings.flush_writes = false;
    settings.file_growth_rate = 42;
    settings.block_table_buckets = 42;
    settings.transaction_table_buckets = 42;
    settings.address_table_buckets = 42;

    data_base_accessor instance(settings);

    const auto bc_settings = bc::system::settings(config::settings::mainnet);
    BOOST_REQUIRE(instance.create(bc_settings.genesis_block));

    const auto block1 = read_block(MAINNET_BLOCK1);
    store_block_transactions(instance, block1, 1);

    BOOST_REQUIRE_EQUAL(instance.push_header(block1.header(), 1, 100), error::success);
    BOOST_REQUIRE_EQUAL(instance.candidate(block1), error::success);
    BOOST_REQUIRE_EQUAL(instance.push_block(block1, 1), error::success);

    // setup ends

    chain::block out_block;
    BOOST_REQUIRE_EQUAL(instance.pop_block(out_block, 1), error::success);

    // test conditions

    BOOST_REQUIRE(out_block.hash() == block1.hash());
    test_block_not_exists(instance, block1, false);
    test_heights(instance, 1u, 0u);
}

BOOST_AUTO_TEST_CASE(data_base__push_all_and_update__already_candidated___success)
{
    create_directory(DIRECTORY);
    bc::database::settings settings;
    settings.directory = DIRECTORY;
    settings.flush_writes = false;
    settings.file_growth_rate = 42;
    settings.block_table_buckets = 42;
    settings.transaction_table_buckets = 42;
    settings.address_table_buckets = 42;
    
    data_base_accessor instance(settings); 
    
    const auto bc_settings = bc::system::settings(config::settings::mainnet);
    const chain::block& genesis = bc_settings.genesis_block;
    BOOST_REQUIRE(instance.create(genesis));
    
    const auto block1 = read_block(MAINNET_BLOCK1);
    
    const auto block1_ptr = std::make_shared<const message::block>(read_block(MAINNET_BLOCK1));
    const auto block2_ptr = std::make_shared<const message::block>(read_block(MAINNET_BLOCK2));
    const auto block3_ptr = std::make_shared<const message::block>(read_block(MAINNET_BLOCK3));
    const auto blocks_push_ptr = std::make_shared<const block_const_ptr_list>(block_const_ptr_list{ block1_ptr, block2_ptr, block3_ptr });
    
    store_block_transactions(instance, *block1_ptr, 1);
    store_block_transactions(instance, *block2_ptr, 1);
    store_block_transactions(instance, *block3_ptr, 1);

    const auto headers_push_ptr = std::make_shared<const header_const_ptr_list>(header_const_ptr_list
    {
        std::make_shared<const message::header>(block1_ptr->header()),
        std::make_shared<const message::header>(block2_ptr->header()),
        std::make_shared<const message::header>(block3_ptr->header())
    });
    
    BOOST_REQUIRE(instance.push_all(headers_push_ptr, config::checkpoint(genesis.hash(), 0)));

    for (const auto block_ptr: *blocks_push_ptr)
        BOOST_REQUIRE_EQUAL(instance.candidate(*block_ptr), error::success);
    
    test_heights(instance, 3u, 0u);
    
    // setup ends
    
    BOOST_REQUIRE(instance.push_all(blocks_push_ptr, config::checkpoint(genesis.hash(), 0)));
    BOOST_REQUIRE_EQUAL(instance.update(*block1_ptr, 1), error::success);
    BOOST_REQUIRE_EQUAL(instance.update(*block2_ptr, 2), error::success);
    BOOST_REQUIRE_EQUAL(instance.update(*block3_ptr, 3), error::success);
    
    // test conditions
    
    test_heights(instance, 3u, 3u);
    test_block_exists(instance, 1, *block1_ptr, false, false);
    test_block_exists(instance, 2, *block2_ptr, false, false);
    test_block_exists(instance, 3, *block3_ptr, false, false);
}

BOOST_AUTO_TEST_CASE(data_base__pop_above_missing_forkpoint_hash___failure)
{
    create_directory(DIRECTORY);
    bc::database::settings settings;
    settings.directory = DIRECTORY;
    settings.flush_writes = false;
    settings.file_growth_rate = 42;
    settings.block_table_buckets = 42;
    settings.transaction_table_buckets = 42;
    settings.address_table_buckets = 42;
  
    data_base_accessor instance(settings); 
   
    const auto bc_settings = bc::system::settings(config::settings::mainnet);
    BOOST_REQUIRE(instance.create(bc_settings.genesis_block));
   
    const auto block1 = read_block(MAINNET_BLOCK1);
    const auto out_headers = std::make_shared<header_const_ptr_list>();

    // setup ends

    const auto result = instance.pop_above(out_headers, config::checkpoint(block1.hash(), 0));
#ifndef NDEBUG
    BOOST_REQUIRE(!result);
#else
    BOOST_REQUIRE(result);
#endif
}

#ifndef NDEBUG
BOOST_AUTO_TEST_CASE(data_base__pop_above__wrong_forkpoint_height___failure)
{
    create_directory(DIRECTORY);
    bc::database::settings settings;
    settings.directory = DIRECTORY;
    settings.flush_writes = false;
    settings.file_growth_rate = 42;
    settings.block_table_buckets = 42;
    settings.transaction_table_buckets = 42;
    settings.address_table_buckets = 42;
  
    data_base_accessor instance(settings); 
   
    const auto bc_settings = bc::system::settings(config::settings::mainnet);
    const chain::block& genesis = bc_settings.genesis_block;
    BOOST_REQUIRE(instance.create(genesis));
   
    const auto block1 = read_block(MAINNET_BLOCK1);
    const auto out_headers = std::make_shared<header_const_ptr_list>();

    // setup ends

    BOOST_REQUIRE(!instance.pop_above(out_headers, config::checkpoint(genesis.hash(), 10)));
}
#endif

BOOST_AUTO_TEST_CASE(data_base__pop_above__pop_zero___success)
{
    create_directory(DIRECTORY);
    bc::database::settings settings;
    settings.directory = DIRECTORY;
    settings.flush_writes = false;
    settings.file_growth_rate = 42;
    settings.block_table_buckets = 42;
    settings.transaction_table_buckets = 42;
    settings.address_table_buckets = 42;
  
    data_base_accessor instance(settings); 
   
    const auto bc_settings = bc::system::settings(config::settings::mainnet);
    const chain::block& genesis = bc_settings.genesis_block;
    BOOST_REQUIRE(instance.create(genesis));
   
    const auto block1 = read_block(MAINNET_BLOCK1);
    const auto out_headers = std::make_shared<header_const_ptr_list>();

    // setup ends
   
    BOOST_REQUIRE(instance.pop_above(out_headers, config::checkpoint(genesis.hash(), 0)));

    // test conditions

    test_heights(instance, 0u, 0u);   
}

BOOST_AUTO_TEST_CASE(data_base__pop_above__candidated_not_confirmed___success)
{
    create_directory(DIRECTORY);
    bc::database::settings settings;
    settings.directory = DIRECTORY;
    settings.flush_writes = false;
    settings.file_growth_rate = 42;
    settings.block_table_buckets = 42;
    settings.transaction_table_buckets = 42;
    settings.address_table_buckets = 42;
  
    data_base_accessor instance(settings); 
   
    const auto bc_settings = bc::system::settings(config::settings::mainnet);
    const chain::block& genesis = bc_settings.genesis_block;
    BOOST_REQUIRE(instance.create(genesis));
   
    const auto block1 = read_block(MAINNET_BLOCK1);

    const auto block1_ptr = std::make_shared<const message::block>(read_block(MAINNET_BLOCK1));
    const auto block2_ptr = std::make_shared<const message::block>(read_block(MAINNET_BLOCK2));
    const auto block3_ptr = std::make_shared<const message::block>(read_block(MAINNET_BLOCK3));
    const auto blocks_push_ptr = std::make_shared<const block_const_ptr_list>(block_const_ptr_list{ block1_ptr, block2_ptr, block3_ptr });
    store_block_transactions(instance, *block1_ptr, 1);
    store_block_transactions(instance, *block2_ptr, 1);
    store_block_transactions(instance, *block3_ptr, 1);

    const auto headers_push_ptr = std::make_shared<const header_const_ptr_list>(header_const_ptr_list
    {
        std::make_shared<const message::header>(block1_ptr->header()),
        std::make_shared<const message::header>(block2_ptr->header()),
        std::make_shared<const message::header>(block3_ptr->header())
    });
      
    BOOST_REQUIRE(instance.push_all(headers_push_ptr, config::checkpoint(genesis.hash(), 0)));
    for (const auto block_ptr: *blocks_push_ptr)
        BOOST_REQUIRE_EQUAL(instance.candidate(*block_ptr), error::success);

    test_heights(instance, 3u, 0u);

    // setup ends
   
    const auto out_headers = std::make_shared<header_const_ptr_list>();
    BOOST_REQUIRE(instance.pop_above(out_headers, config::checkpoint(genesis.hash(), 0)));

    // test conditions

    BOOST_REQUIRE_EQUAL(out_headers->size(), 3);
    test_heights(instance, 0u, 0u);
    test_block_not_exists(instance, *block1_ptr, false);
    test_block_not_exists(instance, *block2_ptr, false);
    test_block_not_exists(instance, *block3_ptr, false);
}

#ifndef NDEBUG
BOOST_AUTO_TEST_CASE(data_base__pop_above2__wrong_forkpoint_height___failure)
{
    create_directory(DIRECTORY);
    bc::database::settings settings;
    settings.directory = DIRECTORY;
    settings.flush_writes = false;
    settings.file_growth_rate = 42;
    settings.block_table_buckets = 42;
    settings.transaction_table_buckets = 42;
    settings.address_table_buckets = 42;

    data_base_accessor instance(settings);

    const auto bc_settings = bc::system::settings(config::settings::mainnet);
    const chain::block& genesis = bc_settings.genesis_block;
    BOOST_REQUIRE(instance.create(genesis));

    const auto block1 = read_block(MAINNET_BLOCK1);
    const auto out_blocks = std::make_shared<block_const_ptr_list>();

    // setup ends

    BOOST_REQUIRE(!instance.pop_above(out_blocks, config::checkpoint(genesis.hash(), 10)));
}
#endif

BOOST_AUTO_TEST_CASE(data_base__pop_above2__pop_zero___success)
{
    create_directory(DIRECTORY);
    bc::database::settings settings;
    settings.directory = DIRECTORY;
    settings.flush_writes = false;
    settings.file_growth_rate = 42;
    settings.block_table_buckets = 42;
    settings.transaction_table_buckets = 42;
    settings.address_table_buckets = 42;

    data_base_accessor instance(settings);

    const auto bc_settings = bc::system::settings(config::settings::mainnet);
    const chain::block& genesis = bc_settings.genesis_block;
    BOOST_REQUIRE(instance.create(genesis));

    const auto block1 = read_block(MAINNET_BLOCK1);
    const auto out_blocks = std::make_shared<block_const_ptr_list>();

    // setup ends

    BOOST_REQUIRE(instance.pop_above(out_blocks, config::checkpoint(genesis.hash(), 0)));

    // test conditions

    test_heights(instance, 0u, 0u);
}

BOOST_AUTO_TEST_CASE(data_base__pop_above2__confirmed___success)
{
    create_directory(DIRECTORY);
    bc::database::settings settings;
    settings.directory = DIRECTORY;
    settings.flush_writes = false;
    settings.file_growth_rate = 42;
    settings.block_table_buckets = 42;
    settings.transaction_table_buckets = 42;
    settings.address_table_buckets = 42;

    data_base_accessor instance(settings);

    const auto bc_settings = bc::system::settings(config::settings::mainnet);
    const chain::block& genesis = bc_settings.genesis_block;
    BOOST_REQUIRE(instance.create(genesis));

    const auto block1 = read_block(MAINNET_BLOCK1);

    const auto block1_ptr = std::make_shared<const message::block>(read_block(MAINNET_BLOCK1));
    const auto block2_ptr = std::make_shared<const message::block>(read_block(MAINNET_BLOCK2));
    const auto block3_ptr = std::make_shared<const message::block>(read_block(MAINNET_BLOCK3));
    const auto blocks_push_ptr = std::make_shared<const block_const_ptr_list>(block_const_ptr_list{ block1_ptr, block2_ptr, block3_ptr });
    store_block_transactions(instance, *block1_ptr, 1);
    store_block_transactions(instance, *block2_ptr, 1);
    store_block_transactions(instance, *block3_ptr, 1);

    const auto headers_push_ptr = std::make_shared<const header_const_ptr_list>(header_const_ptr_list
    {
        std::make_shared<const message::header>(block1_ptr->header()),
        std::make_shared<const message::header>(block2_ptr->header()),
        std::make_shared<const message::header>(block3_ptr->header())
    });

    BOOST_REQUIRE(instance.push_all(headers_push_ptr, config::checkpoint(genesis.hash(), 0)));

    // TODO: remove loop.
    for (const auto block_ptr: *blocks_push_ptr) 
        BOOST_REQUIRE_EQUAL(instance.candidate(*block_ptr), error::success);

    BOOST_REQUIRE(instance.push_all(blocks_push_ptr, config::checkpoint(genesis.hash(), 0)));
    BOOST_REQUIRE_EQUAL(instance.update(*block1_ptr, 1), error::success);
    BOOST_REQUIRE_EQUAL(instance.update(*block2_ptr, 2), error::success);
    BOOST_REQUIRE_EQUAL(instance.update(*block3_ptr, 3), error::success);

    test_heights(instance, 3u, 3u);

    // setup ends

    const auto out_blocks = std::make_shared<block_const_ptr_list>();
    BOOST_REQUIRE(instance.pop_above(out_blocks, config::checkpoint(genesis.hash(), 0)));

    // test conditions

    BOOST_REQUIRE_EQUAL(out_blocks->size(), 3);
    test_heights(instance, 3u, 0u);
    test_block_not_exists(instance, *block1_ptr, false);
    test_block_not_exists(instance, *block2_ptr, false);
    test_block_not_exists(instance, *block3_ptr, false);
}

/// Confirm

BOOST_AUTO_TEST_CASE(data_base__confirm__not_existing___failure)
{
    create_directory(DIRECTORY);
    bc::database::settings settings;
    settings.directory = DIRECTORY;
    settings.flush_writes = false;
    settings.file_growth_rate = 42;
    settings.block_table_buckets = 42;
    settings.transaction_table_buckets = 42;
    settings.address_table_buckets = 42;

    data_base_accessor instance(settings);

    const auto bc_settings = bc::system::settings(config::settings::mainnet);
    BOOST_REQUIRE(instance.create(bc_settings.genesis_block));

    const auto block1 = read_block(MAINNET_BLOCK1);
    store_block_transactions(instance, block1, 1);

    // setup ends

    BOOST_REQUIRE_EQUAL(instance.confirm(block1.hash(), 1), error::operation_failed);

    // test conditions

    test_heights(instance, 0u, 0u);
}

BOOST_AUTO_TEST_CASE(data_base__confirm__incorrect_height___failure)
{
    create_directory(DIRECTORY);
    bc::database::settings settings;
    settings.directory = DIRECTORY;
    settings.flush_writes = false;
    settings.file_growth_rate = 42;
    settings.block_table_buckets = 42;
    settings.transaction_table_buckets = 42;
    settings.address_table_buckets = 42;

    data_base_accessor instance(settings);

    const auto bc_settings = bc::system::settings(config::settings::mainnet);
    BOOST_REQUIRE(instance.create(bc_settings.genesis_block));

    const auto block1 = read_block(MAINNET_BLOCK1);
    store_block_transactions(instance, block1, 1);

    BOOST_REQUIRE_EQUAL(instance.push_header(block1.header(), 1, 100), error::success);
    BOOST_REQUIRE_EQUAL(instance.candidate(block1), error::success);
    test_heights(instance, 1u, 0u);

    // setup ends

    BOOST_REQUIRE_EQUAL(instance.confirm(block1.hash(), 2), error::operation_failed);
}

BOOST_AUTO_TEST_CASE(data_base__confirm__missing_parent___failure)
{
    create_directory(DIRECTORY);
    bc::database::settings settings;
    settings.directory = DIRECTORY;
    settings.flush_writes = false;
    settings.file_growth_rate = 42;
    settings.block_table_buckets = 42;
    settings.transaction_table_buckets = 42;
    settings.address_table_buckets = 42;

    data_base_accessor instance(settings);

    const auto bc_settings = bc::system::settings(config::settings::mainnet);
    BOOST_REQUIRE(instance.create(bc_settings.genesis_block));

    auto block1 = read_block(MAINNET_BLOCK1);
    store_block_transactions(instance, block1, 1);
    block1.set_header(chain::header{});

    // setup ends

    BOOST_REQUIRE_EQUAL(instance.confirm(block1.hash(), 1), error::operation_failed);
}

BOOST_AUTO_TEST_CASE(data_base__confirm__already_candidated___success)
{
    create_directory(DIRECTORY);
    bc::database::settings settings;
    settings.directory = DIRECTORY;
    settings.flush_writes = false;
    settings.file_growth_rate = 42;
    settings.block_table_buckets = 42;
    settings.transaction_table_buckets = 42;
    settings.address_table_buckets = 42;

    data_base_accessor instance(settings);

    const auto bc_settings = bc::system::settings(config::settings::mainnet);
    BOOST_REQUIRE(instance.create(bc_settings.genesis_block));

    const auto block1 = read_block(MAINNET_BLOCK1);
    store_block_transactions(instance, block1, 1);

    BOOST_REQUIRE_EQUAL(instance.push_header(block1.header(), 1, 100), error::success);
    BOOST_REQUIRE_EQUAL(instance.candidate(block1), error::success);
    test_heights(instance, 1u, 0u);
    BOOST_REQUIRE_EQUAL(instance.update(block1, 1), error::success);
    BOOST_REQUIRE_EQUAL(block1.transactions().size(), 1);
    BOOST_REQUIRE_EQUAL(instance.blocks().get(1, true).transaction_count(), 1);

    // setup ends

    BOOST_REQUIRE_EQUAL(instance.confirm(block1.hash(), 1), error::success);

    // test conditions

    test_heights(instance, 1u, 1u);
    const auto& block_result = instance.blocks().get(1, false);
    BOOST_REQUIRE(block_result.hash() == block1.hash());
    test_block_exists(instance, 1, block1, false, false);

    // TODO: remove loop.
    for (const auto& offset: block_result)
        BOOST_REQUIRE(!instance.transactions().get(offset).candidate());
}

/// update

#ifndef NDEBUG
BOOST_AUTO_TEST_CASE(data_base__update__incorrect_height__failure)
{
    create_directory(DIRECTORY);
    bc::database::settings settings;
    settings.directory = DIRECTORY;
    settings.flush_writes = false;
    settings.file_growth_rate = 42;
    settings.block_table_buckets = 42;
    settings.transaction_table_buckets = 42;
    settings.address_table_buckets = 42;

    data_base_accessor instance(settings);

    const auto bc_settings = bc::system::settings(config::settings::mainnet);
    BOOST_REQUIRE(instance.create(bc_settings.genesis_block));

    const auto block1 = read_block(MAINNET_BLOCK1);
    store_block_transactions(instance, block1, 1);

    BOOST_REQUIRE_EQUAL(instance.push_header(block1.header(), 1, 100), error::success);
    BOOST_REQUIRE_EQUAL(instance.candidate(block1), error::success);

    // setup ends

    BOOST_REQUIRE_EQUAL(instance.update(block1, 2), error::not_found);
}
#endif

BOOST_AUTO_TEST_CASE(data_base__update__new_transactions__success)
{
    create_directory(DIRECTORY);
    bc::database::settings settings;
    settings.directory = DIRECTORY;
    settings.flush_writes = false;
    settings.file_growth_rate = 42;
    settings.block_table_buckets = 42;
    settings.transaction_table_buckets = 42;
    settings.address_table_buckets = 42;

    data_base_accessor instance(settings);

    const auto bc_settings = bc::system::settings(config::settings::mainnet);
    BOOST_REQUIRE(instance.create(bc_settings.genesis_block));

    auto block1 = read_block(MAINNET_BLOCK1);
    store_block_transactions(instance, block1, 1);

    BOOST_REQUIRE_EQUAL(instance.push_header(block1.header(), 1, 100), error::success);
    BOOST_REQUIRE_EQUAL(instance.candidate(block1), error::success);

    transaction tx1;
    data_chunk wire_tx1;
    BOOST_REQUIRE(decode_base16(wire_tx1, TRANSACTION1));
    BOOST_REQUIRE(tx1.from_data(wire_tx1));

    block1.set_transactions({ tx1 });

    // setup ends

    BOOST_REQUIRE_EQUAL(instance.update(block1, 1), error::success);

    // new transactions are not candidated
    const auto& found = instance.transactions().get(tx1.hash());
    BOOST_REQUIRE(found);
    BOOST_REQUIRE(!instance.transactions().get(tx1.hash()).candidate());

    // get block and check block_result can access transactions
    const auto& block_result = instance.blocks().get(block1.hash());
    for (const auto& offset: block_result)
        BOOST_REQUIRE(!instance.transactions().get(offset).candidate());
}

// invalidate

#ifndef NDEBUG
BOOST_AUTO_TEST_CASE(data_base__invalidate__missing_header__failure)
{
    create_directory(DIRECTORY);
    bc::database::settings settings;
    settings.directory = DIRECTORY;
    settings.flush_writes = false;
    settings.file_growth_rate = 42;
    settings.block_table_buckets = 42;
    settings.transaction_table_buckets = 42;
    settings.address_table_buckets = 42;

    data_base_accessor instance(settings);

    const auto bc_settings = bc::system::settings(config::settings::mainnet);
    BOOST_REQUIRE(instance.create(bc_settings.genesis_block));

    const auto block1 = read_block(MAINNET_BLOCK1);

    // setup ends
    BOOST_REQUIRE_EQUAL(instance.invalidate(block1.header(), error::success), error::not_found);
}
#endif

BOOST_AUTO_TEST_CASE(data_base__invalidate__validate__success)
{
    create_directory(DIRECTORY);
    bc::database::settings settings;
    settings.directory = DIRECTORY;
    settings.flush_writes = false;
    settings.file_growth_rate = 42;
    settings.block_table_buckets = 42;
    settings.transaction_table_buckets = 42;
    settings.address_table_buckets = 42;

    data_base_accessor instance(settings);

    const auto bc_settings = bc::system::settings(config::settings::mainnet);
    BOOST_REQUIRE(instance.create(bc_settings.genesis_block));

    const auto block1 = read_block(MAINNET_BLOCK1);
    store_block_transactions(instance, block1, 1);

    BOOST_REQUIRE_EQUAL(instance.push_header(block1.header(), 1, 100), error::success);

    // setup ends

    const auto header_result = block1.header();
    BOOST_REQUIRE_EQUAL(instance.invalidate(header_result, error::success), error::success);

    BOOST_REQUIRE_EQUAL(header_result.metadata.error, error::success);
    BOOST_REQUIRE(header_result.metadata.validated);
    const auto state = instance.blocks().get(header_result.hash()).state() & block_state::valid;
    BOOST_REQUIRE_EQUAL(state, block_state::valid);
}

BOOST_AUTO_TEST_CASE(data_base__invalidate__invalidate__success)
{
    create_directory(DIRECTORY);
    bc::database::settings settings;
    settings.directory = DIRECTORY;
    settings.flush_writes = false;
    settings.file_growth_rate = 42;
    settings.block_table_buckets = 42;
    settings.transaction_table_buckets = 42;
    settings.address_table_buckets = 42;

    data_base_accessor instance(settings);

    const auto bc_settings = bc::system::settings(config::settings::mainnet);
    BOOST_REQUIRE(instance.create(bc_settings.genesis_block));

    const auto block1 = read_block(MAINNET_BLOCK1);
    store_block_transactions(instance, block1, 1);

    BOOST_REQUIRE_EQUAL(instance.push_header(block1.header(), 1, 100), error::success);

    // setup ends

    const auto header_result = block1.header();
    BOOST_REQUIRE_EQUAL(instance.invalidate(header_result, error::invalid_proof_of_work), error::success);

    BOOST_REQUIRE_EQUAL(header_result.metadata.error, error::invalid_proof_of_work);
    BOOST_REQUIRE(header_result.metadata.validated);
    const auto state = instance.blocks().get(header_result.hash()).state() & block_state::valid;
    BOOST_REQUIRE(state != block_state::valid);
}

// catalog block

BOOST_AUTO_TEST_CASE(data_base__catalog__enabled__success)
{
    create_directory(DIRECTORY);
    bc::database::settings settings;
    settings.directory = DIRECTORY;
    settings.flush_writes = false;
    settings.file_growth_rate = 42;
    settings.block_table_buckets = 42;
    settings.transaction_table_buckets = 42;
    settings.address_table_buckets = 42;

    data_base_accessor instance(settings, true);

    const auto bc_settings = bc::system::settings(config::settings::mainnet);
    BOOST_REQUIRE(instance.create(bc_settings.genesis_block));

    const auto block1 = read_block(MAINNET_BLOCK1);
    store_block_transactions(instance, block1, 1);

    BOOST_REQUIRE_EQUAL(instance.push_header(block1.header(), 1, 100), error::success);
    // setup ends

    BOOST_REQUIRE_EQUAL(instance.catalog(block1), error::success);

    // Coinbase transactions' inputs are not cataloged
    BOOST_REQUIRE(block1.transactions().front().is_coinbase());
    test_outputs_cataloged(instance.addresses(), block1.transactions().front(), true);
}

// catalog transactions

BOOST_AUTO_TEST_CASE(data_base__catalog2__enabled__success)
{
    create_directory(DIRECTORY);
    bc::database::settings settings;
    settings.directory = DIRECTORY;
    settings.flush_writes = false;
    settings.file_growth_rate = 42;
    settings.block_table_buckets = 42;
    settings.transaction_table_buckets = 42;
    settings.address_table_buckets = 42;

    data_base_accessor instance(settings, true);

    const auto bc_settings = bc::system::settings(config::settings::mainnet);
    BOOST_REQUIRE(instance.create(bc_settings.genesis_block));

    const auto block1 = read_block(MAINNET_BLOCK1);
    store_block_transactions(instance, block1, 1);

    // setup ends

    BOOST_REQUIRE_EQUAL(instance.catalog(block1.transactions()[0]), error::success);

    // Coinbase transactions' inputs are not cataloged
    BOOST_REQUIRE(block1.transactions().front().is_coinbase());
    test_outputs_cataloged(instance.addresses(), block1.transactions().front(), true);
}

/// reorganize headers

BOOST_AUTO_TEST_CASE(data_base__reorganize__pop_and_push__success)
{
    create_directory(DIRECTORY);
    bc::database::settings settings;
    settings.directory = DIRECTORY;
    settings.flush_writes = false;
    settings.file_growth_rate = 42;
    settings.block_table_buckets = 42;
    settings.transaction_table_buckets = 42;
    settings.address_table_buckets = 42;

    data_base_accessor instance(settings);

    const auto bc_settings = bc::system::settings(config::settings::mainnet);
    const chain::block& genesis = bc_settings.genesis_block;
    BOOST_REQUIRE(instance.create(genesis));

    const auto& block1 = read_block(MAINNET_BLOCK1);
    BOOST_REQUIRE_EQUAL(instance.push_header(block1.header(), 1, 100), error::success);
    store_block_transactions(instance, block1, 1);
    BOOST_REQUIRE_EQUAL(instance.candidate(block1), error::success);
    test_heights(instance, 1u, 0u);
    auto block2 = read_block(MAINNET_BLOCK2);
    auto block2_header = block2.header();
    block2_header.set_previous_block_hash(genesis.hash());
    block2.set_header(block2_header);
    auto block3 = read_block(MAINNET_BLOCK3);
    auto block3_header = block3.header();
    block3_header.set_previous_block_hash(block2.hash());
    block3.set_header(block3_header);
    
    const auto outgoing_headers = std::make_shared<header_const_ptr_list>();
    const auto incoming_headers = std::make_shared<const header_const_ptr_list>(header_const_ptr_list
    {
        std::make_shared<const message::header>(block2.header()),
        std::make_shared<const message::header>(block3.header())
    });

    // setup ends
    
    BOOST_REQUIRE_EQUAL(instance.reorganize(config::checkpoint(genesis.hash(), 0), incoming_headers, outgoing_headers), error::success);

    // test conditions
    
    test_heights(instance, 2u, 0u);

    // Verify outgoing have right headers.
    BOOST_REQUIRE_EQUAL(outgoing_headers->size(), 1u);
    BOOST_REQUIRE(outgoing_headers->front()->hash() == block1.hash());

    // Verify outgoing headers are NOT in candidate index.
    BOOST_REQUIRE((instance.blocks().get(outgoing_headers->front()->hash()).state() & block_state::candidate) == 0);

    // Verify incoming are headers are in candidate index.
    const auto state2 = instance.blocks().get((*incoming_headers)[0]->hash()).state();
    BOOST_REQUIRE((state2 & block_state::candidate) != 0);
    const auto state3 = instance.blocks().get((*incoming_headers)[1]->hash()).state();
    BOOST_REQUIRE((state3 & block_state::candidate) != 0);

    // Verify candidate top header.
    BOOST_REQUIRE(instance.blocks().get(2, true).hash() == block3.hash());
}

/// reorganize blocks

BOOST_AUTO_TEST_CASE(data_base__reorganize2__pop_and_push__success)
{
    create_directory(DIRECTORY);
    bc::database::settings settings;
    settings.directory = DIRECTORY;
    settings.flush_writes = false;
    settings.file_growth_rate = 42;
    settings.block_table_buckets = 42;
    settings.transaction_table_buckets = 42;
    settings.address_table_buckets = 42;

    data_base_accessor instance(settings);

    const auto bc_settings = bc::system::settings(config::settings::mainnet);
    const chain::block& genesis = bc_settings.genesis_block;
    BOOST_REQUIRE(instance.create(genesis));

    test_heights(instance, 0u, 0u);

    const auto& block1 = read_block(MAINNET_BLOCK1);
    chain::header block1_header;
    store_block_transactions(instance, block1, 1);
    auto block2 = read_block(MAINNET_BLOCK2);
    auto block2_header = block2.header();
    block2_header.set_previous_block_hash(genesis.hash());
    block2.set_header(block2_header);
    auto block3 = read_block(MAINNET_BLOCK3);
    auto block3_header = block3.header();
    block3_header.set_previous_block_hash(block2.hash());
    block3.set_header(block3_header);

    // Candidate header #1, validate it, then pop it from candidate,
    // and finally confirm it.
    BOOST_REQUIRE_EQUAL(instance.push_header(block1.header(), 1, 100), error::success);
    BOOST_REQUIRE_EQUAL(instance.invalidate(block1.header(), error::success), error::success);
    BOOST_REQUIRE_EQUAL(instance.pop_header(block1_header, 1), error::success);
    BOOST_REQUIRE_EQUAL(instance.push_block(block1, 1), error::success);

    test_heights(instance, 0u, 1u);

    // Candidate header #2, validate and associate transactions.
    BOOST_REQUIRE_EQUAL(instance.push_header(block2.header(), 1, 100), error::success);
    BOOST_REQUIRE_EQUAL(instance.invalidate(block2.header(), error::success), error::success);
    BOOST_REQUIRE_EQUAL(instance.update(block2, 1), error::success);

    test_heights(instance, 1u, 1u);

    // Candidate header #3, validate and associate transactions.
    BOOST_REQUIRE_EQUAL(instance.push_header(block3.header(), 2, 100), error::success);
    BOOST_REQUIRE_EQUAL(instance.invalidate(block3.header(), error::success), error::success);
    BOOST_REQUIRE_EQUAL(instance.update(block3, 2), error::success);

    test_heights(instance, 2u, 1u);


    const auto outgoing_blocks = std::make_shared<block_const_ptr_list>();
    const auto incoming_blocks = std::make_shared<const block_const_ptr_list>(block_const_ptr_list
    {
        std::make_shared<const message::block>(block2),
        std::make_shared<const message::block>(block3)
    });

    // setup ends
    
    BOOST_REQUIRE_EQUAL(instance.reorganize(config::checkpoint(genesis.hash(), 0), incoming_blocks, outgoing_blocks), error::success);

    // test conditions
    
    test_heights(instance, 2u, 2u);

    // Verify outgoing have right blocks.
    BOOST_REQUIRE_EQUAL(outgoing_blocks->size(), 1u);
    BOOST_REQUIRE(outgoing_blocks->front()->hash() == block1.hash());

    // Verify outgoing blocks are NOT in candidate index.
    BOOST_REQUIRE((instance.blocks().get(outgoing_blocks->front()->hash()).state() & block_state::candidate) == 0);

    // Verify incoming are blocks are in confirmed index.
    const auto state2 = instance.blocks().get((*incoming_blocks)[0]->hash()).state();
    BOOST_REQUIRE((state2 & block_state::confirmed) != 0);
    const auto state3 = instance.blocks().get((*incoming_blocks)[1]->hash()).state();
    BOOST_REQUIRE((state3 & block_state::confirmed) != 0);

    // Verify top block on confirmed index.
    BOOST_REQUIRE(instance.blocks().get(2, true).hash() == block3.hash());
}


BOOST_AUTO_TEST_SUITE_END()
