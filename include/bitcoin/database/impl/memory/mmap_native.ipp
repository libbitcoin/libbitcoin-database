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
#ifndef LIBBITCOIN_DATABASE_MEMORY_MMAP_NATIVE_IPP
#define LIBBITCOIN_DATABASE_MEMORY_MMAP_NATIVE_IPP

#include <algorithm>
#include <chrono>
#include <shared_mutex>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/mman.hpp>
#include <bitcoin/database/memory/utilities.hpp>

#if defined(HAVE_MSC)

namespace libbitcoin {
namespace database {

TEMPLATE
void CLASS::scanner_start_() NOEXCEPT
{
    touched_ = zero;
    unlocked_ = zero;
    scanning_.store(true);
    scanner_ = std::thread([this]() NOEXCEPT
    {
        scanner_run_();
    });
}

TEMPLATE
void CLASS::scanner_stop_() NOEXCEPT
{
    if (!scanning_.exchange(false))
        return;

    scanner_cv_.notify_all();
    if (scanner_.joinable())
        scanner_.join();
}

// Heads assert residency, bodies lead the trim. Both are advisory: failure
// alters nothing and is not a fault (unlike the staged eviction primitives,
// which own persistence).
TEMPLATE
void CLASS::scanner_run_() NOEXCEPT
{
    using namespace system;
    const auto page = page_size();
    if (is_zero(page))
        return;

    const auto span = std::max(one, evict_chunk / stride);

    while (scanning_.load())
    {
        std::unique_lock scanner_lock(scanner_mutex_);
        scanner_cv_.wait_for(scanner_lock, std::chrono::seconds(1));
        scanner_lock.unlock();

        if (!scanning_.load())
            return;

        // Steering is only useful against a trim, and a trim only happens
        // under contention, so at plenty the tick is a counter test. This
        // also bounds the idle cost to the wait itself.
        if (system_available() >= (system_memory() / sweep_factor))
            continue;

        std::shared_lock map_lock(remap_mutex_);
        if (!loaded_.load() || fault_.load())
            continue;

        if (staged_)
        {
            // Unlock a wrapping lap of the oldest rows (offset is write age
            // in an append-only body, and re-read probability decays with
            // it), so the trim takes these before the head set.
            const auto rows = logical_.load();
            const auto from = (unlocked_ < rows) ? unlocked_ : zero;
            const auto to = std::min(rows, ceilinged_add(from, span));
            if (to <= from)
                continue;

            const auto at = to_width<zero>(from);
            /* bool */ ::munlock(std::next(memory_map_[zero], at),
                to_width<zero>(to) - at);

            unlocked_ = (to == rows) ? zero : to;
        }
        else
        {
            // Revisit every head page each touch_seconds regardless of
            // instance size. A read sets the access bit without dirtying
            // the page; volatile prevents elision.
            const auto pages = to_width<zero>(logical_.load()) / page;
            const volatile auto* map = memory_map_[zero];
            auto budget = ceilinged_divide(pages, touch_seconds);

            while (!is_zero(pages) && !is_zero(budget))
            {
                if (touched_ >= pages)
                    touched_ = zero;

                const auto at = touched_ * page;
                const auto count = std::min(
                    { touch_span, pages - touched_, budget });

                for (size_t index{}; index < count; ++index)
                    (void)map[at + index * page];

                touched_ += count;
                budget = floored_subtract(budget, count);
            }
        }
    }
}

} // namespace database
} // namespace libbitcoin

#endif // HAVE_MSC

#endif
