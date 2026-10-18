/**
 * Copyright (c) 2011-2026 libbitcoin developers
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
#include "../../test.hpp"
#include "../../mocks/blocks.hpp"
#include "../../mocks/chunk_store.hpp"

BOOST_FIXTURE_TEST_SUITE(query_wire_reader_reader_tests, test::directory_setup_fixture)

// A block of enough txs that the turbo writer uses more than one range.
static system::chain::block many_tx_block() NOEXCEPT
{
    using namespace system;
    const auto version = test::tx4.version();
    const auto inputs = test::tx4.inputs_ptr();
    const auto outputs = test::tx4.outputs_ptr();
    chain::transactions txs{};
    txs.reserve(1100);
    for (uint32_t locktime{}; locktime < 1100; ++locktime)
        txs.emplace_back(version, inputs, outputs, locktime);

    return { test::block1a.header(), txs };
}

// is_witness_match

BOOST_AUTO_TEST_CASE(query_wire_reader__is_witness_match__non_segregated__true)
{
    using namespace system;
    database::settings settings{};
    settings.path = TEST_DIRECTORY;
    test::store_t store{ settings };
    test::query_t query{ store };
    BOOST_CHECK(!store.create(test::events_handler));
    BOOST_CHECK(test::setup_three_block_witness_store(query));

    const auto data = test::genesis.transactions_ptr()->at(0)->to_data(true);
    stream::in::fast istream{ data };
    read::bytes::fast reader{ istream };
    const chain::view::transaction view{ reader, data, true, true };
    BOOST_REQUIRE(view.is_valid());
    BOOST_REQUIRE(!view.is_segregated());
    BOOST_CHECK(query.is_witness_match(0, view));
    BOOST_CHECK(!store.close(test::events_handler));
}

BOOST_AUTO_TEST_CASE(query_wire_reader__is_witness_match__same_witness__true)
{
    using namespace system;
    database::settings settings{};
    settings.path = TEST_DIRECTORY;
    test::store_t store{ settings };
    test::query_t query{ store };
    BOOST_CHECK(!store.create(test::events_handler));
    BOOST_CHECK(test::setup_three_block_witness_store(query));

    const auto data = test::block2a.transactions_ptr()->at(1)->to_data(true);
    stream::in::fast istream{ data };
    read::bytes::fast reader{ istream };
    const chain::view::transaction view{ reader, data, false, true };
    BOOST_REQUIRE(view.is_valid());
    BOOST_REQUIRE(view.is_segregated());
    BOOST_CHECK(query.is_witness_match(3, view));
    BOOST_CHECK(!store.close(test::events_handler));
}

BOOST_AUTO_TEST_CASE(query_wire_reader__is_witness_match__changed_witness__false)
{
    using namespace system;
    database::settings settings{};
    settings.path = TEST_DIRECTORY;
    test::store_t store{ settings };
    test::query_t query{ store };
    BOOST_CHECK(!store.create(test::events_handler));
    BOOST_CHECK(test::setup_three_block_witness_store(query));

    // Last witness byte precedes the four byte locktime.
    auto data = test::block2a.transactions_ptr()->at(1)->to_data(true);
    data.at(data.size() - 5) ^= 0xff;
    stream::in::fast istream{ data };
    read::bytes::fast reader{ istream };
    const chain::view::transaction view{ reader, data, false, true };
    BOOST_REQUIRE(view.is_valid());
    BOOST_REQUIRE(view.is_segregated());
    BOOST_REQUIRE_EQUAL(view.hash(false), test::block2a.transactions_ptr()->at(1)->hash(false));
    BOOST_CHECK(!query.is_witness_match(3, view));
    BOOST_CHECK(!store.close(test::events_handler));
}

BOOST_AUTO_TEST_CASE(query_wire_reader__is_witness_match__other_size__false)
{
    using namespace system;
    database::settings settings{};
    settings.path = TEST_DIRECTORY;
    test::store_t store{ settings };
    test::query_t query{ store };
    BOOST_CHECK(!store.create(test::events_handler));
    BOOST_CHECK(test::setup_three_block_witness_store(query));

    const auto data = test::block2a.transactions_ptr()->at(1)->to_data(true);
    stream::in::fast istream{ data };
    read::bytes::fast reader{ istream };
    const chain::view::transaction view{ reader, data, false, true };
    BOOST_REQUIRE(view.is_valid());
    BOOST_CHECK(!query.is_witness_match(0, view));
    BOOST_CHECK(!store.close(test::events_handler));
}

// get_wire_header

BOOST_AUTO_TEST_CASE(query_wire_reader__get_wire_header__genesis_and_not__expected)
{
    using namespace system;
    database::settings settings{};
    settings.path = TEST_DIRECTORY;
    test::store_t store{ settings };
    test::query_t query{ store };
    BOOST_CHECK(!store.create(test::events_handler));
    BOOST_CHECK(test::setup_three_block_store(query));
    BOOST_CHECK_EQUAL(query.get_wire_header(0), test::genesis.header().to_data());
    BOOST_CHECK_EQUAL(query.get_wire_header(1), test::block1.header().to_data());
    BOOST_CHECK_EQUAL(query.get_wire_header(2), test::block2.header().to_data());
    BOOST_CHECK(!store.close(test::events_handler));
}

// get_wire_tx

BOOST_AUTO_TEST_CASE(query_wire_reader__get_wire_tx__genesis_and_not__expected)
{
    using namespace system;
    database::settings settings{};
    settings.path = TEST_DIRECTORY;
    test::store_t store{ settings };
    test::query_t query{ store };
    BOOST_CHECK(!store.create(test::events_handler));
    BOOST_CHECK(test::setup_three_block_store(query));
    BOOST_CHECK_EQUAL(query.get_wire_tx(0, true), test::genesis.transactions_ptr()->front()->to_data(true));
    BOOST_CHECK_EQUAL(query.get_wire_tx(1, true), test::block1.transactions_ptr()->front()->to_data(true));
    BOOST_CHECK_EQUAL(query.get_wire_tx(2, true), test::block2.transactions_ptr()->front()->to_data(true));
    BOOST_CHECK(!store.close(test::events_handler));
}

BOOST_AUTO_TEST_CASE(query_wire_reader__get_wire_tx__witness_true__expected)
{
    using namespace system;
    database::settings settings{};
    settings.path = TEST_DIRECTORY;
    test::store_t store{ settings };
    test::query_t query{ store };
    BOOST_CHECK(!store.create(test::events_handler));
    BOOST_CHECK(test::setup_three_block_witness_store(query));
    BOOST_CHECK_EQUAL(query.get_wire_tx(0, true), test::genesis.transactions_ptr()->at(0)->to_data(true));
    BOOST_CHECK_EQUAL(query.get_wire_tx(1, true), test::block1a.transactions_ptr()->at(0)->to_data(true));
    BOOST_CHECK_EQUAL(query.get_wire_tx(2, true), test::block2a.transactions_ptr()->at(0)->to_data(true));
    BOOST_CHECK_EQUAL(query.get_wire_tx(3, true), test::block2a.transactions_ptr()->at(1)->to_data(true));
    BOOST_CHECK(!store.close(test::events_handler));
}

BOOST_AUTO_TEST_CASE(query_wire_reader__get_wire_tx__witness_false__expected)
{
    using namespace system;
    database::settings settings{};
    settings.path = TEST_DIRECTORY;
    test::store_t store{ settings };
    test::query_t query{ store };
    BOOST_CHECK(!store.create(test::events_handler));
    BOOST_CHECK(test::setup_three_block_witness_store(query));
    BOOST_CHECK_EQUAL(query.get_wire_tx(0, false), test::genesis.transactions_ptr()->at(0)->to_data(false));
    BOOST_CHECK_EQUAL(query.get_wire_tx(1, false), test::block1a.transactions_ptr()->at(0)->to_data(false));
    BOOST_CHECK_EQUAL(query.get_wire_tx(2, false), test::block2a.transactions_ptr()->at(0)->to_data(false));
    BOOST_CHECK_EQUAL(query.get_wire_tx(3, false), test::block2a.transactions_ptr()->at(1)->to_data(false));
    BOOST_CHECK(!store.close(test::events_handler));
}

// get_wire_block

BOOST_AUTO_TEST_CASE(query_wire_reader__get_wire_block__genesis_and_not__expected)
{
    using namespace system;
    database::settings settings{};
    settings.path = TEST_DIRECTORY;
    test::store_t store{ settings };
    test::query_t query{ store };
    BOOST_CHECK(!store.create(test::events_handler));
    BOOST_CHECK(test::setup_three_block_store(query));
    BOOST_CHECK_EQUAL(query.get_wire_block(0, true), test::genesis.to_data(true));
    BOOST_CHECK_EQUAL(query.get_wire_block(1, true), test::block1.to_data(true));
    BOOST_CHECK(!store.close(test::events_handler));
}

BOOST_AUTO_TEST_CASE(query_wire_reader__get_wire_block__witness_true__expected)
{
    using namespace system;
    database::settings settings{};
    settings.path = TEST_DIRECTORY;
    test::store_t store{ settings };
    test::query_t query{ store };
    BOOST_CHECK(!store.create(test::events_handler));
    BOOST_CHECK(test::setup_three_block_witness_store(query));
    BOOST_CHECK_EQUAL(query.get_wire_block(0, true), test::genesis.to_data(true));
    BOOST_CHECK_EQUAL(query.get_wire_block(1, true), test::block1a.to_data(true));
    BOOST_CHECK(!store.close(test::events_handler));
}

BOOST_AUTO_TEST_CASE(query_wire_reader__get_wire_block__witness_false__expected)
{
    using namespace system;
    database::settings settings{};
    settings.path = TEST_DIRECTORY;
    test::store_t store{ settings };
    test::query_t query{ store };
    BOOST_CHECK(!store.create(test::events_handler));
    BOOST_CHECK(test::setup_three_block_witness_store(query));
    BOOST_CHECK_EQUAL(query.get_wire_block(0, false), test::genesis.to_data(false));
    BOOST_CHECK_EQUAL(query.get_wire_block(1, false), test::block1a.to_data(false));
    BOOST_CHECK(!store.close(test::events_handler));
}

BOOST_AUTO_TEST_CASE(query_wire_reader__get_wire_block__turbo_witness_true__expected)
{
    using namespace system;
    database::settings settings{};
    settings.path = TEST_DIRECTORY;
    test::store_t store{ settings };
    test::query_t query{ store };
    BOOST_CHECK(!store.create(test::events_handler));
    BOOST_CHECK(test::setup_three_block_witness_store(query));
    BOOST_CHECK_EQUAL(query.get_wire_block(0, true, true), test::genesis.to_data(true));
    BOOST_CHECK_EQUAL(query.get_wire_block(1, true, true), test::block1a.to_data(true));
    BOOST_CHECK_EQUAL(query.get_wire_block(2, true, true), test::block2a.to_data(true));
    BOOST_CHECK(!store.close(test::events_handler));
}

BOOST_AUTO_TEST_CASE(query_wire_reader__get_wire_block__turbo_witness_false__expected)
{
    using namespace system;
    database::settings settings{};
    settings.path = TEST_DIRECTORY;
    test::store_t store{ settings };
    test::query_t query{ store };
    BOOST_CHECK(!store.create(test::events_handler));
    BOOST_CHECK(test::setup_three_block_witness_store(query));
    BOOST_CHECK_EQUAL(query.get_wire_block(0, false, true), test::genesis.to_data(false));
    BOOST_CHECK_EQUAL(query.get_wire_block(1, false, true), test::block1a.to_data(false));
    BOOST_CHECK_EQUAL(query.get_wire_block(2, false, true), test::block2a.to_data(false));
    BOOST_CHECK(!store.close(test::events_handler));
}

BOOST_AUTO_TEST_CASE(query_wire_reader__get_wire_block__base16_target__expected)
{
    using namespace system;
    database::settings settings{};
    settings.path = TEST_DIRECTORY;
    test::store_t store{ settings };
    test::query_t query{ store };
    BOOST_CHECK(!store.create(test::events_handler));
    BOOST_CHECK(test::setup_three_block_witness_store(query));

    size_t size{};
    BOOST_CHECK(query.get_block_size(size, 2, true));

    std::string sequential(two * size, '\0');
    BOOST_CHECK(query.get_wire_block<write::base16::fast>(sequential, 2, true, false));
    BOOST_CHECK_EQUAL(sequential, encode_base16(test::block2a.to_data(true)));

    std::string parallel(two * size, '\0');
    BOOST_CHECK(query.get_wire_block<write::base16::fast>(parallel, 2, true, true));
    BOOST_CHECK_EQUAL(parallel, encode_base16(test::block2a.to_data(true)));
    BOOST_CHECK(!store.close(test::events_handler));
}

BOOST_AUTO_TEST_CASE(query_wire_reader__get_wire_block__turbo_ranges__expected)
{
    using namespace system;
    database::settings settings{};
    settings.path = TEST_DIRECTORY;
    test::store_t store{ settings };
    test::query_t query{ store };
    BOOST_CHECK(!store.create(test::events_handler));
    BOOST_CHECK(query.initialize(test::genesis));

    const auto block = many_tx_block();
    BOOST_CHECK(query.set(block, database::context{ 0, 1, 0 }, {}, false, false));
    const auto link = query.to_header(block.hash());

    BOOST_CHECK_EQUAL(query.get_wire_block(link, true, true), block.to_data(true));
    BOOST_CHECK_EQUAL(query.get_wire_block(link, false, true), block.to_data(false));

    size_t size{};
    BOOST_CHECK(query.get_block_size(size, link, true));

    std::string hex(two * size, '\0');
    BOOST_CHECK(query.get_wire_block<write::base16::fast>(hex, link, true, true));
    BOOST_CHECK_EQUAL(hex, encode_base16(block.to_data(true)));
    BOOST_CHECK(!store.close(test::events_handler));
}

BOOST_AUTO_TEST_CASE(query_wire_reader__get_wire_block__turbo_missized_target__false)
{
    using namespace system;
    database::settings settings{};
    settings.path = TEST_DIRECTORY;
    test::store_t store{ settings };
    test::query_t query{ store };
    BOOST_CHECK(!store.create(test::events_handler));
    BOOST_CHECK(test::setup_three_block_witness_store(query));

    size_t size{};
    BOOST_CHECK(query.get_block_size(size, 2, true));

    data_chunk out(add1(size));
    BOOST_CHECK(!query.get_wire_block(out, 2, true, true));
    BOOST_CHECK(!store.close(test::events_handler));
}

BOOST_AUTO_TEST_SUITE_END()
