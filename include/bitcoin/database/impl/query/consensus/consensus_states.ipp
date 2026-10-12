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
inline bool CLASS::is_confirmable(const header_link& link) const NOEXCEPT
{
    return get_header_state(link) == error::block_confirmable;
}

TEMPLATE
bool CLASS::is_validateable(size_t height) const NOEXCEPT
{
    const auto ec = get_block_state(to_candidate(height));

    // First block state should be unvalidated, valid, or confirmable.
    return
        (ec == database::error::unvalidated) ||
        (ec == database::error::block_valid) ||
        (ec == database::error::unknown_state) ||
        (ec == database::error::block_confirmable);
}

// protected
TEMPLATE
inline code CLASS::to_block_code(schema::state::value flags) const NOEXCEPT
{
    using namespace system;
    using value = schema::state::value;
    if (is_nonzero(bit_and<value>(flags, block_state::unconfirmable)))
        return error::block_unconfirmable;

    if (is_nonzero(bit_and<value>(flags, block_state::confirmable)))
        return error::block_confirmable;

    if (is_nonzero(bit_and<value>(flags, block_state::valid)))
        return error::block_valid;

    return error::unvalidated;
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

TEMPLATE
bool CLASS::is_unconfirmable(const header_link& link) const NOEXCEPT
{
    return get_header_state(link) == error::block_unconfirmable;
}

// unvalidated
// block_valid
// block_confirmable
// block_unconfirmable
TEMPLATE
code CLASS::get_header_state(const header_link& link) const NOEXCEPT
{
    return to_block_code(store_.state.at(to_state(link)));
}

// unassociated
// unvalidated
// block_valid
// block_confirmable
// block_unconfirmable
TEMPLATE
code CLASS::get_block_state(const header_link& link) const NOEXCEPT
{
    const auto flags = store_.state.at(to_state(link));
    if (flags == block_state::no_state)
        return is_associated(link) ? error::unvalidated : error::unassociated;

    return to_block_code(flags);
}

TEMPLATE
inline bool CLASS::is_validated(const header_link& link) const NOEXCEPT
{
    // Validated and not invalid (checkpoint/milestone shows false).
    const auto ec = get_header_state(link);
    return ec == error::block_valid || ec == error::block_confirmable;
}

TEMPLATE
bool CLASS::is_block_validated(code& ec, const header_link& link,
    size_t height, size_t checkpoint) const NOEXCEPT
{
    if (height <= checkpoint || is_milestone(link))
    {
        if (!is_associated(link))
        {
            ec = error::unassociated;
            return false;
        }

        ec = error::bypassed;
        return true;
    }
    else
    {
        ec = get_header_state(link);
        return ec == error::block_valid || ec == error::block_confirmable;
    }
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
        table::state::replace{ {}, block_state::no_state });
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
