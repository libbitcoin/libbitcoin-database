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
#include "mocks/dfile.hpp"

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
  : public store<test::dfile>
{
public:
    using path = std::filesystem::path;
    using store<test::dfile>::store;

    system::data_chunk& header_head() NOEXCEPT
    {
        return header_head_.buffer();
    }

    system::data_chunk& header_body() NOEXCEPT
    {
        return header_body_.buffer();
    }

    system::data_chunk& point_head() NOEXCEPT
    {
        return point_head_.buffer();
    }

    system::data_chunk& point_body() NOEXCEPT
    {
        return point_body_.buffer();
    }

    system::data_chunk& input_head() NOEXCEPT
    {
        return input_head_.buffer();
    }

    system::data_chunk& input_body() NOEXCEPT
    {
        return input_body_.buffer();
    }

    system::data_chunk& output_head() NOEXCEPT
    {
        return output_head_.buffer();
    }

    system::data_chunk& output_body() NOEXCEPT
    {
        return output_body_.buffer();
    }

    system::data_chunk& puts_head() NOEXCEPT
    {
        return puts_head_.buffer();
    }

    system::data_chunk& puts_body() NOEXCEPT
    {
        return puts_body_.buffer();
    }

    system::data_chunk& tx_head() NOEXCEPT
    {
        return tx_head_.buffer();
    }

    system::data_chunk& tx_body() NOEXCEPT
    {
        return tx_body_.buffer();
    }

    system::data_chunk& txs_head() NOEXCEPT
    {
        return txs_head_.buffer();
    }

    system::data_chunk& txs_body() NOEXCEPT
    {
        return txs_body_.buffer();
    }
};

constexpr auto parent = system::null_hash;
constexpr auto root = system::base16_array("119192939495969798999a9b9c9d9e9f229192939495969798999a9b9c9d9e9f");
constexpr auto block_hash = system::base16_array("85d0b02a16f6d645aa865fad4a8666f5e7bb2b0c4392a5d675496d6c3defa1f2");
constexpr database::context context
{
    0x01020304, // height
    0x11121314, // flags
    0x21222324  // mtp
};
const system::chain::header header
{
    0x31323334, // version
    parent,     // previous_block_hash
    root,       // merkle_root
    0x41424344, // timestamp
    0x51525354, // bits
    0x61626364  // nonce
};
const auto expected_header_head = system::base16_chunk(
    "010000" // record count
    "ffffff" // bucket[0]...
    "000000" // pk->
    "ffffff"
    "ffffff"
    "ffffff"
    "ffffff"
    "ffffff"
    "ffffff"
    "ffffff"
    "ffffff");
const auto expected_header_body = system::base16_chunk(
    "ffffff"   // next->
    "85d0b02a16f6d645aa865fad4a8666f5e7bb2b0c4392a5d675496d6c3defa1f2" // sk (block.hash)
    "040302"   // height
    "14131211" // flags
    "24232221" // mtp
    "ffffff"   // previous_block_hash (header_fk - not found)
    "34333231" // version
    "44434241" // timestamp
    "54535251" // bits
    "64636261" // nonce
    "119192939495969798999a9b9c9d9e9f229192939495969798999a9b9c9d9e9f"); // merkle_root

BOOST_AUTO_TEST_CASE(query__set_header__mock_default_header__expected)
{
    settings settings1{};
    settings1.header_buckets = 10;
    settings1.dir = TEST_DIRECTORY;
    store_accessor store1{ settings1 };
    query<store<test::dfile>> query1{ store1 };
    BOOST_REQUIRE_EQUAL(store1.create(), error::success);
    BOOST_REQUIRE_EQUAL(store1.open(), error::success);
    {
        const auto transactor = store1.get_transactor();
        BOOST_REQUIRE(query1.set_header(header, context));
    }
    table::header::record element1{};
    BOOST_REQUIRE(store1.header.get(block_hash, element1));
    BOOST_REQUIRE_EQUAL(store1.close(), error::success);
    BOOST_REQUIRE_EQUAL(store1.header_head(), expected_header_head);
    BOOST_REQUIRE_EQUAL(store1.header_body(), expected_header_body);

    BOOST_REQUIRE_EQUAL(element1.height, system::mask_left(context.height, byte_bits));
    BOOST_REQUIRE_EQUAL(element1.flags, context.flags);
    BOOST_REQUIRE_EQUAL(element1.mtp, context.mtp);
    BOOST_REQUIRE_EQUAL(element1.version, header.version());
    BOOST_REQUIRE_EQUAL(element1.parent_fk, linkage<schema::header::pk>::terminal);
    BOOST_REQUIRE_EQUAL(element1.root, header.merkle_root());
    BOOST_REQUIRE_EQUAL(element1.timestamp, header.timestamp());
    BOOST_REQUIRE_EQUAL(element1.bits, header.bits());
    BOOST_REQUIRE_EQUAL(element1.nonce, header.nonce());
}

