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
#ifndef LIBBITCOIN_DATABASE_MEMORY_MSTAGE_HPP
#define LIBBITCOIN_DATABASE_MEMORY_MSTAGE_HPP

#include <bitcoin/database/define.hpp>

#if defined(HAVE_APPLE)
    #define MANAGE_STAGING
#endif

// The native windows mapped-file behavior is the model staging emulates.
#if defined(MANAGE_STAGING) && defined(HAVE_MSC)
    #error "MANAGE_STAGING is not supported on Windows."
#endif

#if defined(MANAGE_STAGING)
    #define STAGING_ONLY(name) name
#else
    #define STAGING_ONLY(name)
#endif

#if defined(MANAGE_STAGING)

/// Reserve inaccessible anonymous address space (MAP_FAILED on failure).
void* mmap_reserve(size_t size) NOEXCEPT;

/// Commit reserved pages as readable/writable anonymous memory.
int mmap_commit(void* address, size_t size) NOEXCEPT;

/// Replace committed pages with a read-only shared mapping of the file.
int mmap_settle(void* address, size_t size, int fd, size_t offset) NOEXCEPT;

/// Replace reserved pages with a writable shared mapping of the file.
int mmap_share(void* address, size_t size, int fd, size_t offset) NOEXCEPT;

/// Replace settled pages with committed anonymous memory (contents undefined).
int mmap_unsettle(void* address, size_t size) NOEXCEPT;

/// Release cached pages of a settled range (writes back any dirty first).
int mmap_evict(void* address, size_t size) NOEXCEPT;

/// Demote settled pages to reclaim-first order (cached until pressure).
int mmap_cold(void* address, size_t size) NOEXCEPT;

/// Name anonymous mappings in the range for diagnostics (no-op elsewhere).
int mmap_name(void* address, size_t size, const char* name) NOEXCEPT;

/// Discard unmapped page cache of a file (mapped pages are unaffected).
int file_discard(int fd) NOEXCEPT;

/// Atomically replace a released (read-only file-backed) range with writable
/// anonymous memory carrying its content (readers never observe zeros).
int mmap_restore(void* address, size_t size) NOEXCEPT;

/// Full-transfer positional file read/write (false on failure or early eof).
bool pread_all(int fd, uint8_t* to, size_t size, size_t offset) NOEXCEPT;
bool pwrite_all(int fd, const uint8_t* from, size_t size,
    size_t offset) NOEXCEPT;

#endif // MANAGE_STAGING

#endif
