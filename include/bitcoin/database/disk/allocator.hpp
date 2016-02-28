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
#ifndef LIBBITCOIN_DATABASE_ALLOCATOR_HPP
#define LIBBITCOIN_DATABASE_ALLOCATOR_HPP

#include <cstdint>
#include <memory>
#include <boost/thread.hpp>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

/// This class provides remap safe write access to file-mapped memory.
/// The memory size is unprotected and unmanaged.
/// The mmfile class uses friend access to upgrade this lock during
/// initialization in the case where a remap is required.
class BCD_API allocator
{
public:
    typedef std::shared_ptr<allocator> ptr;
    typedef boost::upgrade_to_unique_lock<boost::shared_mutex> upgrade;

    allocator(uint8_t* data, boost::shared_mutex& mutex);
    ~allocator();

    /// This class is not copyable.
    allocator(const allocator& other) = delete;

    /// Access the allocated buffer.
    uint8_t* buffer();

protected:

    // Given mmfile public access to get_upgradeable and set_data.
    friend class mmfile;

    // Get the lock, for the purpose of upgrading to unique.
    // The lock should be upgraded during the set data call.
    boost::upgrade_lock<boost::shared_mutex>& get_upgradeable();

    // Modify the data member, not thread safe.
    // This must not be used following initialization of the class.
    void set_data(uint8_t* value);

private:
    uint8_t* data_;
    boost::shared_mutex& mutex_;
    boost::upgrade_lock<boost::shared_mutex> upgradeable_lock_;
};

} // namespace database
} // namespace libbitcoin

#endif
