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

#include <memory>
#include <utility>
#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {
    
BC_PUSH_WARNING(NO_NEW_OR_DELETE)
BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

// bool setters(chain_ref)
// ----------------------------------------------------------------------------

TEMPLATE
bool CLASS::set_header(const header& header, const context& context) NOEXCEPT
{
    return !set_header_link(header, context).is_terminal();
}

TEMPLATE
bool CLASS::set_tx(const transaction& tx) NOEXCEPT
{
    return !set_tx_link(tx).is_terminal();
}

TEMPLATE
bool CLASS::set_block(const block& block, const context& context) NOEXCEPT
{
    return !set_block_link(block, context).is_terminal();
}

TEMPLATE
bool CLASS::set_txs(const hash_digest& key,
    const system::hashes& hashes) NOEXCEPT
{
    // Require header.
    const auto header_fk = store_.header.it(key).self();
    if (header_fk.is_terminal())
        return false;

    // Shortcircuit (redundant with set_txs_ put_if).
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

// chain_ptr getters(key)
// ============================================================================

// TODO: test.
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

// TODO: test.
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

// TODO: test.
TEMPLATE
CLASS::input::cptr CLASS::get_spender(const point::cptr& prevout) NOEXCEPT
{
    using namespace system::chain;

    // Must validate hash_fk because it will be used in a search key.
    const auto hash_fk = store_.point.it(prevout->hash()).self();
    if (hash_fk.is_terminal())
        return {};

    table::input::only_from_prevout in{ {}, prevout };
    const auto fp = table::input::to_point(hash_fk, prevout->index());
    if (!store_.input.get(store_.input.it(fp).self(), in))
        return {};

    return in.input;
}

// TODO: test.
TEMPLATE
CLASS::input::cptr CLASS::get_spender(const point& prevout) NOEXCEPT
{
    return get_spender(system::to_shared(prevout));
}

TEMPLATE
CLASS::transaction::cptr CLASS::get_tx(const hash_digest& key) NOEXCEPT
{
    return get_tx(store_.tx.it(key).self());
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
system::hashes CLASS::get_txs(const hash_digest& key) NOEXCEPT
{
    return get_txs(store_.header.it(key).self());
}

BC_POP_WARNING()
BC_POP_WARNING()

} // namespace database
} // namespace libbitcoin

#endif
