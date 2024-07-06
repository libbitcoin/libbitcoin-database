// mman-win32 based on code.google.com/p/mman-win32 (MIT License).

#ifndef LIBBITCOIN_DATABASE_MMAN_HPP
#define LIBBITCOIN_DATABASE_MMAN_HPP

#ifdef _WIN32

#include <stddef.h>
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

// Flags for madvise (stub).
#define MADV_RANDOM     0

void* mmap(void* addr, size_t len, int prot, int flags, int fd, oft__ off) noexcept;
void* mremap_(void* addr, size_t old_size, size_t new_size, int prot,
    int flags, int fd) noexcept;
int munmap(void* addr, size_t len) noexcept;
int madvise(void* addr, size_t len, int advice) noexcept;
int mprotect(void* addr, size_t len, int prot) noexcept;
int msync(void* addr, size_t len, int flags) noexcept;
int mlock(const void* addr, size_t len) noexcept;
int munlock(const void* addr, size_t len) noexcept;
int fsync(int fd) noexcept;
int ftruncate(int fd, oft__ size) noexcept;

#endif // _WIN32
#endif
