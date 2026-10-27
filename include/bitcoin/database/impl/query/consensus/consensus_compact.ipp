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
    const std::vector<short_id>& short_ids,
    const system::siphash_key& key) const NOEXCEPT
{
    using namespace system;
    out.assign(short_ids.size(), tx_link::terminal);
    if (short_ids.empty() || !store_.pool.enabled())
        return error::success;

    // A short id duplicated within the block is ambiguous.
    std::vector<bool> ambiguous(short_ids.size());
    std::unordered_map<short_id, size_t> positions{};
    positions.reserve(short_ids.size());
    for (size_t index{}; index < short_ids.size(); ++index)
    {
        const auto id = bit_and(short_ids.at(index), chain::short_id::mask);
        if (const auto [it, added] = positions.emplace(id, index); !added)
            ambiguous.at(index) = ambiguous.at(it->second) = true;
    }

    compact_matches matches{};
    if (!get_compact_matches(matches, positions, key))
        return error::integrity;

    for (const auto& [index, fk]: matches)
    {
        if (ambiguous.at(index))
            continue;

        // The columns of a record are written before its spine entry.
        const auto link = to_pool_tx(fk);
        if (link.is_terminal())
            continue;

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

// Scan the id columns, resolving tx links only after the guards release.
// The columns are little-endian words, hashed in place across vector lanes.
TEMPLATE
bool CLASS::get_compact_matches(compact_matches& out,
    const std::unordered_map<short_id, size_t>& positions,
    const system::siphash_key& key) const NOEXCEPT
{
    using namespace system;
    using lane_t = schema::pool::witness_lane;
    using link_t = table::pool::link::integer;
    static_assert(is_same_type<siphash_columns::value_type,
        std::span<const lane_t>>);
    const auto guard = store_.pool.get_memory();
    if (!guard)
        return false;

    // Records are allocated to the spine before their columns are written.
    const auto count = possible_narrow_cast<size_t>(store_.pool.count().value);
    const auto ptr0 = store_.pool.id0.get_raw();
    const auto ptr1 = store_.pool.id1.get_raw();
    const auto ptr2 = store_.pool.id2.get_raw();
    const auto ptr3 = store_.pool.id3.get_raw();

    const auto chunk_size = std::max(short_id_minimum_records,
        ceilinged_divide(count, two * cores()));
    std::vector<compact_matches> found(ceilinged_divide(count, chunk_size));
    std::vector<size_t> chunks(found.size());
    std::iota(chunks.begin(), chunks.end(), zero);
    const auto policy = poolstl::execution::par_if(store_.turbo());

    std::for_each(policy, chunks.cbegin(), chunks.cend(),
        [&](size_t chunk) NOEXCEPT
        {
            const auto first = chunk * chunk_size;
            std::vector<short_id> ids(std::min(chunk_size, count - first));

            const auto column = [&](memory::iterator ptr) NOEXCEPT
            {
                const auto data = pointer_cast<const lane_t>(ptr);
                return std::span<const lane_t>
                {
                    std::next(data, first), ids.size()
                };
            };

            siphash(ids, key, siphash_columns
            {
                column(ptr0), column(ptr1), column(ptr2), column(ptr3)
            });

            auto& matches = found.at(chunk);
            for (size_t index{}; index < ids.size(); ++index)
            {
                const auto id = bit_and(ids[index], chain::short_id::mask);
                if (const auto it = positions.find(id); it != positions.cend())
                    matches.emplace_back(it->second,
                        possible_narrow_cast<link_t>(first + index));
            }
        });

    for (const auto& matches: found)
        out.insert(out.end(), matches.cbegin(), matches.cend());

    return true;
}

} // namespace database
} // namespace libbitcoin

#endif
