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
#ifndef LIBBITCOIN_DATABASE_QUERY_CONSENSUS_STATES_IPP
#define LIBBITCOIN_DATABASE_QUERY_CONSENSUS_STATES_IPP

#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

// State machine for block.
// ----------------------------------------------------------------------------

TEMPLATE
inline block_state CLASS::get_block_state(
    const header_link& link) const NOEXCEPT
{
    return static_cast<block_state>(store_.state.at(to_state(link)));
}

TEMPLATE
inline bool CLASS::is_confirmable(const header_link& link) const NOEXCEPT
{
    return database::is_confirmable(get_block_state(link));
}

TEMPLATE
bool CLASS::is_unconfirmable(const header_link& link) const NOEXCEPT
{
    return database::is_unconfirmable(get_block_state(link));
}

TEMPLATE
inline bool CLASS::is_validated(const header_link& link) const NOEXCEPT
{
    return database::is_validated(get_block_state(link));
}

TEMPLATE
bool CLASS::is_validateable(size_t height) const NOEXCEPT
{
    const auto state = get_block_state(to_candidate(height));
    return database::is_associated(state) &&
        !database::is_unconfirmable(state);
}

// protected
TEMPLATE
inline bool CLASS::is_sufficient(const context& current,
    const context& evaluated) const NOEXCEPT
{
    // Past evaluation at a lesser height and mtp is sufficient, provided that
    // absolute locktime is evaluated against mtp (not block timestamp).
    return evaluated.flags == current.flags
        && evaluated.is_enabled(system::chain::flags::bip113_rule)
        && evaluated.height <= current.height
        && evaluated.mtp <= current.mtp;
}

// writers
// ----------------------------------------------------------------------------

TEMPLATE
bool CLASS::set_block_valid(const header_link& link) NOEXCEPT
{
    return set_block_state(link, block_state::valid);
}

TEMPLATE
bool CLASS::set_block_confirmable(const header_link& link) NOEXCEPT
{
    return set_block_state(link, block_state::confirmable);
}

TEMPLATE
bool CLASS::set_block_unconfirmable(const header_link& link) NOEXCEPT
{
    return !store_.mark_unconfirmable() ||
        set_block_state(link, block_state::unconfirmable);
}

TEMPLATE
bool CLASS::set_block_unknown(const header_link& link) NOEXCEPT
{
    // ========================================================================
    const auto scope = get_transactor();

    // Clean single allocation failure (e.g. disk full).
    return store_.state.set(to_state(link),
        table::state::clear{ {}, validation_states });
    // ========================================================================
}

// private
TEMPLATE
bool CLASS::set_block_state(const header_link& link,
    block_state flags) NOEXCEPT
{
    // ========================================================================
    const auto scope = get_transactor();

    // Clean single allocation failure (e.g. disk full).
    return store_.state.set(to_state(link),
        table::state::merge{ {}, flags });
    // ========================================================================
}

} // namespace database
} // namespace libbitcoin

#endif
