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
#ifndef LIBBITCOIN_DATABASE_TABLES_OPTIONALS_SILENT_HPP
#define LIBBITCOIN_DATABASE_TABLES_OPTIONALS_SILENT_HPP

#include <bitcoin/database/define.hpp>
#include <bitcoin/database/primitives/primitives.hpp>
#include <bitcoin/database/tables/schema.hpp>
#include <bitcoin/database/types/silent.hpp>

namespace libbitcoin {
namespace database {
namespace table {

/// silent is a slab of silent payment scan records indexed by block link.
struct silent
  : public array_map<schema::silent>
{
    using array_map<schema::silent>::arraymap;

    using tx = transaction::link;
    using ix = transaction::ix;

    static link serialized_size(const database::silent& value) NOEXCEPT
    {
        auto size = variable_size(value.records.size());

        for (const auto& record: value.records)
        {
            const auto outputs = record.outputs.size();
            size += tx::size + system::ec_compressed_size +
                variable_size(outputs);
            size += outputs * (ix::size + system::ec_xonly_size);
        }

        return system::possible_narrow_cast<link::integer>(size);
    }

    struct get_records
      : public schema::silent
    {
        inline link count() const NOEXCEPT
        {
            return serialized_size(value);
        }

        inline bool from_data(reader& source) NOEXCEPT
        {
            value.records.resize(source.read_variable());

            for (auto& record: value.records)
            {
                const auto tx_value =
                    source.read_little_endian<tx::integer, tx::size>();
                record.tx = tx{ tx_value };
                record.prevouts_summary =
                    source.read_forward<system::ec_compressed_size>();

                record.outputs.resize(source.read_variable());

                for (auto& output: record.outputs)
                {
                    output.index = source.read_little_endian<ix::integer, ix::size>();
                    output.key = source.read_forward<system::ec_xonly_size>();
                }
            }

            BC_ASSERT(!source || source.get_read_position() == count());
            return source;
        }

        database::silent value{};
    };

    struct put_ref
      : public schema::silent
    {
        inline link count() const NOEXCEPT
        {
            return serialized_size(value);
        }

        inline bool to_data(finalizer& sink) const NOEXCEPT
        {
            sink.write_variable(value.records.size());

            for (const auto& record: value.records)
            {
                sink.write_little_endian<tx::integer, tx::size>(
                    record.tx.value);
                sink.write_bytes(record.prevouts_summary);
                sink.write_variable(record.outputs.size());

                for (const auto& output: record.outputs)
                {
                    const auto index =
                        system::possible_narrow_cast<ix::integer>(
                            output.index);
                    sink.write_little_endian<ix::integer, ix::size>(index);
                    sink.write_bytes(output.key);
                }
            }

            BC_ASSERT(!sink || sink.get_write_position() == count());
            return sink;
        }

        const database::silent& value;
    };
};

} // namespace table
} // namespace database
} // namespace libbitcoin

#endif
