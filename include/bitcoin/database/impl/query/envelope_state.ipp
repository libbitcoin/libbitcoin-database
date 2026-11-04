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
#ifndef LIBBITCOIN_DATABASE_QUERY_ENVELOPE_STATE_IPP
#define LIBBITCOIN_DATABASE_QUERY_ENVELOPE_STATE_IPP

#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

TEMPLATE
bool CLASS::is_pooling() const NOEXCEPT
{
    return store_.is_pooling();
}

TEMPLATE
void CLASS::set_pooling() NOEXCEPT
{
    store_.set_pooling();
}

TEMPLATE
size_t CLASS::get_scan_frontier() const NOEXCEPT
{
    return store_.get_scan_frontier();
}

TEMPLATE
size_t CLASS::get_latch_tx() const NOEXCEPT
{
    return store_.get_latch_tx();
}

TEMPLATE
bool CLASS::get_activation(size_t& out, uint32_t flag) const NOEXCEPT
{
    using namespace system;
    if (!is_one(ones_count(flag)))
        return false;

    const auto activation = store_.get_activation(right_zeros(flag));
    if (!get_right(activation.flags, right_zeros(flag)))
        return false;

    context ctx{};
    const header_link link{ activation.link };
    if (!get_context(ctx, link) ||
        (to_confirmed(ctx.height) != link) ||
        !get_right(ctx.flags, right_zeros(flag)))
        return false;

    out = ctx.height;
    return true;
}

TEMPLATE
bool CLASS::set_activation(uint32_t flags, const header_link& link) NOEXCEPT
{
    using namespace system;
    for (auto bits = flags; !is_zero(bits);
        bits = set_right(bits, right_zeros(bits), false))
    {
        if (!store_.set_activation(right_zeros(bits), flags, link))
            return false;
    }

    return true;
}

TEMPLATE
bool CLASS::clear_activation(uint32_t flags) NOEXCEPT
{
    return set_activation(flags, header_link::terminal);
}

} // namespace database
} // namespace libbitcoin

#endif
