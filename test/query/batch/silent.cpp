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
#include "../../mocks/map_store.hpp"

BOOST_FIXTURE_TEST_SUITE(query_batch_silent_tests, test::directory_setup_fixture)

using namespace system;
using silent_payment = wallet::silent_payment;
using receiver = system::scan::batch::receiver;
using tx_link_t = system::scan::batch::tx_link_t;

constexpr ec_compressed expected_point = base16_array
(
    "024ac253c216532e961988e2a8ce266a447c894c781e52ef6cee902361db960004"
);
constexpr ec_secret scan_secret = base16_array
(
    "0f694e068028a717f8af6b9411f9a133dd3565258714cc226594b34db90c1f2c"
);
constexpr ec_secret spend_secret = base16_array
(
    "9d6ad855ce3417ef84e836892e5a56392bfba05fa5d97ccea30e266f540e08b3"
);

static chain::input to_input(const hash_digest& hash, const data_chunk& script,
    const data_chunk& prevout) NOEXCEPT
{
    chain::input in{ { hash, 0 }, { script, false }, {}, max_uint32 };
    const chain::script prevout_script{ prevout, false };
    in.prevout = to_shared<chain::output>(0u, prevout_script);
    return in;
}

// BIP352 send_and_receive_test_vectors.json: "Simple send: two inputs".
static const chain::input input0 = to_input
(
    base16_hash("f4184fc596403b9d638783cf57adfe4c75c605f6356fbc91338530e9831e9e16"),
    base16_chunk("483046022100ad79e6801dd9a8727f342f31c71c4912866f59dc6e7981878e92c5844a0ce929022100fb0d2393e813968648b9753b7e9871d90ab3d815ebf91820d704b19f4ed224d621025a1e61f898173040e20616d43e9f496fba90338a39faa1ed98fcbaeee4dd9be5"),
    base16_chunk("76a91419c2f3ae0ca3b642bd3e49598b8da89f50c1416188ac")
);
static const chain::input input1 = to_input
(
    base16_hash("a1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d"),
    base16_chunk("48304602210086783ded73e961037e77d49d9deee4edc2b23136e9728d56e4491c80015c3a63022100fda4c0f21ea18de29edbce57f7134d613e044ee150a89e2e64700de2d4e83d4e2103bd85685d03d111699b15d046319febe77f8de5286e9e512703cdee1bf3be3792"),
    base16_chunk("76a914d9317c66f54ff0a152ec50b1d19c25be50c8e15988ac")
);
static const chain::output output0
{
    0u,
    chain::script{ base16_chunk("51203e9fce73d4e77a4809908e3c3a2e54ee147b9312dc5044a193d1fc85de46e3c1"), false }
};

static chain::transaction simple_send() NOEXCEPT
{
    return { 2u, { input0, input1 }, { output0 }, 0u };
}

static receiver get_keys() NOEXCEPT
{
    ec_compressed spend{};
    if (!secret_to_public(spend, spend_secret))
        return {};

    return silent_payment{ scan_secret, spend, {} }.keys();
}

// set_silent
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(query_batch_silent__set_silent__coinbase_only__true)
{
    database::settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set_silent(0, test::genesis));
}

BOOST_AUTO_TEST_CASE(query_batch_silent__set_silent__unarchived_block__false)
{
    database::settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(!query.set_silent(1, test::block_coinbase_spend_1a));
}

BOOST_AUTO_TEST_CASE(query_batch_silent__set_silent__no_taproot_output__no_records)
{
    database::settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set_silent(1, *test::block1.transactions_ptr()->front()));
    BOOST_REQUIRE_EQUAL(query.scan_records(), 0u);
}

BOOST_AUTO_TEST_CASE(query_batch_silent__set_silent__eligible__one_record)
{
    database::settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set_silent(42, simple_send()));
    BOOST_REQUIRE_EQUAL(query.scan_records(), 1u);
}

