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
#ifndef LIBBITCOIN_DATABASE_LOCKS_SEQUENTIAL_LOCK_HPP
#define LIBBITCOIN_DATABASE_LOCKS_SEQUENTIAL_LOCK_HPP

#include <atomic>
#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

/// This class is thread safe.
/// Encapsulation of sequential locking conditions.
class BCD_API sequential_lock
{
public:
    typedef size_t handle;

    static constexpr bool is_write_locked(handle value) NOEXCEPT
    {
        return is_odd(value);
    }

    sequential_lock() NOEXCEPT;

    handle begin_read() const NOEXCEPT;
    bool is_read_valid(handle value) const NOEXCEPT;

    bool begin_write() NOEXCEPT;
    bool end_write() NOEXCEPT;

private:
    std::atomic<size_t> sequence_;
};

} // namespace database
} // namespace libbitcoin

#endif
