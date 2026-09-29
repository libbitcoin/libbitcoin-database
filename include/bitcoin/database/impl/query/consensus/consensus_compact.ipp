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
#ifndef LIBBITCOIN_DATABASE_QUERY_CONSENSUS_COMPACT_IPP
#define LIBBITCOIN_DATABASE_QUERY_CONSENSUS_COMPACT_IPP

#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

// Compact block short ids.
// ----------------------------------------------------------------------------

TEMPLATE
code CLASS::get_compact_links(tx_links& out,
    const std::vector<uint64_t>& short_ids,
    const system::siphash_key& key) const NOEXCEPT
{
    using namespace system;
    constexpr auto mask = unmask_right<uint64_t>(48);
    out.assign(short_ids.size(), tx_link::terminal);
    if (short_ids.empty() || !store_.pool.enabled())
        return error::success;

    // A short id duplicated within the block is ambiguous.
    std::vector<bool> ambiguous(short_ids.size());
    std::unordered_map<uint64_t, size_t> positions{};
    positions.reserve(short_ids.size());
    for (size_t index{}; index < short_ids.size(); ++index)
    {
        const auto id = bit_and(short_ids.at(index), mask);
        if (const auto [it, added] = positions.emplace(id, index); !added)
            ambiguous.at(index) = ambiguous.at(it->second) = true;
    }

    // Scan the id columns, resolving tx links only after guards release.
    std::vector<std::pair<size_t, table::pool::link>> matches{};
    {
        const auto ptr0 = store_.pool.id0.get_memory();
        const auto ptr1 = store_.pool.id1.get_memory();
        const auto ptr2 = store_.pool.id2.get_memory();
        const auto ptr3 = store_.pool.id3.get_memory();
        if (!ptr0 || !ptr1 || !ptr2 || !ptr3)
            return error::integrity;

        table::pool_word id0{}, id1{}, id2{}, id3{};
        const auto rows = store_.pool.count();
        for (table::pool::link row{ 0 }; row < rows; ++row)
        {
            if (!store_.pool.id0.get(ptr0, row, id0) ||
                !store_.pool.id1.get(ptr1, row, id1) ||
                !store_.pool.id2.get(ptr2, row, id2) ||
                !store_.pool.id3.get(ptr3, row, id3))
                return error::integrity;

            const auto words = to_little_endians(std_array<uint64_t, 4>
            {
                id0.word, id1.word, id2.word, id3.word
            });

            const auto id = bit_and(siphash(key, array_cast<uint8_t>(words)),
                mask);

            if (const auto it = positions.find(id); it != positions.end())
                matches.emplace_back(it->second, row);
        }
    }

    for (const auto& [index, row]: matches)
    {
        if (ambiguous.at(index))
            continue;

        const tx_link link{ store_.pool.get_key(row) };
        auto& found = out.at(index);
        if (found == tx_link::terminal)
        {
            found = link;
        }
        else if (found != link)
        {
            found = tx_link::terminal;
            ambiguous.at(index) = true;
        }
    }

    return error::success;
}

// Compact blocks.
/// TODO: apply these to compact block confirmation, as the block will
/// TODO: associate existing txs, making it impossible to rely on the
/// TODO: duplicates table. The full query approach must be used instead.
// ----------------------------------------------------------------------------
// protected

TEMPLATE
bool CLASS::get_double_spenders(tx_links& out, const point& point,
    const ins_link& self) const NOEXCEPT
{
    // This is most of the expense of compact block confirmation.
    // It is not mitigated by the point table filter, since self always exists.

    ins_links points{};
    for (auto it = store_.ins.it(point); it; ++it)
        if (*it != self)
            points.push_back(*it);

    for (auto point: points)
    {
        table::ins_sequence::get_parent get{};
        if (!store_.ins.sequence.get(point, get))
            return false;

        out.push_back(get.parent_fk);
    }

    return true;
}

TEMPLATE
bool CLASS::get_double_spenders(tx_links& out,
    const block& block) const NOEXCEPT
{
    // Empty or coinbase only implies no spends.
    const auto& txs = *block.transactions_ptr();
    if (txs.size() <= one)
        return true;

    for (auto tx = std::next(txs.cbegin()); tx != txs.cend(); ++tx)
        for (const auto& in: *(*tx)->inputs_ptr())
            if (!get_double_spenders(out, in->point(), in->metadata.point_link))
                return false;

    return true;
}

} // namespace database
} // namespace libbitcoin

#endif
