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
#ifndef LIBBITCOIN_DATABASE_PRIMITIVES_ARRAY_HPP
#define LIBBITCOIN_DATABASE_PRIMITIVES_ARRAY_HPP

#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/memory.hpp>
#include <bitcoin/database/primitives/head.hpp>
#include <bitcoin/database/primitives/linkage.hpp>
#include <bitcoin/database/primitives/manager.hpp>


namespace libbitcoin {
namespace database {
    
/// Caution: reader/writer hold body remap lock until disposed.
/// These handles should be used for serialization and immediately disposed.
template <typename Link, size_t Size>
class arraymap
{
public:
    using link = Link;

    arraymap(storage& header, storage& body) NOEXCEPT;

    /// Setup, not thread safe.
    /// -----------------------------------------------------------------------

    bool create() NOEXCEPT;
    bool close() NOEXCEPT;
    bool backup() NOEXCEPT;
    bool restore() NOEXCEPT;
    bool verify() const NOEXCEPT;

    /// Query interface.
    /// -----------------------------------------------------------------------

    /// Get element at link.
    template <typename Element, if_equal<Element::size, Size> = true>
    bool get(const Link& link, Element& element) const NOEXCEPT;

    /// Put element and return link.
    template <typename Element, if_equal<Element::size, Size> = true>
    Link put_link(const Element& element) NOEXCEPT;

protected:
    reader_ptr getter(const Link& link) const NOEXCEPT;
    writer_ptr creater(Link& link, const Link& size) NOEXCEPT;

private:
    static constexpr auto is_slab = (Size == max_size_t);
    using header = database::head<Link, system::data_array<zero>>;
    using manager = database::manager<Link, system::data_array<zero>, Size>;

    // Unsafe with zero buckets (index/top/push).
    // Not thread safe (create/open/close/backup/restore).
    header header_;

    // Thread safe.
    manager manager_;
};

template <typename Element>
using array_map = arraymap<linkage<Element::pk>, Element::size>;

} // namespace database
} // namespace libbitcoin

#define TEMPLATE template <typename Link, size_t Size>
#define CLASS arraymap<Link, Size>

#include <bitcoin/database/impl/primitives/arraymap.ipp>

#undef CLASS
#undef TEMPLATE

#endif
