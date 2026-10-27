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

    default_storage(const path& filename="test",
        const database::storage_settings& settings={},
        bool random=true) NOEXCEPT
      : Storage(filename, settings, random)
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
    chunk_storages(std::optional<uint8_t> fill={}) NOEXCEPT
        requires (is_one(columns))
      : alias_{}, paths_{ path{ "test" } }, logical_{}, fill_{ fill }
    {
    }

    chunk_storages(system::data_chunk& reference,
        std::optional<uint8_t> fill={}) NOEXCEPT requires (is_one(columns))
      : alias_{ &reference }, paths_{ path{ "test" } },
        logical_{ reference.size() }, fill_{ fill }
    {
    }

    chunk_storages(const path& filename,
        const database::storage_settings& settings={}, bool=true,
        bool=false, bool=false) NOEXCEPT requires (is_one(columns))
      : alias_{}, paths_{ filename }, logical_{}, fill_{ settings.fill },
        zeroed_{ settings.zeroed }
    {
    }

    /// Aggregate construction (columns > 1): one backing per column.
    chunk_storages(const paths& filenames,
        const database::storage_settings& settings={}, bool=true,
        bool=false) NOEXCEPT requires (columns > one)
      : alias_{}, paths_{ filenames }, logical_{}, fill_{ settings.fill },
        zeroed_{ settings.zeroed }
    {
    }

    ~chunk_storages() = default;

    /// test side door (column 0 buffer).
    system::data_chunk& buffer() NOEXCEPT
    {
        return at(zero);
    }

    /// test side door (sum of completed counts).
    size_t completed() const NOEXCEPT
    {
        std::shared_lock field_lock(field_mutex_);
        return completed_;
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

    void prepare(size_t, size_t) NOEXCEPT override
    {
    }

    void mark(size_t, size_t) NOEXCEPT override
    {
    }

    void current(bool) NOEXCEPT override
    {
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

    size_t width() const NOEXCEPT override
    {
        return stride;
    }

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

        // Released rows are filled, as an allocation reads the fill.
        if (zeroed_ && fill_.has_value())
            for (size_t column{}; column < columns; ++column)
                std::fill(std::next(at(column).begin(),
                    to_capacity(count, column)), std::next(at(column).begin(),
                    to_capacity(logical_, column)), fill_.value());

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
                at(column).resize(to_capacity(logical_, column), backfill());

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
                at(column).resize(to_capacity(end, column), backfill());

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
                at(column).resize(to_capacity(logical_, column), backfill());

        return link;
    }

    size_t allocate_filled(size_t count) NOEXCEPT override
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
                at(column).resize(to_capacity(logical_, column), backfill());

        return link;
    }

    void complete(size_t, size_t count) NOEXCEPT override
    {
        std::unique_lock field_lock(field_mutex_);
        completed_ += count;
    }

    size_t frontier() const NOEXCEPT override
    {
        return size();
    }

    // access
    // ------------------------------------------------------------------------

    memory get_filled(size_t offset, size_t size) NOEXCEPT override
    {
        {
            std::unique_lock field_lock(field_mutex_);
            if (system::is_add_overflow(offset, size))
                return {};

            std::unique_lock map_lock(map_mutex_);
            auto& buffer = at(zero);
            const auto minimum = offset + size;
            if (minimum > buffer.size())
                buffer.resize(minimum, backfill());

            // Reflect byte growth of column 0 into the shared row count.
            if (!is_zero(widths[0]) && (minimum / widths[0]) > logical_)
                logical_ = (minimum / widths[0]);
        }

        return get(offset);
    }

    memory get_capacity(size_t offset=zero) const NOEXCEPT override
    {
        auto& buffer = at(zero);
        accessor out{ map_mutex_ };
        out.assign(get_raw(offset), std::next(buffer.data(), buffer.size()));
        return out;
    }

    memory::iterator get_raw(size_t offset=zero) const NOEXCEPT override
    {
        return std::next(at(zero).data(), offset);
    }

    memory::iterator get_raw_at(size_t column,
        size_t offset=zero) const NOEXCEPT override
    {
        return std::next(at(column).data(), offset);
    }

    memory get(size_t offset=zero) const NOEXCEPT override
    {
        return get_at(zero, offset);
    }

    memory get_at(size_t column, size_t offset=zero) const NOEXCEPT override
    {
        using namespace system;
        auto data = at(column).data();
        const auto allocated = size() * widths.at(column);
        accessor out(map_mutex_);
        out.assign(std::next(data, offset), std::next(data, allocated));
        return out;
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

    uint8_t backfill() const NOEXCEPT
    {
        return fill_.value_or(uint8_t{});
    }

    // These are protected by mutex.
    system::data_chunk* const alias_;
    size_t logical_{};
    size_t completed_{};

    // These are thread safe.
    const paths paths_;
    const std::optional<uint8_t> fill_{};
    const bool zeroed_{ true };
    mutable shared_mutex field_mutex_{};
    mutable shared_mutex map_mutex_{};
};

BC_POP_WARNING()

using chunk_storage = chunk_storages<one>;

/// The head fill for links (store heads set it through head settings).
constexpr std::optional<uint8_t> terminal_fill{ system::bit_all<uint8_t> };

} // namespace test

#endif
