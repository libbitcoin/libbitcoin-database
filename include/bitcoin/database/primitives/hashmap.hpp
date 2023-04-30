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
#include <bitcoin/database/primitives/linkage.hpp>
#include <bitcoin/database/primitives/manager.hpp>

namespace libbitcoin {
namespace database {

/// Caution: iterator/reader/finalizer hold body remap lock until disposed.
/// These handles should be used for serialization and immediately disposed.
/// Readers and writers are always prepositioned at data, and are limited to
/// the extent the record/slab size is known (limit can always be removed).
/// Streams are always initialized from first element byte up to file limit.
template <typename Link, typename Key, size_t Size, bool Hash>
class hashmap
{
public:
    DEFAULT_COPY_MOVE_DESTRUCT(hashmap);

    using key = Key;
    using link = Link;
    using iterator = database::iterator<Link, Key, Size>;

    hashmap(storage& header, storage& body, const Link& buckets) NOEXCEPT;

    /// Setup, not thread safe.
    /// -----------------------------------------------------------------------

    bool create() NOEXCEPT;
    bool close() NOEXCEPT;
    bool backup() NOEXCEPT;
    bool restore() NOEXCEPT;
    bool verify() const NOEXCEPT;

    /// Sizing.
    /// -----------------------------------------------------------------------

    /// Hash table bucket count.
    size_t buckets() const NOEXCEPT;

    /// Head file bytes.
    size_t head_size() const NOEXCEPT;

    /// Body file bytes.
    size_t body_size() const NOEXCEPT;

    /// Count of records (or body file bytes if slab).
    Link count() const NOEXCEPT;

    /// Query interface, iterator is not thread safe.
    /// -----------------------------------------------------------------------

    /// True if an instance of object with key exists.
    bool exists(const Key& key) const NOEXCEPT;

    /// Return first element or terimnal.
    Link first(const Key& key) const NOEXCEPT;

    /// Iterator holds shared lock on storage remap.
    iterator it(const Key& key) const NOEXCEPT;

    /// Return the link at the top of the conflict list (for table scanning).
    Link top(const Link& list) const NOEXCEPT;

    /// Allocate element at returned link (follow with set|put).
    Link allocate(const Link& size) NOEXCEPT;

    /// Return the associated search key (terminal link returns default).
    Key get_key(const Link& link) NOEXCEPT;

    /// Query interface (iostreams).
    /// -----------------------------------------------------------------------

    /// Get element at link, false if deserialize error.
    template <typename Element, if_equal<Element::size, Size> = true>
    bool get(const Link& link, Element& element) const NOEXCEPT;

    /// Set element into previously allocated link (follow with commit).
    template <typename Element, if_equal<Element::size, Size> = true>
    bool set(const Link& link, const Element& element) NOEXCEPT;

    /// Allocate and set element, and return link (follow with commit).
    template <typename Element, if_equal<Element::size, Size> = true>
    Link set_link(const Element& element) NOEXCEPT;
    template <typename Element, if_equal<Element::size, Size> = true>
    bool set_link(Link& link, const Element& element) NOEXCEPT;

    /// Allocate, set, commit element to key, and return link.
    template <typename Element, if_equal<Element::size, Size> = true>
    Link put_link(const Key& key, const Element& element) NOEXCEPT;
    template <typename Element, if_equal<Element::size, Size> = true>
    bool put_link(Link& link, const Key& key, const Element& element) NOEXCEPT;

    /// Allocate, set, commit element to key.
    template <typename Element, if_equal<Element::size, Size> = true>
    bool put(const Key& key, const Element& element) NOEXCEPT;

    /// Set and commit previously allocated element at link to key.
    template <typename Element, if_equal<Element::size, Size> = true>
    bool put(const Link& link, const Key& key, const Element& element) NOEXCEPT;

    /// Query interface (memory).
    /// -----------------------------------------------------------------------

    /// Get element at link, false if deserialize error.
    template <typename Element, if_equal<Element::size, Size> = true>
    bool get1(const Link& link, Element& element) const NOEXCEPT;

    /// Set element into previously allocated link (follow with commit).
    template <typename Element, if_equal<Element::size, Size> = true>
    bool set1(const Link& link, const Element& element) NOEXCEPT;

    /// Allocate and set element, and return link (follow with commit).
    template <typename Element, if_equal<Element::size, Size> = true>
    Link set_link1(const Element& element) NOEXCEPT;
    template <typename Element, if_equal<Element::size, Size> = true>
    bool set_link1(Link& link, const Element& element) NOEXCEPT;

    /// Allocate, set, commit element to key, and return link.
    template <typename Element, if_equal<Element::size, Size> = true>
    Link put_link1(const Key& key, const Element& element) NOEXCEPT;
    template <typename Element, if_equal<Element::size, Size> = true>
    bool put_link1(Link& link, const Key& key, const Element& element) NOEXCEPT;

    /// Allocate, set, commit element to key.
    template <typename Element, if_equal<Element::size, Size> = true>
    bool put1(const Key& key, const Element& element) NOEXCEPT;

    /// Set and commit previously allocated element at link to key.
    template <typename Element, if_equal<Element::size, Size> = true>
    bool put1(const Link& link, const Key& key, const Element& element) NOEXCEPT;

    /// -----------------------------------------------------------------------

    /// Commit previously set element at link to key.
    bool commit(const Link& link, const Key& key) NOEXCEPT;
    Link commit_link(const Link& link, const Key& key) NOEXCEPT;

protected:
    template <typename Streamer>
    typename Streamer::ptr streamer(const Link& link) const NOEXCEPT;

    finalizer_ptr creater(Link& link, const Key& key,
        const Link& size) NOEXCEPT;
    finalizer_ptr putter(const Link& link, const Key& key,
        const Link& size) NOEXCEPT;

private:
    static constexpr auto is_slab = (Size == max_size_t);
    using head = database::head<Link, Key, Hash>;
    using manager = database::manager<Link, Key, Size>;

    // Thread safe (index/top/push).
    // Not thread safe (create/open/close/backup/restore).
    head head_;

    // Thread safe.
    manager manager_;
};

template <typename Element>
using hash_map = hashmap<linkage<Element::pk>, system::data_array<Element::sk>,
    Element::size, Element::hash_function>;

} // namespace database
} // namespace libbitcoin

#define TEMPLATE template <typename Link, typename Key, size_t Size, bool Hash>
#define CLASS hashmap<Link, Key, Size, Hash>

#include <bitcoin/database/impl/primitives/hashmap.ipp>

#undef CLASS
#undef TEMPLATE

#endif
