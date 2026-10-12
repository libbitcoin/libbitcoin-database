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
#include "../../mocks/chunk_storage.hpp"

BOOST_AUTO_TEST_SUITE(state_tests)

using namespace system;

BOOST_AUTO_TEST_CASE(state__at__unset__no_state)
{
    data_chunk head_file{};
    test::chunk_storage head_store{ head_file };
    table::state instance{ head_store };
    BOOST_REQUIRE(instance.create());
    BOOST_REQUIRE_EQUAL(instance.at(0), block_state::no_state);
}

BOOST_AUTO_TEST_CASE(state__set__merge__merged)
{
    data_chunk head_file{};
    test::chunk_storage head_store{ head_file };
    table::state instance{ head_store };
    BOOST_REQUIRE(instance.create());
    BOOST_REQUIRE(instance.set(1, table::state::merge{ {}, block_state::valid }));
    BOOST_REQUIRE(instance.set(1, table::state::merge{ {}, block_state::confirmable }));
    BOOST_REQUIRE_EQUAL(instance.count(), 2u);
    BOOST_REQUIRE_EQUAL(head_file, base16_chunk("0006"));
    BOOST_REQUIRE_EQUAL(instance.at(0), block_state::no_state);
    BOOST_REQUIRE_EQUAL(instance.at(1), block_state::valid | block_state::confirmable);
}

BOOST_AUTO_TEST_CASE(state__set__merge_after_unconfirmable__retained)
{
    data_chunk head_file{};
    test::chunk_storage head_store{ head_file };
    table::state instance{ head_store };
    BOOST_REQUIRE(instance.create());
    BOOST_REQUIRE(instance.set(0, table::state::merge{ {}, block_state::unconfirmable }));
    BOOST_REQUIRE(instance.set(0, table::state::merge{ {}, block_state::valid }));
    BOOST_REQUIRE_EQUAL(instance.at(0), block_state::unconfirmable | block_state::valid);
}

BOOST_AUTO_TEST_CASE(state__set__clear__cleared)
{
    data_chunk head_file{};
    test::chunk_storage head_store{ head_file };
    table::state instance{ head_store };
    BOOST_REQUIRE(instance.create());
    BOOST_REQUIRE(instance.set(0, table::state::merge{ {}, block_state::associated | block_state::confirmable }));
    BOOST_REQUIRE(instance.set(0, table::state::clear{ {}, validation_states }));
    BOOST_REQUIRE_EQUAL(head_file, base16_chunk("01"));
    BOOST_REQUIRE_EQUAL(instance.at(0), block_state::associated);
}

BOOST_AUTO_TEST_SUITE_END()
