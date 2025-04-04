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
#ifndef LIBBITCOIN_DATABASE_TEST_MOCKS_CHUNK_STORE_HPP
#define LIBBITCOIN_DATABASE_TEST_MOCKS_CHUNK_STORE_HPP

#include "../test.hpp"
#include "chunk_storage.hpp"

namespace test {

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

    system::data_chunk& ins_head() NOEXCEPT
    {
        return ins_head_.buffer();
    }

    system::data_chunk& ins_body() NOEXCEPT
    {
        return ins_body_.buffer();
    }

    system::data_chunk& outs_head() NOEXCEPT
    {
        return outs_head_.buffer();
    }

    system::data_chunk& outs_body() NOEXCEPT
    {
        return outs_body_.buffer();
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

    system::data_chunk& duplicate_head() NOEXCEPT
    {
        return duplicate_head_.buffer();
    }

    system::data_chunk& duplicate_body() NOEXCEPT
    {
        return duplicate_body_.buffer();
    }

    system::data_chunk& prevout_head() NOEXCEPT
    {
        return prevout_head_.buffer();
    }

    system::data_chunk& prevout_body() NOEXCEPT
    {
        return prevout_body_.buffer();
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

    // Optionals.

    system::data_chunk& address_head() NOEXCEPT
    {
        return address_head_.buffer();
    }

    system::data_chunk& address_body() NOEXCEPT
    {
        return address_body_.buffer();
    }

    system::data_chunk& filter_bk_head() NOEXCEPT
    {
        return filter_bk_head_.buffer();
    }

    system::data_chunk& filter_bk_body() NOEXCEPT
    {
        return filter_bk_body_.buffer();
    }

    system::data_chunk& filter_tx_head() NOEXCEPT
    {
        return filter_tx_head_.buffer();
    }

    system::data_chunk& filter_tx_body() NOEXCEPT
    {
        return filter_tx_body_.buffer();
    }
};

using query_accessor = query<store<chunk_storage>>;

} // namespace test

#endif
