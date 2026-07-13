/**
 * Copyright (c) 2011-2026 libbitcoin developers (see AUTHORS)
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
#ifndef LIBBITCOIN_DATABASE_PRIMITIVES_NOMAPS_HPP
#define LIBBITCOIN_DATABASE_PRIMITIVES_NOMAPS_HPP

#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/memory.hpp>

namespace libbitcoin {
namespace database {

/// SoA aggregate array table: one shared head + one unified managers body.
template <typename Link, typename... Columns>
class nomaps
{
public:
    DELETE_COPY_MOVE_DESTRUCT(nomaps);

    using link = Link;
    static_assert((is_same_type<link, typename Columns::link> && ...),
        "all columns must share the row link type");

    template <size_t Column>
    static constexpr size_t width = std::get<Column>
    (
        std::array<size_t, sizeof...(Columns)>{ Columns::width... }
    );

    nomaps(storage& header, storage& body) NOEXCEPT;

    /// Setup.
    /// -----------------------------------------------------------------------

    bool create() NOEXCEPT;
    bool close() NOEXCEPT;
    bool backup(bool=false) NOEXCEPT;
    bool restore() NOEXCEPT;
    bool verify() const NOEXCEPT;

    /// Sizing.
    /// -----------------------------------------------------------------------

    size_t body_size() const NOEXCEPT;
    Link count() const NOEXCEPT;
    Link allocate(const Link& count) NOEXCEPT;
    bool truncate(const Link& count) NOEXCEPT;
    bool drop() NOEXCEPT;

    /// Faults.
    /// -----------------------------------------------------------------------

    code get_fault() const NOEXCEPT;
    size_t get_space() const NOEXCEPT;
    code reload() NOEXCEPT;

    /// Query interface (columnar).
    /// -----------------------------------------------------------------------

    /// Column base ptr for batch processing (holds shared lock on body remap).
    template <size_t Column>
    memory get_memory() const NOEXCEPT;

    /// Get element from column at link using column base ptr (deserialize).
    template <size_t Column, typename Element>
    static bool get(const memory& ptr, const Link& link,
        Element& element) NOEXCEPT;

    /// Get element from column at link.
    template <size_t Column, typename Element>
    bool get(const Link& link, Element& element) const NOEXCEPT;

    /// Put previously allocated element to column at link.
    template <size_t Column, typename Element>
    bool put(const Link& link, const Element& element) NOEXCEPT;
    template <size_t Column, typename Element>
    bool put(const memory& ptr, const Element& element) NOEXCEPT;

protected:
    using head = database::nohead<link>;
    using body = database::managers<link, system::data_array<zero>,
        Columns::width...>;

    // Thread safe (index/top/push).
    // Not thread safe (create/open/close/backup/restore).
    head head_;

    // This is thread safe.
    body manager_;
};

} // namespace database
} // namespace libbitcoin

#define TEMPLATE template <typename Link, typename... Columns>
#define CLASS nomaps<Link, Columns...>

#include <bitcoin/database/impl/primitives/nomaps.ipp>

#undef CLASS
#undef TEMPLATE

#endif
