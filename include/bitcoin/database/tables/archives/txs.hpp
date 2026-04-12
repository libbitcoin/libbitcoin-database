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
#ifndef LIBBITCOIN_DATABASE_TABLES_ARCHIVES_TXS_HPP
#define LIBBITCOIN_DATABASE_TABLES_ARCHIVES_TXS_HPP

#include <algorithm>
#include <iterator>
#include <optional>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/memory.hpp>
#include <bitcoin/database/primitives/primitives.hpp>
#include <bitcoin/database/tables/schema.hpp>

namespace libbitcoin {
namespace database {
namespace table {

// TODO: interval could move to count and sizes made variable.
// TODO: fks can instead be stored as a count and coinbase fk,
// TODO: but will need to be disambiguated from compact blocks.

/// Txs is a slab arraymap of tx fks (first is count), indexed by header.fk.
struct txs
  : public array_map<schema::txs>
{
    using ct = linkage<schema::count_>;
    using tx = schema::transaction::link;
    using keys = std::vector<tx::integer>;
    using bytes = linkage<schema::size, sub1(to_bits(schema::size))>;
    using hash = std::optional<hash_digest>;
    using array_map<schema::txs>::arraymap;
    static constexpr auto offset = bytes::bits;
    static_assert(offset < to_bits(bytes::size));

    static constexpr size_t skip_sizes =
        bytes::size +
        bytes::size;

    static constexpr bytes::integer merge(bool is_interval,
        bytes::integer light) NOEXCEPT
    {
        BC_ASSERT_MSG(!system::get_right(light, offset), "overflow");
        return system::set_right(light, offset, is_interval);
    }

    static constexpr bool is_interval(bytes::integer merged) NOEXCEPT
    {
        return system::get_right(merged, offset);
    }

    static constexpr bytes::integer to_light(bytes::integer merged) NOEXCEPT
    {
        return system::set_right(merged, offset, false);
    }

    // Intervals are optional based on store configuration.
    struct slab
      : public schema::txs
    {
        inline bool is_genesis() const NOEXCEPT
        {
            return !tx_fks.empty() && is_zero(tx_fks.front());
        }

        inline link count() const NOEXCEPT
        {
            return system::possible_narrow_cast<link::integer>(
                skip_sizes + ct::size + (tx_fks.size() * tx::size) +
                (interval.has_value() ? schema::hash : zero) +
                to_int(is_genesis()));
        }

        inline bool from_data(reader& source) NOEXCEPT
        {
            // tx sizes
            const auto merged = source.read_little_endian<bytes::integer, bytes::size>();
            heavy = source.read_little_endian<bytes::integer, bytes::size>();
            light = to_light(merged);

            // tx fks
            tx_fks.resize(source.read_little_endian<ct::integer, ct::size>());
            std::for_each(tx_fks.begin(), tx_fks.end(), [&](auto& fk) NOEXCEPT
            {
                fk = source.read_little_endian<tx::integer, tx::size>();
            });

            // interval (when specified)
            interval.reset();
            if (is_interval(merged)) interval = source.read_hash();

            // depth (genesis only)
            depth = is_genesis() ? source.read_byte() : zero;
            BC_ASSERT(!source || source.get_read_position() == count());
            return source;
        }

        inline bool to_data(finalizer& sink) const NOEXCEPT
        {
            using namespace system;
            BC_ASSERT(tx_fks.size() < power2<uint64_t>(to_bits(ct::size)));

            // tx sizes
            const auto merged = merge(interval.has_value(), light);
            sink.write_little_endian<bytes::integer, bytes::size>(merged);
            sink.write_little_endian<bytes::integer, bytes::size>(heavy);

            // tx fks
            const auto number = possible_narrow_cast<ct::integer>(tx_fks.size());
            sink.write_little_endian<ct::integer, ct::size>(number);
            std::for_each(tx_fks.cbegin(), tx_fks.cend(),
                [&](const auto& fk) NOEXCEPT
                {
                    sink.write_little_endian<tx::integer, tx::size>(fk);
                });

            // interval (when specified)
            if (interval.has_value()) sink.write_bytes(interval.value());

            // depth (genesis only)
            if (is_genesis()) sink.write_byte(depth);
            BC_ASSERT(!sink || sink.get_write_position() == count());
            return sink;
        }

        inline bool operator==(const slab& other) const NOEXCEPT
        {
            return light == other.light
                && heavy == other.heavy
                && tx_fks == other.tx_fks
                && interval == other.interval
                && depth == other.depth;
        }

        bytes::integer light{};
        bytes::integer heavy{};
        keys tx_fks{};
        hash interval{};
        uint8_t depth{};
    };

    // put a contiguous set of tx identifiers.
    struct put_group
      : public schema::txs
    {
        inline bool is_genesis() const NOEXCEPT
        {
            return is_zero(tx_fk);
        }

        inline link count() const NOEXCEPT
        {
            return system::possible_narrow_cast<link::integer>(
                skip_sizes + ct::size + (number * tx::size) +
                (interval.has_value() ? schema::hash : zero) +
                to_int(is_zero(tx_fk)));
        }

        inline bool to_data(finalizer& sink) const NOEXCEPT
        {
            // tx sizes
            const auto merged = merge(interval.has_value(), light);
            sink.write_little_endian<bytes::integer, bytes::size>(merged);
            sink.write_little_endian<bytes::integer, bytes::size>(heavy);

            // tx fks
            sink.write_little_endian<ct::integer, ct::size>(number);
            for (auto fk = tx_fk; fk < (tx_fk + number); ++fk)
                sink.write_little_endian<tx::integer, tx::size>(fk);

            // interval (when specified)
            if (interval.has_value()) sink.write_bytes(interval.value());

            // depth (genesis only)
            if (is_genesis()) sink.write_byte(depth);
            BC_ASSERT(!sink || sink.get_write_position() == count());
            return sink;
        }

        bytes::integer light{};
        bytes::integer heavy{};
        ct::integer number{};
        tx::integer tx_fk{};
        hash interval{};
        uint8_t depth{};
    };

    struct get_interval
      : public schema::txs
    {
        inline link count() const NOEXCEPT
        {
            BC_ASSERT(false);
            return {};
        }

        inline bool from_data(reader& source) NOEXCEPT
        {
            // tx sizes
            const auto merged = source.read_little_endian<bytes::integer, bytes::size>();
            source.skip_bytes(bytes::size);

            // tx fks
            const auto number = source.read_little_endian<ct::integer, ct::size>();
            source.skip_bytes(number * tx::size);

            // interval
            interval.reset();
            if (is_interval(merged)) interval = source.read_hash();
            return source;
        }

        hash interval{};
    };

    // This reader is only applicable to the genesis block.
    struct get_genesis_depth
      : public schema::txs
    {
        inline link count() const NOEXCEPT
        {
            BC_ASSERT(false);
            return {};
        }

        // Stored at end since only read once (at startup).
        inline bool from_data(reader& source) NOEXCEPT
        {
            // tx sizes
            const auto merged = source.read_little_endian<bytes::integer, bytes::size>();
            source.skip_bytes(bytes::size);

            // tx fks
            const auto number = source.read_little_endian<ct::integer, ct::size>();
            source.skip_bytes(number * tx::size);

            // interval
            source.skip_bytes(is_interval(merged) ? schema::hash : zero);

            // depth
            depth = source.read_byte();
            return source;
        }

        uint8_t depth{};
    };

    struct get_position
      : public schema::txs
    {
        inline link count() const NOEXCEPT
        {
            BC_ASSERT(false);
            return {};
        }

        inline bool from_data(reader& source) NOEXCEPT
        {
            // tx sizes
            source.skip_bytes(skip_sizes);

            // tx fks
            const auto number = source.read_little_endian<ct::integer, ct::size>();
            for (position = zero; position < number; ++position)
                if (source.read_little_endian<tx::integer, tx::size>() == tx_fk)
                    return source;

            source.invalidate();
            return source;
        }

        const tx::integer tx_fk{};
        size_t position{};
    };

    struct get_at_position
      : public schema::txs
    {
        inline link count() const NOEXCEPT
        {
            BC_ASSERT(false);
            return {};
        }

        inline bool from_data(reader& source) NOEXCEPT
        {
            // tx sizes
            source.skip_bytes(skip_sizes);

            // tx fks
            const auto number = source.read_little_endian<ct::integer, ct::size>();
            if (position < number)
            {
                source.skip_bytes(position * tx::size);
                tx_fk = source.read_little_endian<tx::integer, tx::size>();
                return source;
            }

            source.invalidate();
            return source;
        }

        const size_t position{};
        tx::integer tx_fk{};
    };

    struct get_coinbase
      : public schema::txs
    {
        inline link count() const NOEXCEPT
        {
            BC_ASSERT(false);
            return {};
        }

        inline bool from_data(reader& source) NOEXCEPT
        {
            // tx sizes
            source.skip_bytes(skip_sizes);

            // tx fks
            if (is_nonzero(source.read_little_endian<ct::integer, ct::size>()))
            {
                coinbase_fk = source.read_little_endian<tx::integer, tx::size>();
                return source;
            }

            source.invalidate();
            return source;
        }

        tx::integer coinbase_fk{};
    };

    struct get_tx
      : public schema::txs
    {
        inline link count() const NOEXCEPT
        {
            BC_ASSERT(false);
            return {};
        }

        inline bool from_data(reader& source) NOEXCEPT
        {
            // tx sizes
            source.skip_bytes(skip_sizes);

            // tx fks
            if (source.read_little_endian<ct::integer, ct::size>() > position)
            {
                source.skip_bytes(position * tx::size);
                tx_fk = source.read_little_endian<tx::integer, tx::size>();
                return source;
            }

            source.invalidate();
            return source;
        }

        const size_t position{};
        tx::integer tx_fk{};
    };

    struct get_sizes
      : public schema::txs
    {
        inline link count() const NOEXCEPT
        {
            BC_ASSERT(false);
            return {};
        }

        inline bool from_data(reader& source) NOEXCEPT
        {
            // tx sizes
            light = to_light(source.read_little_endian<bytes::integer, bytes::size>());
            heavy = source.read_little_endian<bytes::integer, bytes::size>();
            return source;
        }

        bytes::integer light{};
        bytes::integer heavy{};
    };

    struct get_associated
      : public schema::txs
    {
        inline link count() const NOEXCEPT
        {
            BC_ASSERT(false);
            return {};
        }

        inline bool from_data(reader& source) NOEXCEPT
        {
            // tx sizes
            source.skip_bytes(skip_sizes);

            // tx fks
            associated = to_bool(source.read_little_endian<ct::integer, ct::size>());
            return source;
        }

        bool associated{};
    };

    struct get_txs
      : public schema::txs
    {
        inline link count() const NOEXCEPT
        {
            BC_ASSERT(false);
            return {};
        }

        inline bool from_data(reader& source) NOEXCEPT
        {
            // tx sizes
            source.skip_bytes(skip_sizes);

            // tx fks
            tx_fks.resize(source.read_little_endian<ct::integer, ct::size>());
            std::for_each(tx_fks.begin(), tx_fks.end(), [&](auto& fk) NOEXCEPT
            {
                fk = source.read_little_endian<tx::integer, tx::size>();
            });

            return source;
        }

        keys tx_fks{};
    };

    struct get_spending_txs
      : public schema::txs
    {
        inline link count() const NOEXCEPT
        {
            BC_ASSERT(false);
            return {};
        }

        inline bool from_data(reader& source) NOEXCEPT
        {
            // tx sizes
            source.skip_bytes(skip_sizes);

            // tx fks
            const auto number = source.read_little_endian<ct::integer, ct::size>();
            if (number > one)
            {
                source.skip_bytes(one * tx::size);
                tx_fks.resize(sub1(number));
                std::for_each(tx_fks.begin(), tx_fks.end(), [&](auto& fk) NOEXCEPT
                {
                    fk = source.read_little_endian<tx::integer, tx::size>();
                });
            }

            return source;
        }

        keys tx_fks{};
    };

    struct get_tx_count
      : public schema::txs
    {
        inline link count() const NOEXCEPT
        {
            BC_ASSERT(false);
            return {};
        }

        inline bool from_data(reader& source) NOEXCEPT
        {
            // tx sizes
            source.skip_bytes(skip_sizes);

            // tx fks
            number = source.read_little_endian<ct::integer, ct::size>();
            return source;
        }

        size_t number{};
    };

    struct get_coinbase_and_count
      : public schema::txs
    {
        inline link count() const NOEXCEPT
        {
            BC_ASSERT(false);
            return {};
        }

        inline bool from_data(reader& source) NOEXCEPT
        {
            // tx sizes
            source.skip_bytes(skip_sizes);

            // tx fks
            number = source.read_little_endian<ct::integer, ct::size>();
            if (is_nonzero(number))
            {
                coinbase_fk = source.read_little_endian<tx::integer, tx::size>();
                return source;
            }

            source.invalidate();
            return source;
        }

        size_t number{};
        tx::integer coinbase_fk{};
    };
};

} // namespace table
} // namespace database
} // namespace libbitcoin

#endif
