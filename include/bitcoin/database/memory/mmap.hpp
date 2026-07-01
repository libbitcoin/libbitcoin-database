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
#include <bitcoin/database/memory/interfaces/memory.hpp>
#include <bitcoin/database/memory/interfaces/storage.hpp>

namespace libbitcoin {
namespace database {

/// Thread safe access to a memory-mapped file, or to a set of column files
/// sharing one allocation/remap guard set (SoA aggregate).
template <size_t... Widths>
class mmap
  : public storage
{
public:
    DELETE_COPY_MOVE(mmap);

    /// Number of backing columns (1 == scalar map).
    static constexpr size_t columns = sizeof...(Widths);
    static_assert(!is_zero(columns), "requires at least one column");

    using path = std::filesystem::path;
    using paths = std::array<path, columns>;
    using sizes = std::array<size_t, columns>;

    /// Per-column record widths; transpose row<->byte by these (constexpr).
    static constexpr sizes widths{ Widths... };

    /// Bytes per logical row across the aggregate (sum of column widths).
    static constexpr size_t stride = (Widths + ...);

    /// Dispatch.
    /// -----------------------------------------------------------------------

    template <size_t Column, if_lesser<Column, sizeof...(Widths)> = true>
    memory_ptr set_column(size_t offset, size_t size,
        uint8_t backfill) NOEXCEPT;

    template <size_t Column, if_lesser<Column, sizeof...(Widths)> = true>
    memory_ptr get_column(size_t offset=zero) const NOEXCEPT;

    template <size_t Column, if_lesser<Column, sizeof...(Widths)> = true>
    memory_ptr capacity_column(size_t offset=zero) const NOEXCEPT;

    template <size_t Column, if_lesser<Column, sizeof...(Widths)> = true>
    memory::iterator raw_column(size_t offset=zero)  const NOEXCEPT;

    /// Constructors.
    /// -----------------------------------------------------------------------

    /// Scalar construction (columns == 1): unchanged signature and codegen.
    mmap(const std::filesystem::path& filename, size_t minimum=1,
        size_t expansion=0, bool random=true) NOEXCEPT
        requires (columns == one);

    /// Aggregate construction (columns > 1): one file per column, shared guards.
    mmap(const paths& filenames, size_t minimum=1, size_t expansion=0,
        bool random=true) NOEXCEPT
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
    const std::filesystem::path& file() const NOEXCEPT override;

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
    code dump(const std::filesystem::path& path) const NOEXCEPT override;

    /// The current logical size of the memory map (zero if closed).
    size_t size() const NOEXCEPT override;

    /// The current capacity of the memory map (zero if unloaded).
    size_t capacity() const NOEXCEPT override;

    /// Reduce logical size to specified (false if exceeds logical).
    bool truncate(size_t size) NOEXCEPT override;

    /// Increase logical size to specified as required (false if fails).
    bool expand(size_t size) NOEXCEPT override;

    /// Increase capacity by specified (false only if fails).
    bool reserve(size_t size) NOEXCEPT override;

    /// Increase logical by specified, return offset to first (or eof).
    size_t allocate(size_t chunk) NOEXCEPT override;

    /// Remap-protected r/w access to offset (or null) allocated to size.
    memory_ptr set(size_t offset, size_t size,
        uint8_t backfill) NOEXCEPT override;

    /// Remap-protected r/w access to start/offset (or null), within logical.
    memory_ptr get(size_t offset=zero) const NOEXCEPT override;

    /// Remap-protected r/w access to start/offset (or null), within capacity.
    memory_ptr get_capacity(size_t offset=zero) const NOEXCEPT override;

    /// Unprotected r/w access to start/offset (or null), within logical.
    memory::iterator get_raw(size_t offset=zero) const NOEXCEPT override;

protected:
    /// Row<->byte transpose by the constexpr column width (folds for width 1).
    template <size_t Column>
    static constexpr size_t to_width(size_t offset) NOEXCEPT
    {
        return offset * std::get<Column>(widths);
    }

    // Capacity in rows: column 0 bytes transposed back to rows.
    static constexpr size_t capacity_rows(const sizes& capacity) NOEXCEPT
    {
        return std::get<zero>(capacity) / std::get<zero>(widths);
    }

    // Logical rows: column 0 bytes transposed back to rows.
    static constexpr size_t logical_rows(size_t bytes) NOEXCEPT
    {
        return bytes / std::get<zero>(widths);
    }

    size_t to_capacity(size_t required) const NOEXCEPT;
    void set_first_code(const error::error_t& ec) NOEXCEPT;
    void set_disk_space(size_t required) NOEXCEPT;

private:
    static constexpr auto fail = -1;
    using access = accessor<std::shared_mutex>;
    using sequence = std::make_index_sequence<columns>;
    
    // dispatch utils.
    template <size_t Column>
    size_t space_one_(size_t rows) const NOEXCEPT;
    template <size_t... Index>
    size_t space_all_(size_t rows,
        std::index_sequence<Index...>) const NOEXCEPT;

    // mman dispatch, not thread safe.
    template <size_t... Index>
    bool map_all_(std::index_sequence<Index...>) NOEXCEPT;
    template <size_t... Index>
    bool unmap_all_(std::index_sequence<Index...>) NOEXCEPT;
    template <size_t... Index>
    bool flush_all_(std::index_sequence<Index...>) NOEXCEPT;
    template <size_t... Index>
    bool remap_all_(size_t logical, std::index_sequence<Index...>) NOEXCEPT;

    // mman wrappers, not thread safe.
    template <size_t Column>
    bool flush_() NOEXCEPT;
    template <size_t Column>
    bool unmap_() NOEXCEPT;
    template <size_t Column>
    bool map_() NOEXCEPT;
    template <size_t Column>
    bool remap_(size_t size, size_t space) NOEXCEPT;
    template <size_t Column>
    bool resize_(size_t size, size_t space) NOEXCEPT;
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
    // Shared allocation state and per-column file/capacity state.
    // Fields require field_mutex_ exclusive for write, shared for flush/read.
    // logical_ is the shared row count across all columns (bytes for width 1).
    std::array<int, columns> opened_;
    sizes capacity_{};
    size_t logical_{};
    bool fault_{};
    bool loaded_{};
    mutable std::shared_mutex field_mutex_{};

    // These are protected by remap_mutex_.
    std::array<uint8_t*, columns> memory_map_{};
    mutable std::shared_mutex remap_mutex_{};
};

/// Scalar map: single column, width 1. Source and codegen identical to the
/// prior non-aggregate map; existing usage binds the unchanged constructor.
using map = mmap<1>;

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
