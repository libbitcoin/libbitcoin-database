/**
 * Copyright (c) 2011-2018 libbitcoin developers (see AUTHORS)
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
#include <bitcoin/database/verify.hpp>

#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/databases/block_database.hpp>
#include <bitcoin/database/databases/transaction_database.hpp>

namespace libbitcoin {
namespace database {

using namespace bc::chain;

#ifndef NDEBUG
static hash_digest get_block(const block_database& blocks,
    size_t height, bool candidate)
{
    return blocks.get(height, candidate).hash();
}

static bool get_is_empty_block(const block_database& blocks,
    size_t height, bool candidate)
{
    return blocks.get(height, candidate).transaction_count() == 0;
}

static hash_digest get_previous_block(const block_database& blocks,
    size_t height, bool candidate)
{
    return height == 0 ? null_hash : get_block(blocks, height - 1,
        candidate);
}

static size_t get_next_block(const block_database& blocks, bool candidate)
{
    size_t current_height;
    const auto empty_chain = !blocks.top(current_height, candidate);
    return empty_chain ? 0 : current_height + 1;
}
#endif

code verify(const block_database& blocks, const config::checkpoint& fork_point,
    bool candidate)
{
#ifndef NDEBUG
    const auto result = blocks.get(fork_point.hash());

    if (!result)
        return error::not_found;

    if (fork_point.height() != result.height())
        return error::store_block_invalid_height;

    const auto state = result.state();

    if (!is_confirmed(state) && !(candidate && is_candidate(state)))
        return error::store_incorrect_state;
#endif

    return error::success;
}

code verify_top(const block_database& blocks, size_t height, bool candidate)
{
#ifndef NDEBUG
    size_t actual_height;
    if (!blocks.top(actual_height, candidate) || (actual_height != height))
        return error::operation_failed;
#endif

    return error::success;
}

code verify_exists(const block_database& blocks, const header& header)
{
#ifndef NDEBUG
    if (!blocks.get(header.hash()))
        return error::not_found;
#endif

    return error::success;
}

code verify_exists(const transaction_database& transactions,
    const transaction& tx)
{
#ifndef NDEBUG
    if (!transactions.get(tx.hash()))
        return error::not_found;
#endif

    return error::success;
}

code verify_missing(const transaction_database& transactions,
    const transaction& tx)
{
#ifdef NDEBUG
    if (transactions.get(tx.hash()))
        return error::duplicate_transaction;
#endif

    return error::success;
}

code verify_push(const block_database& blocks, const header& header,
    size_t height)
{
#ifndef NDEBUG
    if (get_next_block(blocks, false) != height)
        return error::store_block_invalid_height;

    if (get_previous_block(blocks, height, false) !=
        header.previous_block_hash())
        return error::store_block_missing_parent;
#endif

    return error::success;
}

code verify_push(const block_database& blocks, const block& block,
    size_t height)
{
#ifndef NDEBUG
    if (block.transactions().empty())
        return error::empty_block;

    if (get_next_block(blocks, true) != height)
        return error::store_block_invalid_height;

    if (get_previous_block(blocks, height, true) !=
        block.header().previous_block_hash())
        return error::store_block_missing_parent;
#endif

    return error::success;
}

code verify_update(const block_database& blocks, const block& block,
    size_t height)
{
#ifndef NDEBUG
    if (block.transactions().empty())
        return error::empty_block;

    if (!get_is_empty_block(blocks, height, false))
        return error::operation_failed;

    if (get_block(blocks, height, false) != block.hash())
        return error::not_found;
#endif

    return error::success;
}

code verify_not_failed(const block_database& blocks, const block& block)
{
#ifndef NDEBUG
    const auto result = blocks.get(block.hash());

    if (!result)
        return error::not_found;

    if (is_failed(result.state()))
        return error::operation_failed;
#endif

    return error::success;
}

} // namespace database
} // namespace libbitcoin
