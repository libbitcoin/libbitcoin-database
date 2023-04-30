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
#ifndef LIBBITCOIN_DATABASE_MEMORY_SIMPLE_WRITER_HPP
#define LIBBITCOIN_DATABASE_MEMORY_SIMPLE_WRITER_HPP

#include <functional>
#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/interfaces/memory.hpp>

namespace libbitcoin {
namespace database {
    
BC_PUSH_WARNING(NO_UNSAFE_COPY_N)
BC_PUSH_WARNING(NO_POINTER_ARITHMETIC)

class simple_writer
{
public:
    inline simple_writer(memory& buffer) NOEXCEPT
      : data_(buffer.begin()), begin_(data_), end_(buffer.end())
    {
        if (data_ > end_)
            fault_ = true;
    }

    template <typename Integer, size_t Size = sizeof(Integer)>
    inline void write_big_endian(Integer value) NOEXCEPT
    {
        if (overflow(Size)) return;
        const auto bytes = system::byte_cast(system::native_to_big_end(value));
        std::copy_n(bytes.data(), Size, data_);
        data_ += Size;
    }

    template <typename Integer, size_t Size = sizeof(Integer)>
    inline void write_little_endian(Integer value) NOEXCEPT
    {
        if (overflow(Size)) return;
        const auto bytes = system::byte_cast(system::native_to_little_end(value));
        std::copy_n(bytes.data(), Size, data_);
        data_ += Size;
    }

    inline void write_bytes(const system::data_slice& data) NOEXCEPT
    {
        const auto size = data.size();
        if (overflow(size)) return;
        std::copy_n(data.data(), size, data_);
        data_ += size;
    }

    inline void write_byte(uint8_t value) NOEXCEPT
    {
        if (overflow(one)) return;
        *data_ = value;
        data_ += one;
    }

    /// Normal consensus form.
    inline void write_variable(uint64_t value) NOEXCEPT
    {
        if (value < varint_two_bytes)
        {
            write_byte(system::narrow_cast<uint8_t>(value));
        }
        else if (value <= max_uint16)
        {
            write_byte(varint_two_bytes);
            write_little_endian(system::narrow_cast<uint16_t>(value));
        }
        else if (value <= max_uint32)
        {
            write_byte(varint_four_bytes);
            write_little_endian(system::narrow_cast<uint32_t>(value));
        }
        else
        {
            write_byte(varint_eight_bytes);
            write_little_endian(value);
        }
    }

    inline void skip_bytes(size_t size) NOEXCEPT
    {
        if (overflow(size)) return;
        data_ += size;
    }

    inline void invalidate() NOEXCEPT
    {
        fault_ = true;
    }

    inline size_t get_write_position() const NOEXCEPT
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

    bool fault_{};
    uint8_t* data_;
    const uint8_t* begin_;
    const uint8_t* end_;
};

class simple_finalizer
  : public simple_writer
{
public:
    using finalization = std::function<bool()>;

    using simple_writer::simple_writer;

    inline void set_finalizer(finalization&& functor) NOEXCEPT
    {
        finalize_ = std::move(functor);
    }

    // This is expected to have side effect on the stream buffer, specifically
    // setting the "next" pointer into beginning of the address space.
    inline bool finalize() NOEXCEPT
    {
        if (!*this) return false;

        // std::function does not allow for noexcept.
        BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
        return finalize_();
        BC_POP_WARNING()
    }

private:
    finalization finalize_;
};

BC_POP_WARNING()
BC_POP_WARNING()

} // namespace database
} // namespace libbitcoin

#include <bitcoin/database/impl/memory/simple_writer.ipp>

#endif
