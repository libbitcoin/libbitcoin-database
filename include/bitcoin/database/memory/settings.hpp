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

namespace libbitcoin {
namespace database {

/// Expected read pattern of mapped storage, guiding kernel page advice. This
/// is independent of the write pattern (structural): bodies are appended
/// sequentially but read randomly by validation, so advising the kernel from
/// the write pattern invites eviction of the read set under memory pressure.
enum class advice : uint8_t
{
    /// Let the operating system decide (mixed or unpredictable access).
    normal,

    /// Random access (suppresses read ahead).
    random,

    /// One pass access (allows the kernel to free pages behind).
    sequential
};

/// Storage tuning consumed by memory map construction: the base of table
/// configuration, as network settings bases are derived by server services.
/// Future staging tunables land here without constructor signature changes.
struct storage_settings
{
    /// Minimum body allocation (bytes).
    uint64_t size{ 1 };

    /// Body expansion rate (percentage).
    uint16_t rate{ 5 };

    /// Page advice for mapped reads (see advice).
    advice access{ advice::normal };
};

} // namespace database
} // namespace libbitcoin

#endif
