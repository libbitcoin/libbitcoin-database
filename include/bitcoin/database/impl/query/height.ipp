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
#ifndef LIBBITCOIN_DATABASE_QUERY_HEIGHT_IPP
#define LIBBITCOIN_DATABASE_QUERY_HEIGHT_IPP

#include <algorithm>
#include <ranges>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

// writers
// ----------------------------------------------------------------------------

TEMPLATE
bool CLASS::push_candidate(const header_link& link) NOEXCEPT
{
    if (link.is_terminal())
        return false;

    // Reserve-commit commit for deferred access.
    if (!store_.candidate.reserve(one))
        return false;

    // ========================================================================
    const auto scope = store_.get_transactor();

    // Clean single allocation failure (e.g. disk full).
    const table::height::record candidate{ {}, link };
    return store_.candidate.put(candidate);
    // ========================================================================
}

TEMPLATE
bool CLASS::pop_candidate() NOEXCEPT
{
    using ix = table::transaction::ix::integer;
    const auto top = system::possible_narrow_cast<ix>(get_top_candidate());
    if (is_zero(top))
        return false;

    // Clean single allocation failure (e.g. disk full).
    // ========================================================================
    const auto scope = store_.get_transactor();

    // Candidate pop implies reorg or disorg, which implies future duplicates.
    store_.set_dirty();

    ///////////////////////////////////////////////////////////////////////////
    std::unique_lock interlock{ candidate_reorganization_mutex_ };
    return store_.candidate.truncate(top);
    ///////////////////////////////////////////////////////////////////////////
    // ========================================================================
}

TEMPLATE
bool CLASS::push_confirmed(const header_link& link, bool strong) NOEXCEPT
{
    table::txs::get_coinbase_and_count txs{};
    if (strong && !store_.txs.at(to_txs(link), txs))
        return false;

    // Reserve-commit to ensure disk full safety and deferred access.
    if (!store_.confirmed.reserve(one))
        return false;

    // ========================================================================
    const auto scope = store_.get_transactor();

    // This reservation guard assumes no concurrent writes to the table.
    if (strong && !set_strong(link, txs.number, txs.coinbase_fk, true))
        return false;

    const table::height::record confirmed{ {}, link };
    return store_.confirmed.commit(confirmed);
    // ========================================================================
}

TEMPLATE
bool CLASS::pop_confirmed() NOEXCEPT
{
    using ix = table::transaction::ix::integer;
    const auto top = system::possible_narrow_cast<ix>(get_top_confirmed());
    if (is_zero(top))
        return false;

    const auto link = to_confirmed(top);
    table::txs::get_coinbase_and_count txs{};
    if (!store_.txs.at(to_txs(link), txs))
        return {};

    // ========================================================================
    const auto scope = store_.get_transactor();

    // Clean single allocation failure.
    if (!set_strong(link, txs.number, txs.coinbase_fk, false))
        return false;

    ///////////////////////////////////////////////////////////////////////////
    std::unique_lock interlock{ confirmed_reorganization_mutex_ };
    return store_.confirmed.truncate(top);
    ///////////////////////////////////////////////////////////////////////////
    // ========================================================================
}

} // namespace database
} // namespace libbitcoin

#endif
