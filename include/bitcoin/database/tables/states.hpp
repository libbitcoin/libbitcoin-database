/**
 * Copyright (c) 2011-2025 libbitcoin developers (see AUTHORS)
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
#ifndef LIBBITCOIN_DATABASE_TABLES_STATES_HPP
#define LIBBITCOIN_DATABASE_TABLES_STATES_HPP

#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {
namespace schema {

enum block_state : uint8_t
{
    /// final
    confirmable = 0,

    /// transitional
    valid = 1,

    /// final
    unconfirmable = 2,

    /// transitional (debug)
    block_unknown = 42
};

enum tx_state : uint8_t
{
    /// final
    connected = 0,

    /// final
    disconnected = 1,

    /// transitional (debug)
    tx_unknown = 42
};

} // namespace schema
} // namespace database
} // namespace libbitcoin

#endif
