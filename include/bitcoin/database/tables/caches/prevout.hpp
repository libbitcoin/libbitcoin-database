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
/// The coinbase flag is merged into the tx field, reducing it's domain.
/// Masking is from the right in order to accomodate non-integral domain.
struct prevout
  : public array_map<schema::prevout>
{
    using tx = linkage<schema::tx>;
    using array_map<schema::prevout>::arraymap;
    static constexpr size_t offset = sub1(to_bits(tx::size));

    struct record
      : public schema::prevout
    {
        inline bool coinbase() const NOEXCEPT
        {
            return system::get_right(value, offset);
        }

        inline tx::integer output_tx_fk() const NOEXCEPT
        {
            return system::set_right(value, offset, false);
        }

        inline void set(bool coinbase, tx::integer output_tx_fk) NOEXCEPT
        {
            using namespace system;
            BC_ASSERT_MSG(!get_right(output_tx_fk, offset), "overflow");
            value = set_right(output_tx_fk, offset, coinbase);
        }

        inline bool from_data(reader& source) NOEXCEPT
        {
            value = source.read_little_endian<tx::integer, tx::size>();
            BC_ASSERT(!source || source.get_read_position() == minrow);
            return source;
        }

        inline bool to_data(finalizer& sink) const NOEXCEPT
        {
            sink.write_little_endian<tx::integer, tx::size>(value);
            BC_ASSERT(!sink || sink.get_write_position() == minrow);
            return sink;
        }

        inline bool operator==(const record& other) const NOEXCEPT
        {
            return coinbase() == other.coinbase()
                && output_tx_fk() == other.output_tx_fk();
        }

        tx::integer value{};
    };

    struct record_put_ref
      : public schema::prevout
    {
        static constexpr tx::integer merge(bool coinbase,
            tx::integer output_tx_fk) NOEXCEPT
        {
            using namespace system;
            BC_ASSERT_MSG(!get_right(output_tx_fk, offset), "overflow");
            return system::set_right(output_tx_fk, offset, coinbase);
        }

        // This is called once by put(), and hides base count().
        inline link count() const NOEXCEPT
        {
            const auto spends = block.spends();
            BC_ASSERT(spends < link::terminal);
            return system::possible_narrow_cast<link::integer>(spends);
        }

        inline bool to_data(finalizer& sink) const NOEXCEPT
        {
            const auto txs = *block.transactions_ptr();
            if (txs.empty())
            {
                sink.invalidate();
            }
            else
            {
                const auto write_spend = [&](const auto& in) NOEXCEPT
                {
                    // Sets terminal sentinel for block-internal spends.
                    const auto value = in->metadata.inside ? tx::terminal :
                        merge(in->metadata.coinbase, in->metadata.parent);

                    sink.write_little_endian<tx::integer, tx::size>(value);
                };

                const auto write_tx = [&](const auto& tx) NOEXCEPT
                {
                    const auto& ins = tx->inputs_ptr();
                    return std::for_each(ins->begin(), ins->end(), write_spend);
                };

                std::for_each(std::next(txs.begin()), txs.end(), write_tx);
            }

            BC_ASSERT(!sink || (sink.get_write_position() == count() * minrow));
            return sink;
        }

        const system::chain::block& block{};
    };

    struct record_get
      : public schema::prevout
    {
        // This is called once by assert, and hides base class count().
        inline link count() const NOEXCEPT
        {
            BC_ASSERT(values.size() < link::terminal);
            return system::possible_narrow_cast<link::integer>(values.size());
        }

        inline bool from_data(reader& source) NOEXCEPT
        {
            // Values must be set to read size (i.e. using knowledge of spends).
            std::for_each(values.begin(), values.end(), [&](auto& value) NOEXCEPT
            {
                value = source.read_little_endian<tx::integer, tx::size>();
            });

            BC_ASSERT(!source || source.get_read_position() == count() * minrow);
            return source;
        }

        inline bool inside(size_t index) const NOEXCEPT
        {
            BC_ASSERT(index < count());

            // Identifies terminal sentinel as block-internal spend.
            return values.at(index) == tx::terminal;
        }

        inline bool coinbase(size_t index) const NOEXCEPT
        {
            BC_ASSERT(index < count());
            return system::get_right(values.at(index), offset);
        }

        inline tx::integer output_tx_fk(size_t index) const NOEXCEPT
        {
            BC_ASSERT(index < count());
            return system::set_right(values.at(index), offset, false);
        }

        // Spend count is derived in confirmation by summing block.txs.puts.
        std::vector<tx::integer> values{};
    };
};

} // namespace table
} // namespace database
} // namespace libbitcoin

#endif
