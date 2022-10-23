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
#ifndef LIBBITCOIN_DATABASE_MEMORY_ACCESSOR_IPP
#define LIBBITCOIN_DATABASE_MEMORY_ACCESSOR_IPP

#include <iterator>
#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {


template <typename Mutex>
accessor<Mutex>::accessor(Mutex& mutex) NOEXCEPT
  : data_(nullptr), shared_lock_(mutex)
{
}

template <typename Mutex>
void accessor<Mutex>::assign(uint8_t* data) NOEXCEPT
{
    BC_ASSERT_MSG(!is_null(data), "null buffer");
    data_ = data;
}

template <typename Mutex>
uint8_t* accessor<Mutex>::data() NOEXCEPT
{
    return data_;
}

template <typename Mutex>
void accessor<Mutex>::increment(size_t size) NOEXCEPT
{
    BC_ASSERT_MSG(!is_null(data_), "unassigned buffer");
    BC_ASSERT(reinterpret_cast<size_t>(data_) < max_size_t - size);
    BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
    std::advance(data_, size);
    BC_POP_WARNING()
}

} // namespace database
} // namespace libbitcoin

#endif
