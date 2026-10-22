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
#ifndef LIBBITCOIN_DATABASE_TYPES_UNSPENT_TOTALS_HPP
#define LIBBITCOIN_DATABASE_TYPES_UNSPENT_TOTALS_HPP

#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

/// Summary of unspent outputs, used by bitcoind.
struct BCD_API unspent_totals
{
    /// Unspent outputs.
    size_t outputs{};

    /// Transactions with at least one unspent output.
    size_t transactions{};

    /// Total serialized script size of unspent outputs.
    size_t script_bytes{};

    /// Total coin-serialized (bitcoind) size of unspent outputs.
    size_t coin_bytes{};

    /// Total value of unspent outputs.
    uint64_t value{};

    /// Total value of provably unspendable outputs (excluded from the set).
    uint64_t unspendable{};

    /// Total value of coinbase outputs overwritten by bip30 duplication.
    uint64_t bip30{};
};

/// Funded and spent output counts and values of an address.
struct BCD_API address_totals
{
    size_t funded_count{};
    uint64_t funded_value{};
    size_t spent_count{};
    uint64_t spent_value{};
};

} // namespace database
} // namespace libbitcoin

#endif
