/**
 * Copyright (c) 2011-2022 libbitcoin developers (see AUTHORS)
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
#ifndef LIBBITCOIN_DATABASE_TABLES_ARCHIVES_TXS_HPP
#define LIBBITCOIN_DATABASE_TABLES_ARCHIVES_TXS_HPP

#include <algorithm>
#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/memory.hpp>
#include <bitcoin/database/primitives/primitives.hpp>
#include <bitcoin/database/tables/schema.hpp>

namespace libbitcoin {
namespace database {
namespace table {

/// Txs is a slab hashmap of tx fks (first is count), searchable by header.fk.
struct txs
  : public hash_map<schema::txs>
{
    using tx = linkage<schema::tx>;
    using keys = std_vector<tx::integer>;
    using hash_map<schema::txs>::hashmap;

    struct slab
      : public schema::txs
    {
        link count() const NOEXCEPT
        {
            return system::possible_narrow_cast<link::integer>(pk + sk +
                tx::size * add1(tx_fks.size()));
        }

        template <typename Reader>
        inline bool from_data(Reader& source) NOEXCEPT
        {
            tx_fks.resize(source.read_little_endian<tx::integer, tx::size>());
            std::for_each(tx_fks.begin(), tx_fks.end(), [&](auto& fk) NOEXCEPT
            {
                fk = source.read_little_endian<tx::integer, tx::size>();
            });

            BC_ASSERT(source.get_read_position() == count());
            return source;
        }

        template <typename Writer>
        inline bool to_data(Writer& sink) const NOEXCEPT
        {
            BC_ASSERT(tx_fks.size() < system::power2<uint64_t>(to_bits(tx::size)));
            const auto fks = system::possible_narrow_cast<tx::integer>(tx_fks.size());

            sink.write_little_endian<tx::integer, tx::size>(fks);
            std::for_each(tx_fks.begin(), tx_fks.end(), [&](const auto& fk) NOEXCEPT
            {
                sink.write_little_endian<tx::integer, tx::size>(fk);
            });

            BC_ASSERT(sink.get_write_position() == count());
            return sink;
        }

        inline bool operator==(const slab& other) const NOEXCEPT
        {
            return tx_fks == other.tx_fks;
        }

        keys tx_fks{};
    };

    struct get_position
      : public schema::txs
    {
        template <typename Reader>
        inline bool from_data(Reader& source) NOEXCEPT
        {
            const auto count = source.read_little_endian<tx::integer, tx::size>();
            for (position = zero; position < count; ++position)
                if (source.read_little_endian<tx::integer, tx::size>() == link)
                    return source;

            source.invalidate();
            return source;
        }

        const tx::integer link{};
        size_t position{};
    };

    struct get_coinbase
      : public schema::txs
    {
        template <typename Reader>
        inline bool from_data(Reader& source) NOEXCEPT
        {
            const auto count = source.read_little_endian<tx::integer, tx::size>();
            if (!is_zero(count))
            {
                coinbase_fk = source.read_little_endian<tx::integer, tx::size>();
                return source;
            }

            source.invalidate();
            return source;
        }

        tx::integer coinbase_fk{};
    };
};

} // namespace table
} // namespace database
} // namespace libbitcoin

#endif
