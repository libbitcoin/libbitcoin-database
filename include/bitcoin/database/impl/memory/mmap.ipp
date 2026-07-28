/**
 * Copyright (c) 2011-2026 libbitcoin developers
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
CLASS::mmap(const path& filename, const storage_settings& settings,
    bool random, bool staged) NOEXCEPT
    requires (is_one(columns))
  : filenames_{ filename },
    minimum_(to_rows(settings.size)),
    expansion_(settings.rate),
    access_(settings.access),
    random_(random),
    staged_(staged),
    opened_{ file::invalid }
{
}

TEMPLATE
CLASS::mmap(const paths& filenames, const storage_settings& settings,
    bool random, bool staged) NOEXCEPT
    requires (columns > one)
  : filenames_(filenames),
    minimum_(to_rows(settings.size)),
    expansion_(settings.rate),
    access_(settings.access),
    random_(random),
    staged_(staged),
    opened_{}
{
    opened_.fill(file::invalid);
}

TEMPLATE
CLASS::~mmap() NOEXCEPT
{
#if defined(MANAGE_STAGING)
    // Join a settler left running by an unload bypass (thread safety).
    settler_stop_();
#endif

    BC_ASSERT(!loaded_.load());
    BC_ASSERT(is_zero(logical_.load()));
    BC_ASSERT(is_zero(capacity_.load()));
    BC_ASSERT(std::ranges::all_of(memory_map_,
        [](auto map) NOEXCEPT { return is_null(map); }));
    BC_ASSERT(std::ranges::all_of(opened_,
        [](auto opened) NOEXCEPT { return opened == file::invalid; }));
#if defined(MANAGE_STAGING)
    BC_ASSERT(std::ranges::all_of(reserved_,
        [](auto reserved) NOEXCEPT { return is_zero(reserved); }));
#endif
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

// Disk provisioning: the configured minimum is a file reservation, ensuring
// that allocation within it cannot fail for space (disk full is detected at
// provisioning, where it remains recoverable).
TEMPLATE
size_t CLASS::to_provision() const NOEXCEPT
{
    return std::max(logical_.load(), minimum_);
}

// Memory commitment follows use, not provisioning. Committing the configured
// minimum would demand that much memory (Linux charges the commit) before any
// row is written, failing load on any machine smaller than its store sizing.
// Growth commits in chunks within the standing reservation (no remap), so the
// cost is a syscall per chunk over the life of the store.
TEMPLATE
size_t CLASS::to_commitment() const NOEXCEPT
{
#if defined(MANAGE_STAGING)
    const auto logical = logical_.load();
    return std::min(to_provision(), std::max(logical, to_rows(commit_chunk)));
#else
    // The classic mapping is file-backed, so commitment is provisioning.
    return to_provision();
#endif
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

#if defined(MANAGE_STAGING)
        // A fault may stop draining, so it must release throttled callers.
        signal_();
#endif
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
