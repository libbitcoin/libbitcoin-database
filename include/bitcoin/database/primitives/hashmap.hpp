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
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/memory.hpp>
#include <bitcoin/database/primitives/head.hpp>
#include <bitcoin/database/primitives/iterator.hpp>
#include <bitcoin/database/primitives/manager.hpp>

namespace libbitcoin {
namespace database {

/// Caution: iterator/reader/finalizer hold body remap lock until disposed.
/// These handles should be used for serialization and immediately disposed.
template <typename Link, typename Key, typename Record>
class hashmap
{
public:
    using iterator = database::iterator<Link, Key, Record::size>;

    hashmap(storage& header, storage& body, const Link& buckets) NOEXCEPT;

    /// Create from empty body/head files (not thread safe).
    bool create() NOEXCEPT;

    /// False if head or body file size incorrect (not thread safe).
    bool verify() const NOEXCEPT;

    /// Query interface, iterator is not thread safe.
    bool exists(const Key& key) const NOEXCEPT;
    Record get(const Key& key) const NOEXCEPT;
    Record get(const Link& link) const NOEXCEPT;
    iterator it(const Key& key) const NOEXCEPT;
    bool insert(const Key& key, const Record& record) NOEXCEPT;

protected:
    /// Reader positioned at key.
    reader_ptr at(const Link& link) const NOEXCEPT;

    /// Reader positioned at data.
    reader_ptr find(const Key& key) const NOEXCEPT;

    /// Reader positioned at data.
    finalizer_ptr push(const Key& key, const Link& size=one) NOEXCEPT;

private:
    static constexpr auto is_slab = is_zero(Record::size);
    using header = database::head<Link, Key>;
    using manager = database::manager<Link, Key, Record::size>;

    // hash/head/push thread safe.
    header header_;

    // Thread safe.
    manager manager_;
};

} // namespace database
} // namespace libbitcoin

#define TEMPLATE template <typename Link, typename Key, typename Record>
#define CLASS hashmap<Link, Key, Record>

#include <bitcoin/database/impl/primitives/hashmap.ipp>

#undef CLASS
#undef TEMPLATE

#endif
