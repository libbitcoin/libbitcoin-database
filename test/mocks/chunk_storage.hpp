/**
 * Copyright (c) 2011-2023 libbitcoin developers (see AUTHORS)
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

#include "../test.hpp"
#include <filesystem>

namespace test {

// A thread safe storage implementation built on data_chunk.
class chunk_storage
  : public database::storage
{
public:
    chunk_storage() NOEXCEPT;
    chunk_storage(system::data_chunk& reference) NOEXCEPT;
    chunk_storage(const std::filesystem::path& filename, size_t minimum=1,
        size_t expansion=0) NOEXCEPT;

    // test side door.
    system::data_chunk& buffer() NOEXCEPT;

    // storage interface.
    code open() NOEXCEPT override;
    code close() NOEXCEPT override;
    code load() NOEXCEPT override;
    code reload() NOEXCEPT override;
    code flush() NOEXCEPT override;
    code unload() NOEXCEPT override;
    const std::filesystem::path& file() const NOEXCEPT override;
    size_t capacity() const NOEXCEPT override;
    size_t size() const NOEXCEPT override;
    bool truncate(size_t size) NOEXCEPT override;
    size_t allocate(size_t chunk) NOEXCEPT override;
    memory_ptr get(size_t offset=zero) const NOEXCEPT override;
    memory::iterator get_raw(size_t offset=zero) const NOEXCEPT override;
    code get_fault() const NOEXCEPT override;
    size_t get_space() const NOEXCEPT override;

private:
    // These are protected by mutex.
    system::data_chunk local_{};
    system::data_chunk& buffer_;

    // These are thread safe.
    const std::filesystem::path path_;
    mutable std::shared_mutex field_mutex_{};
    mutable std::shared_mutex map_mutex_{};
};

} // namespace test

#endif
