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
#include <bitcoin/database/memory/utilities.hpp>

#if defined(HAVE_MSC)
    #include <windows.h>
    #include <winioctl.h>
#else
    #include <unistd.h>
#endif
#if defined(HAVE_APPLE)
    #include <dlfcn.h>
    #include <mach/mach.h>
    #include <sys/sysctl.h>
#endif
#if defined(HAVE_LINUX)
    #include <algorithm>
    #include <cinttypes>
    #include <cstdio>
    #include <fstream>
    #include <sys/stat.h>
    #include <sys/sysmacros.h>
    #include <dlfcn.h>
    #include <sys/vfs.h>
#endif
#include <charconv>
#include <filesystem>
#include <system_error>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

using namespace system;

#if defined(HAVE_MSC)

size_t page_size() NOEXCEPT
{
    SYSTEM_INFO info{};
    ::GetSystemInfo(&info);

    BC_ASSERT(!is_limited<size_t>(info.dwPageSize));
    return info.dwPageSize;
}

uint64_t system_memory() NOEXCEPT
{
    MEMORYSTATUSEX status{};
    status.dwLength = sizeof(status);
    return is_zero(::GlobalMemoryStatusEx(&status)) ? zero :
        status.ullTotalPhys;
}

uint64_t physical_memory() NOEXCEPT
{
    // Total physical excludes only hardware reservations, which rounding up
    // to a whole gibibyte absorbs.
    constexpr auto gibibyte = power2<uint64_t>(30u);
    const auto memory = system_memory();
    return ceilinged_multiply(ceilinged_divide(memory, gibibyte), gibibyte);
}

uint64_t system_free() NOEXCEPT
{
    MEMORYSTATUSEX status{};
    status.dwLength = sizeof(status);
    return is_zero(::GlobalMemoryStatusEx(&status)) ? zero :
        status.ullAvailPhys;
}

uint64_t system_available() NOEXCEPT
{
    // Windows available physical memory is the correct semantic directly.
    return system_free();
}

size_t system_pressure() NOEXCEPT
{
    // The kernel low memory resource signal (no configurable threshold).
    BOOL low{ FALSE };
    const auto handle = ::CreateMemoryResourceNotification(
        ::LowMemoryResourceNotification);

    if (handle == NULL)
        return zero;

    const auto success = !is_zero(::QueryMemoryResourceNotification(handle,
        &low));
    ::CloseHandle(handle);

    return success ? (low == FALSE ? 1_size : 4_size) : zero;
}

uint64_t system_compressed() NOEXCEPT
{
    // No cheap source (performance counters only).
    return zero;
}

#else // HAVE_MSC

size_t page_size() NOEXCEPT
{
    errno = 0;
    const auto size = sysconf(_SC_PAGESIZE);
    if (is_zero(errno) && !is_negative(size))
    {
        BC_ASSERT(!is_limited<size_t>(size));
        return possible_narrow_sign_cast<size_t>(size);
    }

    return zero;
}

uint64_t system_memory() NOEXCEPT
{
    errno = 0;
    const int64_t pages = sysconf(_SC_PHYS_PAGES);
    if (is_zero(errno) && !is_negative(pages))
    {
        // Failed page_size also results in zero return.
        const auto size = possible_wide_cast<uint64_t>(page_size());
        return ceilinged_multiply(to_unsigned(pages), size);
    }

    return zero;
}

uint64_t physical_memory() NOEXCEPT
{
#if defined(HAVE_LINUX)
    // Zone present pages exclude only firmware carveouts and address holes,
    // which rounding up to a whole gibibyte absorbs.
    constexpr auto gibibyte = power2<uint64_t>(30u);
    if (const auto file = std::fopen("/proc/zoneinfo", "r"))
    {
        char line[128];
        uint64_t pages{};
        uint64_t present{};
        while (!is_null(std::fgets(line, sizeof(line), file)))
        {
            if (std::sscanf(line, " present %" SCNu64, &present) == 1)
                pages += present;
        }

        std::fclose(file);
        const auto bytes = ceilinged_multiply<uint64_t>(pages, page_size());
        if (!is_zero(bytes))
            return ceilinged_multiply(ceilinged_divide(bytes, gibibyte),
                gibibyte);
    }
#endif

    // Failed or no platform source.
    return system_memory();
}

