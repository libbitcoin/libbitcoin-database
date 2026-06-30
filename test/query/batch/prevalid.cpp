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

BOOST_FIXTURE_TEST_SUITE(query_batch_prevalid_tests, test::directory_setup_fixture)

BOOST_AUTO_TEST_CASE(query_batch_prevalid__get_prevalids__empty__empty)
{
    const database::settings configuration{};
    test::chunk_store store{ configuration };
    test::query_accessor query{ store };

    BOOST_REQUIRE(query.get_prevalids().empty());
}

BOOST_AUTO_TEST_CASE(query_batch_prevalid__set_get__two__round_trips)
{
    const database::settings configuration{};
    test::chunk_store store{ configuration };
    test::query_accessor query{ store };

    const header_links links{ 0x00345678, 0x00cdef12 };
    BOOST_REQUIRE(query.set_prevalids(links));

    const auto out = query.get_prevalids();
    BOOST_REQUIRE_EQUAL(out.size(), 2u);
    BOOST_REQUIRE(out == links);
}

BOOST_AUTO_TEST_CASE(query_batch_prevalid__purge__after_set__empty)
{
    const database::settings configuration{};
    test::chunk_store store{ configuration };
    test::query_accessor query{ store };

    const header_links links{ 0x00345678, 0x00cdef12 };
    BOOST_REQUIRE(query.set_prevalids(links));
    BOOST_REQUIRE(!query.get_prevalids().empty());

    BOOST_REQUIRE(query.purge_prevalids());
    BOOST_REQUIRE(query.get_prevalids().empty());
}

BOOST_AUTO_TEST_CASE(query_batch_prevalid__set_purge_set__reuses_empty__round_trips)
{
    const database::settings configuration{};
    test::chunk_store store{ configuration };
    test::query_accessor query{ store };

    // First dump, consumed and purged (startup-consume pattern).
    const header_links first{ 0x00345678, 0x00cdef12 };
    BOOST_REQUIRE(query.set_prevalids(first));
    BOOST_REQUIRE(query.get_prevalids() == first);
    BOOST_REQUIRE(query.purge_prevalids());

    // Second dump appends into the now-empty table (next-shutdown pattern).
    const header_links second{ 0x00111111 };
    BOOST_REQUIRE(query.set_prevalids(second));

    const auto out = query.get_prevalids();
    BOOST_REQUIRE_EQUAL(out.size(), 1u);
    BOOST_REQUIRE(out == second);
}

BOOST_AUTO_TEST_SUITE_END()
