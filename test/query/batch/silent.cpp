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

BOOST_FIXTURE_TEST_SUITE(query_batch_silent_tests, test::directory_setup_fixture)

using silent_payment = system::wallet::silent_payment;
using receiver = system::silent::batch::receiver;

constexpr system::ec_compressed summary = system::base16_array
(
    "024ac253c216532e961988e2a8ce266a447c894c781e52ef6cee902361db960004"
);

static system::chain::input to_input(const system::hash_digest& hash,
    const system::data_chunk& script,
    const system::data_chunk& prevout) NOEXCEPT
{
    namespace chain = system::chain;
    chain::input in{ { hash, 0 }, { script, false }, {}, max_uint32 };
    in.prevout = system::to_shared<chain::output>(0u,
        chain::script{ prevout, false });
    return in;
}

// BIP352 send_and_receive_test_vectors.json: "Simple send: two inputs".
static system::chain::transaction simple_send() NOEXCEPT
{
    return
    {
        2u,
        system::chain::inputs
        {
            to_input(
                system::base16_hash("f4184fc596403b9d638783cf57adfe4c75c605f6356fbc91338530e9831e9e16"),
                system::base16_chunk("483046022100ad79e6801dd9a8727f342f31c71c4912866f59dc6e7981878e92c5844a0ce929022100fb0d2393e813968648b9753b7e9871d90ab3d815ebf91820d704b19f4ed224d621025a1e61f898173040e20616d43e9f496fba90338a39faa1ed98fcbaeee4dd9be5"),
                system::base16_chunk("76a91419c2f3ae0ca3b642bd3e49598b8da89f50c1416188ac")),
            to_input(
                system::base16_hash("a1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d"),
                system::base16_chunk("48304602210086783ded73e961037e77d49d9deee4edc2b23136e9728d56e4491c80015c3a63022100fda4c0f21ea18de29edbce57f7134d613e044ee150a89e2e64700de2d4e83d4e2103bd85685d03d111699b15d046319febe77f8de5286e9e512703cdee1bf3be3792"),
                system::base16_chunk("76a914d9317c66f54ff0a152ec50b1d19c25be50c8e15988ac"))
        },
        system::chain::outputs
        {
            system::chain::output
            {
                0u,
                system::chain::script
                {
                    system::base16_chunk("51203e9fce73d4e77a4809908e3c3a2e54ee147b9312dc5044a193d1fc85de46e3c1"),
                    false
                }
            }
        },
        0u
    };
}

static receiver get_keys() NOEXCEPT
{
    constexpr system::ec_secret scan = system::base16_array(
        "0f694e068028a717f8af6b9411f9a133dd3565258714cc226594b34db90c1f2c");
    constexpr system::ec_secret spend = system::base16_array(
        "9d6ad855ce3417ef84e836892e5a56392bfba05fa5d97ccea30e266f540e08b3");

    system::ec_compressed point{};
    if (!system::secret_to_public(point, spend))
        return {};

    return silent_payment{ scan, point, {} }.keys();
}

// set_silent
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(query_batch_silent__set_silent__coinbase_only__true)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set_silent(0, test::genesis));
}

BOOST_AUTO_TEST_CASE(query_batch_silent__set_silent__unarchived_block__false)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(!query.set_silent(1, test::block_coinbase_spend_1a));
}

BOOST_AUTO_TEST_CASE(query_batch_silent__set_silent__no_taproot_output__no_records)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set_silent(1, *test::block1.transactions_ptr()->front()));
    BOOST_REQUIRE_EQUAL(query.silent_records(), 0u);
}

BOOST_AUTO_TEST_CASE(query_batch_silent__set_silent__eligible__one_record)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set_silent(42, simple_send()));
    BOOST_REQUIRE_EQUAL(query.silent_records(), 1u);
}

// scan_silent
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(query_batch_silent__scan_silent__match__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set_silent(42, simple_send()));

    size_t calls{};
    uint32_t link{};
    system::ec_compressed point{};
    const stopper cancel{};
    BOOST_REQUIRE(query.scan_silent(cancel, get_keys(),
        [&](const code& ec, uint32_t tx, const system::ec_compressed& tweak) NOEXCEPT
        {
            BOOST_REQUIRE(!ec);
            link = tx;
            point = tweak;
            ++calls;
        }));

    BOOST_REQUIRE_EQUAL(calls, 1u);
    BOOST_REQUIRE_EQUAL(link, 42u);
    BOOST_REQUIRE_EQUAL(point, summary);
}

BOOST_AUTO_TEST_CASE(query_batch_silent__scan_silent__no_match__none)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set_silent(42, simple_send()));

    size_t calls{};
    const stopper cancel{};
    const receiver keys
    {
        .scan = system::base16_array("0000000000000000000000000000000000000000000000000000000000000001"),
        .spend = get_keys().spend
    };

    BOOST_REQUIRE(query.scan_silent(cancel, keys,
        [&](const code&, uint32_t, const system::ec_compressed&) NOEXCEPT
        {
            ++calls;
        }));
    BOOST_REQUIRE_EQUAL(calls, 0u);
}

BOOST_AUTO_TEST_CASE(query_batch_silent__scan_silent__range__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE(query.set_silent(42, simple_send()));
    BOOST_REQUIRE(query.set_silent(43, simple_send()));

    std::vector<uint32_t> links{};
    const stopper cancel{};
    BOOST_REQUIRE(query.scan_silent(cancel, get_keys(), 1, 2,
        [&](const code&, uint32_t tx, const system::ec_compressed&) NOEXCEPT
        {
            links.push_back(tx);
        }));
    BOOST_REQUIRE_EQUAL(links, std::vector<uint32_t>{ 43 });
}

// get_silent_frontier
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(query_batch_silent__get_silent_frontier__empty__zero)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE(!store.create(test::events_handler));
    BOOST_REQUIRE(query.initialize(test::genesis));
    BOOST_REQUIRE_EQUAL(query.get_silent_frontier(0), 0u);
}

BOOST_AUTO_TEST_CASE(query_batch_silent__get_silent_frontier__written__count)
{
    settings settings{};
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