uint64_t system_free() NOEXCEPT
{
#if defined(HAVE_APPLE)
    auto count = HOST_VM_INFO64_COUNT;
    vm_statistics64_data_t statistics{};
    if (::host_statistics64(::mach_host_self(), HOST_VM_INFO64,
        pointer_cast<integer_t>(&statistics), &count) == KERN_SUCCESS)
    {
        return ceilinged_multiply<uint64_t>(statistics.free_count, page_size());
    }
#else
    errno = 0;
    const int64_t pages = sysconf(_SC_AVPHYS_PAGES);
    if (is_zero(errno) && !is_negative(pages))
    {
        // Failed page_size() also results in zero return.
        const auto size = possible_wide_cast<uint64_t>(page_size());
        return ceilinged_multiply(to_unsigned(pages), size);
    }
#endif

    // Failed or no platform source.
    return zero;
}

uint64_t system_available() NOEXCEPT
{
#if defined(HAVE_APPLE)
    // Free plus purgeable plus file-backed (external) pages are available to
    // allocation without compression or swap.
    auto count = HOST_VM_INFO64_COUNT;
    vm_statistics64_data_t statistics{};
    if (::host_statistics64(::mach_host_self(), HOST_VM_INFO64,
        pointer_cast<integer_t>(&statistics), &count) == KERN_SUCCESS)
    {
        const auto pages = statistics.free_count + statistics.purgeable_count +
            statistics.external_page_count;
        return ceilinged_multiply<uint64_t>(pages, page_size());
    }
#elif defined(HAVE_LINUX)
    // MemAvailable is the kernel estimate of memory available to allocation
    // without swapping (includes reclaimable file cache).
    if (const auto file = std::fopen("/proc/meminfo", "r"))
    {
        char line[128];
        uint64_t kilobytes{};
        while (!is_null(std::fgets(line, sizeof(line), file)))
        {
            if (std::sscanf(line, "MemAvailable: %" SCNu64 " kB",
                &kilobytes) == 1)
            {
                std::fclose(file);
                return ceilinged_multiply<uint64_t>(kilobytes, 1024u);
            }
        }

        std::fclose(file);
    }
#endif

    // Failed or no platform source (free approximates on other platforms).
    return system_free();
}

size_t system_pressure() NOEXCEPT
{
#if defined(HAVE_APPLE)
    int level{};
    auto size = sizeof(level);
    if (is_zero(::sysctlbyname("kern.memorystatus_vm_pressure_level", &level,
        &size, nullptr, 0)))
    {
        return possible_narrow_sign_cast<size_t>(level);
    }
#elif defined(HAVE_LINUX)
    // PSI (requires CONFIG_PSI): fraction of recent wall time that tasks
    // stalled on memory reclaim. A tenth of time stalled is treated as genuine
    // pressure, partial (some) as warning and total (full) as critical,
    // mapping to the macos memorystatus level semantics.
    if (const auto file = std::fopen("/proc/pressure/memory", "r"))
    {
        char line[128];
        auto level = one;
        double average10{};
        while (!is_null(std::fgets(line, sizeof(line), file)))
        {
            if ((std::sscanf(line, "some average10=%lf", &average10) == 1) &&
                (average10 >= 10.0))
                level = std::max(level, 2_size);

            if ((std::sscanf(line, "full average10=%lf", &average10) == 1) &&
                (average10 >= 10.0))
                level = std::max(level, 4_size);
        }

        std::fclose(file);
        return level;
    }
#endif

    // Failed or no platform source.
    return zero;
}

