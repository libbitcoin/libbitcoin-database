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
#ifndef LIBBITCOIN_DATABASE_PRIMITIVES_HASHMAP_HPP
#define LIBBITCOIN_DATABASE_PRIMITIVES_HASHMAP_HPP

#include <bitcoin/system.hpp>
#include <bitcoin/database/boost.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/primitives/head.hpp>
#include <bitcoin/database/primitives/manager.hpp>
#include <bitcoin/database/memory/interfaces/memory.hpp>
#include <bitcoin/database/memory/interfaces/storage.hpp>
#include <bitcoin/database/memory/reader.hpp>
#include <bitcoin/database/memory/writer.hpp>

namespace libbitcoin {
namespace database {

template <typename Iterator>
class hashmap
{
public:
    using link = typename Iterator::link;
    using key = typename Iterator::key;

    hashmap(storage& header, storage& body, const link& buckets) NOEXCEPT;

    /// Not thread safe.
    /// -----------------------------------------------------------------------

    /// Create from empty body/head files (no need to verify).
    bool create() NOEXCEPT;

    /// False if head or body file size incorrect.
    bool verify() const NOEXCEPT;

    /// Thread safe.
    /// -----------------------------------------------------------------------

    /// Search table for links of all keys.
    Iterator iterator(const key& key) const NOEXCEPT;
    
    /// Search table for link of first instance of key.
    link first(const key& key) const NOEXCEPT;

    /// Reader positioned at key.
    reader_ptr at(const link& record) const NOEXCEPT;

    /// Reader positioned at data, same as at(first(key)).
    reader_ptr find(const key& key) const NOEXCEPT;

    /// Reader positioned at data, size is one for records and bytes for slabs.
    writer_ptr push(const key& key, const link& size=one) NOEXCEPT;

private:
    static constexpr auto link_size = link::size;
    static constexpr auto key_size = array_count<key>;
    static constexpr auto payload_size = Iterator::payload;
    static constexpr auto slab = is_zero(payload_size);

    using header = database::head<link, key>;
    using manager = database::manager<link, payload_size>;

    // hash/head/push thread safe.
    header header_;

    // Thread safe.
    manager body_;
};

} // namespace database
} // namespace libbitcoin

#define TEMPLATE \
template <typename Iterator>
#define CLASS hashmap<Iterator>

#include <bitcoin/database/impl/primitives/hashmap.ipp>

#undef CLASS
#undef TEMPLATE

#endif
