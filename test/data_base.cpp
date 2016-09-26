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

using namespace boost::system;
using namespace boost::filesystem;
using namespace bc;
using namespace bc::database;
using namespace bc::wallet;

void test_block_exists(const data_base& interface,
    const size_t height, const chain::block block0)
{
    const hash_digest blk_hash = block0.header.hash();
    auto r0 = interface.blocks.get(height);
    auto r0_byhash = interface.blocks.get(blk_hash);

    BOOST_REQUIRE(r0);
    BOOST_REQUIRE(r0_byhash);
    BOOST_REQUIRE(r0.header().hash() == blk_hash);
    BOOST_REQUIRE(r0_byhash.header().hash() == blk_hash);
    BOOST_REQUIRE_EQUAL(r0.height(), height);
    BOOST_REQUIRE_EQUAL(r0_byhash.height(), height);
    BOOST_REQUIRE_EQUAL(r0.transaction_count(), block0.transactions.size());
    BOOST_REQUIRE_EQUAL(r0_byhash.transaction_count(), block0.transactions.size());

    for (size_t i = 0; i < block0.transactions.size(); ++i)
    {
        const chain::transaction& tx = block0.transactions[i];
        const hash_digest tx_hash = tx.hash();
        BOOST_REQUIRE(r0.transaction_hash(i) == tx_hash);
        BOOST_REQUIRE(r0_byhash.transaction_hash(i) == tx_hash);
        auto r0_tx = interface.transactions.get(tx_hash);
        BOOST_REQUIRE(r0_tx);
        BOOST_REQUIRE(r0_byhash);
        BOOST_REQUIRE(r0_tx.transaction().hash() == tx_hash);
        BOOST_REQUIRE_EQUAL(r0_tx.height(), height);
        BOOST_REQUIRE_EQUAL(r0_tx.position(), i);

        if (!tx.is_coinbase())
        {
            for (uint32_t j = 0; j < tx.inputs.size(); ++j)
            {
                const auto& input = tx.inputs[j];
                chain::input_point spend{ tx_hash, j };

                BOOST_REQUIRE_EQUAL(spend.index, j);

                auto r0_spend = interface.spends.get(input.previous_output);
                BOOST_REQUIRE(r0_spend.is_valid());
                BOOST_REQUIRE(r0_spend.hash == spend.hash);
                BOOST_REQUIRE_EQUAL(r0_spend.index, spend.index);

                const auto address = payment_address::extract(input.script);

                if (!address)
                    continue;

                auto history = interface.history.get(address.hash(), 0, 0);
                auto found = false;

                for (const auto& row: history)
                {
                    if (row.point.hash == spend.hash &&
                        row.point.index == spend.index)
                    {
                        BOOST_REQUIRE_EQUAL(row.height, height);
                        found = true;
                        break;
                    }
                }

                BOOST_REQUIRE(found);
            }
        }

        for (size_t j = 0; j < tx.outputs.size(); ++j)
        {
            const auto& output = tx.outputs[j];
            chain::output_point outpoint{ tx_hash, static_cast<uint32_t>(j) };
            const auto address = payment_address::extract(output.script);

            if (!address)
                continue;

            auto history = interface.history.get(address.hash(), 0, 0);
            auto found = false;

            for (const auto& row: history)
            {
                BOOST_REQUIRE(row.point.is_valid());

                if (row.point.hash == outpoint.hash &&
                    row.point.index == outpoint.index)
                {
                    BOOST_REQUIRE_EQUAL(row.height, height);
                    BOOST_REQUIRE_EQUAL(row.value, output.value);
                    found = true;
                    break;
                }
            }

            BOOST_REQUIRE(found);
        }
    }
}

