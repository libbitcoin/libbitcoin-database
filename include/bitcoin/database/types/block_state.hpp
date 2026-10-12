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

enum block_state : uint8_t
{
    /// Not validated.
    no_state = 0,

    /// Satisfies validation rules (prevouts unverified).
    valid = system::bit_right<uint8_t>(0),

    /// Satisfies confirmation rules (final).
    confirmable = system::bit_right<uint8_t>(1),

    /// Does not satisfy validation or confirmation rules (final).
    unconfirmable = system::bit_right<uint8_t>(2)
};

} // namespace database
} // namespace libbitcoin

#endif
