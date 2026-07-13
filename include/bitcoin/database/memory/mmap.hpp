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
#ifndef LIBBITCOIN_DATABASE_MEMORY_MMAP_HPP
#define LIBBITCOIN_DATABASE_MEMORY_MMAP_HPP

#include <atomic>
#include <filesystem>
#include <mutex>
#include <shared_mutex>
#include <tuple>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/file/file.hpp>
#include <bitcoin/database/memory/accessor.hpp>
#include <bitcoin/database/memory/interfaces/storage.hpp>

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

    /// Scalar construction (columns == 1): unchanged signature and codegen.
    mmap(const path& filename, size_t minimum=1, size_t expansion=0,
        bool random=true) NOEXCEPT requires (is_one(columns));

    /// Aggregate construction (columns > 1): one file per column, shared guards.
    mmap(const paths& filenames, size_t minimum=1, size_t expansion=0,
        bool random=true) NOEXCEPT requires (columns > one);

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

    /// Remap-protected r/w access to offset (or null) allocated to size.
    memory_ptr set(size_t offset, size_t size,
        uint8_t backfill) NOEXCEPT override;

    /// Remap-protected r/w access to start/offset (or null), within capacity.
    memory_ptr get_capacity(size_t offset=zero) const NOEXCEPT override;

    /// Unprotected r/w access to start/offset (or null), within logical.
    memory::iterator get_raw(size_t offset=zero) const NOEXCEPT override;

    /// Unprotected r/w access to start/offset (or null), within logical.
    memory::iterator get_at_raw(size_t column,
        size_t offset=zero) const NOEXCEPT override;

    /// Remap-protected r/w access to start/offset (or null), within logical.
    memory_ptr get(size_t offset=zero) const NOEXCEPT override;

    /// Same as get() but within specified column (or null for invalid column).
    memory_ptr get_at(size_t column,
        size_t offset=zero) const NOEXCEPT override;

    /// Remap-protected r/w access to start/offset (or null), within logical.
    memory get1(size_t offset=zero) const NOEXCEPT override;

    /// Same as get() but within specified column (or null for invalid column).
    memory get_at1(size_t column, size_t offset=zero) const NOEXCEPT override;

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

    size_t to_capacity(size_t required) const NOEXCEPT;
    void set_first_code(const error::error_t& ec) NOEXCEPT;
    void set_disk_space(size_t required) NOEXCEPT;

private:
    static constexpr auto fail = -1;
    using sequence = std::make_index_sequence<columns>;

    // mman dispatch, not thread safe.
    template <size_t... Index>
    bool flush_all_(std::index_sequence<Index...>) NOEXCEPT;
    template <size_t... Index>
    bool map_all_(std::index_sequence<Index...>) NOEXCEPT;
    template <size_t... Index>
    bool unmap_all_(std::index_sequence<Index...>) NOEXCEPT;
    template <size_t... Index>
    bool remap_all_(size_t capacity, std::index_sequence<Index...>) NOEXCEPT;

    // mman wrappers, not thread safe.
    template <size_t Column>
    bool flush_() NOEXCEPT;
    template <size_t Column>
    bool map_() NOEXCEPT;
    template <size_t Column>
    bool release_(size_t size) NOEXCEPT;
    template <size_t Column>
    bool unmap_(size_t size) NOEXCEPT;
    template <size_t Column>
    bool remap_(size_t size) NOEXCEPT;
    template <size_t Column>
    bool resize_(size_t size) NOEXCEPT;
    template <size_t Column>
    bool finalize_(size_t size) NOEXCEPT;

    // These are thread safe.
    const paths filenames_;
    const size_t minimum_;
    const size_t expansion_;
    const bool random_;
    std::atomic<size_t> space_{ zero };
    std::atomic<error::error_t> error_{ error::success };

    // These are protected by field_mutex_.
    // Fields require field_mutex_ exclusive for write, shared for flush/read.
    // logical_ and capacity_ are row counts (byte cound if width is one).
    std::array<int, columns> opened_;
    size_t capacity_{};
    size_t logical_{};
    bool fault_{};
    bool loaded_{};
    mutable std::shared_mutex field_mutex_{};

    // These are protected by remap_mutex_.
    std::array<uint8_t*, columns> memory_map_{};
    mutable std::shared_mutex remap_mutex_{};
};

using map = mmap<one>;

} // namespace database
} // namespace libbitcoin

#define TEMPLATE template <size_t... Widths>
#define CLASS mmap<Widths...>

BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

#include <bitcoin/database/impl/memory/mmap.ipp>
#include <bitcoin/database/impl/memory/mmap_dispatch.ipp>
#include <bitcoin/database/impl/memory/mmap_private.ipp>
#include <bitcoin/database/impl/memory/mmap_storage.ipp>

BC_POP_WARNING()

#undef CLASS
#undef TEMPLATE

#endif
