/**
 * Copyright (c) 2011-2026 libbitcoin developers (see AUTHORS)
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

BOOST_AUTO_TEST_SUITE_END()
