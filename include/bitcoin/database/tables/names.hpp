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
#ifndef LIBBITCOIN_DATABASE_TABLES_NAMES_HPP
#define LIBBITCOIN_DATABASE_TABLES_NAMES_HPP

#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {
namespace schema {

namespace dir
{
    constexpr auto heads = "heads";
    constexpr auto primary = "primary";
    constexpr auto secondary = "secondary";
    constexpr auto temporary = "temporary";
}

namespace archive
{
    constexpr auto header = "archive_header";
    constexpr auto input = "archive_input";
    constexpr auto output = "archive_output";
    constexpr auto point = "archive_point";
    constexpr auto ins = "archive_ins";
    constexpr auto outs = "archive_outs";
    constexpr auto spend = "archive_spend";
    constexpr auto tx = "archive_tx";
    constexpr auto txs = "archive_txs";
}

namespace indexes
{
    constexpr auto candidate = "candidate";
    constexpr auto confirmed = "confirmed";
    constexpr auto strong_tx = "strong_tx";
}

namespace caches
{
    constexpr auto duplicate = "duplicate";
    constexpr auto prevout = "prevout";
    constexpr auto validated_bk = "validated_bk";
    constexpr auto validated_tx = "validated_tx";
}

namespace optionals
{
    constexpr auto address = "address";
    constexpr auto filter_bk = "filter_bk";
    constexpr auto filter_tx = "filter_tx";
}

namespace locks
{
    constexpr auto flush = "flush";
    constexpr auto process = "process";
}

namespace ext
{
    constexpr auto head = ".head";
    constexpr auto data = ".data";
    constexpr auto lock = ".lock";
}

} // namespace schema
} // namespace database
} // namespace libbitcoin

#endif
