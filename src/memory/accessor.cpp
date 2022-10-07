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
#include <bitcoin/database/memory/accessor.hpp>

#include <iterator>
#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

using namespace bc::system;

// locks, advance
BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

accessor::accessor(upgrade_mutex& mutex) NOEXCEPT
  : mutex_(mutex), data_(nullptr)
{
    ///////////////////////////////////////////////////////////////////////////
    // Begin Critical Section
    mutex_.lock_upgrade();
}

uint8_t* accessor::buffer() NOEXCEPT
{
    return data_;
}

void accessor::assign(uint8_t* data) NOEXCEPT
{
    //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    mutex_.unlock_upgrade_and_lock_shared();
    data_ = data;
}

void accessor::increment(size_t size) NOEXCEPT
{
    BC_ASSERT_MSG(!is_null(data_), "unassigned buffer");
    BC_ASSERT_MSG(reinterpret_cast<size_t>(data_) < max_size_t - size, "overflow");

    std::advance(data_, size);
}

accessor::~accessor() NOEXCEPT
{
    mutex_.unlock_shared();
    // End Critical Section
    ///////////////////////////////////////////////////////////////////////////
}

BC_POP_WARNING()

} // namespace database
} // namespace libbitcoin
