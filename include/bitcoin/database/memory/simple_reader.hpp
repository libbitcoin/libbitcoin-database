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
#ifndef LIBBITCOIN_DATABASE_MEMORY_SIMPLE_READER_HPP
#define LIBBITCOIN_DATABASE_MEMORY_SIMPLE_READER_HPP

#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/interfaces/memory.hpp>

namespace libbitcoin {
namespace database {
    
BC_PUSH_WARNING(NO_UNSAFE_COPY_N)
BC_PUSH_WARNING(NO_POINTER_ARITHMETIC)

class simple_reader
{
public:
    inline simple_reader(memory& buffer) NOEXCEPT
      : data_(buffer.begin()), begin_(data_), end_(buffer.end())
    {
        if (data_ > end_)
            fault_ = true;
    }

    template <typename Integer, size_t Size = sizeof(Integer)>
    inline Integer read_big_endian() NOEXCEPT
    {
        if (overflow(Size)) return {};
        Integer value{};
        auto& bytes = system::byte_cast(value);
        std::copy_n(data_, Size, bytes.data());
        data_ += Size;
        return system::native_from_big_end(value);
    }

    template <typename Integer, size_t Size = sizeof(Integer)>
    inline Integer read_little_endian() NOEXCEPT
    {
        if (overflow(Size)) return {};
        Integer value{};
        auto& bytes = system::byte_cast(value);
        std::copy_n(data_, Size, bytes.data());
        data_ += Size;
        return system::native_from_little_end(value);
    }

    template <size_t Size>
    inline system::data_array<Size> read_forward() NOEXCEPT
    {
        if (overflow(Size)) return {};
        const auto start = data_;
        data_ += Size;
        return system::unsafe_array_cast<uint8_t, Size>(start);
    }

    inline system::hash_digest read_hash() NOEXCEPT
    {
        return read_forward<system::hash_size>();
    }

    inline system::data_chunk read_bytes(size_t size) NOEXCEPT
    {
        if (overflow(size)) return {};
        const auto start = data_;
        data_ += size;
        return { start, data_ };
    }

    /// Normal consensus form.
    inline uint64_t read_variable() NOEXCEPT
    {
        switch (const auto value = read_byte())
        {
            case varint_eight_bytes:
                return read_little_endian<uint64_t>();
            case varint_four_bytes:
                return read_little_endian<uint32_t>();
            case varint_two_bytes:
                return read_little_endian<uint16_t>();
            default:
                return value;
        }
    }

    inline size_t read_size(size_t limit=max_size_t) NOEXCEPT
    {
        const auto value = read_variable();

        if (value > limit)
        {
            invalidate();
            return {};
        }

        return system::possible_narrow_cast<size_t>(value);
    }

    inline uint8_t read_byte() NOEXCEPT
    {
        if (overflow(one)) return {};
        const auto value = *data_;
        data_ += one;
        return value;
    }

    inline void skip_bytes(size_t size) NOEXCEPT
    {
        if (overflow(size)) return;
        data_ += size;
    }

    inline void rewind_bytes(size_t size) NOEXCEPT
    {
        if (underflow(size)) return;
        data_ -= size;
    }

    inline void set_limit() NOEXCEPT
    {
        // TODO: disable limit in definition of tables that don't use it.
        // TODO: that is presently only the bootstrap cache table.
    }

    inline void invalidate() NOEXCEPT
    {
        fault_ = true;
    }

    inline size_t get_read_position() const NOEXCEPT
    {
        return data_ - begin_;
    }

    inline operator bool() const NOEXCEPT
    {
        return !fault_;
    }

private:
    inline bool overflow(size_t size) NOEXCEPT
    {
        if (fault_)
            return true;

        if (system::is_greater(size, end_ - data_))
        {
            invalidate();
            return true;
        }

        return false;
    }

    inline bool underflow(size_t size) NOEXCEPT
    {
        if (fault_)
            return true;

        if (system::is_greater(size, data_ - begin_))
        {
            invalidate();
            return true;
        }

        return false;
    }

    bool fault_{};
    uint8_t* data_;
    const uint8_t* begin_;
    const uint8_t* end_;
};

BC_POP_WARNING()
BC_POP_WARNING()

} // namespace database
} // namespace libbitcoin

#include <bitcoin/database/impl/memory/simple_reader.ipp>

#endif
