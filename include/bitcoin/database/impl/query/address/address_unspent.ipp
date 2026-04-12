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
#ifndef LIBBITCOIN_DATABASE_QUERY_ADDRESS_UNSPENT_IPP
#define LIBBITCOIN_DATABASE_QUERY_ADDRESS_UNSPENT_IPP

#include <atomic>
#include <algorithm>
#include <utility>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

// Address unspent.
// ----------------------------------------------------------------------------
// A list of all unspent output transactions in canonical order.
// Unconfirmed unspent are included at end of list in consistent order.

// ununsed
TEMPLATE
code CLASS::get_unconfirmed_unspent(const stopper& , unspents& ,
    const hash_digest& , bool ) const NOEXCEPT
{
    return {};
}

// ununsed
TEMPLATE
code CLASS::get_confirmed_unspent(const stopper& , unspents& ,
    const hash_digest& , bool ) const NOEXCEPT
{
    return {};
}

// server/electrum
TEMPLATE
code CLASS::get_unspent(const stopper& , unspents& ,
    const hash_digest& , bool ) const NOEXCEPT
{
    return {};
}

// turbos
// ----------------------------------------------------------------------------
// protected

} // namespace database
} // namespace libbitcoin

#endif
