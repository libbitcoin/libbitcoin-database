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
#ifndef LIBBITCOIN_DATABASE_TABLES_ARCHIVES_TXS_HPP
#define LIBBITCOIN_DATABASE_TABLES_ARCHIVES_TXS_HPP

#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/memory.hpp>
#include <bitcoin/database/primitives/primitives.hpp>
#include <bitcoin/database/tables/schema.hpp>

namespace libbitcoin {
namespace database {
namespace txs {

BC_PUSH_WARNING(NO_METHOD_HIDING)

// Txs is a slab hashmap of tx fks (first is count), searchable by header.fk.

struct slab
{
    // Sizes.
    static constexpr size_t pk = schema::txs;
    static constexpr size_t sk = schema::block;
    static constexpr size_t minsize = zero;
    static constexpr size_t minrow = pk + sk + minsize;
    static constexpr size_t size = max_size_t;
    static_assert(minsize == 0u);
    static_assert(minrow == 7u);

    linkage<pk> count() const NOEXCEPT
    {
        using namespace system;
        using out = typename linkage<pk>::integer;
        return possible_narrow_cast<out>(pk + sk +
            schema::tx * add1(tx_fks.size()));
    }

    // Fields.
    std_vector<uint32_t> tx_fks;
    bool valid{ false };

    // Serialializers.

    inline slab from_data(reader& source) NOEXCEPT
    {
        tx_fks.resize(source.read_4_bytes_little_endian());
        std::for_each(tx_fks.begin(), tx_fks.end(), [&](auto& fk) NOEXCEPT
        {
            fk = source.read_4_bytes_little_endian();
        });

        BC_ASSERT(source.get_position() == count());
        valid = source;
        return *this;
    }

    inline bool to_data(finalizer& sink) const NOEXCEPT
    {
        using namespace system;
        BC_ASSERT(tx_fks.size() < power2<uint64_t>(to_bits(schema::tx)));
        const auto count = possible_narrow_cast<uint32_t>(tx_fks.size());

        sink.write_4_bytes_little_endian(count);
        std::for_each(tx_fks.begin(), tx_fks.end(), [&](const auto& fk) NOEXCEPT
        {
            sink.write_4_bytes_little_endian(fk);
        });

        BC_ASSERT(sink.get_position() == this->count());
        return sink;
    }
};

/// txs::table
class BCD_API table
  : public hash_map<slab>
{
public:
    using hash_map<slab>::hashmap;
};

BC_POP_WARNING()

} // namespace txs
} // namespace database
} // namespace libbitcoin

#endif
