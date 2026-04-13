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
#ifndef LIBBITCOIN_DATABASE_TYPES_HEADER_STATE_HPP
#define LIBBITCOIN_DATABASE_TYPES_HEADER_STATE_HPP

#include <bitcoin/database/define.hpp>
#include <bitcoin/database/types/type.hpp>

namespace libbitcoin {
namespace database {

struct header_state
{
    header_link link;
    code ec;
};

using header_states = std::vector<header_state>;

} // namespace database
} // namespace libbitcoin

#endif
