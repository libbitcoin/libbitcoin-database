/**
/// Copyright (c) 2011-2025 libbitcoin developers (see AUTHORS)
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
#ifndef LIBBITCOIN_DATABASE_PRIMITIVES_NOMAP_HPP
#define LIBBITCOIN_DATABASE_PRIMITIVES_NOMAP_HPP

#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/memory.hpp>
#include <bitcoin/database/primitives/arrayhead.hpp>
#include <bitcoin/database/primitives/linkage.hpp>
#include <bitcoin/database/primitives/manager.hpp>

namespace libbitcoin {
namespace database {
    
/// Caution: reader/writer hold body remap lock until disposed.
/// These handles should be used for serialization and immediately disposed.
template <typename Link, size_t Size>
class nomap
{
public:
    DEFAULT_COPY_MOVE_DESTRUCT(nomap);

    using link = Link;

    nomap(storage& header, storage& body) NOEXCEPT;

    /// Setup, not thread safe.
    /// -----------------------------------------------------------------------

    bool create() NOEXCEPT;
    bool close() NOEXCEPT;
    bool backup() NOEXCEPT;
    bool restore() NOEXCEPT;
    bool verify() const NOEXCEPT;

    /// Sizing.
    /// -----------------------------------------------------------------------

    /// Hash table bucket count (zero).
    size_t buckets() const NOEXCEPT;

    /// Head file bytes.
    size_t head_size() const NOEXCEPT;

    /// Body file bytes.
    size_t body_size() const NOEXCEPT;

    /// Count of records (or body file bytes if slab).
    Link count() const NOEXCEPT;

    /// Reduce count as specified.
    bool truncate(const Link& count) NOEXCEPT;

    /// Errors.
    /// -----------------------------------------------------------------------

    /// Get the fault condition.
    code get_fault() const NOEXCEPT;

    /// Get the space required to clear the disk full condition.
    size_t get_space() const NOEXCEPT;

    /// Resume from disk full condition.
    code reload() NOEXCEPT;

    /// Query interface.
    /// -----------------------------------------------------------------------

    /// Get element at link.
    template <typename Element, if_equal<Element::size, Size> = true>
    bool get(const Link& link, Element& element) const NOEXCEPT;

    /// Put element.
    template <typename Element, if_equal<Element::size, Size> = true>
    bool put(const Element& element) NOEXCEPT;

    /// Put element and return link.
    template <typename Element, if_equal<Element::size, Size> = true>
    bool put_link(Link& link, const Element& element) NOEXCEPT;
    template <typename Element, if_equal<Element::size, Size> = true>
    Link put_link(const Element& element) NOEXCEPT;

private:
    static constexpr auto is_slab = (Size == max_size_t);
    using manager = database::manager<Link, system::data_array<zero>, Size>;
    using head = database::arrayhead<Link>;

    // Thread safe (index/top/push).
    // Not thread safe (create/open/close/backup/restore).
    head head_;

    // Thread safe.
    manager manager_;
};

template <typename Element>
using no_map = nomap<linkage<Element::pk>, Element::size>;

} // namespace database
} // namespace libbitcoin

#define TEMPLATE template <typename Link, size_t Size>
#define CLASS nomap<Link, Size>

#include <bitcoin/database/impl/primitives/nomap.ipp>

#undef CLASS
#undef TEMPLATE

#endif
