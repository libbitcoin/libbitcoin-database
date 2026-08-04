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
#include <algorithm>
#include <cerrno>
#include <cstring>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/mstage.hpp>

#if defined(MANAGE_STAGING)

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#if defined(HAVE_LINUX)
    #include <sys/prctl.h>
#endif

using namespace libbitcoin;
using namespace libbitcoin::system;
static constexpr auto transfer_chunk = power2(30u);

void* mmap_reserve(size_t size) NOEXCEPT
{
    return ::mmap(nullptr, size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS,
        -1, 0);
}

int mmap_commit(void* address, size_t size) NOEXCEPT
{
    return ::mprotect(address, size, PROT_READ | PROT_WRITE);
}

int mmap_settle(void* address, size_t size, int fd, size_t offset) NOEXCEPT
{
    return ::mmap(address, size, PROT_READ, MAP_SHARED | MAP_FIXED, fd,
        possible_narrow_sign_cast<off_t>(offset)) == MAP_FAILED ? -1 : 0;
}

// The lazy writer duplicates the head in page cache: every transferred page
// is written to the file, and the cached copy of that write is redundant
// while the anonymous page remains authoritative (the head is resident, not
// read back). Undiscarded, that duplicate doubles the head footprint and
// exhausts memory, which the kernel resolves by swapping the live copy.
// Mapped pages (released runs, where the file is the live copy) hold a
// reference and are not discarded. Dirty pages are not discarded either, so
// a page discards on the pass following its writeback.
int file_discard(int fd) NOEXCEPT
{
#if defined(POSIX_FADV_DONTNEED)
    return ::posix_fadvise(fd, 0, 0, POSIX_FADV_DONTNEED);
#else
    return is_zero(fd) ? 0 : 0;
#endif
}

int mmap_cold(void* address, size_t size) NOEXCEPT
{
#if defined(MADV_COLD)
    // Deactivation expresses the residency priority the kernel cannot infer:
    // settled bodies stay cached for imminent re-read (validation follows
    // archival) but reclaim first under pressure, before active pages and
    // anonymous heads (which otherwise swap to preserve the body cache). A
    // read reactivates, so genuinely hot pages promote themselves back.
    return ::madvise(address, size, MADV_COLD);
#else
    return 0;
#endif
}

// Diagnostic attribution: names each anonymous vma in the range so smaps and
// per-process accounting decompose by table (heads vs staged bodies vs
// foreign allocations). Refusal (pre-5.17 kernel, name policy) is not an
// error: attribution is best-effort and alters no behavior.
#if defined(PR_SET_VMA) && defined(PR_SET_VMA_ANON_NAME)
int mmap_name(void* address, size_t size, const char* name) NOEXCEPT
{
    ::prctl(PR_SET_VMA, PR_SET_VMA_ANON_NAME, address, size, name);
    return 0;
}
#else
int mmap_name(void*, size_t, const char*) NOEXCEPT
{
    return 0;
}
#endif

// Residency probe for the touch pass guard (the touch must never fault).
int mmap_resident(const void* address, size_t size,
    unsigned char* vector) NOEXCEPT
{
#if defined(HAVE_APPLE)
    return ::mincore(const_cast<void*>(address), size,
        reinterpret_cast<char*>(vector));
#else
    return ::mincore(const_cast<void*>(address), size, vector);
#endif
}

int mmap_share(void* address, size_t size, int fd, size_t offset) NOEXCEPT
{
    return ::mmap(address, size, PROT_READ | PROT_WRITE, MAP_SHARED |
        MAP_FIXED, fd, possible_narrow_sign_cast<off_t>(offset)) == MAP_FAILED ?
        -1 : 0;
}

int mmap_unsettle(void* address, size_t size) NOEXCEPT
{
    // No reserve: an unsettled span can span hundreds of gigabytes (truncate
    // reverting settled rows for re-append), which heuristic overcommit
    // refuses as a single anonymous commitment; pages charge on touch.
#if defined(MAP_NORESERVE)
    constexpr auto flags = MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED |
        MAP_NORESERVE;
#else
    constexpr auto flags = MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED;
#endif

    return ::mmap(address, size, PROT_READ | PROT_WRITE, flags, -1, 0)
        == MAP_FAILED ? -1 : 0;
}

int mmap_evict(void* address, size_t size) NOEXCEPT
{
#if defined(MADV_PAGEOUT)
    // MS_INVALIDATE does not drop clean page cache on linux (it invalidates
    // other mappings of the file), leaving the eviction sweep inert: body
    // cache pressure stands and the kernel preserves it by swapping the
    // anonymous heads. Pageout reclaims the mapped range (dirty pages write
    // back, clean pages drop, later reads fault back from the file). No
    // sync first: pageout never discards dirty content (unlike bare
    // invalidation), and a synchronous flush here serializes the settle
    // write path behind a device flush barrier on every sweep (durability
    // remains the clean close).
    return ::madvise(address, size, MADV_PAGEOUT);
#else
    // Sync first: a settle write may not yet be written back, and bare
    // invalidation of a dirty page discards it (clean pages sync free).
    return ::msync(address, size, MS_SYNC | MS_INVALIDATE);
#endif
}

bool pread_all(int fd, uint8_t* to, size_t size, size_t offset) NOEXCEPT
{
    while (!is_zero(size))
    {
        const auto request = std::min(size, transfer_chunk);
        const auto result = ::pread(fd, to, request,
            possible_narrow_sign_cast<off_t>(offset));

        if (is_negative(result))
        {
            if (errno == EINTR)
                continue;

            return false;
        }

        // Early eof implies the requested range exceeds the file.
        if (is_zero(result))
            return false;

        const auto bytes = possible_narrow_sign_cast<size_t>(result);
        to += bytes;
        offset += bytes;
        size -= bytes;
    }

    return true;
}

bool pwrite_all(int fd, const uint8_t* from, size_t size,
    size_t offset) NOEXCEPT
{
    while (!is_zero(size))
    {
        const auto request = std::min(size, transfer_chunk);
        const auto result = ::pwrite(fd, from, request,
            possible_narrow_sign_cast<off_t>(offset));

        if (result <= 0)
        {
            if (is_negative(result) && (errno == EINTR))
                continue;

            return false;
        }

        const auto bytes = possible_narrow_sign_cast<size_t>(result);
        from += bytes;
        offset += bytes;
        size -= bytes;
    }

    return true;
}

#endif // MANAGE_STAGING
