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
class txs
  : public hash_map<schema::txs>
{
public:
    using hash_map<schema::txs>::hashmap;

    struct slab
      : public schema::txs
    {
        linkage<pk> count() const NOEXCEPT
        {
            using namespace system;
            using out = typename linkage<pk>::integer;
            return possible_narrow_cast<out>(pk + sk +
                schema::tx * add1(tx_fks.size()));
        }

        inline bool from_data(reader& source) NOEXCEPT
        {
            tx_fks.resize(source.read_little_endian<uint32_t, schema::tx>());
            std::for_each(tx_fks.begin(), tx_fks.end(), [&](auto& fk) NOEXCEPT
            {
                fk = source.read_little_endian<uint32_t, schema::tx>();
            });

            BC_ASSERT(source.get_position() == count());
            return source;
        }

        inline bool to_data(finalizer& sink) const NOEXCEPT
        {
            BC_ASSERT(tx_fks.size() < system::power2<uint64_t>(to_bits(schema::tx)));
            const auto fks = system::possible_narrow_cast<uint32_t>(tx_fks.size());

            sink.write_little_endian<uint32_t, schema::tx>(fks);
            std::for_each(tx_fks.begin(), tx_fks.end(), [&](const auto& fk) NOEXCEPT
            {
                sink.write_little_endian<uint32_t, schema::tx>(fk);
            });

            BC_ASSERT(sink.get_position() == count());
            return sink;
        }

        inline bool operator==(const slab& other) const NOEXCEPT
        {
            return tx_fks == other.tx_fks;
        }

        std_vector<uint32_t> tx_fks{};
    };
};

} // namespace table
} // namespace database
} // namespace libbitcoin

#endif
