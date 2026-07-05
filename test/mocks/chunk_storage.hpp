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
#ifndef LIBBITCOIN_DATABASE_TEST_MOCKS_CHUNK_STORAGE_HPP
#define LIBBITCOIN_DATABASE_TEST_MOCKS_CHUNK_STORAGE_HPP

#include <mutex>
#include <shared_mutex>
#include "../test.hpp"

namespace test {

BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

template <typename Storage>
class default_storage
  : public Storage
{
public:
    using path = std::filesystem::path;

    default_storage(const path& filename="test", size_t minimum=1,
        size_t expansion=0, bool random=true) NOEXCEPT
      : Storage(filename, minimum, expansion, random)
    {
    }
};

/// A thread safe storage implementation built on data_chunk(s); the vector-
/// backed substitute for mmap<Widths...>. One data_chunk per column.
template <size_t... Widths>
class chunk_storages
  : public database::storage
{
public:
    using path = std::filesystem::path;

    static constexpr size_t columns = sizeof...(Widths);
    static_assert(columns >= one, "requires at least one column");
    static constexpr std::array<size_t, columns> widths{ Widths... };
    static constexpr size_t stride = (Widths + ...);
    using paths = std::array<path, columns>;

    /// Scalar construction (is_one(columns)): single backing.
    chunk_storages() NOEXCEPT requires (is_one(columns))
      : alias_{}, paths_{ path{ "test" } }, logical_{}
    {
    }

    chunk_storages(system::data_chunk& reference) NOEXCEPT
        requires (is_one(columns))
      : alias_{ &reference }, paths_{ path{ "test" } },
        logical_{ reference.size() }
    {
    }

    chunk_storages(const path& filename, size_t=1, size_t=0, bool=true) NOEXCEPT
        requires (is_one(columns))
      : alias_{}, paths_{ filename }, logical_{}
    {
    }

    /// Aggregate construction (columns > 1): one backing per column.
    chunk_storages(const paths& filenames, size_t=1, size_t=0,
        bool=true) NOEXCEPT requires (columns > one)
      : alias_{}, paths_{ filenames }, logical_{}
    {
    }

    ~chunk_storages() = default;

    /// test side door (column 0 buffer).
    system::data_chunk& buffer() NOEXCEPT
    {
        return at(zero);
    }

    // storage interface (no-op lifecycle).
    // ------------------------------------------------------------------------

    code get_fault() const NOEXCEPT override
    {
        return {};
    }

    size_t get_space() const NOEXCEPT override
    {
        return {};
    }

    code create() const NOEXCEPT override
    {
        return error::success;
    }

    code open() NOEXCEPT override
    {
        return error::success;
    }

    code close() NOEXCEPT override
    {
        return error::success;
    }

    code load() NOEXCEPT override
    {
        return error::success;
    }

    code reload() NOEXCEPT override
    {
        return error::success;
    }

    code flush() NOEXCEPT override
    {
        return error::success;
    }

    code unload() NOEXCEPT override
    {
        return error::success;
    }

    code shrink() NOEXCEPT override
    {
        return error::success;
    }

    code dump(const path&) const NOEXCEPT override
    {
        return error::success;
    }

    const path& file() const NOEXCEPT override
    {
        return paths_[0];
    }

    // sizing (row-denominated, shared across columns).
    // ------------------------------------------------------------------------

    size_t capacity() const NOEXCEPT override
    {
        std::shared_lock field_lock(field_mutex_);
        return is_zero(widths[0]) ? logical_ : at(zero).size() / widths[0];
    }

    size_t size() const NOEXCEPT override
    {
        std::shared_lock field_lock(field_mutex_);
        return logical_;
    }

    bool truncate(size_t count) NOEXCEPT override
    {
        std::unique_lock field_lock(field_mutex_);
        if (count > logical_)
            return false;

        logical_ = count;
        return true;
    }

    bool expand(size_t count) NOEXCEPT override
    {
        std::unique_lock field_lock(field_mutex_);

        for (size_t column{}; column < columns; ++column)
            if (to_capacity(count, column) > at(column).max_size())
                return false;

        if (count <= logical_)
            return true;

        logical_ = count;
        for (size_t column{}; column < columns; ++column)
            if (to_capacity(logical_, column) > at(column).size())
                at(column).resize(to_capacity(logical_, column));

        return true;
    }

    bool reserve(size_t count) NOEXCEPT override
    {
        std::unique_lock field_lock(field_mutex_);
        if (system::is_add_overflow<size_t>(logical_, count))
            return false;

        const auto end = logical_ + count;
        for (size_t column{}; column < columns; ++column)
            if (to_capacity(end, column) > at(column).max_size())
                return false;

        for (size_t column{}; column < columns; ++column)
            if (to_capacity(end, column) > at(column).size())
                at(column).resize(to_capacity(end, column));

        return true;
    }

    size_t allocate(size_t count) NOEXCEPT override
    {
        std::unique_lock field_lock(field_mutex_);
        if (system::is_add_overflow<size_t>(logical_, count))
            return chunk_storages::eof;

        const auto end = logical_ + count;
        for (size_t column{}; column < columns; ++column)
            if (to_capacity(end, column) > at(column).max_size())
                return chunk_storages::eof;

        std::unique_lock map_lock(map_mutex_);
        const auto link = logical_;

        logical_ = end;
        for (size_t column{}; column < columns; ++column)
            if (to_capacity(logical_, column) > at(column).size())
                at(column).resize(to_capacity(logical_, column));

        return link;
    }

    // access
    // ------------------------------------------------------------------------

    memory_ptr get_capacity(size_t offset=zero) const NOEXCEPT override
    {
        using namespace system;
        auto& buffer = at(zero);
        const auto ptr = emplace_shared<accessor<std::shared_mutex>>(map_mutex_);
        ptr->assign(get_raw(offset), std::next(buffer.data(), buffer.size()));
        return ptr;
    }

    memory::iterator get_raw(size_t offset=zero) const NOEXCEPT override
    {
        return std::next(at(zero).data(), offset);
    }

    memory_ptr set(size_t offset, size_t size, uint8_t backfill) NOEXCEPT override
    {
        {
            std::unique_lock field_lock(field_mutex_);
            if (system::is_add_overflow(offset, size))
                return {};

            std::unique_lock map_lock(map_mutex_);
            auto& buffer = at(zero);
            const auto minimum = offset + size;
            if (minimum > buffer.size())
                buffer.resize(minimum, backfill);

            // Reflect byte growth of column 0 into the shared row count.
            if (!is_zero(widths[0]) && (minimum / widths[0]) > logical_)
                logical_ = (minimum / widths[0]);
        }

        return get(offset);
    }

    memory_ptr get(size_t offset=zero) const NOEXCEPT override
    {
        return get_at(zero, offset);
    }

    memory_ptr get_at(size_t column, size_t offset=zero) const NOEXCEPT override
    {
        using namespace system;
        auto data = at(column).data();
        const auto allocated = size() * widths.at(column);
        const auto ptr = emplace_shared<accessor<std::shared_mutex>>(map_mutex_);
        ptr->assign(std::next(data, offset), std::next(data, allocated));
        return ptr;
    }

    // This is protected by mutex.
    mutable std::array<system::data_chunk, columns> buffers_{};

private:
    // Byte capacity of a column for the given shared row count.
    static constexpr size_t to_capacity(size_t rows, size_t column) NOEXCEPT
    {
        return rows * widths.at(column);
    }

    // Column backing selector: N owned column chunks, with an optional
    // external alias overriding column 0 (single-column reference ctor).
    system::data_chunk& at(size_t column) const NOEXCEPT
    {
        return (is_zero(column) && !is_null(alias_)) ? *alias_ :
            buffers_.at(column);
    }

    // These are protected by mutex.
    system::data_chunk* const alias_;
    size_t logical_{};

    // These are thread safe.
    const paths paths_;
    mutable std::shared_mutex field_mutex_{};
    mutable std::shared_mutex map_mutex_{};
};

BC_POP_WARNING()

using chunk_storage = chunk_storages<one>;

} // namespace test

#endif
