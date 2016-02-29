/**
 * Copyright (c) 2011-2015 libbitcoin developers (see AUTHORS)
 *
 * This file is part of libbitcoin.
 *
 * libbitcoin is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License with
 * additional permissions to the one published by the Free Software
 * Foundation, either version 3 of the License, or (at your option)
 * any later version. For more information see LICENSE.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef LIBBITCOIN_DATABASE_ACCESSOR_HPP
#define LIBBITCOIN_DATABASE_ACCESSOR_HPP

#include <cstdint>
#include <memory>
#include <boost/thread.hpp>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

/// This class provides remap safe read access to file-mapped memory.
/// The memory size is unprotected and unmanaged.
class BCD_API accessor
{
public:
    typedef std::shared_ptr<accessor> ptr;

    accessor(uint8_t* data, boost::shared_mutex& mutex);
    ~accessor();

    /// This class is not copyable.
    accessor(const accessor& other) = delete;

    /// Access the buffer.
    uint8_t* buffer();

private:
    uint8_t* data_;
    boost::shared_mutex& mutex_;
    boost::shared_lock<boost::shared_mutex> shared_lock_;
};

} // namespace database
} // namespace libbitcoin

#endif
