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
#ifndef LIBBITCOIN_DATABASE_PRIMITIVES_ARRAYHEAD_HPP
#define LIBBITCOIN_DATABASE_PRIMITIVES_ARRAYHEAD_HPP

#include <shared_mutex>
#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/primitives/manager.hpp>

namespace libbitcoin {
namespace database {

/// Dynamically expanding array map header.
/// Less efficient than a fixed-size header.
template <typename Link>
class arrayhead
{
public:
    DEFAULT_COPY_MOVE_DESTRUCT(arrayhead);

    using bytes = typename Link::bytes;

    /// An array head is disabled it if has one or less buckets.
    arrayhead(storage& head, const Link& buckets) NOEXCEPT;

    /// Sizing is dynamic (thread safe).
    size_t size() const NOEXCEPT;
    size_t buckets() const NOEXCEPT;

    /// Configure initial buckets to zero to disable the table.
    bool enabled() const NOEXCEPT;

    /// Create from empty head file (not thread safe).
    bool create() NOEXCEPT;

    /// False if head file size incorrect (not thread safe).
    bool verify() const NOEXCEPT;

    /// Unsafe if verify false (not thread safe).
    bool get_body_count(Link& count) const NOEXCEPT;
    bool set_body_count(const Link& count) NOEXCEPT;

    /// Convert natural key to head bucket index (validated).
    inline Link index(size_t key) const NOEXCEPT;

    /// Convert natural key to head bucket index (unvalidated).
    inline Link putter_index(size_t key) const NOEXCEPT;

    /// Unsafe if verify false.
    Link at(size_t key) const NOEXCEPT;

    /// Assign value to bucket index.
    bool push(const bytes& current, const Link& index) NOEXCEPT;

private:
    using body = manager<Link, system::data_array<zero>, Link::size>;

    template <size_t Bytes>
    static inline auto& array_cast(memory::iterator buffer) NOEXCEPT
    {
        return system::unsafe_array_cast<uint8_t, Bytes>(buffer);
    }

    static constexpr Link position_to_link(size_t position) NOEXCEPT
    {
        using namespace system;
        static_assert(is_nonzero(Link::size));
        const auto link = floored_subtract(position / Link::size, one);
        return possible_narrow_cast<typename Link::integer>(link);
    }

    // Byte offset of bucket index within head file.
    // [body_size][[bucket[0]...bucket[buckets-1]]]
    static constexpr size_t link_to_position(const Link& index) NOEXCEPT
    {
        using namespace system;
        BC_ASSERT(!is_multiply_overflow<size_t>(index, Link::size));
        BC_ASSERT(!is_add_overflow(Link::size, index * Link::size));
        return possible_narrow_cast<size_t>(Link::size + index * Link::size);
    }

    storage& file_;
    const Link initial_buckets_;
    mutable std::shared_mutex mutex_{};
};

} // namespace database
} // namespace libbitcoin

#define TEMPLATE template <typename Link>
#define CLASS arrayhead<Link>

#include <bitcoin/database/impl/primitives/arrayhead.ipp>

#undef CLASS
#undef TEMPLATE

#endif
