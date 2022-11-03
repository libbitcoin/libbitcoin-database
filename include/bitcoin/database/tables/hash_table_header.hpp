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

template <typename Link, typename Key>
class hash_table_header
{
public:
    hash_table_header(storage& header, const Link& buckets) NOEXCEPT;

    /// Not thread safe.
    /// -----------------------------------------------------------------------

    /// Create from empty header file (no need to verify).
    bool create() NOEXCEPT;

    /// False if header file size incorrect.
    bool verify() const NOEXCEPT;

    /// Unsafe if not verified.
    Link get_body_count() const NOEXCEPT;
    void set_body_count(const Link& count) NOEXCEPT;

    /// Thread safe.
    /// -----------------------------------------------------------------------

    /// Convert natural key to header bucket index.
    Link index(const Key& key) const NOEXCEPT;

    /// Unsafe if not verified.
    Link head(const Key& key) const NOEXCEPT;
    Link head(const Link& index) const NOEXCEPT;
    void push(const Link& current, Link& next, const Key& key) NOEXCEPT;
    void push(const Link& current, Link& next, const Link& index) NOEXCEPT;

private:
    template <size_t Bytes>
    static auto& array_cast(memory& buffer) NOEXCEPT
    {
        return system::unsafe_array_cast<uint8_t, Bytes>(buffer.begin());
    }

    static constexpr size_t offset(const Link& index) NOEXCEPT
    {
        // Byte offset of bucket index within header file.
        // [body_size][[bucket[0]...bucket[buckets-1]]]
        return Link::size + index * Link::size;
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
