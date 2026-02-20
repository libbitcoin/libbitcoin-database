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
#ifndef LIBBITCOIN_DATABASE_TYPES_HPP
#define LIBBITCOIN_DATABASE_TYPES_HPP

#include <set>
#include <utility>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/tables/tables.hpp>

namespace libbitcoin {
namespace database {

/// Database type aliases.
using height_link = table::height::link;
using header_link = table::header::link;
using output_link = table::output::link;
using input_link = table::input::link;
using outs_link = table::outs::link;
using ins_link = table::ins::link;
using point_link = table::point::link;
using tx_link = table::transaction::link;
using filter_link = table::filter_tx::link;
using strong_link = table::strong_tx::link;

using header_links = std::vector<header_link::integer>;
using tx_links = std::vector<tx_link::integer>;
using input_links = std::vector<input_link::integer>;
using output_links = std::vector<output_link::integer>;
using point_links = std::vector<point_link::integer>;
using two_counts = std::pair<size_t, size_t>;
using point_key = table::point::key;

using inpoint = system::chain::point;
using inpoints = std::set<inpoint>;
using outpoint = system::chain::outpoint;
using outpoints = std::set<outpoint>;

struct header_state { header_link link; code ec; };
using header_states = std::vector<header_state>;

struct fee_state { size_t bytes{}; uint64_t fee{}; };
using fee_states = std::vector<fee_state>;
using fee_state_sets = std::vector<fee_states>;

namespace fee
{
    using atomic_size = std::atomic<size_t>;

    /// Estimation modes.
    enum class mode
    {
        basic,
        markov,
        economical,
        conservative 
    };

    /// Estimation confidences.
    namespace confidence
    {
        static constexpr double low = 0.60;
        static constexpr double mid = 0.85;
        static constexpr double high = 0.95;
    }

    /// Bucket count sizing parameters.
    namespace horizon
    {
        static constexpr size_t small  = 12;
        static constexpr size_t medium = 48;
        static constexpr size_t large  = 1008;
    }

    /// Bucket count sizing parameters.
    namespace size
    {
        ////static constexpr double min  = 1.0;
        static constexpr double min = 0.1;
        static constexpr double max  = 100'000.0;
        static constexpr double step = 1.05;

        /// This is derived from min/max/step above.
        ////static constexpr size_t count = 236;
        static constexpr size_t count = 283;
    }

    /// Accumulator (persistent, decay-weighted counters).
    struct accumulator
    {
        template <size_t Depth>
        struct bucket
        {
            /// Total scaled txs in bucket.
            atomic_size total{};

            /// confirmed[n]: scaled txs confirmed in > n blocks.
            std::array<atomic_size, Depth> confirmed;
        };

        /// Current block height of accumulated state.
        size_t top_height{};

        /// Accumulated scaled fee in decayed buckets by horizon.
        /// Array count is the half life of the decay it implies.
        std::array<bucket<horizon::small>,  size::count> small{};
        std::array<bucket<horizon::medium>, size::count> medium{};
        std::array<bucket<horizon::large>,  size::count> large{};
    };
} // namespace fee

} // namespace database
} // namespace libbitcoin

#endif
