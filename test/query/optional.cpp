/**
 * Copyright (c) 2011-2025 libbitcoin developers (see AUTHORS)
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
#include "../mocks/blocks.hpp"
#include "../mocks/chunk_store.hpp"

struct query_optional_setup_fixture
{
    DELETE_COPY_MOVE(query_optional_setup_fixture);
    BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

    query_optional_setup_fixture() NOEXCEPT
    {
        BOOST_REQUIRE(test::clear(test::directory));
    }

    ~query_optional_setup_fixture() NOEXCEPT
    {
        BOOST_REQUIRE(test::clear(test::directory));
    }

    BC_POP_WARNING()
};

BOOST_FIXTURE_TEST_SUITE(query_optional_tests, query_optional_setup_fixture)

// nop event handler.
const auto events_handler = [](auto, auto) {};

const auto genesis_address = test::genesis.transactions_ptr()->front()->outputs_ptr()->front()->script().hash();

BOOST_AUTO_TEST_CASE(query_optional__get_confirmed_balance__genesis__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(events_handler), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));
    ////BOOST_REQUIRE(query.set_address_output(genesis_address, query.to_output(0, 0)));

    uint64_t out{};
    BOOST_REQUIRE(query.get_confirmed_balance(out, genesis_address));
    BOOST_REQUIRE_EQUAL(out, 5000000000u);
}

BOOST_AUTO_TEST_CASE(query_optional__to_address_outputs__genesis__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(events_handler), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));
    ////BOOST_REQUIRE(query.set_address_output(genesis_address, query.to_output(0, 0)));

    output_links out{};
    BOOST_REQUIRE(query.to_address_outputs(out, genesis_address));
    BOOST_REQUIRE_EQUAL(out.size(), 1u);
    BOOST_REQUIRE_EQUAL(out.front(), query.to_output(0, 0));
}

BOOST_AUTO_TEST_CASE(query_optional__to_confirmed_unspent_outputs__genesis__expected)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(events_handler), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));
    ////BOOST_REQUIRE(query.set_address_output(genesis_address, query.to_output(0, 0)));

    output_links out{};
    BOOST_REQUIRE(query.to_confirmed_unspent_outputs(out, genesis_address));
    BOOST_REQUIRE_EQUAL(out.size(), 1u);
    BOOST_REQUIRE_EQUAL(out.front(), 0);
}

////BOOST_AUTO_TEST_CASE(query_optional__to_minimum_unspent_outputs__above__excluded)
////{
////    settings settings{};
////    settings.path = TEST_DIRECTORY;
////    test::chunk_store store{ settings };
////    test::query_accessor query{ store };
////    BOOST_REQUIRE_EQUAL(store.create(events_handler), error::success);
////    BOOST_REQUIRE(query.initialize(test::genesis));
////    BOOST_REQUIRE(query.set_address_output(genesis_address, 0));
////
////    output_links out{};
////    BOOST_REQUIRE(query.to_minimum_unspent_outputs(out, genesis_address, 5000000001));
////    BOOST_REQUIRE(out.empty());
////}

BOOST_AUTO_TEST_CASE(query_optional__to_minimum_unspent_outputs__at__included)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(events_handler), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));
    ////BOOST_REQUIRE(query.set_address_output(genesis_address, 0));

    output_links out{};
    BOOST_REQUIRE(query.to_minimum_unspent_outputs(out, genesis_address, 5000000000));
    BOOST_REQUIRE_EQUAL(out.size(), 1u);
    BOOST_REQUIRE_EQUAL(out.front(), 0);
}

BOOST_AUTO_TEST_CASE(query_optional__to_minimum_unspent_outputs__below__included)
{
    settings settings{};
    settings.path = TEST_DIRECTORY;
    test::chunk_store store{ settings };
    test::query_accessor query{ store };
    BOOST_REQUIRE_EQUAL(store.create(events_handler), error::success);
    BOOST_REQUIRE(query.initialize(test::genesis));
    ////BOOST_REQUIRE(query.set_address_output(genesis_address, 0));

    output_links out{};
    BOOST_REQUIRE(query.to_minimum_unspent_outputs(out, genesis_address, 0));
    BOOST_REQUIRE_EQUAL(out.size(), 1u);
    BOOST_REQUIRE_EQUAL(out.front(), 0);
    BOOST_REQUIRE(query.to_minimum_unspent_outputs(out, genesis_address, 4999999999));
    BOOST_REQUIRE_EQUAL(out.size(), 1u);
    BOOST_REQUIRE_EQUAL(out.front(), 0);
}

////BOOST_AUTO_TEST_CASE(query_optional__set_filter__get_filter_and_head__expected)
////{
////    const auto& filter_head0 = system::null_hash;
////    const auto filter0 = system::base16_chunk("0102030405060708090a0b0c0d0e0f");
////    const auto& filter_head1 = system::one_hash;
////    const auto filter1 = system::base16_chunk("102030405060708090a0b0c0d0e0f0102030405060708090a0b0c0d0e0f0");
////
////    settings settings{};
////    settings.path = TEST_DIRECTORY;
////    test::chunk_store store{ settings };
////    test::query_accessor query{ store };
////    BOOST_REQUIRE_EQUAL(store.create(events_handler), error::success);
////    BOOST_REQUIRE(query.initialize(test::genesis));
////    BOOST_REQUIRE(query.set(test::block1a, context{}, false, false));
////    BOOST_REQUIRE(query.set_filter(0, filter_head0, filter0));
////    BOOST_REQUIRE(query.set_filter(1, filter_head1, filter1));
////
////    hash_digest head{};
////    BOOST_REQUIRE(query.get_filter_head(head, 0));
////    BOOST_REQUIRE_EQUAL(head, filter_head0);
////    BOOST_REQUIRE(query.get_filter_head(head, 1));
////    BOOST_REQUIRE_EQUAL(head, filter_head1);
////
////    system::data_chunk out{};
////    BOOST_REQUIRE(query.get_filter(out, 0));
////    BOOST_REQUIRE_EQUAL(out, filter0);
////    BOOST_REQUIRE(query.get_filter(out, 1));
////    BOOST_REQUIRE_EQUAL(out, filter1);
////}

////BOOST_AUTO_TEST_CASE(query_optional__set_buffered_tx__get_buffered_tx__expected)
////{
////    settings settings{};
////    settings.path = TEST_DIRECTORY;
////    test::chunk_store store{ settings };
////    test::query_accessor query{ store };
////    BOOST_REQUIRE_EQUAL(store.create(events_handler), error::success);
////    BOOST_REQUIRE(query.initialize(test::genesis));
////    BOOST_REQUIRE(query.set(test::tx4));
////    BOOST_REQUIRE(query.set(test::tx5));
////    BOOST_REQUIRE(query.set_buffered_tx(1, test::tx4));
////    BOOST_REQUIRE(query.set_buffered_tx(2, test::tx4));
////    BOOST_REQUIRE(*query.get_buffered_tx(1) == test::tx4);
////    BOOST_REQUIRE(*query.get_buffered_tx(2) == test::tx4);
////}
////
////BOOST_AUTO_TEST_CASE(query_optional__set_buffered_tx__unarchived_transaction__expected)
////{
////    settings settings{};
////    settings.path = TEST_DIRECTORY;
////    test::chunk_store store{ settings };
////    test::query_accessor query{ store };
////    BOOST_REQUIRE_EQUAL(store.create(events_handler), error::success);
////    BOOST_REQUIRE(query.initialize(test::genesis));
////
////    // Use of the tx_link as key is conventional.
////    BOOST_REQUIRE(query.set_buffered_tx(42, {}));
////    BOOST_REQUIRE(*query.get_buffered_tx(42) == system::chain::transaction{});
////}
////
////BOOST_AUTO_TEST_CASE(query_optional__get_buffered_tx__unset_transaction__nullptr)
////{
////    settings settings{};
////    settings.path = TEST_DIRECTORY;
////    test::chunk_store store{ settings };
////    test::query_accessor query{ store };
////    BOOST_REQUIRE_EQUAL(store.create(events_handler), error::success);
////    BOOST_REQUIRE(query.initialize(test::genesis));
////    BOOST_REQUIRE(!query.get_buffered_tx(42));
////}
////
////BOOST_AUTO_TEST_CASE(query_optional__set_bootstrap__above_confirmed__false)
////{
////    settings settings{};
////    settings.path = TEST_DIRECTORY;
////    test::chunk_store store{ settings };
////    test::query_accessor query{ store };
////    BOOST_REQUIRE_EQUAL(store.create(events_handler), error::success);
////    BOOST_REQUIRE(query.initialize(test::genesis));
////    BOOST_REQUIRE(query.set(test::block1, context{}));
////    BOOST_REQUIRE(query.set(test::block2, context{}));
////    BOOST_REQUIRE(query.set(test::block3, context{}));
////    BOOST_REQUIRE(query.push_confirmed(1));
////    BOOST_REQUIRE(!query.set_bootstrap(2));
////    BOOST_REQUIRE(query.set_bootstrap(0));
////}
////
////BOOST_AUTO_TEST_CASE(query_optional__set_bootstrap__twice__clears_previous)
////{
////    settings settings{};
////    settings.path = TEST_DIRECTORY;
////    test::chunk_store store{ settings };
////    test::query_accessor query{ store };
////    BOOST_REQUIRE_EQUAL(store.create(events_handler), error::success);
////    BOOST_REQUIRE(query.initialize(test::genesis));
////    BOOST_REQUIRE(query.set(test::block1, context{}));
////    BOOST_REQUIRE(query.set(test::block2, context{}));
////    BOOST_REQUIRE(query.set(test::block3, context{}));
////    BOOST_REQUIRE(query.push_confirmed(1));
////    BOOST_REQUIRE(query.push_confirmed(2));
////    BOOST_REQUIRE(query.push_confirmed(3));
////    BOOST_REQUIRE(query.set_bootstrap(2));
////    BOOST_REQUIRE(query.set_bootstrap(3));
////
////    const hashes expected
////    {
////        test::genesis.hash(),
////        test::block1.hash(),
////        test::block2.hash(),
////        test::block3.hash()
////    };
////
////    hashes out{};
////    BOOST_REQUIRE(query.get_bootstrap(out));
////    BOOST_REQUIRE_EQUAL(out, expected);
////}

BOOST_AUTO_TEST_SUITE_END()
