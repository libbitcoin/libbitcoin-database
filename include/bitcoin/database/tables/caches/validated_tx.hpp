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
#ifndef LIBBITCOIN_DATABASE_TABLES_CACHES_VALIDATED_TX_HPP
#define LIBBITCOIN_DATABASE_TABLES_CACHES_VALIDATED_TX_HPP

#include <bitcoin/database/define.hpp>
#include <bitcoin/database/primitives/primitives.hpp>
#include <bitcoin/database/tables/context.hpp>
#include <bitcoin/database/tables/schema.hpp>

namespace libbitcoin {
namespace database {
namespace table {

/// validated_tx is a slab hashmap of tx validation state, keyed by tx link.
/// Each input's parent tx link is merged with its coinbase flag (as prevout).
struct validated_tx
  : public hash_map<schema::validated_tx>
{
    using tx = schema::transaction::link;
    using sigop = linkage<schema::sigops>;
    using hash_map<schema::validated_tx>::hashmap;

    struct slab
      : public schema::validated_tx
    {
        inline link count() const NOEXCEPT
        {
            using namespace system;
            return possible_narrow_cast<link::integer>(pk + sk +
                context::size +
                variable_size(fee) +
                variable_size(sigops) +
                prevouts.size() * tx::size);
        }

        inline bool from_data(reader& source) NOEXCEPT
        {
            using namespace system;
            context::from_data(source, ctx);
            fee = source.read_variable();
            sigops = possible_narrow_cast<sigop::integer>(source.read_variable());
            std::ranges::for_each(prevouts, [&](auto& value) NOEXCEPT
            {
                value = source.read_little_endian<tx::integer, tx::size>();
            });

            BC_ASSERT(!source || source.get_read_position() == count());
            return source;
        }

        inline bool to_data(finalizer& sink) const NOEXCEPT
        {
            context::to_data(sink, ctx);
            sink.write_variable(fee);
            sink.write_variable(sigops);
            std::ranges::for_each(prevouts, [&](const auto& value) NOEXCEPT
            {
                sink.write_little_endian<tx::integer, tx::size>(value);
            });

            BC_ASSERT(!sink || sink.get_write_position() == count());
            return sink;
        }

        inline bool operator==(const slab&) const NOEXCEPT = default;

        context ctx{};
        uint64_t fee{};
        sigop::integer sigops{};
        std::vector<tx::integer> prevouts{};
    };
};

} // namespace table
} // namespace database
} // namespace libbitcoin

#endif
