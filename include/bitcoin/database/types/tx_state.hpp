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
#ifndef LIBBITCOIN_DATABASE_TYPES_TX_STATE_HPP
#define LIBBITCOIN_DATABASE_TYPES_TX_STATE_HPP

#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

enum tx_state : uint8_t
{
    /// final
    connected = 0,

    /// final
    disconnected = 1,

    /// transitional (debug)
    tx_unknown = 42
};

} // namespace database
} // namespace libbitcoin

#endif
