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
#ifndef LIBBITCOIN_DATABASE_MEMORY_MMAP_HPP
#define LIBBITCOIN_DATABASE_MEMORY_MMAP_HPP

#include <atomic>
#include <condition_variable>
#include <filesystem>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <thread>
#include <tuple>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/file/file.hpp>
#include <bitcoin/database/memory/accessor.hpp>
#include <bitcoin/database/memory/interfaces/storage.hpp>
#include <bitcoin/database/memory/mstage.hpp>
#include <bitcoin/database/memory/settings.hpp>

namespace libbitcoin {
namespace database {

/// Thread safe access to a memory-mapped file, or to a set of column files
/// sharing one allocation/remap guard set (SoA aggregate).
/// A slab has a row width of 1, so "count" implies "bytes" for slabs below.
template <size_t... Widths>
class mmap
  : public storage
{
public:
    DELETE_COPY_MOVE(mmap);

    /// Number of backing columns (1 == scalar map).
    static constexpr size_t columns = sizeof...(Widths);
    static_assert(!is_zero(columns), "requires at least one column");
    using paths = std::array<path, columns>;
    using sizes = std::array<size_t, columns>;

    /// Per-column record widths; transpose row<->byte by these (constexpr).
    static constexpr sizes widths{ Widths... };

    /// Bytes per logical row across the aggregate (sum of column widths).
    static constexpr size_t stride = (Widths + ...);

    /// Constructors.
    /// -----------------------------------------------------------------------

    /// Staged instances (append-only bodies) stage writes in anonymous memory
    /// and convert flushed rows to a read-only file mapping, hard-faulting any
    /// write into settled space. Unstaged instances (heads) hold all logical
    /// content in anonymous memory for the life of the load. Both write the
    /// file only by explicit transfer, so no dirty file-backed page ever
    /// exists (no effect where the staging backend is not built).

    /// Storage tuning is passed by settings (future tunables land there
    /// without signature change); random and staged are structural.

    /// Scalar construction (columns == 1).
    mmap(const path& filename, const storage_settings& settings={},
        bool random=true, bool staged=false) NOEXCEPT
        requires (is_one(columns));

    /// Aggregate construction (columns > 1): one file per column, shared guards.
    mmap(const paths& filenames, const storage_settings& settings={},
        bool random=true, bool staged=false) NOEXCEPT
        requires (columns > one);

    /// Destruct for debug assertion only.
    virtual ~mmap() NOEXCEPT;

    /// True if the file(s) are open.
    bool is_open() const NOEXCEPT;

    /// True if the memory map(s) are loaded.
    bool is_loaded() const NOEXCEPT;

    /// storage interface
    /// -----------------------------------------------------------------------

    /// Get the fault condition.
    code get_fault() const NOEXCEPT override;

    /// Get the space required to clear the disk full condition.
    size_t get_space() const NOEXCEPT override;

    /// The filesystem path of the (first) backing file.
    const path& file() const NOEXCEPT override;

    /// Create empty file(s), must be closed.
    code create() const NOEXCEPT override;

    /// Open file(s), must be closed.
    code open() NOEXCEPT override;

    /// Close file(s), must be unloaded, idempotent.
    code close() NOEXCEPT override;

    /// Map file(s) to memory, must be loaded.
    code load() NOEXCEPT override;

    /// Clear disk full condition, fails if fault, must be loaded, idempotent.
    code reload() NOEXCEPT override;

    /// Declare content mutation, restoring released pages (unstaged
    /// instances under the staging backend only; no effect otherwise).
    void prepare(size_t offset, size_t size) NOEXCEPT override;

    /// Report content mutation (advisory page-dirty tracking, unstaged
    /// instances under the staging backend only; no effect otherwise).
    void mark(size_t offset, size_t size) NOEXCEPT override;

    /// Flush memory map(s) to disk, suspend writes for call, must be loaded.
    code flush() NOEXCEPT override;

    /// Flush, unmap and truncate to logical, restartable, idempotent.
    code unload() NOEXCEPT override;

    /// Unload and load, causing underyling map(s) to shrink to logical size.
    code shrink() NOEXCEPT override;

    /// Dump current logical map to a new file in path, must not exist.
    code dump(const path& path) const NOEXCEPT override;

    /// The current count of rows/bytes in map (zero if closed).
    size_t size() const NOEXCEPT override;

    /// The current row/byte capacity of the memory map (zero if unmapped).
    size_t capacity() const NOEXCEPT override;

    /// Reduce logical size to specified rows/bytes (false if exceeds logical).
    bool truncate(size_t count) NOEXCEPT override;

    /// Increase logical to specified rows/bytes as required (false if fails).
    bool expand(size_t count) NOEXCEPT override;

    /// Increase capacity by specified rows/bytes (false only if fails).
    bool reserve(size_t count) NOEXCEPT override;

    /// Increase logical by specified rows/bytes, return row of first (or eof).
    size_t allocate(size_t count) NOEXCEPT override;

    /// Report element write completion of count rows/bytes at offset.
    void complete(size_t offset, size_t count) NOEXCEPT override;

    /// Rows/bytes below which all writes are complete (size() when quiescent).
    size_t frontier() const NOEXCEPT override;

    /// Remap-protected r/w access to offset (or null) allocated to size.
    memory get_filled(size_t offset, size_t size,
        uint8_t backfill) NOEXCEPT override;

    /// Remap-protected r/w access to start/offset (or null), within capacity.
    memory get_capacity(size_t offset=zero) const NOEXCEPT override;

    /// Unprotected r/w access to start/offset (or null), within logical.
    memory::iterator get_raw(size_t offset=zero) const NOEXCEPT override;

    /// Unprotected r/w access to start/offset (or null), within logical.
    memory::iterator get_raw_at(size_t column,
        size_t offset=zero) const NOEXCEPT override;

    /// Remap-protected r/w access to start/offset (or null), within logical.
    memory get(size_t offset=zero) const NOEXCEPT override;

    /// Same as get() but within specified column (or null for invalid column).
    memory get_at(size_t column, size_t offset=zero) const NOEXCEPT override;

protected:
    template <size_t Column>
    static constexpr size_t to_width(size_t offset) NOEXCEPT
    {
        return offset * widths.at(Column);
    }

    static constexpr size_t logical_rows(size_t bytes) NOEXCEPT
    {
        return bytes / widths.front();
    }
    
    static constexpr size_t to_rows(size_t bytes) NOEXCEPT
    {
        // Convert constructor's byte minimum to row denomination.
        constexpr auto row = (Widths + ...);
        return system::ceilinged_divide(bytes, row);
    }

    static size_t to_chunk() NOEXCEPT;
    size_t to_capacity(size_t required) const NOEXCEPT;
    size_t to_growth(size_t required) const NOEXCEPT;
    size_t to_provision() const NOEXCEPT;
    size_t to_commitment() const NOEXCEPT;
    void check_invariants_() const NOEXCEPT;
    void set_first_code(const error::error_t& ec) NOEXCEPT;
    void set_disk_space(size_t required) NOEXCEPT;

private:
    static constexpr size_t page_bound = to_bits(sizeof(uint64_t));
    static constexpr size_t settle_chunk = system::power2(28u);
    static constexpr size_t advise_chunk = system::power2(30u);
    static constexpr size_t commit_chunk = system::power2(28u);
    static constexpr size_t chunk_scale = 256;
    static constexpr size_t evict_chunk = system::power2(30u);
    static constexpr size_t compress_factor = 32;
    static constexpr size_t evict_factor = 32;

    // Settled-extent demotion ceiling (installed memory). Demotion trades
    // body cache for head residency, which pays only while the head set
    // contests memory. Above this ceiling nothing contests: demotion just
    // converts warm cache into re-read faults (measured ~2x milestone and
    // validation wall on a high-memory host), so ample hosts retain
    // settled extents at normal priority.
    static constexpr size_t demote_memory = system::power2(35u);

    // Body eviction leads kernel reclaim: sweeping at the reclaim watermark
    // concedes the choice of victim, and the kernel takes anonymous heads
    // alongside the cold body cache (it balances the lists, it does not
    // know that head residency is the store's priority). A higher floor
    // keeps free memory above the watermark, so the sweep is the reclaim.
    static constexpr size_t sweep_factor = 8;
    static constexpr size_t throttle_factor = 8;
    static constexpr size_t active_factor = 32;
    static constexpr size_t urgent_factor = 4;
    static constexpr size_t idle_seconds = 60;
    static constexpr size_t touch_seconds = 4;
    static constexpr size_t touch_span = 16384;

    // Release conversion granularity: chunked runs bound address space
    // fragmentation (each conversion splits a mapping) to the measured flat
    // zone of host memory management (heads / chunk fragments worst case).
    static constexpr size_t release_chunk = system::power2(20u);
    static constexpr size_t release_quiet = 128;
#if defined(HAVE_APPLE)
    // Anonymous overflow feeds the darwin compressor (10.8GB measured at
    // 16GB), which mincore hides from the touch guard; release converts
    // quiet heads to droppable file pages, removing them from its reach.
    static constexpr bool head_release = true;
#else
    static constexpr bool head_release = false;
#endif

    // Map heads writable-shared from their files (the native windows model)
    // instead of anonymously with a dirty-page writer. Head reclaim is then
    // kernel writeback of a bounded rewrite-in-place mapping (clean pages
    // drop) rather than swap, which anonymous pages alone require. Bodies
    // remain staged, so the unbounded append writeback that motivates dirty
    // ratio tuning does not return with it. Excludes head_release (nothing
    // to release) and the dirty bitmap (nothing to transfer).
    static constexpr bool head_shared = false;
    static constexpr size_t headroom = 4;
#if defined(STAGING_TELEMETRY)
    static constexpr size_t telemetry_seconds = 60;
#endif
    static constexpr auto fail = -1;
    static constexpr auto relaxed = std::memory_order_relaxed;
    static constexpr auto release = std::memory_order_release;
    using sequence = std::make_index_sequence<columns>;

    // mman dispatch, not thread safe.
    template <size_t... Index>
    bool flush_all_(size_t rows, std::index_sequence<Index...>) NOEXCEPT;
    template <size_t... Index>
    bool map_all_(std::index_sequence<Index...>) NOEXCEPT;
    template <size_t... Index>
    bool unmap_all_(std::index_sequence<Index...>) NOEXCEPT;
    template <size_t... Index>
    bool remap_all_(size_t capacity, std::index_sequence<Index...>,
        bool final=true) NOEXCEPT;
    bool grow_(size_t end) NOEXCEPT;
    bool probe_(size_t capacity) NOEXCEPT;

    // mman wrappers, not thread safe.
    template <size_t Column>
    bool flush_(size_t rows) NOEXCEPT;
    template <size_t Column>
    bool map_() NOEXCEPT;
    template <size_t Column>
    bool release_(size_t size) NOEXCEPT;
    template <size_t Column>
    bool unmap_(size_t size) NOEXCEPT;
    template <size_t Column>
    bool remap_(size_t size, bool final=true) NOEXCEPT;
    template <size_t Column>
    bool resize_(size_t size, bool final=true) NOEXCEPT;
    template <size_t Column>
    bool finalize_(size_t size) NOEXCEPT;

#if defined(MANAGE_STAGING)
    // staging dispatch, not thread safe.
    template <size_t... Index>
    bool settle_all_(size_t rows, std::index_sequence<Index...>) NOEXCEPT;
    template <size_t... Index>
    bool unsettle_all_(size_t rows, std::index_sequence<Index...>) NOEXCEPT;
    template <size_t... Index>
    bool evict_all_(size_t from, size_t to,
        std::index_sequence<Index...>) NOEXCEPT;

    // staging wrappers, not thread safe.
    template <size_t Column>
    bool stage_() NOEXCEPT;
    template <size_t Column>
    bool commit_(size_t size, bool final=true) NOEXCEPT;
    template <size_t Column>
    bool settle_(size_t from, size_t to) NOEXCEPT;
    template <size_t Column>
    bool unsettle_(size_t rows) NOEXCEPT;
    template <size_t Column>
    bool evict_(size_t from, size_t to) NOEXCEPT;
    template <size_t Column>
    void teardown_(const error::error_t& ec) NOEXCEPT;

    // staging utilities, not thread safe (claim_ is lock-free thread safe).
    struct extent;
    size_t record_(size_t count) NOEXCEPT;
    bool claim_(extent& record, size_t count) NOEXCEPT;
    void maintain_() NOEXCEPT;
    void discard_() NOEXCEPT;
    void throttle_() NOEXCEPT;
    void signal_() NOEXCEPT;

    // dirty page transfer (unstaged instances), lock-free with writers.
    template <size_t Column>
    bool transfer_(size_t bytes) NOEXCEPT;
    template <size_t Column>
    bool sync_() NOEXCEPT;
    void remark_(size_t offset, size_t size) NOEXCEPT;

    // head page release (unstaged instances), synchronized with writers by
    // the prepare/release bit protocol (see release_pages_).
    bool release_pages_() NOEXCEPT;
    void restore_(size_t offset, size_t size) NOEXCEPT;

    // settle scheduler (instance-owned thread, load/unload lifecycle).
    void settler_start_() NOEXCEPT;
    void settler_stop_() NOEXCEPT;
    void settler_run_() NOEXCEPT;
    void head_run_() NOEXCEPT;
    bool settle_next_(size_t chunk) NOEXCEPT;
    bool evict_next_(size_t chunk) NOEXCEPT;
    template <size_t... Index>
    bool settle_write_(size_t from, size_t to,
        std::index_sequence<Index...>) NOEXCEPT;
    bool advise_(uint8_t* map, size_t size) const NOEXCEPT;
    size_t to_reservation(size_t rows) const NOEXCEPT;
    size_t page_floor(size_t bytes) const NOEXCEPT;
    size_t page_ceiling(size_t bytes) const NOEXCEPT;
#endif // MANAGE_STAGING

#if defined(HAVE_MSC)
    // Working-set steering for the native (file-backed) mapping. The cache
    // manager trims without knowing that head residency is the store's
    // priority, so it takes head pages alongside cold body cache and every
    // head miss is a serial fault on the probe path. The scanner asserts head
    // residency (a read sets the access bit) and leads the trim on bodies
    // (unlock moves the range to the standby list, reclaimed first).
    void scanner_start_() NOEXCEPT;
    void scanner_stop_() NOEXCEPT;
    void scanner_run_() NOEXCEPT;

    std::thread scanner_{};
    std::atomic_bool scanning_{};
    std::condition_variable scanner_cv_{};
    mutable std::mutex scanner_mutex_{};
    size_t touched_{};
    size_t unlocked_{};
#endif // HAVE_MSC

    // These are thread safe (const).
    const paths filenames_;
    const size_t minimum_;
    const size_t expansion_;
    const size_t headroom_;
    const advice access_;
    const bool random_;
    const bool staged_;

    // These are thread safe (atomic).
    std::atomic<error::error_t> error_{ error::success };
    std::atomic<size_t> space_{ zero };
    std::atomic<size_t> capacity_{};
    std::atomic<size_t> file_{};
    std::atomic<size_t> logical_{};
    std::atomic_bool fault_{};
    std::atomic_bool loaded_{};

    // This is protected by field_mutex_.
    std::array<int, columns> opened_;
    mutable std::shared_mutex field_mutex_{};

    // This is protected by remap_mutex_.
    std::array<uint8_t*, columns> memory_map_{};
    mutable std::shared_mutex remap_mutex_{};

#if defined(MANAGE_STAGING)
    // Page-dirty bitmap for unstaged (rewrite-in-place head) instances.
    // Marks follow content writes; transfer clears before reading, so a
    // racing mark is never lost (a torn disk page is unreachable state, as
    // live heads are trusted only following a clean close).
    using dirty_bitmaps = std::atomic<uint64_t>[];

    static constexpr size_t extents = 4096;

    // Extent state packs a recycling generation with the outstanding count,
    // so a lock-free claim (cas decrement) cannot land across a recycle: the
    // generation changes before a slot is reused, failing the cas. This
    // replaces the post-decrement start-recheck repair, which misfired when
    // the decrement itself zeroed the extent and the slot recycled before
    // the recheck (the repair then permanently inflated the successor,
    // freezing the frontier). The count is atomic because the lock-free
    // range search reads it against a concurrent recycle. The generation
    // wraps at 16 bits; a stale claimer would need 65536 same-slot recycles
    // (each a full ring lap) within one preempted claim to alias.
    static constexpr size_t generation_shift = 48;
    static constexpr uint64_t generation_mask =
        sub1(system::power2<uint64_t>(16u));
    static constexpr uint64_t outstanding_mask =
        sub1(system::power2<uint64_t>(generation_shift));
    static constexpr uint64_t pack_extent_(uint64_t generation,
        uint64_t outstanding) NOEXCEPT
    {
        using namespace system;
        return bit_or(shift_left<uint64_t>(generation, generation_shift),
            outstanding);
    }

    struct extent
    {
        std::atomic<size_t> start;
        std::atomic<size_t> count;
        std::atomic<uint64_t> state;
    };

    // This is unshared (settler thread only).
    size_t evicted_{};

#if defined(STAGING_TELEMETRY)
    // This is unshared (settler thread only).
    size_t telemetry_{};
#endif

    // These are thread safe (atomic).
    std::atomic<size_t> marks_{};
    std::atomic<size_t> settled_{};
    std::atomic<size_t> frontier_{};
    std::atomic<uint64_t> window_{};

    // These are protected by remap_mutex_.
    std::unique_ptr<dirty_bitmaps> dirty_{};
    std::unique_ptr<dirty_bitmaps> intent_{};
    std::unique_ptr<dirty_bitmaps> released_{};
    size_t words_{};

    // Sweep scratch (candidate/released word snapshots), protected by
    // restore_mutex_.
    std::unique_ptr<uint64_t[]> sweep_{};

    // Set when the first head page releases (gates the prepare fast path).
    std::atomic_bool engaged_{};

    // Writers between prepare and mark (unaged, unlike intent bits), so a
    // release pass cannot settle under a preempted in-flight write.
    std::atomic<size_t> writers_{};

    // Serializes page release against restore (prepare slow path).
    mutable std::mutex restore_mutex_{};

    // Serializes transfer passes (settler tick against flush), as concurrent
    // passes split the claimed dirty set, allowing a flush to complete while
    // claimed pages remain unwritten (a stale snapshot copy).
    mutable std::mutex transfer_mutex_{};

    // These are protected by extent_mutex_.
    size_t page_{};
    std::array<extent, extents> ring_{};
    std::array<size_t, columns> reserved_{};
    mutable std::mutex extent_mutex_{};

    // These are protected by settler_mutex_.
    std::thread settler_{};
    std::atomic_bool settling_{};
    std::condition_variable settler_cv_{};
    mutable std::mutex settler_mutex_{};

    // These are protected by throttle_mutex_.
    size_t limit_{};
    std::condition_variable throttle_cv_{};
    mutable std::mutex throttle_mutex_{};
#endif // MANAGE_STAGING
};

using map = mmap<one>;

} // namespace database
} // namespace libbitcoin

#define TEMPLATE template <size_t... Widths>
#define CLASS mmap<Widths...>

BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

#include <bitcoin/database/impl/memory/mmap.ipp>
#include <bitcoin/database/impl/memory/mmap_dispatch.ipp>
#include <bitcoin/database/impl/memory/mmap_native.ipp>
#include <bitcoin/database/impl/memory/mmap_private.ipp>
#include <bitcoin/database/impl/memory/mmap_staging.ipp>
#include <bitcoin/database/impl/memory/mmap_storage.ipp>

BC_POP_WARNING()

#undef CLASS
#undef TEMPLATE

#endif
