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
#include <bitcoin/database/locks/shared_mutex.hpp>

#if defined(HAVE_APPLE)

#include <cerrno>
#include <pthread.h>
#include <time.h>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

shared_mutex::shared_mutex() NOEXCEPT
{
    ::pthread_rwlock_init(&lock_, nullptr);
}

shared_mutex::~shared_mutex() NOEXCEPT
{
    ::pthread_rwlock_destroy(&lock_);
}

void shared_mutex::lock() NOEXCEPT
{
    ::pthread_rwlock_wrlock(&lock_);
}

bool shared_mutex::try_lock() NOEXCEPT
{
    return ::pthread_rwlock_trywrlock(&lock_) == 0;
}

void shared_mutex::unlock() NOEXCEPT
{
    ::pthread_rwlock_unlock(&lock_);
}

void shared_mutex::lock_shared() NOEXCEPT
{
    while (::pthread_rwlock_rdlock(&lock_) == EAGAIN)
    {
    }
}

bool shared_mutex::try_lock_shared() NOEXCEPT
{
    return ::pthread_rwlock_tryrdlock(&lock_) == 0;
}

void shared_mutex::unlock_shared() NOEXCEPT
{
    ::pthread_rwlock_unlock(&lock_);
}

void shared_mutex::pause() NOEXCEPT
{
    constexpr timespec interval{ 0, 1'000'000 };
    ::nanosleep(&interval, nullptr);
}

} // namespace database
} // namespace libbitcoin

#endif