void test_block_not_exists(const data_base& interface,
    const chain::block block0)
{
    ////const hash_digest blk_hash = hash_block_header(block0.header);
    ////auto r0_byhash = interface.blocks.get(blk_hash);
    ////BOOST_REQUIRE(!r0_byhash);
    for (size_t i = 0; i < block0.transactions.size(); ++i)
    {
        const chain::transaction& tx = block0.transactions[i];
        const hash_digest tx_hash = tx.hash();

        if (!tx.is_coinbase())
        {
            for (size_t j = 0; j < tx.inputs.size(); ++j)
            {
                const auto& input = tx.inputs[j];
                chain::input_point spend{ tx_hash, static_cast<uint32_t>(j) };
                auto r0_spend = interface.spends.get(input.previous_output);
                BOOST_REQUIRE(!r0_spend.is_valid());

                const auto address = payment_address::extract(input.script);
                if (!address)
                    continue;

                auto history = interface.history.get(address.hash(), 0, 0);
                auto found = false;

                for (const auto& row : history)
                {
                    if (row.point.hash == spend.hash &&
                        row.point.index == spend.index)
                    {
                        found = true;
                        break;
                    }
                }

                BOOST_REQUIRE(!found);
            }
        }

        for (size_t j = 0; j < tx.outputs.size(); ++j)
        {
            const auto& output = tx.outputs[j];
            chain::output_point outpoint{ tx_hash, static_cast<uint32_t>(j) };
            const auto address = payment_address::extract(output.script);
            if (!address)
                continue;

            auto history = interface.history.get(address.hash(), 0, 0);
            auto found = false;

            for (const auto& row : history)
            {
                if (row.point.hash == outpoint.hash &&
                    row.point.index == outpoint.index)
                {
                    found = true;
                    break;
                }
            }

            BOOST_REQUIRE(!found);
        }
    }
}

chain::block read_block(const std::string hex)
{
    data_chunk data;
    BOOST_REQUIRE(decode_base16(data, hex));
    chain::block result;
    BOOST_REQUIRE(result.from_data(data));
    return result;
}

