/**
 * Copyright (c) 2011-2022 libbitcoin developers (see AUTHORS)
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
#ifndef LIBBITCOIN_DATABASE_TEST_STORAGE_HPP
#define LIBBITCOIN_DATABASE_TEST_STORAGE_HPP

#include "test.hpp"

namespace test {

// Fake a thread safe memory map implementation.
class storage
  : public database::storage
{
public:
    storage() NOEXCEPT;
    storage(system::data_chunk& reference) NOEXCEPT;

    // storage interface
    size_t capacity() const NOEXCEPT override;
    size_t size() const NOEXCEPT override;
    bool resize(size_t size) NOEXCEPT override;
    size_t allocate(size_t chunk) NOEXCEPT override;
    memory_ptr get(size_t offset=zero) const NOEXCEPT override;

private:
    system::data_chunk local_;
    system::data_chunk& buffer_;
    mutable std::shared_mutex field_mutex_;
    mutable std::shared_mutex map_mutex_;
};

}

#endif
