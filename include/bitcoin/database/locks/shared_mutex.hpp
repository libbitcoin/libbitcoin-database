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
#ifndef LIBBITCOIN_DATABASE_LOCKS_SHARED_MUTEX_HPP
#define LIBBITCOIN_DATABASE_LOCKS_SHARED_MUTEX_HPP

#include <chrono>
#include <shared_mutex>
#include <bitcoin/database/define.hpp>

#if defined(HAVE_APPLE)
    #include <pthread.h>
#endif

namespace libbitcoin {
namespace database {

#if defined(HAVE_APPLE)

/// libc++ implements the std shared mutexes over a mutex, which serializes
/// readers. The posix reader-writer lock admits readers concurrently.
class BCD_API shared_mutex
{
public:
    shared_mutex() NOEXCEPT;
    ~shared_mutex() NOEXCEPT;
    shared_mutex(const shared_mutex&) = delete;
    shared_mutex& operator=(const shared_mutex&) = delete;

    void lock() NOEXCEPT;
    bool try_lock() NOEXCEPT;
    void unlock() NOEXCEPT;

    void lock_shared() NOEXCEPT;
    bool try_lock_shared() NOEXCEPT;
    void unlock_shared() NOEXCEPT;

    template <typename Rep, typename Period>
    bool try_lock_for(
        const std::chrono::duration<Rep, Period>& duration) NOEXCEPT
    {
        const auto deadline = std::chrono::steady_clock::now() + duration;
        while (!try_lock())
        {
            if (std::chrono::steady_clock::now() >= deadline)
                return false;

            pause();
        }

        return true;
    }

private:
    static void pause() NOEXCEPT;

    pthread_rwlock_t lock_{};
};

using shared_timed_mutex = shared_mutex;

#else

using shared_mutex = std::shared_mutex;
using shared_timed_mutex = std::shared_timed_mutex;

#endif

} // namespace database
} // namespace libbitcoin

#endif
