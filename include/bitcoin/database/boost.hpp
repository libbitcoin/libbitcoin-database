/**
 * Copyright (c) 2011-2025 libbitcoin developers (see AUTHORS)
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
#ifndef LIBBITCOIN_DATABASE_BOOST_HPP
#define LIBBITCOIN_DATABASE_BOOST_HPP

// Must pull in any base boost configuration before including boost.
#include <bitcoin/system.hpp>

#include <filesystem>
#include <boost/interprocess/detail/os_file_functions.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/key.hpp>
#include <boost/multi_index/key_extractors.hpp>
#include <boost/multi_index/ordered_index.hpp>

namespace libbitcoin {
namespace database {

namespace interprocess = boost::interprocess;
namespace ipcdetail = interprocess::ipcdetail;
using file_handle_t = interprocess::file_handle_t;

#if defined(HAVE_MSC)
const file_handle_t invalid = interprocess::winapi::invalid_handle_value;
#else
const file_handle_t invalid = -1;
#endif

inline file_handle_t open_existing_file(
    const std::filesystem::path& file) NOEXCEPT
{
    constexpr auto mode = interprocess::read_write;
    const auto filename = system::extended_path(file);

    // Does not throw (unannotated).
    BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
    return ipcdetail::open_existing_file(filename.c_str(), mode);
    BC_POP_WARNING()
}

} // namespace database
} // namespace libbitcoin

#endif
