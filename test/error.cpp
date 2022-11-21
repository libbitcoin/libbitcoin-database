/**
 * Copyright (c) 2011-2022 libbitcoin developers (see AUTHORS)
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
#include "test.hpp"
#include <bitcoin/database.hpp>

BOOST_AUTO_TEST_SUITE(error_tests)

// error_t
// These test std::error_code equality operator overrides.

BOOST_AUTO_TEST_CASE(error_t__code__success__false_exected_message)
{
    constexpr auto value = error::success;
    const auto ec = code(value);
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "success");
}

BOOST_AUTO_TEST_CASE(error_t__code__unknown__true_exected_message)
{
    constexpr auto value = error::unknown;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "unknown error");
}

BOOST_AUTO_TEST_CASE(error_t__code__open_open__true_exected_message)
{
    constexpr auto value = error::open_open;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "opening open file");
}

BOOST_AUTO_TEST_CASE(error_t__code__open_failure__true_exected_message)
{
    constexpr auto value = error::open_failure;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "file failed to open");
}

BOOST_AUTO_TEST_CASE(error_t__code__close_loaded__true_exected_message)
{
    constexpr auto value = error::close_loaded;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "closing loaded file");
}

BOOST_AUTO_TEST_CASE(error_t__code__close_failure__true_exected_message)
{
    constexpr auto value = error::close_failure;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "file failed to close");
}

BOOST_AUTO_TEST_CASE(error_t__code__load_locked__true_exected_message)
{
    constexpr auto value = error::load_locked;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "loading locked file");
}

BOOST_AUTO_TEST_CASE(error_t__code__load_loaded__true_exected_message)
{
    constexpr auto value = error::load_loaded;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "loading loaded file");
}

BOOST_AUTO_TEST_CASE(error_t__code__load_failure__true_exected_message)
{
    constexpr auto value = error::load_failure;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "file failed to load");
}

BOOST_AUTO_TEST_CASE(error_t__code__flush_unloaded__true_exected_message)
{
    constexpr auto value = error::flush_unloaded;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "flushing unloaded file");
}

BOOST_AUTO_TEST_CASE(error_t__code__flush_failure__true_exected_message)
{
    constexpr auto value = error::flush_failure;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "file failed to flush");
}

BOOST_AUTO_TEST_CASE(error_t__code__unload_locked__true_exected_message)
{
    constexpr auto value = error::unload_locked;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "unloading unloaded file");
}

BOOST_AUTO_TEST_CASE(error_t__code__unload_failure__true_exected_message)
{
    constexpr auto value = error::unload_failure;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "file failed to unload");
}

BOOST_AUTO_TEST_SUITE_END()