uint64_t system_compressed() NOEXCEPT
{
#if defined(HAVE_APPLE)
    auto count = HOST_VM_INFO64_COUNT;
    vm_statistics64_data_t statistics{};
    if (::host_statistics64(::mach_host_self(), HOST_VM_INFO64,
        pointer_cast<integer_t>(&statistics), &count) == KERN_SUCCESS)
    {
        const auto pages = statistics.compressor_page_count;
        return ceilinged_multiply<uint64_t>(pages, page_size());
    }
#endif

    // Failed or no platform source adopted. Linux zswap/zram accounting is
    // configuration dependent, PSI carries the pressure signal there.
    return zero;
}

#endif // HAVE_MSC

size_t cores() NOEXCEPT
{
    return std::max(std::thread::hardware_concurrency(), 1_u32);
}

#if defined(HAVE_MSC)

// Volume root of the path, empty if not determinable.
static std::string volume_root(const std::filesystem::path& path) NOEXCEPT
{
    BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

    // The api requires a qualified path, and extension breaks the device path.
    std::wstring root(MAX_PATH, {});
    if (is_zero(::GetVolumePathNameW(system::qualified_path(path).c_str(),
        root.data(), MAX_PATH)))
        return {};

    const auto end = root.find(std::wstring::value_type{});
    if (end == std::wstring::npos)
        return {};

    root.resize(end);
    return system::to_utf8(root);

    BC_POP_WARNING()
}

// Volume device handle, requires no access rights (null desired access).
static HANDLE volume_handle(const std::string& root) NOEXCEPT
{
    BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
    static const system::string_list trims{ "\\" };

    // Trailing separator is invalid in a device path.
    const auto volume = system::trim_right_copy(root, trims);
    if (volume.empty())
        return INVALID_HANDLE_VALUE;

    return ::CreateFileW(system::to_utf16("\\\\.\\" + volume).c_str(), 0,
        FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);

    BC_POP_WARNING()
}

bool solid_state(const std::filesystem::path& path) NOEXCEPT
{
    const auto root = volume_root(path);
    if (root.empty() || ::GetDriveTypeW(system::to_utf16(root).c_str()) !=
        DRIVE_FIXED)
        return true;

    const auto handle = volume_handle(root);
    if (handle == INVALID_HANDLE_VALUE)
        return true;

    DWORD size{};
    DEVICE_SEEK_PENALTY_DESCRIPTOR penalty{};
    STORAGE_PROPERTY_QUERY query{ StorageDeviceSeekPenaltyProperty,
        PropertyStandardQuery, {} };

    const auto result = ::DeviceIoControl(handle, IOCTL_STORAGE_QUERY_PROPERTY,
        &query, sizeof(query), &penalty, sizeof(penalty), &size, nullptr);

    ::CloseHandle(handle);
    return to_bool(result) ? !to_bool(penalty.IncursSeekPenalty) : true;
}

bool internal_storage(const std::filesystem::path& path) NOEXCEPT
{
    const auto root = volume_root(path);
    if (root.empty())
        return true;

    switch (::GetDriveTypeW(system::to_utf16(root).c_str()))
    {
        case DRIVE_REMOVABLE:
        case DRIVE_REMOTE:
        case DRIVE_CDROM:
            return false;
        case DRIVE_FIXED:
            break;
        default:
            return true;
    }

    const auto handle = volume_handle(root);
    if (handle == INVALID_HANDLE_VALUE)
        return true;

    DWORD size{};
    std_array<uint8_t, 512> buffer{};
    STORAGE_PROPERTY_QUERY query{ StorageDeviceProperty, PropertyStandardQuery,
        {} };

    const auto result = ::DeviceIoControl(handle, IOCTL_STORAGE_QUERY_PROPERTY,
        &query, sizeof(query), buffer.data(), possible_narrow_cast<DWORD>(
            buffer.size()), &size, nullptr);

    ::CloseHandle(handle);
    if (!to_bool(result))
        return true;

    switch (pointer_cast<STORAGE_DEVICE_DESCRIPTOR>(buffer.data())->BusType)
    {
        case BusTypeUsb:
        case BusType1394:
        case BusTypeSd:
        case BusTypeMmc:
        case BusTypeiScsi:
        case BusTypeFibre:
            return false;
        default:
            return true;
    }
}

