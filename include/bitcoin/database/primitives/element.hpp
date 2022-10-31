/**
 * Copyright (c) 2011-2022 libbitcoin developers (see AUTHORS)
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
#include <bitcoin/database/primitives/manager.hpp>

namespace libbitcoin {
namespace database {

// keyed:   [[link][key][value]]
// unkeyed: [[link][]   [value]]
// Value may be fixed (record element) or variable (slab element) size.
// Derived elements provide write append and random access read to memory map.

/// Forward linked list element.
/// Key is always serialized as a byte array.
/// Link type is determined by manager (slab/bytes or record/index).
template <typename Link, typename Key, size_t Size = zero>
class element
{
public:
    element(const manager<Link, Size>& manage, Link value) NOEXCEPT;

    /// Advance to next element.
    void advance() NOEXCEPT;

    /// Link of this element (or eof).
    Link self() const NOEXCEPT;

    /// Link to next element (or eof).
    Link get_next() const NOEXCEPT;

    /// The element natural key.
    Key get_key() const NOEXCEPT;

    /// Natural key matches specified value.
    bool is_match(const Key& value) const NOEXCEPT;

    /// False if link is eof (and above methods are undefined).
    operator bool() const NOEXCEPT;

    /// Equality/inequality, compares link value only.
    bool operator==(const element& other) const NOEXCEPT;
    bool operator!=(const element& other) const NOEXCEPT;

protected:
    memory_ptr get() const NOEXCEPT;
    memory_ptr get(size_t offset) const NOEXCEPT;

private:
    template <size_t Bytes>
    static auto array_cast(memory& buffer) NOEXCEPT
    {
        return system::unsafe_array_cast<uint8_t, Bytes>(buffer.begin());
    }

    const manager<Link, Size>& manager_;
    Link link_;
};

} // namespace database
} // namespace libbitcoin

#define TEMPLATE \
template <typename Link, typename Key, size_t Size>
#define CLASS element<Link, Key, Size>

#include <bitcoin/database/impl/primitives/element.ipp>

#undef CLASS
#undef TEMPLATE

#endif
