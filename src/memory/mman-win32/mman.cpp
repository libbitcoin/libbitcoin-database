// mman-win32 based on code.google.com/p/mman-win32 (MIT License).

#include "mman.hpp"

#ifdef _WIN32

#include <stdint.h>
#include <windows.h>
#include <errno.h>
#include <io.h>

// local utilities

static constexpr int large = (sizeof(oft__) > sizeof(DWORD));

static int last_error(int default_value) noexcept
{
    // TODO: implement full mapping to standard codes.

    switch (GetLastError())
    {
        case ERROR_INVALID_HANDLE:
            return EBADF;
        case ERROR_DISK_FULL:
            return ENOSPC;
        default:
            return default_value;
    }
}

static DWORD protect_page(int prot) noexcept
{
    if (prot == PROT_NONE)
        return 0;

    if ((prot & PROT_EXEC) != 0)
    {
        return ((prot & PROT_WRITE) != 0) ?
            PAGE_EXECUTE_READWRITE : PAGE_EXECUTE_READ;
    }
    else
    {
        return ((prot & PROT_WRITE) != 0) ?
            PAGE_READWRITE : PAGE_READONLY;
    }
}

static DWORD protect_file(int prot) noexcept
{
    DWORD desired_access = 0;

    if (prot == PROT_NONE)
        return desired_access;

    if ((prot & PROT_READ) != 0)
        desired_access |= FILE_MAP_READ;

    if ((prot & PROT_WRITE) != 0)
        desired_access |= FILE_MAP_WRITE;

    if ((prot & PROT_EXEC) != 0)
        desired_access |= FILE_MAP_EXECUTE;

    return desired_access;
}

// public interface

void* mmap(void* /*addr*/, size_t len, int prot, int flags, int fd,
    oft__ off) noexcept
{
    const auto access  = protect_file(prot);
    const auto protect = protect_page(prot);

    const oft__ max = off + (oft__)len;

    // 32 bit builds: >> shift too big, undefined behavior (oft__ is size_t).
    const auto max_lo  = large ? (DWORD)((max)       & MAXDWORD) : (DWORD)max;
    const auto max_hi  = large ? (DWORD)((max >> 32) & MAXDWORD) : (DWORD)0;
    const auto file_lo = large ? (DWORD)((off)       & MAXDWORD) : (DWORD)off;
    const auto file_hi = large ? (DWORD)((off >> 32) & MAXDWORD) : (DWORD)0;

    // Disallow fixed, executable, and or anonymous mapping.
    if (len == 0 ||
        (prot == PROT_EXEC) ||
        (flags & MAP_FIXED) != 0 ||
        (flags & MAP_ANONYMOUS) != 0)
    {
        errno = EINVAL;
        return MAP_FAILED;
    }

    // Never call CloseHandle on the return value of this function.
    const auto handle = (HANDLE)_get_osfhandle(fd);
    if (handle == INVALID_HANDLE_VALUE)
    {
        errno = EBADF;
        return MAP_FAILED;
    }

    const auto mapping = CreateFileMappingW(handle, NULL, protect, max_hi, max_lo, NULL);
    if (mapping == NULL)
    {
        errno = last_error(EPERM);
        return MAP_FAILED;
    }

    // "to fully close a file mapping object, an application must unmap all
    // mapped views of the file mapping object by calling UnmapViewOfFile and
    // close the file mapping object handle by calling CloseHandle. These
    // functions can be called in any order." - so we close the handle here and
    // retain only the map.
    const auto map = MapViewOfFile(mapping, access, file_hi, file_lo, len);
    if (map == NULL || CloseHandle(mapping) == FALSE)
    {
        errno = last_error(EPERM);
        return MAP_FAILED;
    }

    errno = 0;
    return map;
}

