/**
/// Copyright (c) 2011-2022 libbitcoin developers (see AUTHORS)
 *
/// This file is part of libbitcoin.
 *
/// This program is free software: you can redistribute it and/or modify
/// it under the terms of the GNU Affero General Public License as published by
/// the Free Software Foundation, either version 3 of the License, or
/// (at your option) any later version.
 *
/// This program is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
/// GNU Affero General Public License for more details.
 *
/// You should have received a copy of the GNU Affero General Public License
/// along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef LIBBITCOIN_DATABASE_TABLES_HASH_TABLE_HPP
#define LIBBITCOIN_DATABASE_TABLES_HASH_TABLE_HPP

#include <bitcoin/system.hpp>
#include <bitcoin/database/boost.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/tables/hash_table_header.hpp>
#include <bitcoin/database/memory/interfaces/memory.hpp>
#include <bitcoin/database/memory/interfaces/storage.hpp>
#include <bitcoin/database/memory/map_sink.hpp>
#include <bitcoin/database/memory/map_source.hpp>

namespace libbitcoin {
namespace database {

template <typename Element>
class hash_table
{
public:
    using link = typename Element::link;
    using key = typename Element::key;

    hash_table(storage& header, storage& body, const link& buckets) NOEXCEPT;

    /// Not thread safe.
    /// -----------------------------------------------------------------------

    /// Create from empty body/header files (no need to verify).
    bool create() NOEXCEPT;

    /// False if header or body file size incorrect.
    bool verify() const NOEXCEPT;

    /// Thread safe.
    /// -----------------------------------------------------------------------

    /// Reader positioned at key.
    reader_ptr at(const link& record) const NOEXCEPT;

    /// Reader positioned at data.
    reader_ptr find(const key& key) const NOEXCEPT;

    /// Reader positioned at data, size must be one/default for records.
    writer_ptr push(const key& key, const link& size=one) NOEXCEPT;

private:
    static constexpr auto link_size = link::size;
    static constexpr auto key_size = array_count<key>;
    static constexpr auto record_size = Element::size;
    static constexpr auto slab = is_zero(record_size);

    using header = hash_table_header<link, key>;
    using manage = manager<link, record_size>;

    // hash/head/push thread safe.
    header header_;

    // Thread safe.
    manage body_;
};

} // namespace database
} // namespace libbitcoin

#define TEMPLATE \
template <typename Element>
#define CLASS hash_table<Element>

#include <bitcoin/database/impl/tables/hash_table.ipp>

#undef CLASS
#undef TEMPLATE

#endif
