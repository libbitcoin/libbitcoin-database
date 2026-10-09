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
#ifndef LIBBITCOIN_DATABASE_TABLES_NAMES_HPP
#define LIBBITCOIN_DATABASE_TABLES_NAMES_HPP

#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {
namespace schema {

/// These should be ASCII only.

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
    constexpr auto ins = "archive_ins";
    constexpr auto outs = "archive_outs";
    constexpr auto tx = "archive_tx";
    constexpr auto txs = "archive_txs";
}

namespace indexes
{
    constexpr auto candidate = "index_candidate";
    constexpr auto confirmed = "index_confirmed";
    constexpr auto strong_tx = "index_strong";
    constexpr auto wtxid = "index_wtxid";
}

namespace caches
{
    // aggregate
    constexpr auto ecdsa0 = "batch_ecdsa0";
    constexpr auto ecdsa1 = "batch_ecdsa1";
    constexpr auto ecdsa_correlate = "identity"_t;
    constexpr auto ecdsa_row = "row"_t;

    // aggregate
    constexpr auto schnorr0 = "batch_schnorr0";
    constexpr auto schnorr1 = "batch_schnorr1";
    constexpr auto schnorr_correlate = "identity"_t;
    constexpr auto schnorr_row = "row"_t;

    // aggregate
    constexpr auto scan = "batch_scan";
    constexpr auto scan_correlate = "identity"_t;
    constexpr auto scan_row = "row"_t;

    // aggregate
    constexpr auto silent0 = "batch_silent0";
    constexpr auto silent1 = "batch_silent1";
    constexpr auto silent_correlate = "identity"_t;
    constexpr auto silent_row = "row"_t;

    constexpr auto envelope = "envelope";
    constexpr auto prevalid0 = "batch_prevalid0";
    constexpr auto prevalid1 = "batch_prevalid1";
    constexpr auto prevout = "cache_prevout";
    constexpr auto duplicate = "cache_duplicate";
    constexpr auto state = "cache_state";
    constexpr auto pool = "cache_pool";
    constexpr auto pool_id0 = "id0"_t;
    constexpr auto pool_id1 = "id1"_t;
    constexpr auto pool_id2 = "id2"_t;
    constexpr auto pool_id3 = "id3"_t;
    constexpr auto spends = "cache_spends";
}

namespace optionals
{
    constexpr auto address = "option_address";
    constexpr auto filter_bk = "option_filter_bk";
    constexpr auto filter_tx = "option_filter_tx";
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
