/**
 * Copyright (c) 2011-2026 libbitcoin developers (see AUTHORS)
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
#ifndef LIBBITCOIN_DATABASE_QUERY_CONSENSUS_STRONG_IPP
#define LIBBITCOIN_DATABASE_QUERY_CONSENSUS_STRONG_IPP

#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {
    
// is_strong
// ----------------------------------------------------------------------------
// True if any tx.link for same tx.hash is associated to a strong block.
    
TEMPLATE
bool CLASS::is_strong_tx(const tx_link& link) const NOEXCEPT
{
    // Try all txs with same hash as self (any instance will suffice).
    return !find_strong_tx(link).is_terminal();
}

TEMPLATE
bool CLASS::is_strong_block(const header_link& link) const NOEXCEPT
{
    return is_strong_tx(to_coinbase(link));
}

// find_strong
// ----------------------------------------------------------------------------
// Return the tx.link for same tx.hash with strong association to block.

TEMPLATE
tx_link CLASS::find_strong_tx(const tx_link& link) const NOEXCEPT
{
    // Shortcuircuit hash-based search by testing self.
    if (!to_block(link).is_terminal())
        return link;

    return find_strong_tx(get_tx_key(link));
}

TEMPLATE
tx_link CLASS::find_strong_tx(const hash_digest& tx_hash) const NOEXCEPT
{
    // Get all tx links for tx_hash.
    tx_links txs{};
    for (auto it = store_.tx.it(tx_hash); it; ++it)
        txs.push_back(*it);

    // Find the first strong tx of the set and return its link.
    for (const auto& tx: txs)
        if (!to_block(tx).is_terminal())
            return tx;

    return {};
}

// find_strong (block)
// ----------------------------------------------------------------------------

TEMPLATE
header_link CLASS::find_strong(const tx_link& link) const NOEXCEPT
{
    // Shortcuircuit hash-based search by testing self.
    if (const auto fk = to_block(link); !link.is_terminal())
        return fk;

    return find_strong(get_tx_key(link));
}

TEMPLATE
header_link CLASS::find_strong(const hash_digest& tx_hash) const NOEXCEPT
{
    // Get all tx links for tx_hash.
    tx_links txs{};
    for (auto it = store_.tx.it(tx_hash); it; ++it)
        txs.push_back(*it);

    // Find the first strong tx of the set and return its block.
    for (const auto& tx: txs)
        if (const auto block = to_block(tx); !block.is_terminal())
            return block;

    return {};
}

// set_strong (all tx_links associated to block)
// ----------------------------------------------------------------------------

// protected
TEMPLATE
bool CLASS::set_strong(const header_link& link, size_t count,
    const tx_link& first_fk, bool positive) NOEXCEPT
{
    using namespace system;
    using link_t = table::strong_tx::link;
    using element_t = table::strong_tx::record;

    // Preallocate all strong_tx records for the block and reuse memory ptr.
    const auto records = possible_narrow_cast<link_t::integer>(count);
    auto record = store_.strong_tx.allocate(records);
    const auto ptr = store_.strong_tx.get_memory();
    const auto end = first_fk + count;

    // Contiguous tx links.
    for (auto fk = first_fk; fk < end; ++fk)
        if (!store_.strong_tx.put(ptr, record++, fk, element_t
            {
                {},
                table::strong_tx::merge(positive, link)
            })) return false;

    return true;
}

TEMPLATE
bool CLASS::set_strong(const header_link& link) NOEXCEPT
{
    table::txs::get_coinbase_and_count txs{};
    if (!store_.txs.at(to_txs(link), txs))
        return {};

    // This should be caught by get_coinbase_and_count return.
    BC_ASSERT(!is_zero(txs.number) && txs.coinbase_fk != tx_link::terminal);

    // ========================================================================
    const auto scope = store_.get_transactor();

    // Clean allocation failure (e.g. disk full).
    return set_strong(link, txs.number, txs.coinbase_fk , true);
    // ========================================================================
}

TEMPLATE
bool CLASS::set_unstrong(const header_link& link) NOEXCEPT
{
    table::txs::get_coinbase_and_count txs{};
    if (!store_.txs.at(to_txs(link), txs))
        return {};

    // This should be caught by get_coinbase_and_count return.
    BC_ASSERT(!is_zero(txs.number) && txs.coinbase_fk != tx_link::terminal);

    // ========================================================================
    const auto scope = store_.get_transactor();

    // Clean allocation failure (e.g. disk full).
    return set_strong(link, txs.number, txs.coinbase_fk, false);
    // ========================================================================
}

} // namespace database
} // namespace libbitcoin

#endif
