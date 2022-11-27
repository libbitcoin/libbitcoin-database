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
#include "mocks/storage.hpp"

struct query_setup_fixture
{
    DELETE4(query_setup_fixture);
    BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

        query_setup_fixture() NOEXCEPT
    {
        BOOST_REQUIRE(test::clear(test::directory));
    }

    ~query_setup_fixture() NOEXCEPT
    {
        BOOST_REQUIRE(test::clear(test::directory));
    }

    BC_POP_WARNING()
};

BOOST_FIXTURE_TEST_SUITE(query_tests, query_setup_fixture)

class store_accessor
  : public store<test::storage>
{
public:
    using path = std::filesystem::path;
    using store<test::storage>::store;

    const system::data_chunk& header_head() NOEXCEPT
    {
        return header_head_.buffer();
    }

    const system::data_chunk& header_body() NOEXCEPT
    {
        return header_body_.buffer();
    }

    const system::data_chunk& point_head() NOEXCEPT
    {
        return point_head_.buffer();
    }

    const system::data_chunk& point_body() NOEXCEPT
    {
        return point_body_.buffer();
    }

    const system::data_chunk& input_head() NOEXCEPT
    {
        return input_head_.buffer();
    }

    const system::data_chunk& input_body() NOEXCEPT
    {
        return input_body_.buffer();
    }

    const system::data_chunk& output_body() NOEXCEPT
    {
        return output_body_.buffer();
    }

    const system::data_chunk& puts_body() NOEXCEPT
    {
        return puts_body_.buffer();
    }

    const system::data_chunk& tx_head() NOEXCEPT
    {
        return tx_head_.buffer();
    }

    const system::data_chunk& tx_body() NOEXCEPT
    {
        return tx_body_.buffer();
    }

    const system::data_chunk& txs_head() NOEXCEPT
    {
        return txs_head_.buffer();
    }

    const system::data_chunk& txs_body() NOEXCEPT
    {
        return txs_body_.buffer();
    }
};

BOOST_AUTO_TEST_CASE(query__set_header__default_header__true)
{
    settings settings_{};
    settings_.header_buckets = 10;
    settings_.dir = TEST_DIRECTORY;
    store_accessor store_{ settings_ };
    query<store<test::storage>> instance{ store_ };
    BOOST_REQUIRE_EQUAL(store_.create(), error::success);
    BOOST_REQUIRE_EQUAL(store_.open(), error::success);

    constexpr context context
    {
        0x01020304, // height
        0x11121314, // flags
        0x21222324  // mtp
    };
    constexpr auto block_hash = system::base16_array("80a911db3f348dc686aa9711444562e0b4c10e5cccbd241be3b01412c42a3d7c");
    constexpr auto parent = system::base16_array("110102030405060708090a0b0c0d0e0f220102030405060708090a0b0c0d0e0f");
    constexpr auto root = system::base16_array("119192939495969798999a9b9c9d9e9f229192939495969798999a9b9c9d9e9f");
    const system::chain::header header
    {
        0x31323334, // version
        parent,     // previous_block_hash
        root,       // merkle_root
        0x41424344, // timestamp
        0x51525354, // bits
        0x61626364  // nonce
    };
    BOOST_REQUIRE_EQUAL(header.hash(), block_hash);

    // transactor scope.
    {
        const auto transactor = store_.get_transactor();
        BOOST_REQUIRE(instance.set_header(header, context));
    }

    BOOST_REQUIRE_EQUAL(store_.close(), error::success);

    ////std::cout << store_.header_head() << std::endl << std::endl;
    const auto expected_head = system::base16_chunk(
        "010000" // record count
        "ffffff" // bucket[0]...
        "ffffff"
        "000000" // pk->
        "ffffff"
        "ffffff"
        "ffffff"
        "ffffff"
        "ffffff"
        "ffffff"
        "ffffff");
    ////std::cout << store_.header_body() << std::endl << std::endl;
    const auto expected_body = system::base16_chunk(
        "ffffff"   // next->
        "80a911db3f348dc686aa9711444562e0b4c10e5cccbd241be3b01412c42a3d7c" // sk (block.hash)
        "040302"   // height
        "14131211" // flags
        "24232221" // mtp
        "ffffff"   // previous_block_hash (header_fk - not found)
        "34333231" // version
        "44434241" // timestamp
        "54535251" // bits
        "64636261" // nonce
        "119192939495969798999a9b9c9d9e9f229192939495969798999a9b9c9d9e9f"); // merkle_root

    BOOST_REQUIRE_EQUAL(store_.header_head(), expected_head);
    BOOST_REQUIRE_EQUAL(store_.header_body(), expected_body);
}

BOOST_AUTO_TEST_CASE(query__set_tx__empty_transaction__false)
{
    settings settings_{};
    settings_.dir = TEST_DIRECTORY;
    store<map> store_{ settings_ };
    query<store<map>> instance{ store_ };
    BOOST_REQUIRE_EQUAL(store_.create(), error::success);
    BOOST_REQUIRE_EQUAL(store_.open(), error::success);

    const system::chain::transaction transaction{};

    // transactor scope.
    {
        const auto transactor = store_.get_transactor();
        BOOST_REQUIRE(!instance.set_tx(transaction));
    }

    BOOST_REQUIRE_EQUAL(store_.close(), error::success);
}

BOOST_AUTO_TEST_SUITE_END()
