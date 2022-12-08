/**
 * Copyright (c) 2011-2022 libbitcoin developers (see AUTHORS)
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
#ifndef LIBBITCOIN_DATABASE_QUERIES_INDEX_IPP
#define LIBBITCOIN_DATABASE_QUERIES_INDEX_IPP

#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

TEMPLATE
CLASS::block::cptr CLASS::get_header(size_t height) NOEXCEPT
{
    return get_header(get_header_fk(height));
}

TEMPLATE
CLASS::block::cptr CLASS::get_block(size_t height) NOEXCEPT
{
    return get_block(get_header_fk(height));
}

TEMPLATE
CLASS::block::cptr CLASS::get_txs(size_t height) NOEXCEPT
{
    return get_txs(get_header_fk(height));
}

TEMPLATE
size_t CLASS::get_fork_point() NOEXCEPT
{
    // TODO:
    return {};
}

TEMPLATE
size_t CLASS::get_validator_start(size_t height) NOEXCEPT
{
    // TODO:
    return {};
}

TEMPLATE
hashes CLASS::get_downloadable_blocks(size_t height) NOEXCEPT
{
    // TODO:
    return {};
}

} // namespace database
} // namespace libbitcoin

#endif
