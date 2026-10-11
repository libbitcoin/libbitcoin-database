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

BOOST_FIXTURE_TEST_SUITE(query_consensus_tests, test::directory_setup_fixture)

using namespace system::chain;
using context = database::context;
constexpr auto bip113 = flags::bip113_rule;

constexpr system::siphash_key compact_key
{
    0x0102030405060708_u64,
    0x1112131415161718_u64
};

static uint64_t to_short_id(const transaction& tx) NOEXCEPT
{
    const auto short_hash = system::siphash(compact_key, tx.hash(true));
    return system::bit_and(short_hash, system::unmask_right<uint64_t>(48));
}

// get_compact_links

BOOST_AUTO_TEST_CASE(query_consensus__get_compact_links__pooled__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));

    const transaction tx4{ test::tx4.to_data(true), true };
    const transaction tx5{ test::tx5.to_data(true), true };
    tx4.inputs_ptr()->at(0)->metadata.parent_tx = 42;
    tx4.inputs_ptr()->at(1)->metadata.parent_tx = 42;
    tx5.inputs_ptr()->at(0)->metadata.parent_tx = 42;

    tx_link link4{};
    tx_link link5{};
    BOOST_REQUIRE(!query.set_code(link4, tx4));
    BOOST_REQUIRE(!query.set_code(link5, tx5));
    BOOST_REQUIRE(query.set_pooled(link4, tx4, context{ bip113, 8, 9 }));
    BOOST_REQUIRE(query.set_pooled(link5, tx5, context{ bip113, 8, 9 }));

    tx_links out{};
    const std::vector<uint64_t> ids{ to_short_id(tx5), 0x0000424242424242_u64, to_short_id(tx4) };
    BOOST_REQUIRE_EQUAL(query.get_compact_links(out, ids, compact_key), error::success);
    BOOST_REQUIRE_EQUAL(out.size(), 3u);
    BOOST_REQUIRE_EQUAL(out.at(0), link5);
    BOOST_REQUIRE_EQUAL(out.at(1), tx_link::terminal);
    BOOST_REQUIRE_EQUAL(out.at(2), link4);
}

BOOST_AUTO_TEST_CASE(query_consensus__get_compact_links__duplicate_short_id__terminal)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));

    const transaction tx5{ test::tx5.to_data(true), true };
    tx5.inputs_ptr()->at(0)->metadata.parent_tx = 42;

    tx_link link5{};
    BOOST_REQUIRE(!query.set_code(link5, tx5));
    BOOST_REQUIRE(query.set_pooled(link5, tx5, context{ bip113, 8, 9 }));

    tx_links out{};
    const std::vector<uint64_t> ids{ to_short_id(tx5), to_short_id(tx5) };
    BOOST_REQUIRE_EQUAL(query.get_compact_links(out, ids, compact_key), error::success);
    BOOST_REQUIRE_EQUAL(out.at(0), tx_link::terminal);
    BOOST_REQUIRE_EQUAL(out.at(1), tx_link::terminal);
}

BOOST_AUTO_TEST_CASE(query_consensus__get_compact_links__ambiguous_pool__terminal)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));

    const transaction tx5{ test::tx5.to_data(true), true };
    tx5.inputs_ptr()->at(0)->metadata.parent_tx = 42;

    // The same wtxid pooled under two distinct links.
    BOOST_REQUIRE(query.set_pooled(1, tx5, context{ bip113, 8, 9 }));
    BOOST_REQUIRE(query.set_pooled(2, tx5, context{ bip113, 8, 9 }));

    tx_links out{};
    const std::vector<uint64_t> ids{ to_short_id(tx5) };
    BOOST_REQUIRE_EQUAL(query.get_compact_links(out, ids, compact_key), error::success);
    BOOST_REQUIRE_EQUAL(out.at(0), tx_link::terminal);
}

BOOST_AUTO_TEST_CASE(query_consensus__get_compact_links__allocated_record__pooled_matched)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));

    const transaction tx5{ test::tx5.to_data(true), true };
    tx5.inputs_ptr()->at(0)->metadata.parent_tx = 42;

    tx_link link5{};
    BOOST_REQUIRE(!query.set_code(link5, tx5));
    BOOST_REQUIRE(query.set_pooled(link5, tx5, context{ bip113, 8, 9 }));

    // A spine record allocated ahead of its columns (a concurrent pool write).
    BOOST_REQUIRE(!store.pool.allocate(1).is_terminal());

    tx_links out{};
    const std::vector<uint64_t> ids{ to_short_id(tx5) };
    BOOST_REQUIRE_EQUAL(query.get_compact_links(out, ids, compact_key), error::success);
    BOOST_REQUIRE_EQUAL(out.at(0), link5);
}

BOOST_AUTO_TEST_CASE(query_consensus__get_compact_links__uncommitted_record__terminal)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));

    // Columns written for a record whose spine entry is not yet committed.
    using lane_t = schema::pool::witness_lane;
    const auto words = system::from_little_endians(system::array_cast<lane_t>(test::tx5.hash(true)));
    const auto link = store.pool.allocate(1);
    BOOST_REQUIRE(!link.is_terminal());
    BOOST_REQUIRE(store.pool.id0.put(link, table::pool_word{ {}, std::get<0>(words) }));
    BOOST_REQUIRE(store.pool.id1.put(link, table::pool_word{ {}, std::get<1>(words) }));
    BOOST_REQUIRE(store.pool.id2.put(link, table::pool_word{ {}, std::get<2>(words) }));
    BOOST_REQUIRE(store.pool.id3.put(link, table::pool_word{ {}, std::get<3>(words) }));

    tx_links out{};
    const std::vector<uint64_t> ids{ to_short_id(test::tx5) };
    BOOST_REQUIRE_EQUAL(query.get_compact_links(out, ids, compact_key), error::success);
    BOOST_REQUIRE_EQUAL(out.at(0), tx_link::terminal);
}

BOOST_AUTO_TEST_CASE(query_consensus__get_compact_links__disabled__terminal)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    settings.pool.buckets = 0;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));

    tx_links out{};
    const std::vector<uint64_t> ids{ to_short_id(test::tx5) };
    BOOST_REQUIRE_EQUAL(query.get_compact_links(out, ids, compact_key), error::success);
    BOOST_REQUIRE_EQUAL(out.size(), 1u);
    BOOST_REQUIRE_EQUAL(out.at(0), tx_link::terminal);
}

BOOST_AUTO_TEST_SUITE_END()
