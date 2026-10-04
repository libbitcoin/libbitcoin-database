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

    compact_matches matches{};
    if (!get_compact_matches(matches, positions, key))
        return error::integrity;

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

// Scan the id columns, resolving tx links only after the guards release.
// The columns are little-endian words, hashed in place across vector lanes.
TEMPLATE
bool CLASS::get_compact_matches(compact_matches& out,
    const std::unordered_map<uint64_t, size_t>& positions,
    const system::siphash_key& key) const NOEXCEPT
{
    using namespace system;
    constexpr auto mask = unmask_right<uint64_t>(48);
    constexpr size_t chunk_rows = 16384;
    const auto ptr0 = store_.pool.id0.get_memory();
    const auto ptr1 = store_.pool.id1.get_memory();
    const auto ptr2 = store_.pool.id2.get_memory();
    const auto ptr3 = store_.pool.id3.get_memory();
    if (!ptr0 || !ptr1 || !ptr2 || !ptr3)
        return false;

    const auto rows = possible_narrow_cast<size_t>(store_.pool.count().value);
    if constexpr (is_little_endian)
    {
        const auto bytes = rows * sizeof(uint64_t);
        if (is_lesser(ptr0.size(), bytes) || is_lesser(ptr1.size(), bytes) ||
            is_lesser(ptr2.size(), bytes) || is_lesser(ptr3.size(), bytes))
            return false;
    }

    std::atomic_bool fail{};
    std::vector<compact_matches> found(ceilinged_divide(rows, chunk_rows));
    std::vector<size_t> chunks(found.size());
    std::iota(chunks.begin(), chunks.end(), zero);
    const auto policy = poolstl::execution::par_if(store_.turbo());

    std::for_each(policy, chunks.cbegin(), chunks.cend(),
        [&](size_t chunk) NOEXCEPT
        {
            const auto first = chunk * chunk_rows;
            std::vector<uint64_t> ids(std::min(chunk_rows, rows - first));

            if constexpr (is_little_endian)
            {
                const auto column = [&](const memory& ptr) NOEXCEPT
                {
                    return std::span<const uint64_t>
                    {
                        std::next(pointer_cast<const uint64_t>(ptr.data()),
                            first), ids.size()
                    };
                };

                siphash(ids, key, siphash_columns
                {
                    column(ptr0), column(ptr1), column(ptr2), column(ptr3)
                });
            }
            else
            {
                table::pool_word id0{}, id1{}, id2{}, id3{};
                for (size_t row{}; row < ids.size(); ++row)
                {
                    const table::pool::link link
                    {
                        possible_narrow_cast<table::pool::link::integer>(
                            first + row)
                    };

                    if (!store_.pool.id0.get(ptr0, link, id0) ||
                        !store_.pool.id1.get(ptr1, link, id1) ||
                        !store_.pool.id2.get(ptr2, link, id2) ||
                        !store_.pool.id3.get(ptr3, link, id3))
                    {
                        fail.store(true, std::memory_order_relaxed);
                        return;
                    }

                    ids[row] = siphash(key, siphash_words
                    {
                        id0.word, id1.word, id2.word, id3.word
                    });
                }
            }

            auto& matches = found.at(chunk);
            for (size_t row{}; row < ids.size(); ++row)
                if (const auto it = positions.find(bit_and(ids[row], mask));
                    it != positions.end())
                    matches.emplace_back(it->second,
                        possible_narrow_cast<table::pool::link::integer>(
                            first + row));
        });

    if (fail.load(std::memory_order_relaxed))
        return false;

    for (const auto& matches: found)
        out.insert(out.end(), matches.begin(), matches.end());

    return true;
}

} // namespace database
} // namespace libbitcoin

#endif