#elif defined(HAVE_LINUX)

// Block device sysfs directory for the path, empty if not determinable.
static std::filesystem::path device(
    const std::filesystem::path& path) NOEXCEPT
{
    struct ::stat status{};
    if (!is_zero(::stat(path.c_str(), &status)))
        return {};

    const auto node = std::filesystem::path{ "/sys/dev/block" } /
        (std::to_string(major(status.st_dev)) + ":" +
            std::to_string(minor(status.st_dev)));

    std::error_code ec{};
    const auto disk = std::filesystem::canonical(node, ec);
    if (ec)
        return {};

    // Partitions carry no queue, the parent disk holds device attributes.
    return std::filesystem::is_directory(disk / "queue", ec) ? disk :
        disk.parent_path();
}

// Single character sysfs flag, false if not determinable.
static bool flagged(const std::filesystem::path& file, bool& out) NOEXCEPT
{
    BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
    std::ifstream stream{ file };
    char value{};
    if (!stream.good() || !stream.get(value))
        return false;
    BC_POP_WARNING()

    out = (value == '1');
    return true;
}

bool solid_state(const std::filesystem::path& path) NOEXCEPT
{
    const auto disk = device(path);
    if (disk.empty())
        return true;

    bool rotational{};
    return flagged(disk / "queue" / "rotational", rotational) ? !rotational :
        true;
}

bool internal_storage(const std::filesystem::path& path) NOEXCEPT
{
    constexpr auto nfs = 0x6969_u32;
    constexpr auto smb = 0x517b_u32;
    constexpr auto cifs = 0xff534d42_u32;
    constexpr auto smb2 = 0xfe534d42_u32;

    // Network mounts carry no block device, so are excluded by type.
    struct ::statfs stats{};
    if (is_zero(::statfs(path.c_str(), &stats)))
    {
        // The file system type varies in width and sign by platform.
        const auto type = possible_narrow_and_sign_cast<uint32_t>(stats.f_type);
        if (type == nfs || type == smb || type == cifs || type == smb2)
            return false;
    }

    const auto disk = system::from_path(device(path));
    if (disk.empty())
        return true;

    // The canonical device path carries the transport topology.
    return disk.find("/usb") == std::string::npos
        && disk.find("/mmc") == std::string::npos
        && disk.find("/firewire") == std::string::npos;
}

#else

bool solid_state(const std::filesystem::path&) NOEXCEPT
{
    return true;
}

bool internal_storage(const std::filesystem::path&) NOEXCEPT
{
    return true;
}

#endif // HAVE_MSC

#if defined(HAVE_MSC) || defined(HAVE_LINUX) || defined(HAVE_APPLE)

// Compute runtimes install with the device driver, so their device counts
// answer for the backends that batch acceleration requires when compiled.
using cu_init_t = int32_t(*)(uint32_t);
using cu_count_t = int32_t(*)(int32_t*);
using cu_get_t = int32_t(*)(int32_t*, int32_t);
using cu_attribute_t = int32_t(*)(int32_t*, int32_t, int32_t);
using cl_platform_t = int32_t(*)(uint32_t, void**, uint32_t*);
using cl_device_t = int32_t(*)(void*, uint64_t, uint32_t, void**, uint32_t*);
using cl_info_t = int32_t(*)(void*, uint32_t, size_t, void*, size_t*);
using mtl_device_t = void*(*)();
using objc_release_t = void(*)(void*);

