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

BC_FIXTURE_TEST_SUITE(query_consensus_tests, test::directory_setup_fixture)

// get_block_state

BC_AUTO_TEST_CASE(query_consensus__get_block_state__genesis__expected)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));
    BC_REQUIRE_EQUAL(query.get_block_state(0), block_state::associated | block_state::checkpointed | block_state::scanned | block_state::filtered);
}

BC_AUTO_TEST_CASE(query_consensus__get_block_state__invalid_link__no_state)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));
    BC_REQUIRE_EQUAL(query.get_block_state(1), block_state::no_state);
    BC_REQUIRE(!query.is_associated(1));
}

BC_AUTO_TEST_CASE(query_consensus__get_block_state__unassociated_link__scanned)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));
    BC_REQUIRE(query.set(test::block1.header(), context{ 0, 1, 0 }, {}, false));
    BC_REQUIRE(query.set(*test::block1.transactions_ptr()->front()));
    BC_REQUIRE_EQUAL(query.get_block_state(1), block_state::scanned);
    BC_REQUIRE(!query.is_associated(1));
}

BC_AUTO_TEST_CASE(query_consensus__get_block_state__associated_link__associated)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));
    BC_REQUIRE(query.set(test::block1, context{ 0, 1, 0 }, {}, false, false));
    BC_REQUIRE_EQUAL(query.get_block_state(1), block_state::associated | block_state::scanned);
    BC_REQUIRE(query.is_associated(1));
}

BC_AUTO_TEST_CASE(query_consensus__get_block_state__milestone__associated_milestoned)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));
    BC_REQUIRE(query.set(test::block1, context{ 0, 1, 0 }, {}, true, false));
    BC_REQUIRE_EQUAL(query.get_block_state(1), block_state::associated | block_state::milestoned | block_state::scanned);
    BC_REQUIRE(query.is_milestone(1));
    BC_REQUIRE(query.is_bypass(1));
}

BC_AUTO_TEST_CASE(query_consensus__get_block_state__checkpoint__associated_checkpointed)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));
    BC_REQUIRE(query.set(test::block1, context{}, {}, false, false));
    BC_REQUIRE_EQUAL(query.get_block_state(1), block_state::associated | block_state::checkpointed | block_state::scanned);
    BC_REQUIRE(!query.is_milestone(1));
    BC_REQUIRE(query.is_bypass(1));
}

BC_AUTO_TEST_CASE(query_consensus__get_block_state__confirmable__associated_confirmable)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));
    BC_REQUIRE(query.set(test::block1, context{ 0, 1, 0 }, {}, false, false));
    BC_REQUIRE(query.set_block_confirmable(1));
    BC_REQUIRE_EQUAL(query.get_block_state(1), block_state::associated | block_state::confirmable | block_state::scanned);
    BC_REQUIRE(query.is_confirmable(1));
    BC_REQUIRE(query.is_validated(1));
}

BC_AUTO_TEST_CASE(query_consensus__get_block_state__valid__associated_valid)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));
    BC_REQUIRE(query.set(test::block1, context{ 0, 1, 0 }, {}, false, false));
    BC_REQUIRE(query.set_block_valid(1));
    BC_REQUIRE_EQUAL(query.get_block_state(1), block_state::associated | block_state::valid | block_state::scanned);
    BC_REQUIRE(!query.is_confirmable(1));
    BC_REQUIRE(query.is_validated(1));
}

BC_AUTO_TEST_CASE(query_consensus__get_block_state__unconfirmable__associated_unconfirmable)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));
    BC_REQUIRE(query.set(test::block1, context{ 0, 1, 0 }, {}, false, false));
    BC_REQUIRE(query.set_block_unconfirmable(1));
    BC_REQUIRE_EQUAL(query.get_block_state(1), block_state::associated | block_state::unconfirmable | block_state::scanned);
    BC_REQUIRE(query.is_unconfirmable(1));
}

BC_AUTO_TEST_CASE(query_consensus__is_validateable__candidate__expected)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));
    BC_REQUIRE(!query.is_validateable(1));
    BC_REQUIRE(query.set(test::block1, context{ 0, 1, 0 }, {}, false, false));
    BC_REQUIRE(query.push_candidate(1));
    BC_REQUIRE(query.is_validateable(1));
    BC_REQUIRE(query.set_block_unconfirmable(1));
    BC_REQUIRE(!query.is_validateable(1));
}

BC_AUTO_TEST_CASE(query_consensus__get_block_state__valid_then_unconfirmable__unconfirmable)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));
    BC_REQUIRE(query.set(test::block1, context{ 0, 1, 0 }, {}, false, false));
    BC_REQUIRE(query.set_block_valid(1));
    BC_REQUIRE(query.set_block_unconfirmable(1));
    BC_REQUIRE(query.is_unconfirmable(1));
    BC_REQUIRE(!query.is_validated(1));
}

BC_AUTO_TEST_CASE(query_consensus__set_block_unknown__valid__associated)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));
    BC_REQUIRE(query.set(test::block1, context{ 0, 1, 0 }, {}, false, false));
    BC_REQUIRE(query.set_block_valid(1));
    BC_REQUIRE(query.set_block_unknown(1));
    BC_REQUIRE_EQUAL(query.get_block_state(1), block_state::associated | block_state::scanned);
}

BC_AUTO_TEST_CASE(query_consensus__get_block_state__filters_disabled__filtered)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    settings.filter_tx.buckets = 0;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));
    BC_REQUIRE(query.set(test::block1, context{ 0, 1, 0 }, {}, false, false));
    BC_REQUIRE(is_filtered(query.get_block_state(1)));
}

BC_AUTO_TEST_CASE(query_consensus__set_filter_body__filters_enabled__filtered)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));
    BC_REQUIRE(query.set(test::block1, context{ 0, 1, 0 }, {}, false, false));
    BC_REQUIRE(!is_filtered(query.get_block_state(1)));
    BC_REQUIRE(query.set_filter_body(1, test::block1));
    BC_REQUIRE(is_filtered(query.get_block_state(1)));
}

BC_AUTO_TEST_SUITE_END()