// HACK: fd parameter used to pass file descripter, not linux compatible.
// HACK: No mremap equivalent, munmap and mmap are required to change size.
void* mremap_(void* addr, size_t old_size, size_t new_size, int prot,
    int flags, int fd) noexcept
{
    if (munmap(addr, old_size) == -1)
        return MAP_FAILED;

    return mmap(NULL, new_size, prot, flags, fd, 0);
}

int munmap(void* addr, size_t) noexcept
{
    if (UnmapViewOfFile(addr) == FALSE)
    {
        errno = last_error(EPERM);
        return -1;
    }

    errno = 0;
    return 0;
}

int madvise(void*, size_t, int) noexcept
{
    // Not implemented.
    errno = 0;
    return 0;
}

// unused
int mprotect(void* addr, size_t len, int prot) noexcept
{
    DWORD old_protect = 0;
    const auto new_protect = protect_page(prot);
    if (VirtualProtect(addr, len, new_protect, &old_protect) == FALSE)
    {
        errno = last_error(EPERM);
        return -1;
    }

    errno = 0;
    return 0;
}

// The FlushViewOfFile function does not flush the file metadata, and it does
// not wait to return until the changes are flushed from the underlying hardware
// disk cache and physically written to disk. To flush all the dirty pages plus
// the metadata for the file and ensure that they are physically written to disk,
// call FlushViewOfFile and then call the FlushFileBuffers function [fsync].
int msync(void* addr, size_t len, int) noexcept
{
    if ((len != 0) && FlushViewOfFile(addr, len) == FALSE)
    {
        errno = last_error(EPERM);
        return -1;
    }

    errno = 0;
    return 0;
}

// unused
int mlock(const void* addr, size_t len) noexcept
{
    if (VirtualLock((LPVOID)addr, len) == FALSE)
    {
        errno = last_error(EPERM);
        return -1;
    }

    errno = 0;
    return 0;
}

// unused
int munlock(const void* addr, size_t len) noexcept
{
    if (VirtualUnlock((LPVOID)addr, len) == FALSE)
    {
        errno = last_error(EPERM);
        return -1;
    }

    errno = 0;
    return 0;
}

int fsync(int fd) noexcept
{
    // Never call CloseHandle on the return value of this function.
    const auto handle = (HANDLE)(_get_osfhandle(fd));
    if (FlushFileBuffers(handle) == FALSE)
    {
        errno = last_error(EPERM);
        return -1;
    }

    errno = 0;
    return 0;
}

int fallocate(int fd, int, oft__ offset, oft__ size) noexcept
{
    return ftruncate(fd, offset + size);
}

int ftruncate(int fd, oft__ size) noexcept
{
    LARGE_INTEGER big{};

    if (fd < 0)
    {
        errno = EBADF;
        return -1;
    }

    // guard against overflow from unsigned to signed
    if (size >= (uint64_t)(large ? MAXINT64 : MAXINT32))
    {
        errno = EFBIG;
        return -1;
    }

    // Never call CloseHandle on the return value of this function.
    const auto handle = (HANDLE)_get_osfhandle(fd);
    if (handle == INVALID_HANDLE_VALUE)
    {
        // sets errno
        return -1;
    }

    big.QuadPart = (LONGLONG)size;
    if (SetFilePointerEx(handle, big, NULL, FILE_BEGIN) == FALSE)
    {
        // sets errno
        return -1;
    }

    // "UnmapViewOfFile must be called first to unmap all views and call
    // CloseHandle to close file mapping object before can call SetEndOfFile."
    // We have earlier called CloseHandle to close file mapping object (in mmap)
    // but have not called UnmapViewOfFile before calling this from remap_() and
    // it is apparently working.

    // This sets the physical size of the file, making this an fallocate.
    if (SetEndOfFile(handle) == FALSE)
    {
        errno = last_error(EIO);
        return -1;
    }

    errno = 0;
    return 0;
}

// stub
int sysconf(int) noexcept
{
    return {};
}

#endif // _WIN32
