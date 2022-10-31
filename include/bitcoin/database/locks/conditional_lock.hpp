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
#ifndef LIBBITCOIN_DATABASE_LOCKS_CONDITIONAL_LOCK_HPP
#define LIBBITCOIN_DATABASE_LOCKS_CONDITIONAL_LOCK_HPP

#include <memory>
#include <boost/thread.hpp>
#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

class BCD_API conditional_lock final
{
public:
    DELETE4(conditional_lock);

    /// Conditional lock using internally-managed mutex pointer.
    conditional_lock(bool lock) NOEXCEPT;

    /// Conditional lock using parameterized mutex pointer (may be null).
    conditional_lock(std::shared_ptr<boost::shared_mutex> mutex_ptr) NOEXCEPT;

    /// Unlock.
    ~conditional_lock() NOEXCEPT;

private:
    const std::shared_ptr<boost::shared_mutex> mutex_ptr_;
};

} // namespace database
} // namespace libbitcoin

#endif