BOOST_AUTO_TEST_CASE(query_batch_silent__set_silent__eligible__rows_complete)
{
    database::settings settings{};
    settings.path = TEST_DIRECTORY;
    test::map_store store{ settings };
    database::query<test::map_store> instance{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(instance.initialize(test::genesis));
    BOOST_REQUIRE(instance.set_silent(42, simple_send()));
    BOOST_REQUIRE_EQUAL(instance.scan_records(), 1u);
    BOOST_REQUIRE_EQUAL(store.scan_frontier_(), store.scan_logical_());
    BOOST_REQUIRE(!store.close(test::events_handler));
}

// batch
// ----------------------------------------------------------------------------

static chain::block silent_block() NOEXCEPT
{
    const auto& coinbase = *test::block1.transactions_ptr()->front();
    return { test::block1.header(), { coinbase, simple_send() } };
}

static data_chunk to_prevouts(const chain::block& block) NOEXCEPT
{
    data_chunk out{};
    const auto& txs = *block.transactions_ptr();
    for (auto tx = std::next(txs.cbegin()); tx != txs.cend(); ++tx)
    {
        for (const auto& input: *(*tx)->inputs_ptr())
        {
            const auto data = input->prevout->to_data();
            out.insert(out.end(), data.cbegin(), data.cend());
        }
    }

    return out;
}

BOOST_AUTO_TEST_CASE(query_batch_silent__set_silents__coinbase_only__no_rows)
{
    database::settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));

    size_t rows{};
    BOOST_REQUIRE(query.set_silents(rows, 0, test::genesis, false));
    BOOST_REQUIRE_EQUAL(rows, 0u);
    BOOST_REQUIRE_EQUAL(query.silent_records(false), 0u);
}

BOOST_AUTO_TEST_CASE(query_batch_silent__set_silents__eligible__banked_row)
{
    database::settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));

    const auto block = silent_block();
    BOOST_REQUIRE(query.set(block, database::context{ 0, 1, 0 }, {}, false, false));
    const auto link = query.to_header(block.hash());

    size_t rows{};
    BOOST_REQUIRE(query.set_silents(rows, link, block, true));
    BOOST_REQUIRE_EQUAL(rows, 1u);
    BOOST_REQUIRE_EQUAL(query.silent_records(true), 1u);
    BOOST_REQUIRE_EQUAL(query.silent_records(false), 0u);
    BOOST_REQUIRE_EQUAL(query.scan_records(), 0u);
}

BOOST_AUTO_TEST_CASE(query_batch_silent__set_silents__eligible_view__banked_row)
{
    database::settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));

    const auto block = silent_block();
    BOOST_REQUIRE(query.set(block, database::context{ 0, 1, 0 }, {}, false, false));
    const auto link = query.to_header(block.hash());

    chain::view::block view{ block.to_data(true), true };
    BOOST_REQUIRE(!view.populate(chain::context{}, to_prevouts(block)));

    size_t rows{};
    BOOST_REQUIRE(query.set_silents(rows, link, view, true));
    BOOST_REQUIRE_EQUAL(rows, 1u);
    BOOST_REQUIRE_EQUAL(query.silent_records(true), 1u);
    BOOST_REQUIRE_EQUAL(query.silent_records(false), 0u);
}

static const chain::script taproot_script
{
    base16_chunk("51203e9fce73d4e77a4809908e3c3a2e54ee147b9312dc5044a193d1fc85de46e3c1"),
    false
};
static const chain::script witness_script
{
    base16_chunk("001419c2f3ae0ca3b642bd3e49598b8da89f50c14161"),
    false
};

static chain::block spend_block(const chain::script& script) NOEXCEPT
{
    const auto& parent = *test::block1.transactions_ptr()->front();
    const chain::inputs ins{ { { parent.hash(false), 0 }, {}, max_uint32 } };
    const chain::outputs outs{ { 0, script } };
    const auto& coinbase = *test::block2.transactions_ptr()->front();
    return { test::block2.header(), { coinbase, { 1, ins, outs, 0 } } };
}

BOOST_AUTO_TEST_CASE(query_batch_silent__get_silent_prevouts__taproot_spend__selected)
{
    database::settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, database::context{ 0, 1, 0 }, {}, false, false));

    const auto block = spend_block(taproot_script);
    BOOST_REQUIRE(query.set(block, database::context{ 0, 2, 0 }, {}, false, false));
    const auto link = query.to_header(block.hash());

    data_chunk prevouts{};
    std::vector<bool> selected{};
    const chain::view::block view{ block.to_data(true), true };
    const auto& parent = *test::block1.transactions_ptr()->front();
    BOOST_REQUIRE(query.get_silent_prevouts(prevouts, selected, link, view));
    BOOST_REQUIRE(selected == (std::vector<bool>{ false, true }));
    BOOST_REQUIRE_EQUAL(prevouts, parent.outputs_ptr()->front()->to_data());
}

BOOST_AUTO_TEST_CASE(query_batch_silent__get_silent_prevouts__no_taproot_output__unselected)
{
    database::settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set(test::block1, database::context{ 0, 1, 0 }, {}, false, false));

    const auto block = spend_block(witness_script);
    BOOST_REQUIRE(query.set(block, database::context{ 0, 2, 0 }, {}, false, false));
    const auto link = query.to_header(block.hash());

    data_chunk prevouts{};
    std::vector<bool> selected{};
    const chain::view::block view{ block.to_data(true), true };
    BOOST_REQUIRE(query.get_silent_prevouts(prevouts, selected, link, view));
    BOOST_REQUIRE(selected == (std::vector<bool>{ false, false }));
    BOOST_REQUIRE(prevouts.empty());
}

