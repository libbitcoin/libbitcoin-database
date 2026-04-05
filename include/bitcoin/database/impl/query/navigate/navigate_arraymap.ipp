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
#ifndef LIBBITCOIN_DATABASE_QUERY_NAVIGATE_ARRAYMAP_IPP
#define LIBBITCOIN_DATABASE_QUERY_NAVIGATE_ARRAYMAP_IPP

#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

// header to arraymap tables (guard domain transitions)
// ----------------------------------------------------------------------------

TEMPLATE
constexpr size_t CLASS::to_validated_bk(const header_link& link) const NOEXCEPT
{
    static_assert(header_link::terminal <= table::validated_bk::link::terminal);
    return link.is_terminal() ? table::validated_bk::link::terminal : link.value;
}

TEMPLATE
constexpr size_t CLASS::to_filter_bk(const header_link& link) const NOEXCEPT
{
    static_assert(header_link::terminal <= table::filter_bk::link::terminal);
    return link.is_terminal() ? table::filter_bk::link::terminal : link.value;
}

TEMPLATE
constexpr size_t CLASS::to_filter_tx(const header_link& link) const NOEXCEPT
{
    static_assert(header_link::terminal <= table::filter_tx::link::terminal);
    return link.is_terminal() ? table::filter_tx::link::terminal : link.value;
}

TEMPLATE
constexpr size_t CLASS::to_prevout(const header_link& link) const NOEXCEPT
{
    static_assert(header_link::terminal <= table::prevout::link::terminal);
    return link.is_terminal() ? table::prevout::link::terminal : link.value;
}

TEMPLATE
constexpr size_t CLASS::to_txs(const header_link& link) const NOEXCEPT
{
    static_assert(header_link::terminal <= table::txs::link::terminal);
    return link.is_terminal() ? table::txs::link::terminal : link.value;
}

} // namespace database
} // namespace libbitcoin

#endif
