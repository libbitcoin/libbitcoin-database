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
#include <bitcoin/database/memory/interfaces/memory.hpp>

namespace libbitcoin {
namespace database {

/// Shared r/w access to a memory buffer, mutex blocks memory remap.
/// Zero/negative size is allowed (automatically handled by bc streams).
class accessor
  : public memory
{
public:
    DELETE_COPY(accessor);
    DEFAULT_MOVE(accessor);
    virtual ~accessor() = default;

    /// Mutex guards against remap while object in scope.
    inline accessor(std::shared_mutex& mutex) NOEXCEPT
      : shared_lock_(mutex)
    {
    }

    /// Set the buffer, where end is within allocated space.
    /// End should be initialized to logical space though that may contract or
    /// expand during accessor lifetime. The only guarantee offered by end is
    /// that it remains within allocated space and is initially logical space.
    inline void assign(uint8_t* begin, uint8_t* end) NOEXCEPT
    {
        begin_ = begin;
        end_ = end;
        ////BC_ASSERT(!system::is_negative(size()));
    }

    /// memory interface
    /// -----------------------------------------------------------------------

    /// Return an offset from begin, nullptr if end or past end.
    inline uint8_t* offset(size_t bytes) const NOEXCEPT override
    {
        if (system::is_greater(bytes, size()))
            return nullptr;

        BC_PUSH_WARNING(NO_POINTER_ARITHMETIC)
        return begin_ + bytes;
        BC_POP_WARNING()
        ////return std::next(begin_, bytes);
    }

    /// The logical buffer size (from begin to end).
    inline ptrdiff_t size() const NOEXCEPT override
    {
        BC_PUSH_WARNING(NO_POINTER_ARITHMETIC)
        return system::possible_narrow_and_sign_cast<ptrdiff_t>(end_ - begin_);
        BC_POP_WARNING()
        ////return std::distance(begin_, end_);
    }

    /// Alias for begin.
    inline uint8_t* data() const NOEXCEPT override
    {
        return begin();
    }

    /// Get logical buffer (guarded against remap only).
    inline operator system::data_slab() const NOEXCEPT override
    {
        return { begin(), end() };
    };

    /// Buffer start.
    inline uint8_t* begin() const NOEXCEPT override
    {
        return begin_;
    }

    /// Buffer end.
    inline uint8_t* end() const NOEXCEPT override
    {
        return end_;
    }

private:
    using lock = std::shared_lock<std::shared_mutex>;

    lock shared_lock_;
    uint8_t* begin_{};
    uint8_t* end_{};
};

} // namespace database
} // namespace libbitcoin

#endif
