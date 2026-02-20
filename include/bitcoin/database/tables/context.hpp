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
#ifndef LIBBITCOIN_DATABASE_CONTEXT_HPP
#define LIBBITCOIN_DATABASE_CONTEXT_HPP

#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/memory.hpp>
#include <bitcoin/database/tables/schema.hpp>

namespace libbitcoin {
namespace database {

struct context
{
    using height_t = linkage<schema::height_>;
    using flag_t = linkage<schema::flags>;
    using mtp_t = uint32_t;
    static constexpr auto size = flag_t::size + height_t::size + sizeof(mtp_t);

    static inline void from_data(reader& source, context& context) NOEXCEPT
    {
        context.flags  = source.template read_little_endian<flag_t::integer, flag_t::size>();
        context.height = source.template read_little_endian<height_t::integer, height_t::size>();
        context.mtp    = source.template read_little_endian<uint32_t>();
    };

    static inline void to_data(finalizer& sink, const context& context) NOEXCEPT
    {
        sink.template write_little_endian<flag_t::integer, flag_t::size>(context.flags);
        sink.template write_little_endian<height_t::integer, height_t::size>(context.height);
        sink.template write_little_endian<mtp_t>(context.mtp);
    };

    constexpr bool is_enabled(system::chain::flags rule) const NOEXCEPT
    {
        return system::chain::script::is_enabled(flags, rule);
    }

    inline bool operator==(const context& other) const NOEXCEPT
    {
        return flags  == other.flags
            && height == other.height
            && mtp    == other.mtp;
    }

    height_t::integer flags{};
    flag_t::integer height{};
    mtp_t mtp{};
};

} // namespace database
} // namespace libbitcoin

#endif
