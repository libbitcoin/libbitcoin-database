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
#if defined(HAVE_APPLE)
    #include <mach/mach.h>
    #include <mach/mach_vm.h>
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
    // Sync first: a settle write may not yet be written back, and bare
    // invalidation of a dirty page discards it (clean pages sync free).
#if defined(MADV_PAGEOUT)
    // MS_INVALIDATE does not drop clean page cache on linux (it invalidates
    // other mappings of the file), leaving the eviction sweep inert: body
    // cache pressure stands and the kernel preserves it by swapping the
    // anonymous heads. Pageout reclaims the mapped range (dirty pages write
    // back, clean pages drop, later reads fault back from the file).
    if (::msync(address, size, MS_SYNC) == -1)
        return -1;

    return ::madvise(address, size, MADV_PAGEOUT);
#else
    return ::msync(address, size, MS_SYNC | MS_INVALIDATE);
#endif
}

// Atomically install the source anonymous mapping over the target range,
// consuming the source (readers see old or new bytes, never zeros).
static int mmap_install(void* address, void* source, size_t size) NOEXCEPT
{
#if defined(HAVE_APPLE)
    auto target = reinterpret_cast<mach_vm_address_t>(address);
    vm_prot_t current{};
    vm_prot_t maximum{};
    if (::mach_vm_remap(::mach_task_self(), &target, size, 0,
        VM_FLAGS_FIXED | VM_FLAGS_OVERWRITE, ::mach_task_self(),
        reinterpret_cast<mach_vm_address_t>(source), TRUE, &current,
        &maximum, VM_INHERIT_DEFAULT) != KERN_SUCCESS)
        return -1;

    return ::munmap(source, size);
#else
    return ::mremap(source, size, size, MREMAP_MAYMOVE | MREMAP_FIXED,
        address) == MAP_FAILED ? -1 : 0;
#endif
}

int mmap_restore(void* address, size_t size) NOEXCEPT
{
    // Anonymous staging copy of the current (file) content.
    const auto source = ::mmap(nullptr, size, PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if (source == MAP_FAILED)
        return -1;

    std::memcpy(source, address, size);
    if (mmap_install(address, source, size) == -1)
    {
        ::munmap(source, size);
        return -1;
    }

    return 0;
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
