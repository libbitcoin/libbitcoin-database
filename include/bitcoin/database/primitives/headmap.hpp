/**
 * Copyright (c) 2011-2026 libbitcoin developers
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
#ifndef LIBBITCOIN_DATABASE_PRIMITIVES_HEADMAP_HPP
#define LIBBITCOIN_DATABASE_PRIMITIVES_HEADMAP_HPP

#include <atomic>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/memory.hpp>
#include <bitcoin/database/primitives/linkage.hpp>

namespace libbitcoin {
namespace database {

/// A head-only array of links indexed by position (no body, no count cell).
/// The storage logical size is the count, providing count atomicity and
/// bucket publication as with bodies. Content restores with the heads.
template <class Link, bool Align>
class headmap
{
public:
    DELETE_COPY_MOVE_DESTRUCT(headmap);

    using link = Link;

    headmap(storage& head) NOEXCEPT;

    /// Setup, not thread safe.
    /// -----------------------------------------------------------------------

    bool create() NOEXCEPT;
    bool close() NOEXCEPT;
    bool backup(bool=false) NOEXCEPT;
    bool restore() NOEXCEPT;
    bool verify() const NOEXCEPT;

    /// Sizing.
    /// -----------------------------------------------------------------------

    /// Head file bytes.
    size_t head_size() const NOEXCEPT;

    /// Body file bytes (zero, no body).
    size_t body_size() const NOEXCEPT;

    /// Count of buckets (links).
    Link count() const NOEXCEPT;

    /// Reduce count as specified (truncated buckets are rewritten by push).
    bool truncate(const Link& count) NOEXCEPT;

    /// Reserve additional count buckets to guard against disk full.
    /// Not writer-writer thread safe.
    bool reserve(const Link& count) NOEXCEPT;

    /// Errors.
    /// -----------------------------------------------------------------------

    /// Get the fault condition.
    code get_fault() const NOEXCEPT;

    /// Get the space required to clear the disk full condition.
    size_t get_space() const NOEXCEPT;

    /// Resume from disk full condition (nothing to reload, no body).
    code reload() NOEXCEPT;

    /// Query interface.
    /// -----------------------------------------------------------------------

    /// Return link at index, terminal if at or above count.
    Link at(size_t index) const NOEXCEPT;

    /// Append link at count into reserved capacity (single writer).
    bool push(const Link& link) NOEXCEPT;

private:
    using integer = Link::integer;
    static_assert(std::atomic<integer>::is_always_lock_free);
    static_assert(is_nonzero(Link::size));
    static constexpr auto bucket_size = Align ? sizeof(integer) : Link::size;

    // Alignment ensures atomic bucket access.
    static_assert(bucket_size == sizeof(integer));

    // Byte offset of bucket index within head file.
    // [[bucket[0]...bucket[count-1]]]
    INLINE static constexpr size_t link_to_position(size_t index) NOEXCEPT
    {
        BC_ASSERT(!system::is_multiply_overflow(index, bucket_size));
        return index * bucket_size;
    }

    INLINE static constexpr Link position_to_link(size_t position) NOEXCEPT
    {
        using namespace system;
        return possible_narrow_cast<integer>(
            floored_divide(position, bucket_size));
    }

    // This is thread safe.
    storage& file_;
};

template <typename Schema>
using head_map = headmap<typename Schema::link, Schema::align>;

} // namespace database
} // namespace libbitcoin

#define TEMPLATE template <class Link, bool Align>
#define CLASS headmap<Link, Align>

#include <bitcoin/database/impl/primitives/headmap.ipp>

#undef CLASS
#undef TEMPLATE

#endif
