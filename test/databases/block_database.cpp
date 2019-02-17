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
#include <bitcoin/system.hpp>
#include <bitcoin/database.hpp>
#include "../utility/utility.hpp"

using namespace boost::system;
using namespace boost::filesystem;
using namespace bc;
using namespace bc::database;
using namespace bc::system;
using namespace bc::system::chain;

transaction random_tx(size_t fudge)
{
    static const auto settings = system::settings(
        system::config::settings::mainnet);
    static const chain::block genesis = settings.genesis_block;
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

    ~block_database_directory_setup_fixture()
    {
        test::clear_path(DIRECTORY);
    }
};

BOOST_FIXTURE_TEST_SUITE(block_database_tests, block_database_directory_setup_fixture)

BOOST_AUTO_TEST_CASE(block_database__get__not_found__success)
{
    const auto block_table = DIRECTORY "/block_table";
    const auto candidate_index = DIRECTORY "/candidate_index";
    const auto confirmed_index = DIRECTORY "/confirmed_index";
    const auto tx_index = DIRECTORY "/tx_index";

    test::create(block_table);
    test::create(candidate_index);
    test::create(confirmed_index);
    test::create(tx_index);
    block_database instance(block_table, candidate_index, confirmed_index, tx_index, 1, 1, 1, 1, 1000, 50);
    BOOST_REQUIRE(instance.create());

    BOOST_REQUIRE(!instance.get(0, true));
    BOOST_REQUIRE(!instance.get(0, false));
}

