/**
 * Copyright (c) 2011-2015 libbitcoin developers (see AUTHORS)
 *
 * This file is part of libbitcoin.
 *
 * libbitcoin is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License with
 * additional permissions to the one published by the Free Software
 * Foundation, either version 3 of the License, or (at your option)
 * any later version. For more information see LICENSE.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>
#include <bitcoin/database.hpp>

using namespace bc;
using namespace bc::chain;
using namespace bc::database;
using namespace bc::wallet;
using namespace boost::system;
using namespace boost::filesystem;

void test_block_exists(const data_base& interface,
    const size_t height, const block block0, bool indexed)
{
    const hash_digest blk_hash = block0.header().hash();
    auto r0 = interface.blocks().get(height);
    auto r0_byhash = interface.blocks().get(blk_hash);

    BOOST_REQUIRE(r0);
    BOOST_REQUIRE(r0_byhash);
    BOOST_REQUIRE(r0.header().hash() == blk_hash);
    BOOST_REQUIRE(r0_byhash.header().hash() == blk_hash);
    BOOST_REQUIRE_EQUAL(r0.height(), height);
    BOOST_REQUIRE_EQUAL(r0_byhash.height(), height);
    BOOST_REQUIRE_EQUAL(r0.transaction_count(), block0.transactions().size());
    BOOST_REQUIRE_EQUAL(r0_byhash.transaction_count(), block0.transactions().size());

    for (size_t i = 0; i < block0.transactions().size(); ++i)
    {
        const transaction& tx = block0.transactions()[i];
        const hash_digest tx_hash = tx.hash();
        BOOST_REQUIRE(r0.transaction_hash(i) == tx_hash);
        BOOST_REQUIRE(r0_byhash.transaction_hash(i) == tx_hash);

        auto r0_tx = interface.transactions().get(tx_hash, max_size_t);
        BOOST_REQUIRE(r0_tx);
        BOOST_REQUIRE(r0_byhash);
        BOOST_REQUIRE(r0_tx.transaction().hash() == tx_hash);
        BOOST_REQUIRE_EQUAL(r0_tx.height(), height);
        BOOST_REQUIRE_EQUAL(r0_tx.position(), i);

        if (!tx.is_coinbase())
        {
            for (uint32_t j = 0; j < tx.inputs().size(); ++j)
            {
                const auto& input = tx.inputs()[j];
                input_point spend{ tx_hash, j };
                BOOST_REQUIRE_EQUAL(spend.index(), j);

                auto r0_spend = interface.spends().get(input.previous_output());
                BOOST_REQUIRE(r0_spend.is_valid());
                BOOST_REQUIRE(r0_spend.hash() == spend.hash());
                BOOST_REQUIRE_EQUAL(r0_spend.index(), spend.index());

                if (!indexed)
                    continue;

                const auto address = payment_address::extract(input.script());

                if (!address)
                    continue;

                auto history = interface.history().get(address.hash(), 0, 0);
                auto found = false;

                for (const auto& row: history)
                {
                    if (row.point.hash() == spend.hash() &&
                        row.point.index() == spend.index())
                    {
                        BOOST_REQUIRE_EQUAL(row.height, height);
                        found = true;
                        break;
                    }
                }

                BOOST_REQUIRE(found);
            }
        }

        if (!indexed)
            return;

        for (size_t j = 0; j < tx.outputs().size(); ++j)
        {
            const auto& output = tx.outputs()[j];
            output_point outpoint{ tx_hash, static_cast<uint32_t>(j) };
            const auto address = payment_address::extract(output.script());

            if (!address)
                continue;

            auto history = interface.history().get(address.hash(), 0, 0);
            auto found = false;

            for (const auto& row: history)
            {
                BOOST_REQUIRE(row.point.is_valid());

                if (row.point.hash() == outpoint.hash() &&
                    row.point.index() == outpoint.index())
                {
                    BOOST_REQUIRE_EQUAL(row.height, height);
                    BOOST_REQUIRE_EQUAL(row.value, output.value());
                    found = true;
                    break;
                }
            }

            BOOST_REQUIRE(found);
        }
    }
}

void test_block_not_exists(const data_base& interface,
    const block block0, bool indexed)
{
    ////const hash_digest blk_hash = hash_block_header(block0.header);
    ////auto r0_byhash = interface.blocks().get(blk_hash);
    ////BOOST_REQUIRE(!r0_byhash);
    for (size_t i = 0; i < block0.transactions().size(); ++i)
    {
        const transaction& tx = block0.transactions()[i];
        const hash_digest tx_hash = tx.hash();

        if (!tx.is_coinbase())
        {
            for (size_t j = 0; j < tx.inputs().size(); ++j)
            {
                const auto& input = tx.inputs()[j];
                input_point spend{ tx_hash, static_cast<uint32_t>(j) };
                auto r0_spend = interface.spends().get(input.previous_output());
                BOOST_REQUIRE(!r0_spend.is_valid());

                if (!indexed)
                    continue;

                const auto address = payment_address::extract(input.script());

                if (!address)
                    continue;

                auto history = interface.history().get(address.hash(), 0, 0);
                auto found = false;

                for (const auto& row: history)
                {
                    if (row.point.hash() == spend.hash() &&
                        row.point.index() == spend.index())
                    {
                        found = true;
                        break;
                    }
                }

                BOOST_REQUIRE(!found);
            }
        }

        if (!indexed)
            return;

        for (size_t j = 0; j < tx.outputs().size(); ++j)
        {
            const auto& output = tx.outputs()[j];
            output_point outpoint{ tx_hash, static_cast<uint32_t>(j) };
            const auto address = payment_address::extract(output.script());

            if (!address)
                continue;

            auto history = interface.history().get(address.hash(), 0, 0);
            auto found = false;

            for (const auto& row: history)
            {
                if (row.point.hash() == outpoint.hash() &&
                    row.point.index() == outpoint.index())
                {
                    found = true;
                    break;
                }
            }

            BOOST_REQUIRE(!found);
        }
    }
}

block read_block(const std::string hex)
{
    data_chunk data;
    BOOST_REQUIRE(decode_base16(data, hex));
    block result;
    BOOST_REQUIRE(result.from_data(data));
    return result;
}

void compare_blocks(const block& popped, const block& original)
{
    BOOST_REQUIRE(popped.header().hash() == original.header().hash());
    BOOST_REQUIRE(popped.transactions().size() == original.transactions().size());

    for (size_t i = 0; i < popped.transactions().size(); ++i)
    {
        BOOST_REQUIRE(popped.transactions()[i].hash() ==
            original.transactions()[i].hash());
    }
}

#define DIRECTORY "data_base"

class data_base_directory_and_thread_priority_setup_fixture
{
public:
    data_base_directory_and_thread_priority_setup_fixture()
    {
        error_code ec;
        remove_all(DIRECTORY, ec);
        BOOST_REQUIRE(create_directories(DIRECTORY, ec));
        set_thread_priority(thread_priority::lowest);
    }

    ////~data_base_directory_and_thread_priority_setup_fixture()
    ////{
    ////    error_code ec;
    ////    remove_all(DIRECTORY, ec);
    ////    set_thread_priority(thread_priority::normal);
    ////}
};

BOOST_FIXTURE_TEST_SUITE(data_base_tests, data_base_directory_and_thread_priority_setup_fixture)

#define MAINNET_BLOCK1 \
"010000006fe28c0ab6f1b372c1a6a246ae63f74f931e8365e15a089c68d6190000000000982" \
"051fd1e4ba744bbbe680e1fee14677ba1a3c3540bf7b1cdb606e857233e0e61bc6649ffff00" \
"1d01e3629901010000000100000000000000000000000000000000000000000000000000000" \
"00000000000ffffffff0704ffff001d0104ffffffff0100f2052a0100000043410496b538e8" \
"53519c726a2c91e61ec11600ae1390813a627c66fb8be7947be63c52da7589379515d4e0a60" \
"4f8141781e62294721166bf621e73a82cbf2342c858eeac00000000"

#define MAINNET_BLOCK2 \
"010000004860eb18bf1b1620e37e9490fc8a427514416fd75159ab86688e9a8300000000d5f" \
"dcc541e25de1c7a5addedf24858b8bb665c9f36ef744ee42c316022c90f9bb0bc6649ffff00" \
"1d08d2bd6101010000000100000000000000000000000000000000000000000000000000000" \
"00000000000ffffffff0704ffff001d010bffffffff0100f2052a010000004341047211a824" \
"f55b505228e4c3d5194c1fcfaa15a456abdf37f9b9d97a4040afc073dee6c89064984f03385" \
"237d92167c13e236446b417ab79a0fcae412ae3316b77ac00000000"

#define MAINNET_BLOCK3 \
"01000000bddd99ccfda39da1b108ce1a5d70038d0a967bacb68b6b63065f626a0000000044f" \
"672226090d85db9a9f2fbfe5f0f9609b387af7be5b7fbb7a1767c831c9e995dbe6649ffff00" \
"1d05e0ed6d01010000000100000000000000000000000000000000000000000000000000000" \
"00000000000ffffffff0704ffff001d010effffffff0100f2052a0100000043410494b9d3e7" \
"6c5b1629ecf97fff95d7a4bbdac87cc26099ada28066c6ff1eb9191223cd897194a08d0c272" \
"6c5747f1db49e8cf90e75dc3e3550ae9b30086f3cd5aaac00000000"
 
// TODO: parameterize bucket sizes to control test cost.
BOOST_AUTO_TEST_CASE(data_base__pushpop__test)
{
    std::cout << "begin data_base pushpop test" << std::endl;

    create_directory(DIRECTORY);
    const auto block0 = block::genesis_mainnet();

    settings configuration;

    configuration.directory = DIRECTORY;
    configuration.file_growth_rate = 42;
    configuration.index_start_height = 0;
    configuration.block_table_buckets = 42;
    configuration.transaction_table_buckets = 42;
    configuration.spend_table_buckets = 42;
    configuration.history_table_buckets = 42;

    // If index_height is set to anything other than 0 or max it can cause
    // false negatives since it excludes entries below the specified height.
    auto indexed = configuration.index_start_height < store::without_indexes;

    data_base instance(configuration);
    BOOST_REQUIRE(instance.create(block0));

    size_t height = 42;
    BOOST_REQUIRE(instance.blocks().top(height));
    BOOST_REQUIRE_EQUAL(height, 0);
    test_block_exists(instance, 0, block0, indexed);

    std::cout << "pushpop: block #1" << std::endl;

    // Block #1
    block block1 = read_block(MAINNET_BLOCK1);
    test_block_not_exists(instance, block1, indexed);
    BOOST_REQUIRE(instance.push(block1, 1));

    test_block_exists(instance, 1, block1, indexed);
    BOOST_REQUIRE(instance.blocks().top(height));
    BOOST_REQUIRE_EQUAL(height, 1u);

    std::cout << "pushpop: block #2" << std::endl;

    // Block #2
    block block2 = read_block(MAINNET_BLOCK2);
    test_block_not_exists(instance, block2, indexed);
    instance.push(block2, 2);
    test_block_exists(instance, 2, block2, indexed);

    BOOST_REQUIRE(instance.blocks().top(height));
    BOOST_REQUIRE_EQUAL(height, 2u);

    std::cout << "pushpop: block #3" << std::endl;

    // Block #3
    block block3 = read_block(MAINNET_BLOCK3);
    test_block_not_exists(instance, block3, indexed);
    instance.push(block::list{ block3 }, 3);
    test_block_exists(instance, 3, block3, indexed);

    std::cout << "pushpop: cleanup tests" << std::endl;

    block::list block3_popped;
    BOOST_REQUIRE(instance.blocks().top(height));
    BOOST_REQUIRE_EQUAL(height, 3u);
    const auto& previous3 = block3.header().previous_block_hash();
    BOOST_REQUIRE(instance.pop_above(block3_popped, previous3));
    BOOST_REQUIRE(instance.blocks().top(height));
    BOOST_REQUIRE_EQUAL(height, 2u);

    compare_blocks(block3_popped.front(), block3);
    test_block_not_exists(instance, block3, indexed);
    test_block_exists(instance, 2, block2, indexed);
    test_block_exists(instance, 1, block1, indexed);
    test_block_exists(instance, 0, block0, indexed);

    block::list block2_popped;
    const auto& previous2 = block2.header().previous_block_hash();
    BOOST_REQUIRE(instance.pop_above(block2_popped, previous2));
    BOOST_REQUIRE(instance.blocks().top(height));
    BOOST_REQUIRE_EQUAL(height, 1u);

    compare_blocks(block2_popped.front(), block2);
    test_block_not_exists(instance, block3, indexed);
    test_block_not_exists(instance, block2, indexed);
    test_block_exists(instance, 1, block1, indexed);
    test_block_exists(instance, 0, block0, indexed);

    std::cout << "end pushpop test" << std::endl;
}

BOOST_AUTO_TEST_SUITE_END()
