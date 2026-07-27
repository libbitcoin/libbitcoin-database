// mman_win32 based on code.google.com/p/mman-win32 (MIT License).

#ifndef LIBBITCOIN_DATABASE_MMAN_HPP
#define LIBBITCOIN_DATABASE_MMAN_HPP

#include <bitcoin/database/define.hpp>

#if !defined(HAVE_MSC)
    #include <fcntl.h>
    #include <sys/mman.h>
    #include <sys/stat.h>
    #include <sys/types.h>
    #include <unistd.h>
#endif

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
int mlock(const void* addr, size_t len) noexcept;
int munlock(const void* addr, size_t len) noexcept;
int fsync(int fd) noexcept;
int fallocate(int fd, int mode, oft__ offset, oft__ size) noexcept;
int ftruncate(int fd, oft__ size) noexcept;

#elif defined(HAVE_APPLE)

int fallocate(int fd, int, off_t offset, off_t len) NOEXCEPT;

#endif // HAVE_MSC

/// Exclude descriptor read/write from file system caching (advisory,
/// effective only on macOS). Staged bytes are read only through the map
/// once settled, so cache retention of the written pages can only displace
/// more useful pages.
#if defined(HAVE_APPLE)
inline int uncache(int fd) NOEXCEPT
{
    return ::fcntl(fd, F_NOCACHE, 1);
}
#else
inline int uncache(int) NOEXCEPT
{
    return 0;
}
#endif

/// Drop cached pages of a written range (advisory, effective only on Linux,
/// where uncache is a no-op). The sync makes the range clean so that the
/// eviction is not skipped, which only hastens settled write durability.
#if defined(HAVE_LINUX)
inline int evict(int fd, size_t offset, size_t size) NOEXCEPT
{
    return (::fdatasync(fd) == -1) ? -1 : ::posix_fadvise(fd,
        static_cast<off_t>(offset), static_cast<off_t>(size),
        POSIX_FADV_DONTNEED);
}
#else
inline int evict(int, size_t, size_t) NOEXCEPT
{
    return 0;
}
#endif

#endif
