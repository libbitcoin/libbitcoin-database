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
#ifndef LIBBITCOIN_DATABASE_TYPES_FEE_RATE_HPP
#define LIBBITCOIN_DATABASE_TYPES_FEE_RATE_HPP

#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

/// Fee rate of a tx, with the first height at which a pooled tx could confirm.
struct BCD_API fee_rate
{
    size_t bytes{};
    uint64_t fee{};
    size_t pooled_height{ max_size_t };
};

using fee_rates = std::vector<fee_rate>;

/// Count, virtual size, sigops and fee of a set of txs.
struct BCD_API fee_totals
{
    size_t count{};
    size_t bytes{};
    size_t sigops{};
    uint64_t fee{};
};

} // namespace database
} // namespace libbitcoin

#endif
