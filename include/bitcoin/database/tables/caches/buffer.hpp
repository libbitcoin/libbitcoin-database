/**
 * Copyright (c) 2011-2023 libbitcoin developers (see AUTHORS)
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
#ifndef LIBBITCOIN_DATABASE_TABLES_CACHES_BUFFER_HPP
#define LIBBITCOIN_DATABASE_TABLES_CACHES_BUFFER_HPP

#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/primitives/primitives.hpp>
#include <bitcoin/database/tables/schema.hpp>

namespace libbitcoin {
namespace database {
namespace table {

/// buffer is a slab hashmap of txs.
struct buffer
  : public hash_map<schema::buffer>
{
    using hash_map<schema::buffer>::hashmap;

    struct slab
      : public schema::buffer
    {
        link count() const NOEXCEPT
        {
            return system::possible_narrow_cast<link::integer>(pk + sk +
                tx.serialized_size(true));
        }

        inline bool from_data(reader& source) NOEXCEPT
        {
            tx = system::chain::transaction{ source, true };
            BC_ASSERT(source.get_read_position() == count());
            return source;
        }

        inline bool to_data(finalizer& sink) const NOEXCEPT
        {
            tx.to_data(sink, true);
            BC_ASSERT(sink.get_write_position() == count());
            return sink;
        }

        inline bool operator==(const slab& other) const NOEXCEPT
        {
            return tx == other.tx;
        }

        system::chain::transaction tx{};
    };

    struct slab_ptr
      : public schema::buffer
    {
        link count() const NOEXCEPT
        {
            return system::possible_narrow_cast<link::integer>(pk + sk +
                tx->serialized_size(true));
        }

        inline bool from_data(reader& source) NOEXCEPT
        {
            using namespace system;
            tx = system::to_shared<chain::transaction>(source, true);
            return source;
        }

        inline bool to_data(finalizer& sink) const NOEXCEPT
        {
            BC_ASSERT(tx);
            tx->to_data(sink, true);
            return sink;
        }

        system::chain::transaction::cptr tx{};
    };

    struct put_ref
      : public schema::buffer
    {
        link count() const NOEXCEPT
        {
            return system::possible_narrow_cast<link::integer>(pk + sk +
                tx.serialized_size(true));
        }

        inline bool to_data(finalizer& sink) const NOEXCEPT
        {
            tx.to_data(sink, true);
            return sink;
        }

        const system::chain::transaction& tx{};
    };
};

} // namespace table
} // namespace database
} // namespace libbitcoin

#endif
