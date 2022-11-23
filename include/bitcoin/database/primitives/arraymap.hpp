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


namespace libbitcoin {
namespace database {
    
/// Caution: reader/writer hold body remap lock until disposed.
/// These handles should be used for serialization and immediately disposed.
template <typename Link, size_t Size>
class arraymap
{
public:
    arraymap(storage& header, storage& body) NOEXCEPT;

    /// Create from empty body/head files (not thread safe).
    bool create() NOEXCEPT;

    /// False if head or body file size incorrect (not thread safe).
    bool verify() const NOEXCEPT;

    /// Truncate body to header body size (not thread safe).
    bool snap() NOEXCEPT;

    /// Query interface.
    /// -----------------------------------------------------------------------

    template <typename Element, if_equal<Element::size, Size> = true>
    bool get(const Link& link, Element& element) const NOEXCEPT;

    template <typename Element, if_equal<Element::size, Size> = true>
    bool put(const Element& element) NOEXCEPT;

protected:
    reader_ptr getter(const Link& link) const NOEXCEPT;
    writer_ptr creater(const Link& size) NOEXCEPT;

private:
    static constexpr auto is_slab = (Size == max_size_t);
    static constexpr size_t link_to_position(const Link& link) NOEXCEPT;
    using header = database::head<Link, system::data_array<zero>>;

    // Not thread safe (create/verify/snap).
    header header_;

    // Thread safe.
    storage& body_;
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
