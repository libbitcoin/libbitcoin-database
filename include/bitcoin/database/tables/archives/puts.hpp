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
#ifndef LIBBITCOIN_DATABASE_TABLES_ARCHIVES_PUTS_HPP
#define LIBBITCOIN_DATABASE_TABLES_ARCHIVES_PUTS_HPP

#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/memory.hpp>
#include <bitcoin/database/primitives/primitives.hpp>
#include <bitcoin/database/tables/schema.hpp>

namespace libbitcoin {
namespace database {
namespace puts {

BC_PUSH_WARNING(NO_METHOD_HIDING)

// Puts is an array of input or output fk records.
// Multiple may be allocated, put_fks.size() (from tx) determines read extent.

struct record
{
    // Sizes.
    static constexpr size_t pk = schema::puts;
    ////static constexpr size_t sk = zero;
    static constexpr size_t minsize = schema::put;
    static constexpr size_t minrow = minsize;
    static constexpr size_t size = minsize;
    static_assert(minsize == 5u);
    static_assert(minrow == 5u);

    linkage<pk> count() const NOEXCEPT
    {
        using namespace system;
        using out = typename linkage<pk>::integer;
        BC_ASSERT(put_fks.size() < power2<uint64_t>(to_bits(schema::put)));
        return possible_narrow_cast<out>(put_fks.size());
    }

    // Fields.
    std_vector<uint64_t> put_fks;
    bool valid{ false };

    // Serialializers.

    inline record from_data(reader& source) NOEXCEPT
    {
        std::for_each(put_fks.begin(), put_fks.end(), [&](auto& fk) NOEXCEPT
        {
            fk = source.read_5_bytes_little_endian();
        });

        BC_ASSERT(source.get_position() == minrow);
        valid = source;
        return *this;
    }

    inline bool to_data(finalizer& sink) const NOEXCEPT
    {
        std::for_each(put_fks.begin(), put_fks.end(), [&](const auto& fk) NOEXCEPT
        {
            sink.write_5_bytes_little_endian(fk);
        });

        BC_ASSERT(sink.get_position() == minrow);
        return sink;
    }
};

/// puts::table
class BCD_API table
  : public array_map<record>
{
public:
    using array_map<record>::arraymap;
};

BC_POP_WARNING()

} // namespace puts
} // namespace database
} // namespace libbitcoin

#endif
