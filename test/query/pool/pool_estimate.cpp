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
#include "../../mocks/pooling.hpp"

BC_FIXTURE_TEST_SUITE(query_pool_tests, test::directory_setup_fixture)

using namespace system::chain;
using test::block_limit;
using test::pool_totals;
using test::pool_projection;
using test::pool_arrivals;
using test::to_spend;
using test::genesis_coinbase;
using test::archive_confirm;
using test::pool_populated;

// get_pool_projection
// ----------------------------------------------------------------------------

BC_AUTO_TEST_CASE(query_pool__get_pool_projection__empty__exhausted)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    store.set_pooling();
    pool_projection projection{};
    const auto ec = query.get_pool_projection(projection, 2, block_limit, 25, 0, 20, 1'024);

    BC_REQUIRE_EQUAL(ec, error::success);
    BC_REQUIRE(projection.blocks.empty());
    BC_REQUIRE(projection.exhausted);
}

BC_AUTO_TEST_CASE(query_pool__get_pool_projection__chain_over_budget__block_each)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    store.set_pooling();
    const auto parent = to_spend(genesis_coinbase(), 4'999'999'000);
    const auto child = to_spend(parent.hash(false), 4'999'990'000);
    const auto size = system::possible_narrow_cast<uint32_t>(parent.virtual_size());
    const auto pooled = pool_populated(query, parent) && pool_populated(query, child);
    pool_projection projection{};
    const auto ec = query.get_pool_projection(projection, 2, { 0, size, 80'000 }, 25, 0, 20, 1'024);

    BC_REQUIRE(pooled);
    BC_REQUIRE_EQUAL(ec, error::success);
    BC_REQUIRE_EQUAL(projection.blocks.size(), 2u);
    BC_REQUIRE_EQUAL(projection.blocks[0].size(), 1u);
    BC_REQUIRE_EQUAL(projection.blocks[0][0].rate, 1'000'000u / size);
    BC_REQUIRE_EQUAL(projection.blocks[0][0].size, size);
    BC_REQUIRE_EQUAL(projection.blocks[1].size(), 1u);
    BC_REQUIRE_EQUAL(projection.blocks[1][0].rate, 9'000'000u / size);
    BC_REQUIRE_EQUAL(projection.blocks[1][0].size, size);
    BC_REQUIRE(projection.exhausted);
}

BC_AUTO_TEST_CASE(query_pool__get_pool_projection__one_block_of_two__not_exhausted)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    store.set_pooling();
    const auto parent = to_spend(genesis_coinbase(), 4'999'999'000);
    const auto child = to_spend(parent.hash(false), 4'999'990'000);
    const auto size = system::possible_narrow_cast<uint32_t>(parent.virtual_size());
    const auto pooled = pool_populated(query, parent) && pool_populated(query, child);
    pool_projection projection{};
    const auto ec = query.get_pool_projection(projection, 1, { 0, size, 80'000 }, 25, 0, 20, 1'024);

    BC_REQUIRE(pooled);
    BC_REQUIRE_EQUAL(ec, error::success);
    BC_REQUIRE_EQUAL(projection.blocks.size(), 1u);
    BC_REQUIRE(!projection.exhausted);
}

BC_AUTO_TEST_CASE(query_pool__get_pool_projection__after_template__continued)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    store.set_pooling();
    const auto parent = to_spend(genesis_coinbase(), 4'999'999'000);
    const auto child = to_spend(parent.hash(false), 4'999'990'000);
    const auto size = system::possible_narrow_cast<uint32_t>(parent.virtual_size());
    const auto pooled = pool_populated(query, parent) && pool_populated(query, child);
    tx_links out{};
    pool_projection projection{};
    const pool_totals budget{ 0, size, 80'000 };
    const auto template_ec = query.get_pool_template(out, projection, budget, 25, 0, 20, 1'024);
    const auto ec = query.get_pool_projection(projection, 2, budget, 25, 0, 20, 1'024);

    BC_REQUIRE(pooled);
    BC_REQUIRE_EQUAL(template_ec, error::success);
    BC_REQUIRE_EQUAL(ec, error::success);
    BC_REQUIRE_EQUAL(projection.blocks.size(), 2u);
    BC_REQUIRE_EQUAL(projection.blocks[1][0].rate, 9'000'000u / size);
    BC_REQUIRE(projection.exhausted);
}

BC_AUTO_TEST_CASE(query_pool__get_pool_projection__marks_of_other_rows__restarted)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    store.set_pooling();
    const auto parent = to_spend(genesis_coinbase(), 4'999'999'000);
    const auto child = to_spend(parent.hash(false), 4'999'990'000);
    const auto size = system::possible_narrow_cast<uint32_t>(parent.virtual_size());
    const auto pooled = pool_populated(query, parent) && pool_populated(query, child);
    pool_projection projection{};
    projection.marks.resize(42);
    projection.blocks.resize(3);
    const auto ec = query.get_pool_projection(projection, 1, { 0, size, 80'000 }, 25, 0, 20, 1'024);

    BC_REQUIRE(pooled);
    BC_REQUIRE_EQUAL(ec, error::success);
    BC_REQUIRE_EQUAL(projection.marks.size(), 2u);
    BC_REQUIRE_EQUAL(projection.blocks.size(), 1u);
    BC_REQUIRE_EQUAL(projection.blocks[0][0].rate, 1'000'000u / size);
}

// get_pool_continuation
// ----------------------------------------------------------------------------

BC_AUTO_TEST_CASE(query_pool__get_pool_continuation__more_blocks__continued)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    store.set_pooling();
    const auto parent = to_spend(genesis_coinbase(), 4'999'999'000);
    const auto child = to_spend(parent.hash(false), 4'999'990'000);
    const auto size = system::possible_narrow_cast<uint32_t>(parent.virtual_size());
    const auto pooled = pool_populated(query, parent) && pool_populated(query, child);
    pool_projection projection{};
    const pool_totals budget{ 0, size, 80'000 };
    const auto first = query.get_pool_projection(projection, 1, budget, 25, 0, 20, 1'024);
    const auto ec = query.get_pool_continuation(projection, 2, budget, 25, 20);

    BC_REQUIRE(pooled);
    BC_REQUIRE_EQUAL(first, error::success);
    BC_REQUIRE_EQUAL(ec, error::success);
    BC_REQUIRE_EQUAL(projection.blocks.size(), 2u);
    BC_REQUIRE_EQUAL(projection.blocks[1][0].rate, 9'000'000u / size);
    BC_REQUIRE(projection.exhausted);
}

BC_AUTO_TEST_CASE(query_pool__get_pool_continuation__later_row__left_out)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    store.set_pooling();
    const auto parent = to_spend(genesis_coinbase(), 4'999'999'000);
    const auto child = to_spend(parent.hash(false), 4'999'990'000);
    const auto size = system::possible_narrow_cast<uint32_t>(parent.virtual_size());
    const auto pooled_parent = pool_populated(query, parent);
    pool_projection projection{};
    const pool_totals budget{ 0, size, 80'000 };
    const auto first = query.get_pool_projection(projection, 1, budget, 25, 0, 20, 1'024);
    const auto pooled_child = pool_populated(query, child);
    const auto ec = query.get_pool_continuation(projection, 2, budget, 25, 20);

    BC_REQUIRE(pooled_parent);
    BC_REQUIRE_EQUAL(first, error::success);
    BC_REQUIRE(pooled_child);
    BC_REQUIRE_EQUAL(ec, error::success);
    BC_REQUIRE_EQUAL(projection.marks.size(), 2u);
    BC_REQUIRE(projection.marks[1] == system::pool::algorithm::mark::excluded);
    BC_REQUIRE_EQUAL(projection.blocks.size(), 1u);
    BC_REQUIRE(projection.exhausted);
}

BC_AUTO_TEST_CASE(query_pool__get_pool_continuation__marks_over_rows__integrity)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    store.set_pooling();
    pool_projection projection{};
    projection.marks.resize(42);
    BC_REQUIRE_EQUAL(query.get_pool_continuation(projection, 1, block_limit, 25, 20), error::integrity);
}

// get_pool_arrivals
// ----------------------------------------------------------------------------

BC_AUTO_TEST_CASE(query_pool__get_pool_arrivals__empty__empty_heights)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    store.set_pooling();
    pool_arrivals out{};
    const auto ec = query.get_pool_arrivals(out, 10, 3, 20);

    BC_REQUIRE_EQUAL(ec, error::success);
    BC_REQUIRE_EQUAL(out.size(), 3u);
    BC_REQUIRE(out[0].empty());
    BC_REQUIRE(out[1].empty());
    BC_REQUIRE(out[2].empty());
}

BC_AUTO_TEST_CASE(query_pool__get_pool_arrivals__two_heights__top_first)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    store.set_pooling();
    const auto parent = to_spend(genesis_coinbase(), 4'999'999'000);
    const auto child = to_spend(parent.hash(false), 4'999'990'000);
    const auto size = system::possible_narrow_cast<uint32_t>(parent.virtual_size());
    const auto pooled = pool_populated(query, parent, 8) && pool_populated(query, child, 9);
    pool_arrivals out{};
    const auto ec = query.get_pool_arrivals(out, 10, 3, 20);

    BC_REQUIRE(pooled);
    BC_REQUIRE_EQUAL(ec, error::success);
    BC_REQUIRE_EQUAL(out.size(), 3u);
    BC_REQUIRE_EQUAL(out[0].size(), 1u);
    BC_REQUIRE_EQUAL(out[0][0].rate, 9'000'000u / size);
    BC_REQUIRE_EQUAL(out[0][0].size, size);
    BC_REQUIRE_EQUAL(out[1].size(), 1u);
    BC_REQUIRE_EQUAL(out[1][0].rate, 1'000'000u / size);
    BC_REQUIRE_EQUAL(out[1][0].size, size);
    BC_REQUIRE(out[2].empty());
}

BC_AUTO_TEST_CASE(query_pool__get_pool_arrivals__top_height__excluded)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    store.set_pooling();
    const auto parent = to_spend(genesis_coinbase(), 4'999'999'000);
    const auto child = to_spend(parent.hash(false), 4'999'990'000);
    const auto size = system::possible_narrow_cast<uint32_t>(parent.virtual_size());
    const auto pooled = pool_populated(query, parent, 8) && pool_populated(query, child, 9);
    pool_arrivals out{};
    const auto ec = query.get_pool_arrivals(out, 9, 2, 20);

    BC_REQUIRE(pooled);
    BC_REQUIRE_EQUAL(ec, error::success);
    BC_REQUIRE_EQUAL(out.size(), 2u);
    BC_REQUIRE_EQUAL(out[0].size(), 1u);
    BC_REQUIRE_EQUAL(out[0][0].rate, 1'000'000u / size);
    BC_REQUIRE(out[1].empty());
}

BC_AUTO_TEST_CASE(query_pool__get_pool_arrivals__below_count__excluded)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    store.set_pooling();
    const auto parent = to_spend(genesis_coinbase(), 4'999'999'000);
    const auto child = to_spend(parent.hash(false), 4'999'990'000);
    const auto size = system::possible_narrow_cast<uint32_t>(parent.virtual_size());
    const auto pooled = pool_populated(query, parent, 8) && pool_populated(query, child, 9);
    pool_arrivals out{};
    const auto ec = query.get_pool_arrivals(out, 10, 1, 20);

    BC_REQUIRE(pooled);
    BC_REQUIRE_EQUAL(ec, error::success);
    BC_REQUIRE_EQUAL(out.size(), 1u);
    BC_REQUIRE_EQUAL(out[0].size(), 1u);
    BC_REQUIRE_EQUAL(out[0][0].rate, 9'000'000u / size);
}

BC_AUTO_TEST_CASE(query_pool__get_pool_arrivals__confirmed_rival__confirmed_only)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    store.set_pooling();
    const auto parent = to_spend(genesis_coinbase(), 4'999'999'000);
    const auto rival = to_spend(genesis_coinbase(), 4'999'998'500);
    const auto size = system::possible_narrow_cast<uint32_t>(rival.virtual_size());
    const auto pooled = pool_populated(query, parent) && pool_populated(query, rival);
    const auto confirmed = !archive_confirm(query, rival).is_terminal();
    pool_arrivals out{};
    const auto ec = query.get_pool_arrivals(out, 9, 1, 20);

    BC_REQUIRE(pooled);
    BC_REQUIRE(confirmed);
    BC_REQUIRE_EQUAL(ec, error::success);
    BC_REQUIRE_EQUAL(out[0].size(), 1u);
    BC_REQUIRE_EQUAL(out[0][0].rate, 1'500'000u / size);
}

BC_AUTO_TEST_CASE(query_pool__get_pool_arrivals__zero_count__empty)
{
    settings settings{ test::database_settings() };
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BC_REQUIRE(!store.create(test::events_handler));
    BC_REQUIRE(query.initialize(test::genesis));

    store.set_pooling();
    const auto parent = to_spend(genesis_coinbase(), 4'999'999'000);
    const auto pooled = pool_populated(query, parent);
    pool_arrivals out{ {} };
    const auto ec = query.get_pool_arrivals(out, 9, 0, 20);

    BC_REQUIRE(pooled);
    BC_REQUIRE_EQUAL(ec, error::success);
    BC_REQUIRE(out.empty());
}

BC_AUTO_TEST_SUITE_END()
