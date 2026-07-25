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
#ifndef LIBBITCOIN_DATABASE_TYPES_HISTORY_HPP
#define LIBBITCOIN_DATABASE_TYPES_HISTORY_HPP

#include <bitcoin/database/define.hpp>
#include <bitcoin/database/types/type.hpp>

namespace libbitcoin {
namespace database {

struct BCD_API history
{
    /// unrooted_height maps to -1 in the Electrum json-rpc.
    static constexpr size_t rooted_height = zero;
    static constexpr size_t unrooted_height = max_size_t;
    static constexpr size_t unconfirmed_position = max_size_t;
    static constexpr uint64_t missing_prevout = max_uint64;

    /// Filter out invalid history elements (txs) and sort.
    static void filter_sort_and_dedup(std::vector<history>& history) NOEXCEPT;

    /// The tx is valid.
    bool valid() const NOEXCEPT;

    /// A fault occured (and the tx is not valid).
    bool fault() const NOEXCEPT;

    /// The unconfirmed tx is rooted in chain (all prevouts confirmed).
    bool rooted() const NOEXCEPT;

    /// The tx is confirmed in a block.
    bool confirmed() const NOEXCEPT;

    /// Comparison operator based on Electrum history status sort.
    bool operator<(const history& other) const NOEXCEPT;

    /// Equivalence: !LT && !GT (note that fee is never considered).
    bool operator==(const history& other) const NOEXCEPT;

    /// Tx hash and block height, or rooted/unrooted_height if unconfirmed.
    checkpoint tx{};

    /// Tx fee, or history::missing_prevout for missing, confirmed, or -fee.
    uint64_t fee{};

    /// Tx's position in confirmed block, or history::unconfirmed_position.
    size_t position{};
};

using histories = std::vector<history>;

} // namespace database
} // namespace libbitcoin

#endif
