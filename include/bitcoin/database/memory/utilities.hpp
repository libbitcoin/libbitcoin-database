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
#ifndef LIBBITCOIN_DATABASE_MEMORY_UTILITIES_HPP
#define LIBBITCOIN_DATABASE_MEMORY_UTILITIES_HPP

#include <atomic>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

/// The byte size of system pages, zero if failed.
BCD_API size_t page_size() NOEXCEPT;

/// The bytes of physical memory, zero if failed.
BCD_API uint64_t system_memory() NOEXCEPT;

/// The bytes of unused physical memory, zero if failed. Scarcity precedes
/// pressure: clean file cache is reclaimable, so the pressure level does not
/// raise while free memory exhausts.
BCD_API uint64_t system_free() NOEXCEPT;

/// The system memory pressure level (1 normal, 2 warning, 4 critical), zero
/// if failed or the platform provides no source.
BCD_API size_t system_pressure() NOEXCEPT;

/// The bytes held by system memory compression (a pressure precursor), zero
/// if failed or the platform provides no source.
BCD_API uint64_t system_compressed() NOEXCEPT;

/// C++26: std::atomic<size_t>::fetch_max
template <typename Integral, if_integral_integer<Integral> = true>
Integral fetch_max(std::atomic<Integral>& atomic, Integral value) NOEXCEPT
{
    constexpr auto relaxed = std::memory_order_relaxed;

    auto atom = atomic.load(relaxed);
    while (value > atom && !atomic.compare_exchange_weak(atom, value, relaxed))
    {
    }

    return atom;
}

} // namespace database
} // namespace libbitcoin

#endif
