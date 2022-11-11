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
#ifndef LIBBITCOIN_DATABASE_PRIMITIVES_HEAD_HPP
#define LIBBITCOIN_DATABASE_PRIMITIVES_HEAD_HPP

#include <bitcoin/system.hpp>
#include <bitcoin/database/boost.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/memory.hpp>

namespace libbitcoin {
namespace database {

template <typename Link, typename Key>
class head
{
public:
    using bytes = typename Link::bytes;

    head(storage& head, const Link& buckets) NOEXCEPT;

    /// Create from empty head file (not thread safe).
    bool create() NOEXCEPT;

    /// False if head file size incorrect (not thread safe).
    bool verify() const NOEXCEPT;

    /// Unsafe if verify false (not thread safe).
    bool get_body_count(Link& count) const NOEXCEPT;
    bool set_body_count(const Link& count) NOEXCEPT;

    /// Convert natural key to head bucket index.
    Link index(const Key& key) const NOEXCEPT;

    /// Unsafe if verify false.
    Link top(const Key& key) const NOEXCEPT;
    Link top(const Link& index) const NOEXCEPT;
    bool push(const bytes& current, bytes& next, const Key& key) NOEXCEPT;
    bool push(const bytes& current, bytes& next, const Link& index) NOEXCEPT;

private:
    template <size_t Bytes>
    static auto& array_cast(memory& buffer) NOEXCEPT
    {
        return system::unsafe_array_cast<uint8_t, Bytes>(buffer.begin());
    }

    static constexpr size_t offset(const Link& index) NOEXCEPT
    {
        using namespace system;
        BC_ASSERT(!is_multiply_overflow<size_t>(index, Link::size));
        BC_ASSERT(!is_add_overflow(Link::size, index * Link::size));

        // Byte offset of bucket index within head file.
        // [body_size][[bucket[0]...bucket[buckets-1]]]
        return possible_narrow_cast<size_t>(Link::size + index * Link::size);
    }

    storage& file_;
    const Link buckets_;
    mutable boost::upgrade_mutex mutex_;
};

} // namespace database
} // namespace libbitcoin


#define TEMPLATE template <typename Link, typename Key>
#define CLASS head<Link, Key>

#include <bitcoin/database/impl/primitives/head.ipp>

#undef CLASS
#undef TEMPLATE

#endif