BOOST_AUTO_TEST_CASE(query_batch_silent__compute_silents__banked_row__scanned)
{
    database::settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));

    const auto block = silent_block();
    BOOST_REQUIRE(query.set(block, database::context{ 0, 1, 0 }, {}, false, false));
    const auto link = query.to_header(block.hash());
    const auto tx = query.to_tx(simple_send().hash(false));

    size_t rows{};
    const stopper cancel{};
    BOOST_REQUIRE(query.set_silents(rows, link, block, false));
    BOOST_REQUIRE_EQUAL(query.compute_silents(cancel, false), database::error::success);
    BOOST_REQUIRE_EQUAL(query.scan_records(), 1u);

    std::vector<tx_link_t> links{};
    const auto handler = [&](const code&, tx_link_t link, const ec_compressed&) NOEXCEPT
    {
        links.push_back(link);
    };

    BOOST_REQUIRE(query.scan_silent(cancel, get_keys(), handler));
    BOOST_REQUIRE_EQUAL(links, std::vector<tx_link_t>{ tx.value });
}

BOOST_AUTO_TEST_CASE(query_batch_silent__compute_silents__empty__success)
{
    database::settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));

    const stopper cancel{};
    BOOST_REQUIRE_EQUAL(query.compute_silents(cancel, false), database::error::success);
    BOOST_REQUIRE_EQUAL(query.scan_records(), 0u);
}

BOOST_AUTO_TEST_CASE(query_batch_silent__purge_silents__banked_row__empty)
{
    database::settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));

    const auto block = silent_block();
    BOOST_REQUIRE(query.set(block, database::context{ 0, 1, 0 }, {}, false, false));

    size_t rows{};
    BOOST_REQUIRE(query.set_silents(rows, query.to_header(block.hash()), block, false));
    BOOST_REQUIRE(query.purge_silents(false));
    BOOST_REQUIRE_EQUAL(query.silent_records(false), 0u);
}

// scan_silent
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(query_batch_silent__scan_silent__match__expected)
{
    database::settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set_silent(42, simple_send()));

    code error{};
    size_t calls{};
    tx_link_t link{};
    ec_compressed point{};
    const auto handler = [&](const code& ec, tx_link_t tx, const ec_compressed& key) NOEXCEPT
    {
        error = ec;
        link = tx;
        point = key;
        ++calls;
    };

    const stopper cancel{};
    BOOST_REQUIRE(query.scan_silent(cancel, get_keys(), handler));
    BOOST_REQUIRE(!error);
    BOOST_REQUIRE_EQUAL(calls, 1u);
    BOOST_REQUIRE_EQUAL(link, 42u);
    BOOST_REQUIRE_EQUAL(point, expected_point);
}

BOOST_AUTO_TEST_CASE(query_batch_silent__scan_silent__no_match__none)
{
    database::settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set_silent(42, simple_send()));

    size_t calls{};
    const auto handler = [&](const code&, tx_link_t, const ec_compressed&) NOEXCEPT
    {
        ++calls;
    };

    const receiver keys
    {
        .scan = base16_array("0000000000000000000000000000000000000000000000000000000000000001"),
        .spend = get_keys().spend
    };

    const stopper cancel{};
    BOOST_REQUIRE(query.scan_silent(cancel, keys, handler));
    BOOST_REQUIRE_EQUAL(calls, 0u);
}

BOOST_AUTO_TEST_CASE(query_batch_silent__scan_silent__range__expected)
{
    database::settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set_silent(42, simple_send()));
    BOOST_REQUIRE(query.set_silent(43, simple_send()));

    std::vector<tx_link_t> links{};
    const auto handler = [&](const code&, tx_link_t tx, const ec_compressed&) NOEXCEPT
    {
        links.push_back(tx);
    };

    const stopper cancel{};
    BOOST_REQUIRE(query.scan_silent(cancel, get_keys(), 1, 2, handler));
    BOOST_REQUIRE_EQUAL(links, std::vector<tx_link_t>{ 43 });
}

// get_silent_frontier
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(query_batch_silent__get_silent_frontier__empty__zero)
{
    database::settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE_EQUAL(query.get_silent_frontier(0), 0u);
}

BOOST_AUTO_TEST_CASE(query_batch_silent__get_silent_frontier__written__count)
{
    database::settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set_silent(42, simple_send()));
    BOOST_REQUIRE(query.set_silent(43, simple_send()));
    BOOST_REQUIRE_EQUAL(query.get_silent_frontier(0), 2u);
    BOOST_REQUIRE_EQUAL(query.get_silent_frontier(1), 2u);
}

BOOST_AUTO_TEST_SUITE_END()
