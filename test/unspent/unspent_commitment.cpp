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

BC_AUTO_TEST_SUITE(unspent_writer_tests)

using namespace system;

constexpr auto txid = base16_array(
    "0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20");

static unspent_coin coin(size_t height, bool coinbase, uint64_t value,
    const data_chunk& script)
{
    unspent_coin out{};
    out.first = true;
    out.out = { chain::point{ txid, 7 }, value };
    out.height = height;
    out.coinbase = coinbase;
    out.script = script;
    return out;
}

BC_AUTO_TEST_CASE(unspent_writer__write__coinbase__expected_head)
{
    const auto script = base16_chunk("76a914000000000000000000000000000000000000000088ac");
    const auto subject = coin(1234, true, 5'000'000'000, script);
    const auto index = to_little_endian<uint32_t>(7);
    const auto code = to_little_endian<uint32_t>((1234 << 1) | 1);
    const auto expected = build_chunk({ txid, index, code });

    data_chunk head{};
    write::bytes::data sink{ head };
    unspent_writer::write(sink, subject);
    sink.flush();
    BC_REQUIRE_EQUAL(head, expected);
}

BC_AUTO_TEST_CASE(unspent_writer__write__non_coinbase__expected_head)
{
    const data_chunk script(300, 0x51);
    const auto subject = coin(42, false, 1, script);
    const auto index = to_little_endian<uint32_t>(7);
    const auto code = to_little_endian<uint32_t>(42 << 1);
    const auto expected = build_chunk({ txid, index, code });

    data_chunk head{};
    write::bytes::data sink{ head };
    unspent_writer::write(sink, subject);
    sink.flush();
    BC_REQUIRE_EQUAL(head, expected);
}

BC_AUTO_TEST_CASE(unspent_writer__size__script_sizes__expected)
{
    BC_REQUIRE_EQUAL(unspent_writer::size(0), 48u + 1u);
    BC_REQUIRE_EQUAL(unspent_writer::size(25), 48u + 1u + 25u);
    BC_REQUIRE_EQUAL(unspent_writer::size(300), 48u + 3u + 300u);
}

BC_AUTO_TEST_CASE(unspent_writer__add__coin__accumulated)
{
    const auto script = base16_chunk("51");
    const auto subject = coin(1, false, 100, script);
    unspent_totals totals{};
    unspent_writer::add(totals, subject);
    unspent_writer::add(totals, subject);
    BC_REQUIRE_EQUAL(totals.transactions, 2u);
    BC_REQUIRE_EQUAL(totals.outputs, 2u);
    BC_REQUIRE_EQUAL(totals.value, 200u);
    BC_REQUIRE_EQUAL(totals.script_bytes, 2u);
    BC_REQUIRE_EQUAL(totals.coin_bytes, 2u * (48u + 1u + 1u));
}

BC_AUTO_TEST_CASE(unspent_writer__add__totals__summed)
{
    unspent_totals left{};
    left.transactions = 1;
    left.outputs = 2;
    left.value = 3;
    left.script_bytes = 4;
    left.coin_bytes = 5;
    unspent_totals right{ left };
    unspent_writer::add(left, right);
    BC_REQUIRE_EQUAL(left.transactions, 2u);
    BC_REQUIRE_EQUAL(left.outputs, 4u);
    BC_REQUIRE_EQUAL(left.value, 6u);
    BC_REQUIRE_EQUAL(left.script_bytes, 8u);
    BC_REQUIRE_EQUAL(left.coin_bytes, 10u);
}

BC_AUTO_TEST_SUITE_END()
