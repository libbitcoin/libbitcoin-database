/**
 * Copyright (c) 2011-2019 libbitcoin developers (see AUTHORS)
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
#ifndef LIBBITCOIN_DATABASE_PRIMITIVES_RECORD_MANAGER_IPP
#define LIBBITCOIN_DATABASE_PRIMITIVES_RECORD_MANAGER_IPP

#include <mutex>
#include <shared_mutex>
#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/memory.hpp>
#include <bitcoin/database/memory/storage.hpp>

namespace libbitcoin {
namespace database {
namespace primitives {
    
TEMPLATE
CLASS::record_manager(Storage& file) NOEXCEPT
  : file_(file), count_(position_to_link(file_.logical()))
{
}

TEMPLATE
Link CLASS::size() const NOEXCEPT
{
    std::shared_lock lock(mutex_);
    return file_.is_closed() ? eof : count_;
}

TEMPLATE
bool CLASS::truncate(Link value) NOEXCEPT
{
    std::unique_lock lock(mutex_);

    if ((value < count_) && file_.resize(link_to_position(value)))
    {
        count_ = value;
        return true;
    }

    return false;
}

TEMPLATE
Link CLASS::allocate(size_t count) NOEXCEPT
{
    BC_ASSERT_MSG(count < eof, "count excess");
    const auto records = system::possible_narrow_cast<Link>(count);

    std::unique_lock lock(mutex_);
    BC_ASSERT_MSG(!system::is_add_overflow(count_, records), "count overflow");

    if (!file_.reserve(link_to_position(count_ + records)))
        return eof;

    const auto next = count_;
    count_ += records;
    return next;
}

TEMPLATE
memory_ptr CLASS::get(Link link) const NOEXCEPT
{
    BC_ASSERT_MSG(link < eof, "link excess");
    const auto memory = file_.access();

    if (memory)
        memory->increment(link_to_position(link));

    return memory;
}

} // namespace primitives
} // namespace database
} // namespace libbitcoin

#endif
