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
#ifndef LIBBITCOIN_DATABASE_TYPES_TX_STATE_HPP
#define LIBBITCOIN_DATABASE_TYPES_TX_STATE_HPP

#include <bitcoin/database/define.hpp>
#include <bitcoin/database/tables/tables.hpp>

namespace libbitcoin {
namespace database {

/// Validated tx fee and sigops, with each input's parent tx and coinbase.
/// The caller sizes prevouts to the tx input count before reading.
struct tx_state
{
    struct prevout
    {
        table::transaction::link parent{};
        bool coinbase{};
    };

    uint64_t fee{};
    size_t sigops{};
    std::vector<prevout> prevouts{};
};

} // namespace database
} // namespace libbitcoin

#endif
