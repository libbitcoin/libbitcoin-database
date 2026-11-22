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
#ifndef LIBBITCOIN_DATABASE_MEMORY_SETTINGS_HPP
#define LIBBITCOIN_DATABASE_MEMORY_SETTINGS_HPP

#include <bitcoin/database/define.hpp>
#include <bitcoin/database/file/file.hpp>

namespace libbitcoin {
namespace database {

/// Storage tuning consumed by memory map construction: base of table
/// configuration, as network settings bases are derived by server services.
/// Future staging tunables land here without constructor signature changes.
struct storage_settings
{
    /// Minimum body allocation (bytes).
    uint64_t size{ 1 };

    /// Body expansion rate (percentage).
    uint16_t rate{ 5 };

    /// Headroom (bytes): growth admitted only while it leaves this much of
    /// backing resource (memory commitment, disk space) unclaimed.
    uint64_t headroom{ system::power2(28u) };

    /// Unused physical memory restored by body cache release while store is
    /// current (bytes, zero disables).
    uint64_t reserve{};

    /// Resident process memory above which body cache is released while store
    /// is current (bytes, zero disables).
    uint64_t ceiling{};

    /// Page advice for mapped reads (see advice). Bodies default to
    /// scattered: they are far too large to reside, and validation
    /// reads prevouts from arbitrary earlier blocks, so read-ahead
    /// manufactures cache that is never used and displaces resident
    /// head set. Heads override to random (preloaded) at construction.
    /// Darwin fault clustering does not serve scattered prevout reads
    /// (measured ~4 pages per fault, one with this advice).
    advice access{ advice::scattered };

    /// Rows released by truncate are zeroed, so reallocation reads zero until
    /// written. Heads override to retain at construction: cell once within
    /// count holds its last store until next, so reader that took that count
    /// reads valid (possibly just-popped) link.
    bool zeroed{ true };

    /// Byte written over capacity above logical size, at load,
    /// on growth and in place of zeroing on truncate. Unset
    /// for dense tables, whose rows are written before read.
    std::optional<uint8_t> fill{};

    /// Clamp fixed width to size_t.
    size_t size_() const NOEXCEPT
    {
        return system::limit<size_t>(size);
    }

    /// Clamp fixed width to size_t.
    size_t headroom_() const NOEXCEPT
    {
        return system::limit<size_t>(headroom);
    }
};

} // namespace database
} // namespace libbitcoin

#endif