#if !defined(HAVE_APPLE)
// CU_DEVICE_ATTRIBUTE_COMPUTE_CAPABILITY_MAJOR/MINOR, and the floor of the
// compiled cuda kernels (ada lovelace).
constexpr int32_t cu_major = 75;
constexpr int32_t cu_minor = 76;
constexpr int32_t cuda_major = 8;
constexpr int32_t cuda_minor = 9;
#endif

// CL_DEVICE_TYPE_GPU, excludes cpu devices exposed by installable clients.
constexpr auto gpu_type = 4_u64;

// CL_DEVICE_VERSION, and the floor of the compiled opencl kernels.
constexpr auto cl_version = 0x102f_u32;
constexpr auto opencl_major = 1_u32;
constexpr auto opencl_minor = 2_u32;

#if defined(HAVE_MSC)

constexpr auto cuda_library = "nvcuda.dll";
constexpr auto opencl_library = "OpenCL.dll";
using library_t = HMODULE;

static library_t load_library(const std::string& name) NOEXCEPT
{
    BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
    return ::LoadLibraryW(system::to_utf16(name).c_str());
    BC_POP_WARNING()
}

static void* load_symbol(library_t library, const char* name) NOEXCEPT
{
    BC_PUSH_WARNING(NO_REINTERPRET_CAST)
    return reinterpret_cast<void*>(::GetProcAddress(library, name));
    BC_POP_WARNING()
}

static void free_library(library_t library) NOEXCEPT
{
    ::FreeLibrary(library);
}

#else

#if defined(HAVE_APPLE)
constexpr auto opencl_library = "/System/Library/Frameworks/OpenCL.framework/OpenCL";
constexpr auto metal_library = "/System/Library/Frameworks/Metal.framework/Metal";
constexpr auto objc_library = "/usr/lib/libobjc.A.dylib";
#else
constexpr auto cuda_library = "libcuda.so.1";
constexpr auto opencl_library = "libOpenCL.so.1";
#endif
using library_t = void*;

static library_t load_library(const std::string& name) NOEXCEPT
{
    return ::dlopen(name.c_str(), RTLD_LAZY);
}

static void* load_symbol(library_t library, const char* name) NOEXCEPT
{
    return ::dlsym(library, name);
}

static void free_library(library_t library) NOEXCEPT
{
    ::dlclose(library);
}

#endif

#if !defined(HAVE_APPLE)

static bool cuda_device() NOEXCEPT
{
    const auto library = load_library(cuda_library);
    if (is_null(library))
        return false;

    BC_PUSH_WARNING(NO_REINTERPRET_CAST)
    const auto initialize = reinterpret_cast<cu_init_t>(
        load_symbol(library, "cuInit"));
    const auto count = reinterpret_cast<cu_count_t>(
        load_symbol(library, "cuDeviceGetCount"));
    const auto get = reinterpret_cast<cu_get_t>(
        load_symbol(library, "cuDeviceGet"));
    const auto attribute = reinterpret_cast<cu_attribute_t>(
        load_symbol(library, "cuDeviceGetAttribute"));
    BC_POP_WARNING()

    auto found = false;
    int32_t devices{};
    if (!is_null(initialize) && !is_null(count) && !is_null(get) &&
        !is_null(attribute) && is_zero(initialize(0)) &&
        is_zero(count(&devices)))
    {
        for (int32_t ordinal{}; ordinal < devices; ++ordinal)
        {
            int32_t device{}, major{}, minor{};
            if (is_zero(get(&device, ordinal)) &&
                is_zero(attribute(&major, cu_major, device)) &&
                is_zero(attribute(&minor, cu_minor, device)) &&
                ((major > cuda_major) ||
                    (major == cuda_major && minor >= cuda_minor)))
            {
                found = true;
                break;
            }
        }
    }

    free_library(library);
    return found;
}

#endif

