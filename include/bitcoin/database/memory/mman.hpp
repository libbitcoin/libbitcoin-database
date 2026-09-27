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
#ifndef LIBBITCOIN_DATABASE_MMAN_HPP
#define LIBBITCOIN_DATABASE_MMAN_HPP

#include <bitcoin/database/define.hpp>

#if !defined(HAVE_MSC)
    #include <sys/mman.h>
    #include <sys/stat.h>
    #include <sys/types.h>
    #include <unistd.h>
#endif

// based on:
// code.google.com/p/mman-win32

#if defined(HAVE_MSC)

typedef size_t oft__;

#define PROT_NONE           0
#define PROT_READ           1
#define PROT_WRITE          2
#define PROT_EXEC           4

#define MAP_FILE            0
#define MAP_SYNC            0
#define MAP_SHARED          1
#define MAP_SHARED_VALIDATE MAP_SHARED
#define MAP_PRIVATE         2
#define MAP_TYPE            0xf
#define MAP_FIXED           0x10
#define MAP_ANONYMOUS       0x20
#define MAP_ANON            MAP_ANONYMOUS

#define MAP_FAILED      ((void*)-1)

// Flags for msync.
#define MS_ASYNC        1
#define MS_SYNC         2
#define MS_INVALIDATE   4

void* mmap(void* addr, size_t len, int prot, int flags, int fd,
    oft__ off) noexcept;
void* mremap_(void* addr, size_t old_size, size_t new_size, int prot,
    int flags, int fd) noexcept;
int munmap(void* addr, size_t len) noexcept;
int madvise(void* addr, size_t len, int advice) noexcept;
int mprotect(void* addr, size_t len, int prot) noexcept;
int msync(void* addr, size_t len, int flags) noexcept;
int working_floor(size_t minimum, size_t maximum) noexcept;
int mlock(const void* addr, size_t len) noexcept;
int munlock(const void* addr, size_t len) noexcept;
int fsync(int fd) noexcept;
int fallocate(int fd, int mode, oft__ offset, oft__ size) noexcept;
int ftruncate(int fd, oft__ size) noexcept;

#elif defined(HAVE_APPLE)

int fallocate(int fd, int, off_t offset, off_t len) NOEXCEPT;

#endif // HAVE_MSC

#endif