void compare_blocks(const chain::block& popped, const chain::block& original)
{
    BOOST_REQUIRE(popped.header.hash() == original.header.hash());
    BOOST_REQUIRE(popped.transactions.size() == original.transactions.size());

    for (size_t i = 0; i < popped.transactions.size(); ++i)
    {
        BOOST_REQUIRE(popped.transactions[i].hash() ==
            original.transactions[i].hash());
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

#define MAINNET_BLOCK1 "010000006fe28c0ab6f1b372c1a6a246ae63f74f931e8365e15a089c68d6190000000000982051fd1e4ba744bbbe680e1fee14677ba1a3c3540bf7b1cdb606e857233e0e61bc6649ffff001d01e362990101000000010000000000000000000000000000000000000000000000000000000000000000ffffffff0704ffff001d0104ffffffff0100f2052a0100000043410496b538e853519c726a2c91e61ec11600ae1390813a627c66fb8be7947be63c52da7589379515d4e0a604f8141781e62294721166bf621e73a82cbf2342c858eeac00000000"
#define MAINNET_BLOCK2 "010000004860eb18bf1b1620e37e9490fc8a427514416fd75159ab86688e9a8300000000d5fdcc541e25de1c7a5addedf24858b8bb665c9f36ef744ee42c316022c90f9bb0bc6649ffff001d08d2bd610101000000010000000000000000000000000000000000000000000000000000000000000000ffffffff0704ffff001d010bffffffff0100f2052a010000004341047211a824f55b505228e4c3d5194c1fcfaa15a456abdf37f9b9d97a4040afc073dee6c89064984f03385237d92167c13e236446b417ab79a0fcae412ae3316b77ac00000000"
#define MAINNET_BLOCK3 "01000000bddd99ccfda39da1b108ce1a5d70038d0a967bacb68b6b63065f626a0000000044f672226090d85db9a9f2fbfe5f0f9609b387af7be5b7fbb7a1767c831c9e995dbe6649ffff001d05e0ed6d0101000000010000000000000000000000000000000000000000000000000000000000000000ffffffff0704ffff001d010effffffff0100f2052a0100000043410494b9d3e76c5b1629ecf97fff95d7a4bbdac87cc26099ada28066c6ff1eb9191223cd897194a08d0c2726c5747f1db49e8cf90e75dc3e3550ae9b30086f3cd5aaac00000000"
////#define MAINNET_BLOCK4 "010000004944469562ae1c2c74d9a535e00b6f3e40ffbad4f2fda3895501b582000000007a06ea98cd40ba2e3288262b28638cec5337c1456aaf5eedc8e9e5a20f062bdf8cc16649ffff001d2bfee0a90101000000010000000000000000000000000000000000000000000000000000000000000000ffffffff0704ffff001d011affffffff0100f2052a01000000434104184f32b212815c6e522e66686324030ff7e5bf08efb21f8b00614fb7690e19131dd31304c54f37baa40db231c918106bb9fd43373e37ae31a0befc6ecaefb867ac00000000"
////#define MAINNET_BLOCK5 "0100000085144a84488ea88d221c8bd6c059da090e88f8a2c99690ee55dbba4e00000000e11c48fecdd9e72510ca84f023370c9a38bf91ac5cae88019bee94d24528526344c36649ffff001d1d03e4770101000000010000000000000000000000000000000000000000000000000000000000000000ffffffff0704ffff001d0120ffffffff0100f2052a0100000043410456579536d150fbce94ee62b47db2ca43af0a730a0467ba55c79e2a7ec9ce4ad297e35cdbb8e42a4643a60eef7c9abee2f5822f86b1da242d9c2301c431facfd8ac00000000"

// TODO: parameterize bucket sizes to control test cost.
BOOST_AUTO_TEST_CASE(data_base__pushpop__test)
{
    std::cout << "begin data_base pushpop test" << std::endl;

    database::settings settings;
    settings.directory = { DIRECTORY };
    settings.stealth_start_height = 0;

    const auto block0 = chain::block::genesis_mainnet();
    boost::filesystem::create_directory(settings.directory);
    BOOST_REQUIRE(data_base::initialize(settings.directory, block0));

    data_base instance(settings);
    BOOST_REQUIRE(instance.start());

    size_t height = 42;
    BOOST_REQUIRE(instance.blocks.top(height));
    BOOST_REQUIRE_EQUAL(height, 0);

    test_block_exists(instance, 0, block0);

    std::cout << "pushpop: block #1" << std::endl;

    // Block #1
    chain::block block1 = read_block(MAINNET_BLOCK1);
    test_block_not_exists(instance, block1);
    BOOST_REQUIRE(instance.push(block1, 1));

    test_block_exists(instance, 1, block1);
    BOOST_REQUIRE(instance.blocks.top(height));
    BOOST_REQUIRE_EQUAL(height, 1u);

    std::cout << "pushpop: block #2" << std::endl;

    // Block #2
    chain::block block2 = read_block(MAINNET_BLOCK2);
    test_block_not_exists(instance, block2);
    instance.push(block2, 2);
    test_block_exists(instance, 2, block2);

    BOOST_REQUIRE(instance.blocks.top(height));
    BOOST_REQUIRE_EQUAL(height, 2u);

    std::cout << "pushpop: block #3" << std::endl;

    // Block #183
    chain::block block3 = read_block(MAINNET_BLOCK3);
    test_block_not_exists(instance, block3);
    instance.push(block3, 3);
    test_block_exists(instance, 3, block3);

    std::cout << "pushpop: cleanup tests" << std::endl;

    BOOST_REQUIRE(instance.blocks.top(height));
    BOOST_REQUIRE_EQUAL(height, 3u);

    chain::block block3_popped = instance.pop();
    BOOST_REQUIRE(instance.blocks.top(height));
    BOOST_REQUIRE_EQUAL(height, 2u);
    compare_blocks(block3_popped, block3);

    test_block_not_exists(instance, block3);
    test_block_exists(instance, 2, block2);
    test_block_exists(instance, 1, block1);
    test_block_exists(instance, 0, block0);

    chain::block block2_popped = instance.pop();
    BOOST_REQUIRE(instance.blocks.top(height));
    BOOST_REQUIRE_EQUAL(height, 1u);
    compare_blocks(block2_popped, block2);

    test_block_not_exists(instance, block3);
    test_block_not_exists(instance, block2);
    test_block_exists(instance, 1, block1);
    test_block_exists(instance, 0, block0);

    std::cout << "end pushpop test" << std::endl;
}

BOOST_AUTO_TEST_SUITE_END()
