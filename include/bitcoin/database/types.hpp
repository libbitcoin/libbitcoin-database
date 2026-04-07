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

#include <set>
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
using address_link = table::address::link;

using header_links = std::vector<header_link::integer>;
using tx_links = std::vector<tx_link::integer>;
using input_links = std::vector<input_link::integer>;
using output_links = std::vector<output_link::integer>;
using point_links = std::vector<point_link::integer>;
using two_counts = std::pair<size_t, size_t>;
using point_key = table::point::key;

using checkpoint = system::chain::checkpoint;
using inpoint = system::chain::point;
using inpoints = std::set<inpoint>;
using outpoint = system::chain::outpoint;
using outpoints = std::set<outpoint>;

using data_chunk = system::data_chunk;

struct header_state { header_link link; code ec; };
using header_states = std::vector<header_state>;

struct fee_rate { size_t bytes{}; uint64_t fee{}; };
using fee_rates = std::vector<fee_rate>;
using fee_rate_sets = std::vector<fee_rates>;

struct encode_hash_less
{
    bool operator()(const hash_digest& a, const hash_digest& b) const NOEXCEPT
    {
        using namespace system;
        constexpr auto hi = to_half(byte_bits);
        constexpr auto lo = sub1(power2<uint8_t>(hi));

        // return encode_hash(a) < encode_hash(a)
        for (auto byte{ hash_size }; !is_zero(byte); --byte)
        {
            const auto byte_a = a[sub1(byte)];
            const auto byte_b = b[sub1(byte)];

            const auto hi_a = shift_right(byte_a, hi);
            const auto hi_b = shift_right(byte_b, hi);
            if (hi_a != hi_b)
                return hi_a < hi_b;

            const auto lo_a = bit_and(byte_a, lo);
            const auto lo_b = bit_and(byte_b, lo);
            if (lo_a != lo_b)
                return lo_a < lo_b;
        }

        return false;
    }
};

struct history { checkpoint tx{}; size_t position{}; uint64_t fee{}; };
struct history_less
{
    bool operator()(const history& a, const history& b) const NOEXCEPT
    {
        using namespace system;
        const auto a_height = a.tx.height();
        const auto b_height = b.tx.height();
        const auto a_confirmed = !is_min(a_height) && !is_max(a_height);
        const auto b_confirmed = !is_min(b_height) && !is_max(b_height);

        // Confirmed before unconfirmed.
        if (a_confirmed != b_confirmed)
            return a_confirmed;

        // Chain.block height ascending (0 < max | x < y).
        if (a_height != b_height)
            return a_height < b_height;

        // Block.tx position ascending (positions must differ).
        if (a_confirmed)
            return a.position < b.position;

        // Both unconfirmed (0 or max), base16 lexical txid ascending.
        return encode_hash_less{}(a.tx.hash(), b.tx.hash());
    }
};
using histories = std::set<history, history_less>;

struct unspent { outpoint tx{}; size_t height{}; size_t position{}; };
struct unspent_less
{
    bool operator()(const unspent& a, const unspent& b) const NOEXCEPT
    {
        const auto a_point = a.tx.point();
        const auto b_point = b.tx.point();
        const bool a_confirmed = !is_zero(a.height);
        const bool b_confirmed = !is_zero(b.height);

        // Confirmed before unconfirmed.
        if (a_confirmed != b_confirmed)
            return a_confirmed;

        if (a_confirmed)
        {
            // Chain.block height ascending (x < y).
            if (a.height != b.height)
                return a.height < b.height;

            // Block.tx position ascending.
            if (a.position != b.position)
                return a.position < b.position;

            // Tx.output index ascending.
            return a_point.index() < b_point.index();
        }

        // Unconfirmed have 0 height and position, arbitrary sort (hash:index).
        return a_point < b_point;
    }
};
using unspents = std::set<unspent, unspent_less>;

} // namespace database
} // namespace libbitcoin

#endif
