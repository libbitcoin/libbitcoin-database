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
#ifndef LIBBITCOIN_DATABASE_QUERY_HEIGHT_IPP
#define LIBBITCOIN_DATABASE_QUERY_HEIGHT_IPP

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

    // Reserve-push to ensure disk full safety and deferred access.
    if (!store_.candidate.reserve(one))
        return false;

    // ========================================================================
    const auto scope = get_transactor();

    // Clean single allocation failure (e.g. disk full).
    const auto pushed = store_.candidate.push(link);
    if (pushed)
        store_.evaluate_currency();

    return pushed;
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
    const auto scope = get_transactor();

    // Candidate pop implies reorg or disorg, which implies future duplicates.
    store_.set_dirty();

    ///////////////////////////////////////////////////////////////////////////
    std::unique_lock interlock{ candidate_reorganization_mutex_ };
    const auto popped = store_.candidate.truncate(top);
    if (popped)
        store_.evaluate_currency();

    return popped;
    ///////////////////////////////////////////////////////////////////////////
    // ========================================================================
}

TEMPLATE
bool CLASS::push_confirmed(const header_link& link, bool strong) NOEXCEPT
{
    table::txs::get_txs txs{};
    if (strong && (!store_.txs.at(to_txs(link), txs) || txs.tx_fks.empty()))
        return false;

    // Reserve-push to ensure disk full safety and deferred access.
    if (!store_.confirmed.reserve(one))
        return false;

    // ========================================================================
    const auto scope = get_transactor();

    // This reservation guard assumes no concurrent writes to the table.
    if (strong && !set_strong(link, txs.tx_fks, true))
        return false;

    const auto pushed = store_.confirmed.push(link);
    if (pushed)
        store_.evaluate_currency();

    return pushed;
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
    table::txs::get_txs txs{};
    if (!store_.txs.at(to_txs(link), txs) || txs.tx_fks.empty())
        return false;

    // ========================================================================
    const auto scope = get_transactor();

    // Clean single allocation failure.
    if (!set_strong(link, txs.tx_fks, false))
        return false;

    ///////////////////////////////////////////////////////////////////////////
    std::unique_lock interlock{ confirmed_reorganization_mutex_ };
    const auto popped = store_.confirmed.truncate(top);
    if (popped)
        store_.evaluate_currency();

    return popped;
    ///////////////////////////////////////////////////////////////////////////
    // ========================================================================
}

} // namespace database
} // namespace libbitcoin

#endif
