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
#ifndef LIBBITCOIN_DATABASE_TABLES_HASH_TABLE_HEADER_HPP
#define LIBBITCOIN_DATABASE_TABLES_HASH_TABLE_HEADER_HPP

#include <bitcoin/system.hpp>
#include <bitcoin/database/boost.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/interfaces/memory.hpp>
#include <bitcoin/database/memory/interfaces/storage.hpp>

namespace libbitcoin {
namespace database {

/// The maximum "index" (buckets) is the max value of Link.
/// The maximum table body "size" is the max value of Link.
template <typename Link, typename Key>
class hash_table_header
{
public:
    hash_table_header(storage& header, Link buckets) NOEXCEPT;

    /// Not thread safe.
    bool create() NOEXCEPT;
    bool set_body_size(Link size) NOEXCEPT;
    bool get_body_size(Link& size) const NOEXCEPT;

    /// Thread safe.
    Link hash(const Key& key) const NOEXCEPT;
    Link head(Link index) const NOEXCEPT;
    Link head(const Key& key) const NOEXCEPT;
    bool push(Link current, Link& next, Link index) NOEXCEPT;
    bool push(Link current, Link& next, const Key& key) NOEXCEPT;

private:
    template <size_t Bytes>
    static auto array_cast(memory& buffer) NOEXCEPT
    {
        return system::unsafe_array_cast<uint8_t, Bytes>(buffer.begin());
    }

    static constexpr size_t offset(Link index) NOEXCEPT
    {
        // Byte offset of bucket index within header file.
        // [body_size][[bucket[0]...bucket[buckets-1]]]
        return sizeof(Link) + index * sizeof(Link);
    }

    storage& file_;
    const Link buckets_;
    mutable boost::upgrade_mutex mutex_;
};

} // namespace database
} // namespace libbitcoin


#define TEMPLATE template <typename Link, typename Key>
#define CLASS hash_table_header<Link, Key>

#include <bitcoin/database/impl/tables/hash_table_header.ipp>

#undef CLASS
#undef TEMPLATE

#endif
