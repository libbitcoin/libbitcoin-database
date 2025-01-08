/**
 * Copyright (c) 2011-2025 libbitcoin developers (see AUTHORS)
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
#ifndef LIBBITCOIN_DATABASE_TABLES_OPTIONALS_BUFFER_HPP
#define LIBBITCOIN_DATABASE_TABLES_OPTIONALS_BUFFER_HPP

#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/primitives/primitives.hpp>
#include <bitcoin/database/tables/schema.hpp>

namespace libbitcoin {
namespace database {
namespace table {

/// prevout is an array map index of previous outputs by block.
struct prevout
  : public array_map<schema::prevout>
{
    using tx = linkage<schema::tx>;
    using spend = linkage<schema::spend_>;
    using array_map<schema::prevout>::arraymap;

    struct record
      : public schema::prevout
    {
        inline bool from_data(reader& source) NOEXCEPT
        {
            coinbase = source.read_byte();
            spend_fk = source.read_little_endian<spend::integer, spend::size>();
            output_tx_fk = source.read_little_endian<tx::integer, tx::size>();
            BC_ASSERT(source.get_read_position() == count());
            return source;
        }

        inline bool to_data(finalizer& sink) const NOEXCEPT
        {
            sink.write_byte(coinbase);
            sink.write_little_endian<spend::integer, spend::size>(spend_fk);
            sink.write_little_endian<tx::integer, tx::size>(output_tx_fk);
            BC_ASSERT(sink.get_write_position() == count());
            return sink;
        }

        inline bool operator==(const record& other) const NOEXCEPT
        {
            return coinbase == other.coinbase
                && spend_fk == other.spend_fk
                && output_tx_fk == other.output_tx_fk;
        }

        bool coinbase{};
        spend::integer spend_fk{};
        tx::integer output_tx_fk{};
    };
};

} // namespace table
} // namespace database
} // namespace libbitcoin

#endif
