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
#ifndef LIBBITCOIN_DATABASE_TEST_MOCKS_CHUNK_STORE_HPP
#define LIBBITCOIN_DATABASE_TEST_MOCKS_CHUNK_STORE_HPP

#include "../test.hpp"
#include "chunk_storage.hpp"

namespace test {

// blockchain.info/rawblock/[block-hash]?format=hex
constexpr auto block1_hash = system::base16_hash(
    "00000000839a8e6886ab5951d76f411475428afc90947ee320161bbf18eb6048");
constexpr auto block1_data = system::base16_array(
    "010000006fe28c0ab6f1b372c1a6a246ae63f74f931e8365e15a089c68d61900"
    "00000000982051fd1e4ba744bbbe680e1fee14677ba1a3c3540bf7b1cdb606e8"
    "57233e0e61bc6649ffff001d01e3629901010000000100000000000000000000"
    "00000000000000000000000000000000000000000000ffffffff0704ffff001d"
    "0104ffffffff0100f2052a0100000043410496b538e853519c726a2c91e61ec1"
    "1600ae1390813a627c66fb8be7947be63c52da7589379515d4e0a604f8141781"
    "e62294721166bf621e73a82cbf2342c858eeac00000000");
constexpr auto block2_hash = system::base16_hash(
    "000000006a625f06636b8bb6ac7b960a8d03705d1ace08b1a19da3fdcc99ddbd");
constexpr auto block2_data = system::base16_array(
    "010000004860eb18bf1b1620e37e9490fc8a427514416fd75159ab86688e9a83"
    "00000000d5fdcc541e25de1c7a5addedf24858b8bb665c9f36ef744ee42c3160"
    "22c90f9bb0bc6649ffff001d08d2bd6101010000000100000000000000000000"
    "00000000000000000000000000000000000000000000ffffffff0704ffff001d"
    "010bffffffff0100f2052a010000004341047211a824f55b505228e4c3d5194c"
    "1fcfaa15a456abdf37f9b9d97a4040afc073dee6c89064984f03385237d92167"
    "c13e236446b417ab79a0fcae412ae3316b77ac00000000");
constexpr auto block3_hash = system::base16_hash(
    "0000000082b5015589a3fdf2d4baff403e6f0be035a5d9742c1cae6295464449");
constexpr auto block3_data = system::base16_array(
    "01000000bddd99ccfda39da1b108ce1a5d70038d0a967bacb68b6b63065f626a"
    "0000000044f672226090d85db9a9f2fbfe5f0f9609b387af7be5b7fbb7a1767c"
    "831c9e995dbe6649ffff001d05e0ed6d01010000000100000000000000000000"
    "00000000000000000000000000000000000000000000ffffffff0704ffff001d"
    "010effffffff0100f2052a0100000043410494b9d3e76c5b1629ecf97fff95d7"
    "a4bbdac87cc26099ada28066c6ff1eb9191223cd897194a08d0c2726c5747f1d"
    "b49e8cf90e75dc3e3550ae9b30086f3cd5aaac00000000");

// Store accessor built on chunk_storage.
class chunk_store
  : public store<test::chunk_storage>
{
public:
    using path = std::filesystem::path;
    using store<test::chunk_storage>::store;

    // Archives.

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

    // Indexes.

    system::data_chunk& address_head() NOEXCEPT
    {
        return address_head_.buffer();
    }

    system::data_chunk& address_body() NOEXCEPT
    {
        return address_body_.buffer();
    }

    system::data_chunk& candidate_head() NOEXCEPT
    {
        return candidate_head_.buffer();
    }

    system::data_chunk& candidate_body() NOEXCEPT
    {
        return candidate_body_.buffer();
    }

    system::data_chunk& confirmed_head() NOEXCEPT
    {
        return confirmed_head_.buffer();
    }

    system::data_chunk& confirmed_body() NOEXCEPT
    {
        return confirmed_body_.buffer();
    }

    system::data_chunk& strong_tx_head() NOEXCEPT
    {
        return strong_tx_head_.buffer();
    }

    system::data_chunk& strong_tx_body() NOEXCEPT
    {
        return strong_tx_body_.buffer();
    }

    // Caches.

    system::data_chunk& bootstrap_head() NOEXCEPT
    {
        return bootstrap_head_.buffer();
    }

    system::data_chunk& bootstrap_body() NOEXCEPT
    {
        return bootstrap_body_.buffer();
    }

    system::data_chunk& buffer_head() NOEXCEPT
    {
        return buffer_head_.buffer();
    }

    system::data_chunk& buffer_body() NOEXCEPT
    {
        return buffer_body_.buffer();
    }

    system::data_chunk& neutrino_head() NOEXCEPT
    {
        return neutrino_head_.buffer();
    }

    system::data_chunk& neutrino_body() NOEXCEPT
    {
        return neutrino_body_.buffer();
    }

    system::data_chunk& validated_bk_head() NOEXCEPT
    {
        return validated_bk_head_.buffer();
    }

    system::data_chunk& validated_bk_body() NOEXCEPT
    {
        return validated_bk_body_.buffer();
    }

    system::data_chunk& validated_tx_head() NOEXCEPT
    {
        return validated_tx_head_.buffer();
    }

    system::data_chunk& validated_tx_body() NOEXCEPT
    {
        return validated_tx_body_.buffer();
    }
};

} // namespace test

#endif
