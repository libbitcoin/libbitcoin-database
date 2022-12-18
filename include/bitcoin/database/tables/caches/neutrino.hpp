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
#ifndef LIBBITCOIN_DATABASE_TABLES_CACHES_NEUTRINO_HPP
#define LIBBITCOIN_DATABASE_TABLES_CACHES_NEUTRINO_HPP

#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/primitives/primitives.hpp>
#include <bitcoin/database/tables/schema.hpp>

namespace libbitcoin {
namespace database {
namespace table {

/// neutrino is a slab hashmap of neutrino filters.
struct neutrino
  : public hash_map<schema::neutrino>
{
    using hash_map<schema::neutrino>::hashmap;

    struct slab
      : public schema::neutrino
    {
        link count() const NOEXCEPT
        {
            return system::possible_narrow_cast<link::integer>(pk + sk +
                schema::hash +
                variable_size(filter.size()) +
                filter.size());
        }

        inline bool from_data(reader& source) NOEXCEPT
        {
            filter_head = source.read_hash();
            filter = source.read_bytes(source.read_variable());
            BC_ASSERT(source.get_read_position() == count());
            return source;
        }

        inline bool to_data(finalizer& sink) const NOEXCEPT
        {
            sink.write_bytes(filter_head);
            sink.write_variable(filter.size());
            sink.write_bytes(filter);
            BC_ASSERT(sink.get_write_position() == count());
            return sink;
        }

        inline bool operator==(const slab& other) const NOEXCEPT
        {
            return filter_head == other.filter_head
                && filter      == other.filter;
        }

        hash_digest filter_head{};
        system::data_chunk filter{};
    };

    struct slab_get_filter
      : public schema::neutrino
    {
        link count() const NOEXCEPT
        {
            return system::possible_narrow_cast<link::integer>(pk + sk +
                schema::hash +
                variable_size(filter.size()) +
                filter.size());
        }

        inline bool from_data(reader& source) NOEXCEPT
        {
            source.skip_bytes(schema::hash);
            filter = source.read_bytes(source.read_variable());
            BC_ASSERT(source.get_read_position() == count());
            return source;
        }

        system::data_chunk filter{};
    };

    struct slab_get_head
        : public schema::neutrino
    {
        inline bool from_data(reader& source) NOEXCEPT
        {
            filter_head = source.read_hash();
            return source;
        }

        hash_digest filter_head{};
    };

    struct slab_put_ref
      : public schema::neutrino
    {
        link count() const NOEXCEPT
        {
            return system::possible_narrow_cast<link::integer>(pk + sk +
                schema::hash +
                variable_size(filter.size()) +
                filter.size());
        }

        inline bool to_data(finalizer& sink) const NOEXCEPT
        {
            sink.write_bytes(filter_head);
            sink.write_variable(filter.size());
            sink.write_bytes(filter);
            BC_ASSERT(sink.get_write_position() == count());
            return sink;
        }

        const hash_digest& filter_head{};
        const system::data_chunk& filter{};
    };
};

} // namespace table
} // namespace database
} // namespace libbitcoin

#endif
