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
#ifndef LIBBITCOIN_DATABASE_QUERIES_CACHE_IPP
#define LIBBITCOIN_DATABASE_QUERIES_CACHE_IPP

#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {
    
// state
// ----------------------------------------------------------------------------

TEMPLATE
code CLASS::header_state(const hash_digest& key) NOEXCEPT
{
    const auto header_fk = store_.header.it(key).self();
    return header_fk.is_terminal() ? error::not_found : header_state(header_fk);
}

TEMPLATE
code CLASS::block_state(const hash_digest& key) NOEXCEPT
{
    const auto header_fk = store_.header.it(key).self();
    if (header_fk.is_terminal())
        return error::not_found;

    // If there is block state then txs are populated, otherwise check.
    // A block must always be fully populated before setting valid/invalid.
    const auto state = block_state(header_fk);
    return (state == error::no_entry) &&
        store_.txs.it(key).self().is_terminal() ? error::unpopulated : state;
}

TEMPLATE
code CLASS::tx_state(const hash_digest& key, const context& context) NOEXCEPT
{
    const auto tx_fk = store_.tx.it(key).self();
    if (tx_fk.is_terminal())
        return error::not_found;

    // tx validation state is contextual, while blocks carry their own context.
    return tx_state(tx_fk, context);
}

// confiration
// ----------------------------------------------------------------------------

TEMPLATE
bool CLASS::block_confirmed(const hash_digest& key) NOEXCEPT
{
    return store_.strong_bk.exists(key);
}

TEMPLATE
bool CLASS::tx_confirmed(const hash_digest& key) NOEXCEPT
{
    return store_.strong_tx.exists(key);
}

TEMPLATE
bool CLASS::output_confirmed(const hash_digest&, uint32_t) NOEXCEPT
{
    // TODO: relate output to a strong_tx.
    return {};
}

TEMPLATE
bool CLASS::spent_confirmed(const hash_digest&, uint32_t) NOEXCEPT
{
    // TODO: relate spenders of output to a strong_tx (bool).
    return {};
}

// bootstrap
// ----------------------------------------------------------------------------

TEMPLATE
bool CLASS::set_bootstrap(size_t) NOEXCEPT
{
    // TODO: copy confirmed block hashes up to height to bootstrap, or false.
    return {};
}

TEMPLATE
hashes CLASS::get_bootstrap(size_t, size_t) NOEXCEPT
{
    // TODO: get contiguous set of hashes for network get_headers.
    return {};
}

} // namespace database
} // namespace libbitcoin

#endif
