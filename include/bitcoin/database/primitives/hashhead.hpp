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
#ifndef LIBBITCOIN_DATABASE_PRIMITIVES_HEAD_HPP
#define LIBBITCOIN_DATABASE_PRIMITIVES_HEAD_HPP

#include <atomic>
#include <shared_mutex>
#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/memory.hpp>
#include <bitcoin/database/primitives/keys.hpp>
#include <bitcoin/database/primitives/linkage.hpp>

namespace libbitcoin {
namespace database {

// TODO: Link::terminal is byte aligned, which now wastes bits.
// TODO: change to bitwise link domain and sentinal as max value.
// TODO: Add unused link bits in filter size.
template <class Link, class Key, size_t CellSize = Link::size,
    if_not_greater<Link::size, CellSize> = true>
class hashhead
{
public:
    DEFAULT_COPY_MOVE_DESTRUCT(hashhead);

    using bytes = typename Link::bytes;

    /// An hash head is disabled it if has one or less buckets (0 bits).
    hashhead(storage& head, size_t bits) NOEXCEPT;

    /// Sizing (thread safe).
    inline size_t size() const NOEXCEPT;
    inline size_t buckets() const NOEXCEPT;

    /// Create from empty head file (not thread safe).
    bool create() NOEXCEPT;

    /// False if head file size incorrect (not thread safe).
    bool verify() const NOEXCEPT;

    /// Unsafe if verify false (not thread safe).
    bool get_body_count(Link& count) const NOEXCEPT;
    bool set_body_count(const Link& count) NOEXCEPT;

    /// Convert natural key to head bucket index (all keys are valid).
    /// Terminal is a valid bucket index (just not a valid bucket value).
    inline Link index(const Key& key) const NOEXCEPT;

    /// Unsafe if verify false.
    inline Link top(const Key& key) const NOEXCEPT;
    inline Link top(const Link& index) const NOEXCEPT;
    inline bool push(const Link& current, bytes& next, const Key& key) NOEXCEPT;
    inline bool push(const Link& current, bytes& next, const Link& index) NOEXCEPT;
    inline bool push(bool& collision, const Link& current, bytes& next,
        const Link& index) NOEXCEPT;

private:
    using bucket_integer = Link::integer;
    using cell_integer = unsigned_type<CellSize>;
    static_assert(std::atomic<cell_integer>::is_always_lock_free);
    static constexpr auto aligned = (CellSize == sizeof(cell_integer));
    static constexpr auto bucket_size = Link::size;
    static constexpr auto filter_size = CellSize - bucket_size;

    template <size_t Bytes>
    static inline auto& to_array(memory::iterator it) NOEXCEPT
    {
        return system::unsafe_array_cast<uint8_t, Bytes>(it);
    }

    // Byte offset of bucket index within head file.
    // [body_size][[bucket[0]...bucket[buckets-1]]]
    static constexpr size_t link_to_position(const Link& index) NOEXCEPT
    {
        using namespace system;
        BC_ASSERT(!is_multiply_overflow<size_t>(index, CellSize));
        BC_ASSERT(!is_add_overflow(size_, index * CellSize));
        return possible_narrow_cast<size_t>(CellSize + index * CellSize);
    }

    // These are thread safe.
    storage& file_;
    const Link buckets_;
    const Link mask_;
    mutable std::shared_mutex mutex_{};
};

} // namespace database
} // namespace libbitcoin

#define TEMPLATE template <class Link, class Key, size_t CellSize, \
    if_not_greater<Link::size, CellSize> If>
#define CLASS hashhead<Link, Key, CellSize, If>

#include <bitcoin/database/impl/primitives/hashhead.ipp>

#undef CLASS
#undef TEMPLATE

#endif
