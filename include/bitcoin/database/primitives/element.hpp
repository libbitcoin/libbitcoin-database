/**
 * Copyright (c) 2011-2019 libbitcoin developers (see AUTHORS)
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
#ifndef LIBBITCOIN_DATABASE_PRIMITIVES_ELEMENT_HPP
#define LIBBITCOIN_DATABASE_PRIMITIVES_ELEMENT_HPP

#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/memory.hpp>

namespace libbitcoin {
namespace database {

// keyed:   [[link][key][value]]
// unkeyed: [[link][]   [value]]
// Value may be fixed (record element) or variable (slab element) size.
// Derived elements provide write append and random access read to memory map.

/// Forward linked list element.
template <typename Manager>
class element
{
public:
    using link = typename Manager::link;

    static constexpr auto eof = system::bit_all<link>;

    /// Advance this to next element.
    bool advance() NOEXCEPT;

    /// The storage-relative pointer to this element (or eof).
    link self() const NOEXCEPT;

    /// True if the element is an eof indicator.
    operator bool() const NOEXCEPT;

    /// Compare element link values (identity only).
    bool operator==(element other) const NOEXCEPT;
    bool operator!=(element other) const NOEXCEPT;

protected:
    element(Manager& manager, link value) NOEXCEPT;

    memory_ptr get() const NOEXCEPT;
    memory_ptr get(size_t offset) const NOEXCEPT;
    memory_ptr allocate(size_t size) const NOEXCEPT;

private:
    Manager& manager_;
    link link_;
};

} // namespace database
} // namespace libbitcoin

#define TEMPLATE template <typename Manager>
#define CLASS element<Manager>

#include <bitcoin/database/impl/primitives/element.ipp>

#undef CLASS
#undef TEMPLATE

#endif
