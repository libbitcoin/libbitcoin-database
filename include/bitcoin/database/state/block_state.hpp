/**
 * Copyright (c) 2011-2017 libbitcoin developers (see AUTHORS)
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
#ifndef LIBBITCOIN_DATABASE_BLOCK_STATE_HPP
#define LIBBITCOIN_DATABASE_BLOCK_STATE_HPP

#include <cstdint>

namespace libbitcoin {
namespace database {

// Stored block headers are always valid (PoW) with height, states are:
enum block_state : uint8_t
{
    /// This is not a stored state.
    missing = 0,

    /// Stored headers are always valid, these refer to their blocks.
    /// Mutually-exclusive (invalid is not pooled, only pent may be empty).
    failed = 1 << 0,
    pent = 1 << 1,
    valid = 1 << 2,

    /// Mutually-exclusive (confirmed must be valid, confirmed can't be empty).
    pooled = 1 << 3,
    indexed = 1 << 4,
    confirmed = 1 << 5,

    validations = invalid | pent | valid,
    confirmations = pooled | indexed | confirmed
};

// validation states

// This is not the same as !valid (could be pent).
inline bool is_failed(uint8_t state)
{
    return (state & block_state::failed) != 0;
}

inline bool is_pent(uint8_t state)
{
    return (state & block_state::pent) != 0;
}

inline bool is_valid(uint8_t state)
{
    return (state & block_state::valid) != 0;
}

// confirmation states

inline bool is_pooled(uint8_t state)
{
    return (state & block_state::pooled) != 0;
}

inline bool is_indexed(uint8_t state)
{
    return (state & block_state::indexed) != 0;
}

inline bool is_confirmed(uint8_t state)
{
    return (state & block_state::confirmed) != 0;
}

} // namespace database
} // namespace libbitcoin

#endif
