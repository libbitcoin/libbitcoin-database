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
    requires (is_one(columns))
  : filenames_{ filename },
    minimum_(to_rows(minimum)),
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
    minimum_(to_rows(minimum)),
    expansion_(expansion),
    random_(random),
    opened_{}
{
    opened_.fill(file::invalid);
}

TEMPLATE
CLASS::~mmap() NOEXCEPT
{
    BC_ASSERT(!loaded_.load());
    BC_ASSERT(is_zero(logical_.load()));
    BC_ASSERT(is_zero(capacity_.load()));
    BC_ASSERT(std::ranges::all_of(memory_map_,
        [](auto map) NOEXCEPT { return is_null(map); }));
    BC_ASSERT(std::ranges::all_of(opened_,
        [](auto opened) NOEXCEPT { return opened == file::invalid; }));
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
    return loaded_.load();
}

// protected
// ----------------------------------------------------------------------------

TEMPLATE
size_t CLASS::to_capacity(size_t required) const NOEXCEPT
{
    // Covert required rows to capacity-padded rows.
    using namespace system;
    const auto growth = ceilinged_multiply(required, expansion_) / 100u;
    return std::max(minimum_, ceilinged_add(required, growth));
}

// Write-write protected by remap_mutex.
TEMPLATE
void CLASS::set_first_code(const error::error_t& ec) NOEXCEPT
{
    if (!fault_.load())
    {
        fault_.store(true);

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
