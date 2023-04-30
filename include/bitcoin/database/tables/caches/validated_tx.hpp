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
#ifndef LIBBITCOIN_DATABASE_TABLES_CACHES_VALIDATED_TX_HPP
#define LIBBITCOIN_DATABASE_TABLES_CACHES_VALIDATED_TX_HPP

#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/primitives/primitives.hpp>
#include <bitcoin/database/tables/context.hpp>
#include <bitcoin/database/tables/schema.hpp>

namespace libbitcoin {
namespace database {
namespace table {

/// validated_tx is a slab multimap of tx validation state.
/// context is not incorporated into a composite key because of sufficiency.
struct validated_tx
  : public hash_map<schema::validated_tx>
{
    using coding = linkage<schema::code>;
    using sigop = linkage<schema::sigops>;
    using hash_map<schema::validated_tx>::hashmap;

    struct slab
      : public schema::validated_tx
    {
        link count() const NOEXCEPT
        {
            using namespace system;
            return possible_narrow_cast<link::integer>(pk + sk +
                context::size +
                coding::size +
                variable_size(fee) +
                variable_size(sigops));
        }

        template <typename Reader>
        inline bool from_data(Reader& source) NOEXCEPT
        {
            using namespace system;
            context::from_data(source, ctx);
            code = source.read_little_endian<coding::integer, coding::size>();
            fee = source.read_variable();
            sigops = possible_narrow_cast<sigop::integer>(source.read_variable());
            BC_ASSERT(source.get_read_position() == count());
            return source;
        }

        template <typename Writer>
        inline bool to_data(Writer& sink) const NOEXCEPT
        {
            context::to_data(sink, ctx);
            sink.write_little_endian<coding::integer, coding::size>(code);
            sink.write_variable(fee);
            sink.write_variable(sigops);
            BC_ASSERT(sink.get_write_position() == count());
            return sink;
        }

        inline bool operator==(const slab& other) const NOEXCEPT
        {
            return ctx    == other.ctx
                && code   == other.code
                && fee    == other.fee
                && sigops == other.sigops;
        }

        context ctx{};
        coding::integer code{};
        uint64_t fee{};
        sigop::integer sigops{};
    };

    struct slab_get_code
      : public schema::validated_tx
    {
        template <typename Reader>
        inline bool from_data(Reader& source) NOEXCEPT
        {
            context::from_data(source, ctx);
            code = source.read_little_endian<coding::integer, coding::size>();
            return source;
        }
    
        context ctx{};
        coding::integer code{};
    };
};

} // namespace table
} // namespace database
} // namespace libbitcoin

#endif
