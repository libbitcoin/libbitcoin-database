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
#ifndef LIBBITCOIN_DATABASE_TABLES_ARCHIVES_OUTS_HPP
#define LIBBITCOIN_DATABASE_TABLES_ARCHIVES_OUTS_HPP

#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/memory.hpp>
#include <bitcoin/database/primitives/primitives.hpp>
#include <bitcoin/database/tables/schema.hpp>

namespace libbitcoin {
namespace database {
namespace table {

/// Address spine (column zero): conflict link, no stored value or key.
struct outs_address
{
    using link = schema::address::link;
    static constexpr auto width = schema::address::minrow;
    static constexpr auto suffix = "address"_t;

    struct record
      : public schema::address
    {
        inline bool from_data(reader& source) NOEXCEPT
        {
            BC_ASSERT(!source || is_zero(source.get_read_position()));
            return source;
        }

        inline bool to_data(flipper& sink) const NOEXCEPT
        {
            BC_ASSERT(!sink || is_zero(sink.get_write_position()));
            return sink;
        }
    };
};

/// Outs column: output fk records (rows aligned with the address spine).
struct outs_puts
{
    using ix = linkage<schema::index>;
    using out = schema::output::link;
    using tx = schema::transaction::link;
    using link = schema::outs::link;
    using output_links = std::vector<out::integer>;
    using excluded_outputs = std::vector<std::pair<ix::integer, out::integer>>;
    static constexpr auto width = schema::outs::size;
    static constexpr auto suffix = "puts"_t;
    static constexpr auto offset = out::bits;
    static_assert(offset < to_bits(out::size));

    static constexpr out::integer merge(bool excluded,
        out::integer out_fk) NOEXCEPT
    {
        BC_ASSERT_MSG(!system::get_right(out_fk, offset), "overflow");
        return system::set_right(out_fk, offset, excluded);
    }

    static constexpr out::integer to_out_fk(out::integer merged) NOEXCEPT
    {
        return system::set_right(merged, offset, false);
    }

    static constexpr bool is_excluded(out::integer merged) NOEXCEPT
    {
        return system::get_right(merged, offset);
    }

    static inline bool is_excluded(const system::chain::output& out) NOEXCEPT
    {
        using namespace system;
        const auto& script = out.script();
        const auto size = script.serialized_size(false);
        return (size > chain::max_script_size) || (!is_zero(size) &&
            (script.ops().front().code() == chain::opcode::op_return));
    }

    static inline bool peek_excluded(auto& source, size_t bytes) NOEXCEPT
    {
        using namespace system;
        constexpr auto op_return = to_value(chain::opcode::op_return);
        return (bytes > chain::max_script_size) ||
            (!is_zero(bytes) && (source.peek_byte() == op_return));
    }

    struct record
      : public schema::outs
    {
        inline link count() const NOEXCEPT
        {
            return system::possible_narrow_cast<link::integer>(out_fks.size());
        }

        inline bool from_data(reader& source) NOEXCEPT
        {
            std::ranges::for_each(out_fks, [&](auto& fk) NOEXCEPT
            {
                fk = to_out_fk(source.read_little_endian<out::integer, out::size>());
            });

            BC_ASSERT(!source || source.get_read_position() == count() * out::size);
            return source;
        }

        inline bool to_data(flipper& sink) const NOEXCEPT
        {
            std::ranges::for_each(out_fks, [&](const auto& fk) NOEXCEPT
            {
                sink.write_little_endian<out::integer, out::size>(fk);
            });

            BC_ASSERT(!sink || sink.get_write_position() == count() * minrow);
            return sink;
        }

        inline bool operator==(const record&) const NOEXCEPT = default;

        output_links out_fks{};
    };

    struct get_output
      : public schema::outs
    {
        inline link count() const NOEXCEPT
        {
            return 1;
        }

        inline bool from_data(reader& source) NOEXCEPT
        {
            out_fk = to_out_fk(source.read_little_endian<out::integer, out::size>());
            return source;
        }

        out::integer out_fk{};
    };

