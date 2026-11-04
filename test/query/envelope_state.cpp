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
#include "../test.hpp"
#include "../mocks/blocks.hpp"
#include "../mocks/chunk_store.hpp"

BC_FIXTURE_TEST_SUITE(query_envelope_state_tests, test::directory_setup_fixture)

// get_activation
// ----------------------------------------------------------------------------

constexpr uint32_t activated = system::chain::flags::bip68_rule | system::chain::flags::bip141_rule;

BC_AUTO_TEST_CASE(query_envelope_state__get_activation__unset__false)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    size_t height{};
    BC_REQUIRE(!query.get_activation(height, system::chain::flags::bip68_rule));
}

BC_AUTO_TEST_CASE(query_envelope_state__get_activation__confirmed_group__each_flag_expected)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));
    BC_REQUIRE(query.set(test::block1a, context{ activated, 1, 0 }, {}, false, false));
    BC_REQUIRE(query.push_confirmed(1, false));

    size_t height{};
    BC_REQUIRE(query.set_activation(activated, 1));
    BC_REQUIRE(query.get_activation(height, system::chain::flags::bip68_rule));
    BC_REQUIRE_EQUAL(height, 1u);
    BC_REQUIRE(query.get_activation(height, system::chain::flags::bip141_rule));
    BC_REQUIRE_EQUAL(height, 1u);
    BC_REQUIRE(!query.get_activation(height, system::chain::flags::bip341_rule));
}

BC_AUTO_TEST_CASE(query_envelope_state__get_activation__confirmed_unflagged__false)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));
    BC_REQUIRE(query.set(test::block1a, context{ 0, 1, 0 }, {}, false, false));
    BC_REQUIRE(query.push_confirmed(1, false));

    size_t height{};
    BC_REQUIRE(query.set_activation(system::chain::flags::bip68_rule, 1));
    BC_REQUIRE(!query.get_activation(height, system::chain::flags::bip68_rule));
}

BC_AUTO_TEST_CASE(query_envelope_state__get_activation__candidate__false)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));
    BC_REQUIRE(query.set(test::block1a, context{ activated, 1, 0 }, {}, false, false));
    BC_REQUIRE(query.push_candidate(1));

    size_t height{};
    BC_REQUIRE(query.set_activation(activated, 1));
    BC_REQUIRE(!query.get_activation(height, system::chain::flags::bip68_rule));
}

// set_activation
// ----------------------------------------------------------------------------

BC_AUTO_TEST_CASE(query_envelope_state__set_activation__same_flags__replaced)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));
    BC_REQUIRE(query.set(test::block1a, context{ activated, 1, 0 }, {}, false, false));
    BC_REQUIRE(query.push_confirmed(1, false));

    size_t height{};
    BC_REQUIRE(query.set_activation(activated, 2));
    BC_REQUIRE(query.set_activation(activated, 1));
    BC_REQUIRE(query.get_activation(height, system::chain::flags::bip68_rule));
    BC_REQUIRE_EQUAL(height, 1u);
}

BC_AUTO_TEST_CASE(query_envelope_state__set_activation__group__slot_per_flag)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    BC_REQUIRE(query.set_activation(activated, 1));
    BC_REQUIRE_EQUAL(store.get_activation(8).flags, activated);
    BC_REQUIRE_EQUAL(store.get_activation(11).flags, activated);
    BC_REQUIRE_EQUAL(store.get_activation(9).flags, 0u);
}

// clear_activation
// ----------------------------------------------------------------------------

BC_AUTO_TEST_CASE(query_envelope_state__clear_activation__set__false)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));
    BC_REQUIRE(query.set(test::block1a, context{ activated, 1, 0 }, {}, false, false));
    BC_REQUIRE(query.push_confirmed(1, false));

    size_t height{};
    BC_REQUIRE(query.set_activation(activated, 1));
    BC_REQUIRE(query.clear_activation(activated));
    BC_REQUIRE(!query.get_activation(height, system::chain::flags::bip68_rule));
}

BC_AUTO_TEST_CASE(query_envelope_state__clear_activation__reactivated__expected)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));
    BC_REQUIRE(query.set(test::block1a, context{ activated, 1, 0 }, {}, false, false));
    BC_REQUIRE(query.push_confirmed(1, false));

    size_t height{};
    BC_REQUIRE(query.set_activation(activated, 2));
    BC_REQUIRE(query.clear_activation(activated));
    BC_REQUIRE(query.set_activation(activated, 1));
    BC_REQUIRE(query.get_activation(height, system::chain::flags::bip68_rule));
    BC_REQUIRE_EQUAL(height, 1u);
}

BC_AUTO_TEST_SUITE_END()