BOOST_AUTO_TEST_CASE(query__get_header__mock_default_header__expected)
{
    settings settings1{};
    settings1.header_buckets = 10;
    settings1.dir = TEST_DIRECTORY;
    store_accessor store1{ settings1 };
    query<store<test::dfile>> query1{ store1 };
    BOOST_REQUIRE_EQUAL(store1.create(), error::success);
    BOOST_REQUIRE_EQUAL(store1.open(), error::success);

    store1.header_head() = expected_header_head;
    store1.header_body() = expected_header_body;
    const auto pointer = query1.get_header(block_hash);
    BOOST_REQUIRE(pointer);

    BOOST_REQUIRE_EQUAL(store1.close(), error::success);
    BOOST_REQUIRE_EQUAL(pointer->hash(), block_hash);
    BOOST_REQUIRE_EQUAL(pointer->version(), header.version());
    BOOST_REQUIRE_EQUAL(pointer->previous_block_hash(), header.previous_block_hash());
    BOOST_REQUIRE_EQUAL(pointer->merkle_root(), header.merkle_root());
    BOOST_REQUIRE_EQUAL(pointer->timestamp(), header.timestamp());
    BOOST_REQUIRE_EQUAL(pointer->bits(), header.bits());
    BOOST_REQUIRE_EQUAL(pointer->nonce(), header.nonce());
}

BOOST_AUTO_TEST_CASE(query__set_get_header__mmap_default_header__expected)
{
    settings settings1{};
    settings1.header_buckets = 10;
    settings1.dir = TEST_DIRECTORY;
    store<map> store1{ settings1 };
    query<store<map>> query1{ store1 };
    BOOST_REQUIRE_EQUAL(store1.create(), error::success);
    BOOST_REQUIRE_EQUAL(store1.open(), error::success);
    {
        const auto transactor = store1.get_transactor();
        BOOST_REQUIRE(query1.set_header(header, context));
    }
    table::header::record element1{};
    BOOST_REQUIRE(store1.header.get(block_hash, element1));

    const auto pointer = query1.get_header(block_hash);
    BOOST_REQUIRE(pointer);

    BOOST_REQUIRE_EQUAL(store1.close(), error::success);

    BOOST_REQUIRE_EQUAL(element1.height, system::mask_left(context.height, byte_bits));
    BOOST_REQUIRE_EQUAL(element1.flags, context.flags);
    BOOST_REQUIRE_EQUAL(element1.mtp, context.mtp);
    BOOST_REQUIRE_EQUAL(element1.version, header.version());
    BOOST_REQUIRE_EQUAL(element1.parent_fk, linkage<schema::header::pk>::terminal);
    BOOST_REQUIRE_EQUAL(element1.root, header.merkle_root());
    BOOST_REQUIRE_EQUAL(element1.timestamp, header.timestamp());
    BOOST_REQUIRE_EQUAL(element1.bits, header.bits());
    BOOST_REQUIRE_EQUAL(element1.nonce, header.nonce());

    BOOST_REQUIRE_EQUAL(pointer->hash(), block_hash);
    BOOST_REQUIRE_EQUAL(pointer->version(), header.version());
    BOOST_REQUIRE_EQUAL(pointer->previous_block_hash(), header.previous_block_hash());
    BOOST_REQUIRE_EQUAL(pointer->merkle_root(), header.merkle_root());
    BOOST_REQUIRE_EQUAL(pointer->timestamp(), header.timestamp());
    BOOST_REQUIRE_EQUAL(pointer->bits(), header.bits());
    BOOST_REQUIRE_EQUAL(pointer->nonce(), header.nonce());
}

