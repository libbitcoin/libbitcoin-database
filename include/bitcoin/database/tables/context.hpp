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
#ifndef LIBBITCOIN_DATABASE_CONTEXT_HPP
#define LIBBITCOIN_DATABASE_CONTEXT_HPP

#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/tables/schema.hpp>

// TODO: rationalize with chain::context.

namespace libbitcoin {
namespace database {

struct context
{
    static constexpr size_t size = schema::block + schema::flags +
        sizeof(uint32_t);

    template <typename Source>
    static inline void read(Source& source, context& context) NOEXCEPT
    {
        context.height = source.template read_little_endian<uint32_t, schema::block>();
        context.flags  = source.template read_little_endian<uint32_t, schema::flags>();
        context.mtp    = source.template read_little_endian<uint32_t>();
    };

    template <typename Sink>
    static inline void write(Sink& sink, const context& context) NOEXCEPT
    {
        sink.template write_little_endian<uint32_t, schema::block>(context.height);
        sink.template write_little_endian<uint32_t, schema::flags>(context.flags);
        sink.template write_little_endian<uint32_t>(context.mtp);
    };

    inline bool operator==(const context& other) const NOEXCEPT
    {
        return height == other.height
            && flags  == other.flags
            && mtp    == other.mtp;
    }

    uint32_t height{};
    uint32_t flags{};
    uint32_t mtp{};
};

} // namespace database
} // namespace libbitcoin

#endif
