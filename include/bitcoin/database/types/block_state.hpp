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
#ifndef LIBBITCOIN_DATABASE_TYPES_BLOCK_STATE_HPP
#define LIBBITCOIN_DATABASE_TYPES_BLOCK_STATE_HPP

#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

/// Block state flags.
enum block_state : uint8_t
{
    /// No state.
    no_state = 0,

    /// Transactions are archived.
    associated = system::bit_right<uint8_t>(0),

    /// Satisfies validation rules (prevouts unverified).
    valid = system::bit_right<uint8_t>(1),

    /// Satisfies confirmation rules (final).
    confirmable = system::bit_right<uint8_t>(2),

    /// Does not satisfy validation or confirmation rules (final).
    unconfirmable = system::bit_right<uint8_t>(3),

    /// At or below the milestone (bypassed).
    milestoned = system::bit_right<uint8_t>(4),

    /// At or below the top checkpoint (bypassed).
    checkpointed = system::bit_right<uint8_t>(5)
};

/// Validation flags, cleared by reset.
constexpr uint8_t validation_states = valid | confirmable | unconfirmable;

/// Bypass flags.
constexpr uint8_t bypass_states = milestoned | checkpointed;

/// True if any of the flags are set.
constexpr bool is_any(block_state state, uint8_t flags) NOEXCEPT
{
    return is_nonzero(system::bit_and<uint8_t>(state, flags));
}

/// Transactions are archived.
constexpr bool is_associated(block_state state) NOEXCEPT
{
    return is_any(state, associated);
}

/// Does not satisfy validation or confirmation rules.
constexpr bool is_unconfirmable(block_state state) NOEXCEPT
{
    return is_any(state, unconfirmable);
}

/// Satisfies confirmation rules.
constexpr bool is_confirmable(block_state state) NOEXCEPT
{
    return is_any(state, confirmable) && !is_unconfirmable(state);
}

/// Satisfies validation or confirmation rules.
constexpr bool is_validated(block_state state) NOEXCEPT
{
    return is_any(state, valid | confirmable) && !is_unconfirmable(state);
}

/// At or below the milestone.
constexpr bool is_milestone(block_state state) NOEXCEPT
{
    return is_any(state, milestoned);
}

/// At or below the milestone or top checkpoint.
constexpr bool is_bypass(block_state state) NOEXCEPT
{
    return is_any(state, bypass_states);
}

} // namespace database
} // namespace libbitcoin

#endif
