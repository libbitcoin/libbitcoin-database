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
#ifndef LIBBITCOIN_DATABASE_PRIMITIVES_ITERATOR_HPP
#define LIBBITCOIN_DATABASE_PRIMITIVES_ITERATOR_HPP

#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/interfaces/memory.hpp>
#include <bitcoin/database/memory/interfaces/storage.hpp>
#include <bitcoin/database/primitives/manager.hpp>

namespace libbitcoin {
namespace database {

/// Size non-zero implies record manager (ordinal record links).
template <typename Link, typename Key, size_t Size = zero>
class iterator
{
public:
    static constexpr auto payload = array_count<Key> + Size;
    using link = Link;
    using key = Key;

    /// Caller must keep key value in scope.
    iterator(const manager<Link, payload>& manage, const Link& start,
        const Key& key) NOEXCEPT;

    /// Advance to and return next iterator.
    bool next() NOEXCEPT;
    Link self() NOEXCEPT;

protected:
    bool is_match() const NOEXCEPT;
    Link get_next() const NOEXCEPT;
    memory_ptr get(size_t offset) const NOEXCEPT;

private:
    template <size_t Bytes>
    static auto& array_cast(memory& buffer) NOEXCEPT
    {
        return system::unsafe_array_cast<uint8_t, Bytes>(buffer.begin());
    }

    const manager<Link, payload>& manager_;
    const Key& key_;
    Link link_;
};

} // namespace database
} // namespace libbitcoin

#define TEMPLATE \
template <typename Link, typename Key, size_t Size>
#define CLASS iterator<Link, Key, Size>

#include <bitcoin/database/impl/primitives/iterator.ipp>

#undef CLASS
#undef TEMPLATE

#endif
