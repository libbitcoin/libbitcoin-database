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

BOOST_AUTO_TEST_SUITE(wtxid_tests)

using namespace system;

BOOST_AUTO_TEST_CASE(wtxid__put__next_record__found)
{
    test::chunk_storage head_store{};
    test::chunk_storage body_store{};
    table::wtxid instance{ head_store, body_store, 8 };
    BOOST_REQUIRE(instance.create());
    BOOST_REQUIRE(instance.put(0, one_hash));
    BOOST_REQUIRE_EQUAL(instance.count(), 1u);
    BOOST_REQUIRE_EQUAL(instance.first({ one_hash }), 0u);
}

BOOST_AUTO_TEST_CASE(wtxid__put__skipped_records__padded_unlinked)
{
    test::chunk_storage head_store{};
    test::chunk_storage body_store{};
    table::wtxid instance{ head_store, body_store, 8 };
    BOOST_REQUIRE(instance.create());
    BOOST_REQUIRE(instance.put(2, one_hash));
    BOOST_REQUIRE_EQUAL(instance.count(), 3u);
    BOOST_REQUIRE_EQUAL(instance.first({ one_hash }), 2u);

    auto it = instance.it({ one_hash });
    BOOST_REQUIRE(it);
    BOOST_REQUIRE_EQUAL(*it, 2u);
    BOOST_REQUIRE(!++it);
}

BOOST_AUTO_TEST_CASE(wtxid__put__allocated_record__false)
{
    test::chunk_storage head_store{};
    test::chunk_storage body_store{};
    table::wtxid instance{ head_store, body_store, 8 };
    BOOST_REQUIRE(instance.create());
    BOOST_REQUIRE(instance.put(1, one_hash));
    BOOST_REQUIRE(!instance.put(0, null_hash));
    BOOST_REQUIRE(!instance.put(1, null_hash));
    BOOST_REQUIRE_EQUAL(instance.count(), 2u);
}

BOOST_AUTO_TEST_SUITE_END()
