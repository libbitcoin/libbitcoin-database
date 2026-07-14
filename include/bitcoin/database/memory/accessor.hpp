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
#ifndef LIBBITCOIN_DATABASE_MEMORY_ACCESSOR_HPP
#define LIBBITCOIN_DATABASE_MEMORY_ACCESSOR_HPP

#include <shared_mutex>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

/// Not thread safe.
/// Shared r/w access to a memory buffer, mutex blocks memory remap.
class accessor final
{
public:
    typedef uint8_t value_type;
    typedef value_type* iterator;
    typedef const value_type* const_iterator;

    DELETE_COPY(accessor);
    DEFAULT_MOVE(accessor);

    /// Fault construction, invalid object, no lock taken.
    inline accessor() NOEXCEPT
      : shared_lock_{}
    {
    }

    /// Mutex guards against remap while object in scope.
    inline accessor(std::shared_mutex& mutex) NOEXCEPT
      : shared_lock_{ mutex }
    {
    }

    /// True if holds lock on memory buffer.
    inline operator bool() const NOEXCEPT
    {
        return !is_null(begin_);
    }

    /// Set the buffer, where end is within allocated space.
    /// Zero/negative size is allowed (automatically handled by bc streams).
    /// End should be initialized to logical space though that may contract or
    /// expand during accessor lifetime. The only guarantee offered by end is
    /// that it remains within allocated space and is initially logical space.
    inline void assign(uint8_t* begin, uint8_t* end) NOEXCEPT
    {
        begin_ = begin;
        end_ = end;
    }

    /// Release lock and invalidate pointers (idempotent).
    inline void reset() NOEXCEPT
    {
        if (!is_null(begin_))
        {
            shared_lock_.unlock();
            begin_ = nullptr;
            end_ = nullptr;
        }
    }

    /// Return an offset from begin, nullptr if past buffer.
    inline uint8_t* offset(size_t bytes) const NOEXCEPT
    {
        if (system::is_greater(bytes, size()))
            return nullptr;

        return std::next(begin_, bytes);
    }

    /// The logical buffer size (from begin to end).
    inline ptrdiff_t size() const NOEXCEPT
    {
        return std::distance(begin_, end_);
    }

    /// Alias for begin.
    inline uint8_t* data() const NOEXCEPT
    {
        return begin();
    }

    /// Buffer start.
    inline uint8_t* begin() const NOEXCEPT
    {
        return begin_;
    }

    /// Buffer end.
    inline uint8_t* end() const NOEXCEPT
    {
        return end_;
    }

    /// Get logical buffer (guarded against remap only).
    inline operator system::data_slab() const NOEXCEPT
    {
        return { begin(), end() };
    };

private:
    // These are not thread safe.
    uint8_t* begin_{};
    uint8_t* end_{};

    // This is thread safe.
    std::shared_lock<std::shared_mutex> shared_lock_;
};

using memory = accessor;

} // namespace database
} // namespace libbitcoin

#endif
