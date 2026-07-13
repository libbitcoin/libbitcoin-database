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
#ifndef LIBBITCOIN_DATABASE_PRIMITIVES_COLUMN_HPP
#define LIBBITCOIN_DATABASE_PRIMITIVES_COLUMN_HPP

#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/memory.hpp>

namespace libbitcoin {
namespace database {

/// Named column forwarder: binds a column index onto an aggregate table.
template <typename Table, size_t Column>
class column
{
public:
    using link = typename Table::link;

    INLINE column(Table& table) NOEXCEPT
      : table_(table)
    {
    }

    INLINE memory get_memory() const NOEXCEPT
    {
        return table_.template get_memory<Column>();
    }

    template <typename Element>
    INLINE bool get(const link& record, Element& element) const NOEXCEPT
    {
        return table_.template get<Column>(record, element);
    }

    template <typename Element>
    static INLINE bool get(const memory& ptr, const link& record,
        Element& element) NOEXCEPT
    {
        return Table::template get<Column>(ptr, record, element);
    }

    template <typename Element>
    INLINE bool put(const link& record, const Element& element) NOEXCEPT
    {
        return table_.template put<Column>(record, element);
    }

    template <typename Element>
    INLINE bool put(const memory& ptr, const Element& element) NOEXCEPT
    {
        return table_.template put<Column>(ptr, element);
    }

private:
    Table& table_;
};

} // namespace database
} // namespace libbitcoin

#endif