BOOST_AUTO_TEST_CASE(block_database__test)
{
    // TODO: replace.
    static const auto settings = system::settings(system::config::settings::mainnet);

    chain::block block0 = settings.genesis_block;
    block0.set_transactions(
    {
       random_tx(0),
       random_tx(1)
    });
    const auto h0 = block0.hash();

    BOOST_REQUIRE_EQUAL(block0.transactions().size(), 2);

    auto header1 = block0.header();
    header1.set_nonce(4);
    block block1(header1, { random_tx(2), random_tx(3), random_tx(4), random_tx(5) });
    const auto h1 = block1.hash();
    BOOST_REQUIRE(h0 != h1);

    auto header2 = block0.header();
    header2.set_nonce(110);
    block block2(header2, { random_tx(6), random_tx(7), random_tx(8), random_tx(9), random_tx(10) });
    const auto h2 = block2.hash();
    BOOST_REQUIRE(h0 != h2);

    auto header3 = block0.header();
    header3.set_nonce(88);
    block block3(header3, { random_tx(11), random_tx(12), random_tx(13) });
    const auto h3 = block3.hash();
    BOOST_REQUIRE(h0 != h3);

    auto header4a = block0.header();
    header4a.set_nonce(63);
    block block4a(header4a, { random_tx(14), random_tx(15), random_tx(16) });
    const auto h4a = block4a.hash();
    BOOST_REQUIRE(h0 != h4a);

    auto header4b = block0.header();
    header4b.set_nonce(633);
    block block4b(header4b, { random_tx(22), random_tx(23), random_tx(24) });
    const auto h4b = block4b.hash();
    BOOST_REQUIRE(h0 != h4b);

    auto header5a = block0.header();
    header5a.set_nonce(99);
    block block5a(header5a, { random_tx(17), random_tx(18), random_tx(19), random_tx(20), random_tx(21) });
    const auto h5a = block5a.hash();
    BOOST_REQUIRE(h0 != h5a);

    auto header5b = block0.header();
    header5b.set_nonce(222);
    block block5b(header5b, { random_tx(25), random_tx(26), random_tx(27), random_tx(28), random_tx(29) });
    const auto h5b = block5b.hash();
    BOOST_REQUIRE(h0 != h5b);

    const auto block_table = DIRECTORY "/block_table";
    const auto candidate_index = DIRECTORY "/candidate_index";
    const auto confirmed_index = DIRECTORY "/confirmed_index";
    const auto tx_index = DIRECTORY "/tx_index";

    test::create(block_table);
    test::create(candidate_index);
    test::create(confirmed_index);
    test::create(tx_index);
    block_database instance(block_table, candidate_index, confirmed_index, tx_index, 1, 1, 1, 1, 1000, 50);
    BOOST_REQUIRE(instance.create());

    size_t candidate_height = 0;
    size_t confirmed_height = 0;
    BOOST_REQUIRE(!instance.top(candidate_height, true));
    BOOST_REQUIRE(!instance.top(confirmed_height, false));

    BOOST_REQUIRE_EQUAL(block0.transactions().size(), 2);

    // Store blocks 0-4
    instance.store(block0.header(), 0, 0);
    instance.store(block1.header(), 1, 0);
    instance.store(block2.header(), 2, 0);
    instance.store(block3.header(), 3, 0);

    // without being indexed, heights are not set
    BOOST_REQUIRE(!instance.top(candidate_height, true));
    BOOST_REQUIRE(!instance.top(confirmed_height, false));

    // without being indexed, get returns false
    BOOST_REQUIRE(instance.get(h0).hash() == h0);
    BOOST_REQUIRE(!instance.get(0, true));

    // Add blocks 0-4 to candidate index
    instance.promote(h0, 0, true);
    instance.promote(h1, 1, true);
    instance.promote(h2, 2, true);
    instance.promote(h3, 3, true);

    // Check heights for candidate and confirmed index
    BOOST_REQUIRE(instance.top(candidate_height, true));
    BOOST_REQUIRE(!instance.top(confirmed_height, false));
    BOOST_REQUIRE_EQUAL(candidate_height, 3u);

    // block 0 is in candidate index, with candidate state
    BOOST_REQUIRE(instance.get(0, true));
    BOOST_REQUIRE_EQUAL(instance.get(0, true).state(), block_state::candidate);

    // unindex block 0 without clearing top should fail
    BOOST_REQUIRE(!instance.demote(h0, 2, true));

    // get 4a without indexing it should fail
    BOOST_REQUIRE(!instance.get(h4a));

    // unindex block 0 - 4 from candidate index
    instance.demote(h3, 3, true);
    instance.demote(h2, 2, true);
    instance.demote(h1, 1, true);
    instance.demote(h0, 0, true);

    //validate block 1
    BOOST_REQUIRE(instance.validate(h1, error::success));
    BOOST_REQUIRE(instance.get(0, true).state() | block_state::valid);

    // Add blocks 0-4 to confirmed index (with required validation)
    BOOST_REQUIRE(instance.validate(h0, error::success));
    instance.promote(h0, 0, false);
    instance.promote(h1, 1, false);
    BOOST_REQUIRE(instance.validate(h2, error::success));
    instance.promote(h2, 2, false);
    BOOST_REQUIRE(instance.validate(h3, error::success));
    instance.promote(h3, 3, false);

    // block 0 stored, not updated, tx_count is not yet set
    BOOST_REQUIRE_EQUAL(instance.get(h0).transaction_count(), 0);

    // Update blocks
    instance.update(block0);
    instance.update(block1);
    instance.update(block2);
    instance.update(block3);

    // Updated blocks set tx_count
    BOOST_REQUIRE_EQUAL(instance.get(h0).transaction_count(), 2);

    // block 0 is only in confirmed index, with valid & confirmed state
    BOOST_REQUIRE(!instance.get(0, true));
    BOOST_REQUIRE(instance.get(0, false));
    BOOST_REQUIRE_EQUAL(instance.get(0, false).state(), block_state::valid | block_state::confirmed);

    // Check heights for candidate and confirmed index
    BOOST_REQUIRE(!instance.top(candidate_height, true));
    BOOST_REQUIRE(instance.top(confirmed_height, false));
    BOOST_REQUIRE_EQUAL(confirmed_height, 3u);

    // Fetch block 0 by hash.
    const auto result0 = instance.get(h0);

    auto it0 = result0.begin();
    BOOST_REQUIRE(it0 != result0.end());
    BOOST_REQUIRE_EQUAL(*it0++, 0u);
    BOOST_REQUIRE_EQUAL(*it0++, 1u);
    BOOST_REQUIRE(it0 == result0.end());

    // Fetch block 2 by hash.
    const auto result2 = instance.get(h2);
    BOOST_REQUIRE(result2);
    BOOST_REQUIRE(result2.hash() == h2);

    auto it2 = result2.begin();
    BOOST_REQUIRE(it2 != result2.end());
    BOOST_REQUIRE_EQUAL(*it2++, 6u);
    BOOST_REQUIRE_EQUAL(*it2++, 7u);
    BOOST_REQUIRE_EQUAL(*it2++, 8u);
    BOOST_REQUIRE_EQUAL(*it2++, 9u);
    BOOST_REQUIRE_EQUAL(*it2++, 10u);
    BOOST_REQUIRE(it2 == result2.end());

    // no metadata for missing blocks
    instance.get_header_metadata(block4a.header());

    BOOST_REQUIRE(!block4b.header().metadata.candidate);
    BOOST_REQUIRE(!block4b.header().metadata.confirmed);

    // Try a fork event.
    instance.store(block4a.header(), 4, 0);
    instance.store(block5a.header(), 5, 0);
    instance.update(block4a);
    instance.update(block5a);

    BOOST_REQUIRE(instance.validate(h4a, error::success));
    instance.promote(h4a, 4, false);
    BOOST_REQUIRE(instance.validate(h5a, error::success));
    instance.promote(h5a, 5, false);

    // Fetch blocks 4/5.
    const auto result4a = instance.get(h4a);
    const auto result5a = instance.get(h5a);

    // Unlink blocks 4a/5a.
    BOOST_REQUIRE(instance.top(confirmed_height, false));
    BOOST_REQUIRE_EQUAL(confirmed_height, 5u);
    instance.demote(h5a, 5, false);
    instance.demote(h4a, 4, false);
    BOOST_REQUIRE(instance.top(confirmed_height, false));
    BOOST_REQUIRE_EQUAL(confirmed_height, 3u);

    // Block 3 exists.
    const auto result3 = instance.get(3, false);
    BOOST_REQUIRE(result3);

    // Blocks 4a/5a are missing (verify index guard).
    const auto result4 = instance.get(4, false);
    BOOST_REQUIRE(!result4);
    const auto result5 = instance.get(5, false);
    BOOST_REQUIRE(!result5);

    // Add new blocks 4b/5b.
    instance.store(block4b.header(), 4, 0);
    instance.store(block5b.header(), 5, 0);
    BOOST_REQUIRE(instance.validate(h4b, error::success));
    instance.promote(h4b, 4, false);
    BOOST_REQUIRE(instance.validate(h5b, error::success));
    instance.promote(h5b, 5, false);
    instance.update(block4b);
    instance.update(block5b);

    BOOST_REQUIRE(instance.top(confirmed_height, false));
    BOOST_REQUIRE_EQUAL(confirmed_height, 5u);

    // Fetch blocks 4b/5b.
    const auto result4b = instance.get(4, false);
    BOOST_REQUIRE(result4b);
    BOOST_REQUIRE(result4b.hash() == h4b);
    const auto result5b = instance.get(5, false);
    BOOST_REQUIRE(result5b);
    BOOST_REQUIRE(result5b.hash() == h5b);

    // Test also fetch by hash.
    const auto result_h5b = instance.get(h5b);
    BOOST_REQUIRE(result_h5b);
    BOOST_REQUIRE(result_h5b.hash() == h5b);

    // header metadata
    instance.get_header_metadata(block4b.header());
    BOOST_REQUIRE_EQUAL(block4b.header().metadata.error, error::success);
    BOOST_REQUIRE(block4b.header().metadata.exists);
    BOOST_REQUIRE(block4b.header().metadata.populated);
    BOOST_REQUIRE(block4b.header().metadata.validated);
    BOOST_REQUIRE(!block4b.header().metadata.candidate);
    BOOST_REQUIRE(block4b.header().metadata.confirmed);

    instance.commit();
}

BOOST_AUTO_TEST_SUITE_END()
