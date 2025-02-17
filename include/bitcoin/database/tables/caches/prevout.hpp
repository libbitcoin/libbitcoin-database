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

#include <utility>
#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/primitives/primitives.hpp>
#include <bitcoin/database/tables/schema.hpp>

namespace libbitcoin {
namespace database {
namespace table {

/// prevout is an array map index of previous outputs by block link.
/// The coinbase flag is merged into the tx field, reducing it's domain.
/// Masking is from the right in order to accomodate integral tx domain.
struct prevout
  : public array_map<schema::prevout>
{
    using tx = linkage<schema::tx>;
    using header = linkage<schema::block>;
    using array_map<schema::prevout>::arraymap;
    static constexpr size_t offset = sub1(to_bits(tx::size));

    // The below implementation overloads tx-sized record count with sequences.
    static_assert(tx::size == sizeof(uint32_t), "sequence-tx overload error");

    // This supports only a single record (not too useful).
    struct slab
      : public schema::prevout
    {
        inline link count() const NOEXCEPT
        {
            return tx::size;
        }

        inline bool coinbase() const NOEXCEPT
        {
            return system::get_right(prevout_tx, offset);
        }

        inline tx::integer output_tx_fk() const NOEXCEPT
        {
            return system::set_right(prevout_tx, offset, false);
        }

        inline void set(bool coinbase, tx::integer output_tx_fk) NOEXCEPT
        {
            using namespace system;
            BC_ASSERT_MSG(!get_right(output_tx_fk, offset), "overflow");
            prevout_tx = set_right(output_tx_fk, offset, coinbase);
        }

        inline bool from_data(reader& source) NOEXCEPT
        {
            prevout_tx = source.read_little_endian<tx::integer, tx::size>();
            BC_ASSERT(!source || source.get_read_position() == count() * minrow);
            return source;
        }

        inline bool to_data(finalizer& sink) const NOEXCEPT
        {
            sink.write_little_endian<tx::integer, tx::size>(prevout_tx);
            BC_ASSERT(!sink || sink.get_write_position() == count() * minrow);
            return sink;
        }

        inline bool operator==(const slab& other) const NOEXCEPT
        {
            return coinbase() == other.coinbase()
                && output_tx_fk() == other.output_tx_fk();
        }

        tx::integer prevout_tx{};
    };

    struct slab_put_ref
      : public schema::prevout
    {
        inline link count() const NOEXCEPT
        {
            // TODO: assert overflow.
            using namespace system;
            const auto conflicts_ = conflicts.size();
            return variable_size(conflicts_) + (conflicts_ * tx::size) +
                (block.spends() * (tx::size + sizeof(uint32_t)));
        }

        static constexpr tx::integer merge(bool coinbase,
            tx::integer output_tx_fk) NOEXCEPT
        {
            using namespace system;
            BC_ASSERT_MSG(!get_right(output_tx_fk, offset), "overflow");
            return set_right(output_tx_fk, offset, coinbase);
        }

        inline bool to_data(finalizer& sink) const NOEXCEPT
        {
            const auto write_con = [&](const auto& con) NOEXCEPT
            {
                sink.write_little_endian<tx::integer, tx::size>(con);
            };

            const auto write_tx = [&](const auto& tx) NOEXCEPT
            {
                const auto& ins = tx->inputs_ptr();
                return std::for_each(ins->begin(), ins->end(),
                    [&](const auto& in) NOEXCEPT
                    {
                        // Sets terminal sentinel for block-internal spends.
                        const auto value = in->metadata.inside ? tx::terminal :
                            merge(in->metadata.coinbase, in->metadata.parent);

                        sink.write_little_endian<tx::integer, tx::size>(value);
                        sink.write_little_endian<uint32_t>(in->sequence());
                    });
            };

            using namespace system;
            const auto& cons = conflicts;
            const auto& txs = *block.transactions_ptr();
            const auto number = possible_narrow_cast<tx::integer>(cons.size());
            BC_ASSERT_MSG(txs.size() > one, "empty block");

            // Count is written as a tx link so the table can remain an array.
            sink.write_variable(number);
            std::for_each(cons.begin(), cons.end(), write_con);
            std::for_each(std::next(txs.begin()), txs.end(), write_tx);

            BC_ASSERT(!sink || (sink.get_write_position() == count() * minrow));
            return sink;
        }

        const std::vector<tx::integer>& conflicts{};
        const system::chain::block& block{};
    };

    struct slab_get
      : public schema::prevout
    {
        inline link count() const NOEXCEPT
        {
            // TODO: assert overflow.
            using namespace system;
            const auto conflicts_ = conflicts.size();
            return variable_size(conflicts_) + (conflicts_ * tx::size) +
                (spends.size() * (tx::size + sizeof(uint32_t)));
        }

        inline bool from_data(reader& source) NOEXCEPT
        {
            auto& cons = conflicts;
            cons.resize(source.read_variable());
            std::for_each(cons.begin(), cons.end(), [&](auto& value) NOEXCEPT
            {
                value = source.read_little_endian<tx::integer, tx::size>();
            });

            std::for_each(spends.begin(), spends.end(), [&](auto& value) NOEXCEPT
            {
                value.first = source.read_little_endian<tx::integer, tx::size>();
                value.second = source.read_little_endian<uint32_t>();
            });

            BC_ASSERT(!source || source.get_read_position() == count() * minrow);
            return source;
        }

        static inline bool coinbase(tx::integer spend) NOEXCEPT
        {
            // Inside are always reflected as coinbase.
            return system::get_right(spend, offset);
        }

        static inline tx::integer output_tx_fk(tx::integer spend) NOEXCEPT
        {
            // Inside spends are mapped to terminal.
            using namespace system;
            return spend == tx::terminal ? spend : set_right(spend, offset, false);
        }

        // Spend count is derived in confirmation from block.txs.puts.
        std::vector<tx::integer> conflicts{};
        std::vector<std::pair<tx::integer, uint32_t>> spends{};
    };
};

} // namespace table
} // namespace database
} // namespace libbitcoin

#endif
