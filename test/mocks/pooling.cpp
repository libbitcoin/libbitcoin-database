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
#include "../test.hpp"
#include "pooling.hpp"

namespace test {

using namespace system;
using namespace system::chain;

transaction to_spend(const hash_digest& hash, uint64_t value) NOEXCEPT
{
    const input input{ point{ hash, 0 }, script{}, max_uint32 };
    return { 1u, inputs{ input }, outputs{ output{ value, script{} } }, 0u };
}

const hash_digest& genesis_coinbase() NOEXCEPT
{
    static const auto hash = genesis.transactions_ptr()->front()->hash(false);
    return hash;
}

bool archive_pool(query_t& query, const transaction& tx,
    uint32_t height) NOEXCEPT
{
    const auto stored = query.set(tx);
    const auto link = query.to_tx(tx.hash(false));
    const database::context ctx{ bip113, height, 9 };
    return stored && query.set_pooled(link, tx, ctx, 0);
}

block to_block(const transaction& tx) NOEXCEPT
{
    const point null_point{ null_hash, point::null_index };
    const input null{ null_point, script{}, max_uint32 };
    const output reward{ 42u, script{} };
    const transaction coinbase{ 1u, inputs{ null }, outputs{ reward }, 0u };
    const header header{ 1u, genesis.hash(), null_hash, 1u, 2u, 3u };
    return { header, { coinbase, tx } };
}

database::header_link archive_confirm(query_t& query,
    const transaction& tx) NOEXCEPT
{
    const auto block = to_block(tx);
    const database::context ctx{ 0, 1, 0 };
    const auto stored = query.set(block, ctx, {}, false, true);
    const auto link = query.to_header(block.hash());
    const auto candidate = stored && query.push_candidate(link);
    const auto confirmed = candidate && query.push_confirmed(link, true);
    const auto applied = confirmed && query.set_pool_block(link, true);
    return applied ? link : database::header_link{};
}

bool pool_populated(query_t& query, const transaction& tx,
    uint32_t height) NOEXCEPT
{
    return query.populate_without_metadata(tx) &&
        archive_pool(query, tx, height);
}

uint32_t pool_cell(query_t& query, store_t& store,
    const transaction& tx) NOEXCEPT
{
    const auto row = store.pool.first(query.to_tx(tx.hash(false)));
    return store.pool_index.cells.at(row.value);
}

bool pool_candidate(transaction_cptrs& out, query_t& query,
    const transaction& tx) NOEXCEPT
{
    out.push_back(to_shared(tx));
    return query.populate_with_metadata(*out.back(), true, true);
}

} // namespace test
