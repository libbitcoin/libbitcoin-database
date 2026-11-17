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
#ifndef LIBBITCOIN_DATABASE_QUERY_POOL_SUFFICE_IPP
#define LIBBITCOIN_DATABASE_QUERY_POOL_SUFFICE_IPP

#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

TEMPLATE
bool CLASS::set_pool_context(size_t& out, const context& ctx) NOEXCEPT
{
    using algorithm = system::pool::algorithm;
    out = zero;
    if (!store_.pool.enabled())
        return true;

    // ========================================================================
    const auto scope = get_transactor();

    pool_guards guards{};
    algorithm::columns_t columns{};
    if (!get_pool_columns(columns, guards))
        return false;

    system::pool::suffice(out, columns, { ctx.flags, ctx.height, ctx.mtp },
        system::chain::flags::bip113_rule);

    system::pool::settle(columns);
    system::pool::label(columns);
    return true;
    // ========================================================================
}

} // namespace database
} // namespace libbitcoin

#endif