    struct get_excluded
      : public schema::outs
    {
        inline link count() const NOEXCEPT
        {
            return system::possible_narrow_cast<link::integer>(number);
        }

        inline bool from_data(reader& source) NOEXCEPT
        {
            excluded.clear();
            for (ix::integer index{}; index < number; ++index)
            {
                const auto merged = source.read_little_endian<out::integer,
                    out::size>();

                if (is_excluded(merged))
                    excluded.emplace_back(index, to_out_fk(merged));
            }

            return source;
        }

        // Output count is assigned by the caller, from the tx record.
        ix::integer number{};
        excluded_outputs excluded{};
    };

    struct put_ref
      : public schema::outs
    {
        inline link count() const NOEXCEPT
        {
            return system::possible_narrow_cast<link::integer>(tx_.outputs());
        }

        inline bool to_data(flipper& sink) const NOEXCEPT
        {
            using namespace system;
            static_assert(tx::size <= sizeof(uint64_t));
            constexpr auto value_parent = sizeof(uint64_t) - tx::size;

            auto out_fk = output_fk;
            const auto& outs = *tx_.outputs_ptr();
            std::ranges::for_each(outs, [&](const auto& out) NOEXCEPT
            {
                const auto merged = merge(is_excluded(*out), out_fk);
                sink.write_little_endian<out::integer, out::size>(merged);

                // Calculate next corresponding output fk from serialized size.
                // (variable_size(value) + (value + script)) - (value - parent)
                out_fk += (variable_size(out->value()) +
                    out->serialized_size() - value_parent);
            });

            BC_ASSERT(!sink || sink.get_write_position() == count() * minrow);
            return sink;
        }

        const out::integer output_fk{};
        const system::chain::transaction& tx_{};
    };

    struct put_view
      : public schema::outs
    {
        inline link count() const NOEXCEPT
        {
            return system::possible_narrow_cast<link::integer>(tx_.outputs());
        }

        inline bool to_data(flipper& sink) const NOEXCEPT
        {
            using namespace system;
            auto out_fk = output_fk;
            auto stream = tx_.get_outputs_stream();
            read::bytes::fast source{ stream };
            for (size_t out{}; out < tx_.outputs(); ++out)
            {
                const auto value = source.read_8_bytes_little_endian();
                const auto bytes = source.read_size();
                const auto excluded = peek_excluded(source, bytes);
                source.skip_bytes(bytes);
                const auto merged = merge(excluded, out_fk);
                sink.write_little_endian<out::integer, out::size>(merged);
                out_fk += tx::size + variable_size(value)
                    + variable_size(bytes) + bytes;
            }

            BC_ASSERT(source);
            BC_ASSERT(!sink || sink.get_write_position() == count() * minrow);
            return sink && source;
        }

        const out::integer output_fk{};
        const system::chain::view::transaction& tx_;
    };
};

/// The outs association: address spine (searched by output script hash) with
/// the aligned output fk column, shared row count and allocation.
struct outs
  : public hash_maps<schema::address, outs_puts>
{
    using base = hash_maps<schema::address, outs_puts>;
    using base::hashmaps;

    using out = schema::output::link;
    using output_links = std::vector<out::integer>;

    /// Column element aliases.
    using record = outs_puts::record;
    using get_output = outs_puts::get_output;
    using get_excluded = outs_puts::get_excluded;
    using put_ref = outs_puts::put_ref;
    using put_view = outs_puts::put_view;
    using excluded_outputs = outs_puts::excluded_outputs;

    /// The output fk column (rows aligned with the address spine).
    column<base, one> puts{ *this };
};

/// Address spine (column zero) without backing (address indexing disabled).
struct outs_address_unbacked
{
    using link = schema::address::link;
    static constexpr size_t width = zero;
    static constexpr auto suffix = outs_address::suffix;
};

/// Aggregate (files).
template <template <size_t...> class Storage>
using outs_storage = mmaps<Storage, outs_address, outs_puts>;

/// Aggregate (files) without address spine backing (indexing disabled).
template <template <size_t...> class Storage>
using outs_storage_unbacked = mmaps<Storage, outs_address_unbacked, outs_puts>;

} // namespace table
} // namespace database
} // namespace libbitcoin

#endif