BOOST_AUTO_TEST_CASE(query__set_tx__mock_default_tx__expected)
{
    const auto expected_head4_array = system::base16_chunk("00000000");
    const auto expected_head5_array = system::base16_chunk("0000000000");
    const auto expected_head4_hash = system::base16_chunk(
        "00000000" // record count
        "ffffffff" // bucket[0]...
        "ffffffff"
        "ffffffff"
        "ffffffff"
        "ffffffff");
    const auto expected_head5_hash = system::base16_chunk(
        "0000000000" // record count
        "ffffffffff" // bucket[0]...
        "ffffffffff"
        "ffffffffff"
        "ffffffffff"
        "ffffffffff");
    const system::chain::transaction transaction{};

    // data_chunk store.
    settings settings1{};
    settings1.tx_buckets = 5;
    settings1.point_buckets = 5;
    settings1.input_buckets = 5;
    settings1.txs_buckets = 5;
    settings1.dir = TEST_DIRECTORY;
    store_accessor store1{ settings1 };
    query<store<test::dfile>> query1{ store1 };
    BOOST_REQUIRE_EQUAL(store1.create(), error::success);
    BOOST_REQUIRE_EQUAL(store1.open(), error::success);
    {
        const auto transactor = store1.get_transactor();
        BOOST_REQUIRE(!query1.set_tx(transaction));
    }
    BOOST_REQUIRE_EQUAL(store1.close(), error::success);
    BOOST_REQUIRE_EQUAL(store1.tx_head(), expected_head4_hash);
    BOOST_REQUIRE_EQUAL(store1.point_head(), expected_head4_hash);
    BOOST_REQUIRE_EQUAL(store1.input_head(), expected_head5_hash);
    BOOST_REQUIRE_EQUAL(store1.output_head(), expected_head5_array);
    BOOST_REQUIRE_EQUAL(store1.puts_head(), expected_head4_array);
    BOOST_REQUIRE_EQUAL(store1.txs_head(), expected_head4_hash);
    BOOST_REQUIRE(store1.tx_body().empty());
    BOOST_REQUIRE(store1.point_body().empty());
    BOOST_REQUIRE(store1.input_body().empty());
    BOOST_REQUIRE(store1.output_body().empty());
    BOOST_REQUIRE(store1.puts_body().empty());
    BOOST_REQUIRE(store1.txs_body().empty());
}

BOOST_AUTO_TEST_CASE(query__set_tx__mock_tx__expected)
{
    const system::chain::transaction transaction
    {
        // TODO: non-default.
    };

    // TODO: populate, add bodies.
    const auto expected_head4_array = system::base16_chunk("00000000");
    const auto expected_head5_array = system::base16_chunk("0000000000");
    const auto expected_head4_hash = system::base16_chunk(
        "00000000" // record count
        "ffffffff" // bucket[0]...
        "ffffffff"
        "ffffffff"
        "ffffffff"
        "ffffffff");
    const auto expected_head5_hash = system::base16_chunk(
        "0000000000" // record count
        "ffffffffff" // bucket[0]...
        "ffffffffff"
        "ffffffffff"
        "ffffffffff"
        "ffffffffff");

    // data_chunk store.
    settings settings1{};
    settings1.tx_buckets = 5;
    settings1.point_buckets = 5;
    settings1.input_buckets = 5;
    settings1.txs_buckets = 5;
    settings1.dir = TEST_DIRECTORY;
    store_accessor store1{ settings1 };
    query<store<test::dfile>> query1{ store1 };
    BOOST_REQUIRE_EQUAL(store1.create(), error::success);
    BOOST_REQUIRE_EQUAL(store1.open(), error::success);
    {
        const auto transactor = store1.get_transactor();
        BOOST_REQUIRE(!query1.set_tx(transaction));
    }
    BOOST_REQUIRE_EQUAL(store1.close(), error::success);
    BOOST_REQUIRE_EQUAL(store1.tx_head(), expected_head4_hash);
    BOOST_REQUIRE_EQUAL(store1.point_head(), expected_head4_hash);
    BOOST_REQUIRE_EQUAL(store1.input_head(), expected_head5_hash);
    BOOST_REQUIRE_EQUAL(store1.output_head(), expected_head5_array);
    BOOST_REQUIRE_EQUAL(store1.puts_head(), expected_head4_array);
    BOOST_REQUIRE_EQUAL(store1.txs_head(), expected_head4_hash);
    BOOST_REQUIRE(store1.tx_body().empty());
    BOOST_REQUIRE(store1.point_body().empty());
    BOOST_REQUIRE(store1.input_body().empty());
    BOOST_REQUIRE(store1.output_body().empty());
    BOOST_REQUIRE(store1.puts_body().empty());
    BOOST_REQUIRE(store1.txs_body().empty());
}

BOOST_AUTO_TEST_SUITE_END()
