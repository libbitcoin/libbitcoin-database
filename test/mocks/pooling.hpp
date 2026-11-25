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
#ifndef LIBBITCOIN_DATABASE_TEST_MOCKS_POOLING_HPP
#define LIBBITCOIN_DATABASE_TEST_MOCKS_POOLING_HPP

#include "../test.hpp"
#include "../mocks/blocks.hpp"
#include "../mocks/chunk_store.hpp"

namespace test {

using pool_totals = system::pool::algorithm::totals_t;
using pool_projection = system::pool::algorithm::projection_t;
using pool_arrivals = std::vector<system::pool::algorithm::takes_t>;
constexpr auto bip113 = system::chain::flags::bip113_rule;
constexpr pool_totals block_limit{ 0, 4'000'000, 80'000 };
constexpr pool_totals tight_limit{ 0, 10, 80'000 };

/// A tx of one output spending the first output of the hash.
system::chain::transaction to_spend(const system::hash_digest& hash,
    uint64_t value) NOEXCEPT;

/// The hash of the genesis coinbase.
const system::hash_digest& genesis_coinbase() NOEXCEPT;

/// Archive the tx and pool it under a context of the height.
bool archive_pool(query_t& query, const system::chain::transaction& tx,
    uint32_t height=8) NOEXCEPT;

/// Confirm a block of a coinbase and the tx above genesis, returning its link.
database::header_link archive_confirm(query_t& query,
    const system::chain::transaction& tx) NOEXCEPT;

/// Block 1 of a coinbase and the tx.
system::chain::block to_block(const system::chain::transaction& tx) NOEXCEPT;

/// Populate the tx prevouts, then archive and pool it.
bool pool_populated(query_t& query, const system::chain::transaction& tx,
    uint32_t height=8) NOEXCEPT;

/// The pool index cell of the tx.
uint32_t pool_cell(query_t& query, store_t& store,
    const system::chain::transaction& tx) NOEXCEPT;

/// Append the tx to the package, populated with metadata.
bool pool_candidate(system::chain::transaction_cptrs& out, query_t& query,
    const system::chain::transaction& tx) NOEXCEPT;

} // namespace test

#endif
