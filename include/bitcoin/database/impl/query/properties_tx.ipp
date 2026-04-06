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
#ifndef LIBBITCOIN_DATABASE_QUERY_PROPERTIES_TX_IPP
#define LIBBITCOIN_DATABASE_QUERY_PROPERTIES_TX_IPP

#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {
    
// boolean
// ----------------------------------------------------------------------------

TEMPLATE
inline bool CLASS::is_tx(const hash_digest& key) const NOEXCEPT
{
    return store_.tx.exists(key);
}

TEMPLATE
inline bool CLASS::is_coinbase(const tx_link& link) const NOEXCEPT
{
    table::transaction::get_coinbase tx{};
    return store_.tx.get(link, tx) && tx.coinbase;
}

TEMPLATE
inline bool CLASS::is_tx_segregated(const tx_link& link) const NOEXCEPT
{
    size_t light{}, heavy{};
    return get_tx_sizes(light, heavy, link) && heavy != light;
}

// association
// ----------------------------------------------------------------------------

TEMPLATE
bool CLASS::get_tx_height(size_t& out, const tx_link& link) const NOEXCEPT
{
    return get_height(out, find_strong(link));
}

TEMPLATE
bool CLASS::get_tx_position(size_t& out, const tx_link& link) const NOEXCEPT
{
    return get_tx_position(out, link, find_strong(link));
}

TEMPLATE
bool CLASS::get_tx_position(size_t& out, const tx_link& link,
    const header_link& block) const NOEXCEPT
{
    table::txs::get_position txs{ {}, link };
    if (!store_.txs.at(to_txs(block), txs))
        return false;

    out = txs.position;
    return true;
}

} // namespace database
} // namespace libbitcoin

#endif