// Formatted as "OpenCL <major>.<minor> <vendor-specific>".
static bool opencl_version(const char* text, uint32_t& major,
    uint32_t& minor) NOEXCEPT
{
    constexpr std::string_view prefix{ "OpenCL " };
    const std::string_view version{ text };
    if (!version.starts_with(prefix))
        return false;

    const auto end = std::next(version.data(), version.size());
    const auto first = std::from_chars(std::next(version.data(),
        prefix.size()), end, major);
    if (first.ec != std::errc{} || first.ptr == end || *first.ptr != '.')
        return false;

    return std::from_chars(std::next(first.ptr), end, minor).ec == std::errc{};
}

static bool opencl_device() NOEXCEPT
{
    const auto library = load_library(opencl_library);
    if (is_null(library))
        return false;

    BC_PUSH_WARNING(NO_REINTERPRET_CAST)
    const auto platforms = reinterpret_cast<cl_platform_t>(
        load_symbol(library, "clGetPlatformIDs"));
    const auto devices = reinterpret_cast<cl_device_t>(
        load_symbol(library, "clGetDeviceIDs"));
    BC_POP_WARNING()

    auto found = false;
    uint32_t count{};
    if (!is_null(platforms) && !is_null(devices) &&
        is_zero(platforms(0, nullptr, &count)) && !is_zero(count))
    {
        BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
        std::vector<void*> identifiers(count);
        BC_POP_WARNING()

        BC_PUSH_WARNING(NO_REINTERPRET_CAST)
        const auto information = reinterpret_cast<cl_info_t>(
            load_symbol(library, "clGetDeviceInfo"));
        BC_POP_WARNING()

        if (!is_null(information) &&
            is_zero(platforms(count, identifiers.data(), nullptr)))
        {
            for (const auto identifier: identifiers)
            {
                uint32_t gpus{};
                if (!is_zero(devices(identifier, gpu_type, 0, nullptr, &gpus)) ||
                    is_zero(gpus))
                    continue;

                BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
                std::vector<void*> handles(gpus);
                BC_POP_WARNING()

                if (!is_zero(devices(identifier, gpu_type, gpus, handles.data(),
                    nullptr)))
                    continue;

                for (const auto handle: handles)
                {
                    char version[64]{};
                    uint32_t major{}, minor{};
                    if (is_zero(information(handle, cl_version,
                        sizeof(version), version, nullptr)) &&
                        opencl_version(version, major, minor) &&
                        ((major > opencl_major) ||
                            (major == opencl_major && minor >= opencl_minor)))
                    {
                        found = true;
                        break;
                    }
                }

                if (found)
                    break;
            }
        }
    }

    free_library(library);
    return found;
}

#if defined(HAVE_APPLE)

static bool metal_device() NOEXCEPT
{
    const auto library = load_library(metal_library);
    if (is_null(library))
        return false;

    BC_PUSH_WARNING(NO_REINTERPRET_CAST)
    const auto create = reinterpret_cast<mtl_device_t>(
        load_symbol(library, "MTLCreateSystemDefaultDevice"));
    BC_POP_WARNING()

    const auto device = is_null(create) ? nullptr : create();
    if (!is_null(device))
    {
        // The device is returned retained, so is released by the runtime.
        if (const auto objc = load_library(objc_library))
        {
            BC_PUSH_WARNING(NO_REINTERPRET_CAST)
            const auto release = reinterpret_cast<objc_release_t>(
                load_symbol(objc, "objc_release"));
            BC_POP_WARNING()

            if (!is_null(release))
                release(device);

            free_library(objc);
        }
    }

    free_library(library);
    return !is_null(device);
}

bool gpu_device() NOEXCEPT
{
    return metal_device() || opencl_device();
}

#else

bool gpu_device() NOEXCEPT
{
    return cuda_device() || opencl_device();
}

#endif

#else

bool gpu_device() NOEXCEPT
{
    return false;
}

#endif

} // namespace database
} // namespace libbitcoin
