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

BOOST_AUTO_TEST_SUITE(scan_start_tests)

using namespace system;

BOOST_AUTO_TEST_CASE(scan_start__at__unset__terminal)
{
    data_chunk head_file{};
    test::chunk_storage head_store{ head_file };
    table::scan_start instance{ head_store };
    BOOST_REQUIRE(instance.create());
    BOOST_REQUIRE(instance.at(0).is_terminal());
}

BOOST_AUTO_TEST_CASE(scan_start__set__gapped__unset_terminal)
{
    data_chunk head_file{};
    test::chunk_storage head_store{ head_file };
    table::scan_start instance{ head_store };
    BOOST_REQUIRE(instance.create());
    BOOST_REQUIRE(instance.set(1, table::scan_start::record{ {}, 0x01020304 }));
    BOOST_REQUIRE_EQUAL(instance.count(), 2u);
    BOOST_REQUIRE_EQUAL(head_file, base16_chunk("ffffffff04030201"));
    BOOST_REQUIRE(instance.at(0).is_terminal());
    BOOST_REQUIRE_EQUAL(instance.at(1), 0x01020304u);
    BOOST_REQUIRE(instance.at(2).is_terminal());
}

BOOST_AUTO_TEST_CASE(scan_start__set__replace__replaced)
{
    data_chunk head_file{};
    test::chunk_storage head_store{ head_file };
    table::scan_start instance{ head_store };
    BOOST_REQUIRE(instance.create());
    BOOST_REQUIRE(instance.set(0, table::scan_start::record{ {}, 42 }));
    BOOST_REQUIRE(instance.set(0, table::scan_start::record{ {}, 7 }));
    BOOST_REQUIRE_EQUAL(instance.at(0), 7u);
}

BOOST_AUTO_TEST_SUITE_END()
