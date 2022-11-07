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
#ifndef LIBBITCOIN_DATABASE_LOCKS_SCOPE_LOCK_HPP
#define LIBBITCOIN_DATABASE_LOCKS_SCOPE_LOCK_HPP

#include <memory>
#include <shared_mutex>
#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

/// This class is thread safe.
/// Reserve exclusive access to a resource while this object is in scope.
class BCD_API scope_lock final
{
public:
    DELETE4(scope_lock);

    typedef std::shared_ptr<scope_lock> ptr;

    /// Lock using the specified mutex reference.
    scope_lock(std::shared_mutex& mutex) NOEXCEPT;

    /// Unlock.
    ~scope_lock() NOEXCEPT;

private:
    std::shared_mutex& mutex_;
};

} // namespace database
} // namespace libbitcoin

#endif
