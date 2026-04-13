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
#ifndef LIBBITCOIN_DATABASE_TYPES_TYPE_HPP
#define LIBBITCOIN_DATABASE_TYPES_TYPE_HPP

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

using checkpoint = system::chain::checkpoint;

using inpoint = system::chain::point;
using inpoints = std::vector<inpoint>;

using outpoint = system::chain::outpoint;
using outpoints = std::vector<outpoint>;

/// Custom data carriers.
/// ---------------------------------------------------------------------------
/// Also: position, header_state, fee_rate, span, unspent, history.

// std_vector required for network::messages compat.
using heights = std_vector<size_t>;
using sizes = std::pair<size_t, size_t>;
using counts = std::pair<size_t, size_t>;

} // namespace database
} // namespace libbitcoin

#endif
