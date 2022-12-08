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
#ifndef LIBBITCOIN_DATABASE_QUERIES_ARCHIVE_IPP
#define LIBBITCOIN_DATABASE_QUERIES_ARCHIVE_IPP

#include <algorithm>
#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {
    
BC_PUSH_WARNING(NO_NEW_OR_DELETE)
BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

// setters
// ----------------------------------------------------------------------------

TEMPLATE
bool CLASS::set_header(const header& header, const context& ctx) NOEXCEPT
{
    return !set_header_link(header, ctx).is_terminal();
}

TEMPLATE
bool CLASS::set_block(const block& block, const context& ctx) NOEXCEPT
{
    return !set_block_link(block, ctx).is_terminal();
}

TEMPLATE
bool CLASS::set_tx(const transaction& tx) NOEXCEPT
{
    return !set_tx_link(tx).is_terminal();
}

TEMPLATE
bool CLASS::set_txs(const hash_digest& key, const hashes& hashes) NOEXCEPT
{
    // Require header.
    const auto header_fk = store_.header.it(key).self();
    if (header_fk.is_terminal())
        return false;

    // Shortcircuit (redundant with set_txs...put_if).
    if (store_.txs.exists(header_fk))
        return true;

    // Get foreign key for each tx.
    table::txs::slab keys{};
    keys.tx_fks.reserve(hashes.size());
    for (const auto& hash: hashes)
        keys.tx_fks.push_back(store_.tx.it(hash).self());

    // Set is idempotent, requires that none are terminal.
    return set_txs(header_fk, keys);
}

// getters
// ============================================================================

TEMPLATE
bool CLASS::header_exists(const hash_digest& key) NOEXCEPT
{
    return store_.header.exists(key);
}

TEMPLATE
bool CLASS::block_exists(const hash_digest& key) NOEXCEPT
{
    // Validate header_fk because it will be used as a search key.
    const auto header_fk = store_.header.it(key).self();

    // All transactions are populated if the hash table entry exists.
    return !header_fk.is_terminal() &&
        !store_.txs.it(header_fk).self().is_terminal();
}

TEMPLATE
bool CLASS::tx_exists(const hash_digest& key) NOEXCEPT
{
    return store_.tx.exists(key);
}

TEMPLATE
bool CLASS::populate(const block& block) NOEXCEPT
{
    // TODO: evaluate concurrency for larger tx counts.
    auto result = true;
    const auto& txs = *block.transactions_ptr();
    std::for_each(txs.begin(), txs.end(), [&](const auto& tx) NOEXCEPT
    {
        result &= populate(*tx);
    });

    return result;
}

TEMPLATE
bool CLASS::populate(const transaction& tx) NOEXCEPT
{
    // TODO: evaluate concurrency for larger input counts.
    auto result = true;
    const auto& ins = *tx.inputs_ptr();
    std::for_each(ins.begin(), ins.end(), [&](const auto& in) NOEXCEPT
    {
        result &= populate(*in);
    });

    return result;
}

TEMPLATE
bool CLASS::populate(const input& input) NOEXCEPT
{
    return ((input.prevout = get_prevout(input)));
}

TEMPLATE
CLASS::header::cptr CLASS::get_header(const hash_digest& key) NOEXCEPT
{
    return get_header(store_.header.it(key).self());
}

TEMPLATE
CLASS::block::cptr CLASS::get_block(const hash_digest& key) NOEXCEPT
{
    return get_block(store_.header.it(key).self());
}

TEMPLATE
hashes CLASS::get_txs(const hash_digest& key) NOEXCEPT
{
    return get_txs(store_.header.it(key).self());
}

TEMPLATE
CLASS::transaction::cptr CLASS::get_tx(const hash_digest& key) NOEXCEPT
{
    return get_tx(store_.tx.it(key).self());
}

TEMPLATE
CLASS::input::cptr CLASS::get_spender(const hash_digest& tx_hash,
    uint32_t index) NOEXCEPT
{
    // Validate hash_fk because it will be used as a search key.
    const auto hash_fk = store_.point.it(tx_hash).self();
    if (hash_fk.is_terminal())
        return {};

    // Pass spent point for attachment to input (no need to read).
    table::input::only_from_prevout in
    {
        {},
        system::to_shared(point{ tx_hash, index })
    };
    const auto fp = table::input::compose(hash_fk, index);
    const auto input_fk = store_.input.it(fp).self();
    if (!store_.input.get(input_fk, in))
        return {};

    return in.input;
}

TEMPLATE
CLASS::input::cptr CLASS::get_input(const hash_digest& tx_hash,
    uint32_t index) NOEXCEPT
{
    table::transaction::record_input tx{ {}, index };
    if (!store_.tx.get(tx_hash, tx))
        return {};

    table::puts::record_get_one input{};
    if (!store_.puts.get(tx.input_fk, input))
        return {};

    return get_input(input.put_fk);
}

TEMPLATE
CLASS::output::cptr CLASS::get_output(const hash_digest& tx_hash,
    uint32_t index) NOEXCEPT
{
    table::transaction::record_output tx{ {}, index };
    if (!store_.tx.get(tx_hash, tx))
        return {};

    table::puts::record_get_one output{};
    if (!store_.puts.get(tx.output_fk, output))
        return {};

    return get_output(output.put_fk);
}

TEMPLATE
CLASS::output::cptr CLASS::get_prevout(const input& input) NOEXCEPT
{
    const auto& point = input.point();
    return get_output(point.hash(), point.index());
}

BC_POP_WARNING()
BC_POP_WARNING()

} // namespace database
} // namespace libbitcoin

#endif
