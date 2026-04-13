/**
 * Copyright (c) 2011-2026 libbitcoin developers (see AUTHORS)
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

#include <atomic>
#include <optional>
#include <utility>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/tables/tables.hpp>

namespace libbitcoin {
namespace database {

/// Common table aliases.
/// ---------------------------------------------------------------------------

/// Singles.
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
using address_link = table::address::link;

/// Multiples.
using header_links = std::vector<header_link::integer>;
using tx_links = std::vector<tx_link::integer>;
using input_links = std::vector<input_link::integer>;
using output_links = std::vector<output_link::integer>;
using point_links = std::vector<point_link::integer>;
using point_key = table::point::key;

/// Point index (uint32_t).
using index = table::transaction::ix::integer;

/// Common std aliases.
/// ---------------------------------------------------------------------------
using stopper = std::atomic_bool;
using hash_option = std::optional<hash_digest>;

/// Common system aliases.
/// ---------------------------------------------------------------------------

using filter = system::data_chunk;
using data_chunk = system::data_chunk;

/// Common system::chain aliases.
/// ---------------------------------------------------------------------------

using inpoint = system::chain::point;
using outpoint = system::chain::outpoint;
using inpoints = std::vector<inpoint>;
using outpoints = std::vector<outpoint>;
using checkpoint = system::chain::checkpoint;

/// Common carriers.
/// ---------------------------------------------------------------------------

using counts = std::pair<size_t, size_t>;
using sizes = std::pair<size_t, size_t>;
using heights = std_vector<size_t>;

struct position { size_t sibling; size_t width; };
using positions = std::vector<position>;

struct header_state { header_link link; code ec; };
using header_states = std::vector<header_state>;

struct fee_rate { size_t bytes{}; uint64_t fee{}; };
using fee_rates = std::vector<fee_rate>;
using fee_rate_sets = std::vector<fee_rates>;

struct span
{
    inline size_t size() const NOEXCEPT
    {
        return system::floored_subtract(end, begin);
    }

    size_t begin;
    size_t end;
};

struct BCD_API unspent
{
    static constexpr size_t excluded_position = max_size_t;

    struct less_than
    {
        bool operator()(const unspent& a, const unspent& b) const NOEXCEPT;
    };

    struct equal_to
    {
        bool operator()(const unspent& a, const unspent& b) const NOEXCEPT;
    };

    struct exclude
    {
        bool operator()(const unspent& element) const NOEXCEPT;
    };

    static void sort_and_dedup(std::vector<unspent>& unspent) NOEXCEPT;

    outpoint tx{};
    size_t height{};
    size_t position{};
};
using unspents = std::vector<unspent>;

struct BCD_API history
{
    static constexpr size_t rooted_height = zero;
    static constexpr size_t unrooted_height = max_size_t;
    static constexpr size_t missing_prevout = max_uint64;
    static constexpr size_t unconfirmed_position = zero;

    struct less_than
    {
        bool operator()(const history& a, const history& b) const NOEXCEPT;
    };

    struct equal_to
    {
        bool operator()(const history& a, const history& b) const NOEXCEPT;
    };

    struct exclude
    {
        bool operator()(const history& element) const NOEXCEPT;
    };

    static void sort_and_dedup(std::vector<history>& history) NOEXCEPT;

    checkpoint tx{};
    uint64_t fee{};
    size_t position{};
};
using histories = std::vector<history>;

} // namespace database
} // namespace libbitcoin

#endif
