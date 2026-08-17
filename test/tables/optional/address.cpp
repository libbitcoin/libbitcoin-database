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

BOOST_AUTO_TEST_SUITE(address_tests)

using namespace system;

using body_storages = test::chunk_storages<schema::address::minrow,
    schema::outs::size>;
static const body_storages::paths body_paths{ "address", "puts" };

// Both keys select bucket zero (of 8), forming one chain of two rows.
constexpr auto key1 = base16_array("1000000000000000aa000000000000000000000000000000000000000000000a");
constexpr auto key2 = base16_array("2000000000000000bb000000000000000000000000000000000000000000000b");

const table::outs::record in{ {}, { 0x7890abcdef, 0x1234567890 } };
const data_chunk expected_address_body
{
    0xff, 0xff, 0xff, 0xff,  // next->end
    0x00, 0x00, 0x00, 0x00   // next->0
};
const data_chunk expected_puts_body
{
    0xef, 0xcd, 0xab, 0x90, 0x78,  // output0_fk
    0x90, 0x78, 0x56, 0x34, 0x12   // output1_fk
};

BOOST_AUTO_TEST_CASE(address__commit__two__expected_bodies)
{
    test::chunk_storage head_store{};
    body_storages body_store{ body_paths };
    table::outs instance{ head_store, body_store, 8 };
    BOOST_REQUIRE(instance.create());
    BOOST_REQUIRE(instance.enabled());

    const auto link = instance.allocate(2);
    BOOST_REQUIRE_EQUAL(link, 0u);
    BOOST_REQUIRE(instance.puts.put(link, in));

    const auto ptr = instance.get_memory();
    BOOST_REQUIRE(instance.commit(ptr, 0u, { key1 }));
    BOOST_REQUIRE(instance.commit(ptr, 1u, { key2 }));

    BOOST_REQUIRE_EQUAL(body_store.buffers_.at(0), expected_address_body);
    BOOST_REQUIRE_EQUAL(body_store.buffers_.at(1), expected_puts_body);
}

BOOST_AUTO_TEST_CASE(address__it__co_bucket__all_candidates)
{
    test::chunk_storage head_store{};
    body_storages body_store{ body_paths };
    table::outs instance{ head_store, body_store, 8 };
    BOOST_REQUIRE(instance.create());

    const auto link = instance.allocate(2);
    BOOST_REQUIRE(instance.puts.put(link, in));
    const auto ptr = instance.get_memory();
    BOOST_REQUIRE(instance.commit(ptr, 0u, { key1 }));
    BOOST_REQUIRE(instance.commit(ptr, 1u, { key2 }));

    // Nothing is stored, so both rows are candidates for either key.
    std::vector<table::outs::link::integer> links{};
    for (auto it = instance.it({ key1 }); it; ++it)
        links.push_back(*it);

    BOOST_REQUIRE_EQUAL(links.size(), 2u);
    BOOST_REQUIRE_EQUAL(links.at(0), 1u);
    BOOST_REQUIRE_EQUAL(links.at(1), 0u);

    table::outs::get_output out{};
    BOOST_REQUIRE(instance.puts.get(links.at(0), out));
    BOOST_REQUIRE_EQUAL(out.out_fk, 0x1234567890_u64);
    BOOST_REQUIRE(instance.puts.get(links.at(1), out));
    BOOST_REQUIRE_EQUAL(out.out_fk, 0x7890abcdef_u64);
}

BOOST_AUTO_TEST_CASE(address__enabled__one_bucket__false)
{
    test::chunk_storage head_store{};
    body_storages body_store{ body_paths };
    table::outs instance{ head_store, body_store, 1 };
    BOOST_REQUIRE(instance.create());
    BOOST_REQUIRE(!instance.enabled());
}

BOOST_AUTO_TEST_SUITE_END()
