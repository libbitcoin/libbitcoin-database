/**
 * Copyright (c) 2011-2026 libbitcoin developers (see AUTHORS)
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
#ifndef LIBBITCOIN_DATABASE_MEMORY_MMAP_IPP
#define LIBBITCOIN_DATABASE_MEMORY_MMAP_IPP

#include <algorithm>
#include <filesystem>
#include <shared_mutex>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/file/file.hpp>

namespace libbitcoin {
namespace database {

// Constructors.
// ----------------------------------------------------------------------------

TEMPLATE
CLASS::mmap(const path& filename, size_t minimum, size_t expansion,
    bool random) NOEXCEPT
    requires (columns == one)
  : filenames_{ filename },
    minimum_(minimum),
    expansion_(expansion),
    random_(random),
    opened_{ file::invalid }
{
}

TEMPLATE
CLASS::mmap(const paths& filenames, size_t minimum, size_t expansion,
    bool random) NOEXCEPT
    requires (columns > one)
  : filenames_(filenames),
    minimum_(minimum),
    expansion_(expansion),
    random_(random),
    opened_{}
{
    opened_.fill(file::invalid);
}

TEMPLATE
CLASS::~mmap() NOEXCEPT
{
    BC_ASSERT_MSG(!loaded_, "...");
    BC_ASSERT_MSG(is_zero(logical_), "...");

    BC_ASSERT_MSG(std::ranges::all_of(memory_map_,
        [](auto map) NOEXCEPT { return is_null(map); }), "...");

    BC_ASSERT_MSG(std::ranges::all_of(capacity_,
        [](auto closed) NOEXCEPT { return is_zero(closed); }), "...");

    BC_ASSERT_MSG(std::ranges::all_of(opened_,
        [](auto opened) NOEXCEPT { return opened == file::invalid; }), "...");
}

TEMPLATE
bool CLASS::is_open() const NOEXCEPT
{
    std::shared_lock field_lock(field_mutex_);
    return opened_.front() != file::invalid;
}

TEMPLATE
bool CLASS::is_loaded() const NOEXCEPT
{
    std::shared_lock field_lock(field_mutex_);
    return loaded_;
}

// protected
// ----------------------------------------------------------------------------

TEMPLATE
size_t CLASS::to_capacity(size_t required) const NOEXCEPT
{
    BC_PUSH_WARNING(NO_STATIC_CAST)
    const auto resize = required * ((expansion_ + 100.0) / 100.0);
    const auto target = std::max(minimum_, static_cast<size_t>(resize));
    BC_POP_WARNING()

    BC_ASSERT(target >= required);
    return target;
}

// Read-write protected by atomic, write-write protected by remap_mutex.
TEMPLATE
void CLASS::set_first_code(const error::error_t& ec) NOEXCEPT
{
    if (!fault_)
    {
        // fault is not exposed so requires no atomic (fast read).
        fault_ = true;

        // error is atomic for public read exposure.
        error_.store(ec);
    }
}

TEMPLATE
void CLASS::set_disk_space(size_t required) NOEXCEPT
{
    space_.store(required);
}

} // namespace database
} // namespace libbitcoin

#endif
