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
#ifndef LIBBITCOIN_DATABASE_UNSPENT_UNSPENT_WRITER_HPP
#define LIBBITCOIN_DATABASE_UNSPENT_UNSPENT_WRITER_HPP

#include <bitcoin/database/define.hpp>
#include <bitcoin/database/types/types.hpp>

namespace libbitcoin {
namespace database {

/// The unspent output commitment element (bitcoind coin serialization).
struct unspent_writer
{
    static constexpr auto point_size = system::hash_size + sizeof(uint32_t);
    static constexpr auto code_size = sizeof(uint32_t);
    static constexpr auto value_size = sizeof(uint64_t);
    static constexpr auto fixed_size = point_size + code_size + value_size;

    /// Write element head (txid, index, height code) of coin to sink, tail
    /// (value, script) streamed by output::wire_script.
    static void write(system::writer& sink, const unspent_coin& coin) NOEXCEPT;

    /// The element size given the script size.
    static size_t size(size_t script) NOEXCEPT;

    /// Add an output to the totals.
    static void add(unspent_totals& out, bool first, uint64_t value,
        size_t script) NOEXCEPT;

    /// Add the coin (script copied) to the totals.
    static void add(unspent_totals& out, const unspent_coin& coin) NOEXCEPT;

    /// Add the totals to the totals.
    static void add(unspent_totals& out, const unspent_totals& totals) NOEXCEPT;
};

} // namespace database
} // namespace libbitcoin

#include <bitcoin/database/impl/unspent/unspent_writer.ipp>

#endif
