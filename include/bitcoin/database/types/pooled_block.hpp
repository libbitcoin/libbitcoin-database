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
#ifndef LIBBITCOIN_DATABASE_TYPES_POOLED_BLOCK_HPP
#define LIBBITCOIN_DATABASE_TYPES_POOLED_BLOCK_HPP

#include <unordered_map>
#include <unordered_set>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/tables/tables.hpp>
#include <bitcoin/database/types/pooled_tx.hpp>
#include <bitcoin/database/types/type.hpp>

namespace libbitcoin {
namespace database {

/// Working set of a block validated from its pooled txs.
struct pooled_block
{
    using spend = table::prevout::spend;

    tx_links txs{};
    std::vector<table::transaction::record> records{};
    std::vector<pooled_tx> pooled{};
    std::unordered_map<system::hash_digest, size_t> positions{};
    std::unordered_set<system::chain::point> points{};
    std::vector<spend> spends{};
    tx_links conflicts{};
    uint64_t fees{};
    size_t sigops{};
    size_t light{};
    size_t heavy{};
};

} // namespace database
} // namespace libbitcoin

#endif
