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
#ifndef LIBBITCOIN_DATABASE_PRIMITIVES_ARRAYMAP_HPP
#define LIBBITCOIN_DATABASE_PRIMITIVES_ARRAYMAP_HPP

#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/memory.hpp>
#include <bitcoin/database/primitives/arrayhead.hpp>
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
template <class Link, size_t RowSize, bool Align>
class arraymap
{
public:
    DEFAULT_COPY_MOVE_DESTRUCT(arraymap);

    using link = Link;

    arraymap(storage& header, storage& body, const Link& buckets) NOEXCEPT;

    /// Setup, not thread safe.
    /// -----------------------------------------------------------------------

    bool create() NOEXCEPT;
    bool close() NOEXCEPT;
    bool reset() NOEXCEPT;
    bool backup() NOEXCEPT;
    bool restore() NOEXCEPT;
    bool verify() const NOEXCEPT;

    /// Sizing.
    /// -----------------------------------------------------------------------

    /// The instance is enabled (more than 1 bucket).
    bool enabled() const NOEXCEPT;

    /// Hash table bucket count.
    size_t buckets() const NOEXCEPT;

    /// Head file bytes.
    size_t head_size() const NOEXCEPT;

    /// Body file bytes.
    size_t body_size() const NOEXCEPT;

    /// Count of body records (or bytes if slab).
    Link count() const NOEXCEPT;

    /// Capacity of body in bytes.
    size_t capacity() const NOEXCEPT;

    /// Increase count as neccesary to specified.
    bool expand(const Link& count) NOEXCEPT;

    /// Errors.
    /// -----------------------------------------------------------------------

    /// Get the fault condition.
    code get_fault() const NOEXCEPT;

    /// Get the space required to clear the disk full condition.
    size_t get_space() const NOEXCEPT;

    /// Resume from disk full condition.
    code reload() NOEXCEPT;

    /// Query interface, iterator is not thread safe.
    /// -----------------------------------------------------------------------

    /// Return element link at key, terminal if not found/error (unverified).
    inline Link at(size_t key) const NOEXCEPT;

    /// True if an instance of object with key exists.
    inline Link exists(size_t key) const NOEXCEPT;

    /// Get first element matching key, false if not found/error (unverified).
    template <typename Element, if_equal<Element::size, RowSize> = true>
    inline bool at(size_t key, Element& element) const NOEXCEPT;

    /// Get element at link, false if deserialize error.
    template <typename Element, if_equal<Element::size, RowSize> = true>
    inline bool get(const Link& link, Element& element) const NOEXCEPT;

    /// Allocate, set, commit element to key.
    /// Expands table AND HEADER as necessary.
    template <typename Element, if_equal<Element::size, RowSize> = true>
    bool put(size_t key, const Element& element) NOEXCEPT;

private:
    static constexpr auto is_slab = (RowSize == max_size_t);
    using head = database::arrayhead<Link, Align>;
    using body = database::manager<Link, system::data_array<0>, RowSize>;

    // Thread safe (index/top/push).
    // Not thread safe (create/open/close/backup/restore).
    head head_;

    // Thread safe.
    body body_;
};

template <typename Element>
using array_map = arraymap<typename Element::link, Element::size, Element::align>;

} // namespace database
} // namespace libbitcoin

#define TEMPLATE template <class Link, size_t RowSize, bool Align>
#define CLASS arraymap<Link, RowSize, Align>
#define ELEMENT_CONSTRAINT template <class Element, \
    if_equal<Element::size, RowSize>>

#include <bitcoin/database/impl/primitives/arraymap.ipp>

#undef CLASS
#undef TEMPLATE

#endif
