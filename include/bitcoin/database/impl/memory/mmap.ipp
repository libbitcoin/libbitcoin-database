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
#include <bitcoin/database/memory/utilities.hpp>

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

// Commitment growth target for the capacity slow paths. Unlike to_capacity
// this never floors to the configured minimum: under lazy commitment that
// floor would commit the full provisioning on first growth (the load failure
// this design exists to prevent, moved from create to first touch). Growth is
// chunked to bound slow path frequency, and clamped so that small tables do
// not over-commit (the provisioned file requires no memory until committed).
// The commit chunk is scaled to memory (bounded by commit_chunk), as the
// committed but unused overhang is otherwise up to one full chunk for every
// instance, an outsized share of a small system under memory pressure.
TEMPLATE
size_t CLASS::to_chunk() NOEXCEPT
{
    static const auto chunk = std::min(commit_chunk,
        system::possible_narrow_cast<size_t>(system_memory() / chunk_scale));
    return chunk;
}

TEMPLATE
size_t CLASS::to_growth(size_t required) const NOEXCEPT
{
#if defined(MANAGE_STAGING)
    using namespace system;
    const auto expand = ceilinged_multiply(required, expansion_) / 100u;
    const auto expanded = ceilinged_add(required, expand);
    const auto chunked = std::max(expanded,
        ceilinged_add(capacity_.load(), to_rows(to_chunk())));

    return std::min(chunked, std::max(expanded, to_provision()));
#else
    // The classic mapping is file-backed, so growth is capacity.
    return to_capacity(required);
#endif
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
    return std::min(to_provision(), std::max(logical, to_rows(to_chunk())));
#else
    // The classic mapping is file-backed, so commitment is provisioning.
    return to_provision();
#endif
}

// The counter chain is the spine of the design (debug assertion only):
//
//     settled_ <= frontier_ <= logical_ <= capacity_ <= file_
//
// settled: rows durable and converted (staged) or drained (unstaged).
// frontier: completed-write prefix bound (extent ring floor).
// logical: rows claimed by writers (fast path CAS).
// capacity: rows claimable (committed memory).
// file: rows provisioned on disk (fallocate extent).
//
// Lower bounds are read first: the lock-free fast path can advance logical_
// concurrently, so stale-low reads of lower bounds cannot falsify the chain,
// while upper bounds only grow under locks held at every call site.
TEMPLATE
void CLASS::check_invariants_() const NOEXCEPT
{
#if !defined(NDEBUG)
    if (!loaded_.load())
        return;

#if defined(MANAGE_STAGING)
    if (staged_)
    {
        const auto settled = settled_.load();
        const auto frontier = frontier_.load();
        BC_ASSERT(frontier <= logical_.load());
        BC_ASSERT(settled <= frontier);
    }
#endif

    const auto logical = logical_.load();
    const auto capacity = capacity_.load();
    BC_ASSERT(capacity <= file_.load());
    BC_ASSERT(logical <= capacity);
#endif // NDEBUG
}

// Write-write protected by remap_mutex.
TEMPLATE
void CLASS::set_first_code(const error::error_t& ec) NOEXCEPT
{
    if (!fault_.load())
    {
        fault_.store(true);
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
