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
#ifndef LIBBITCOIN_DATABASE_MEMORY_MAP_SINK_HPP
#define LIBBITCOIN_DATABASE_MEMORY_MAP_SINK_HPP

#include <memory>
#include <utility>
#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/interfaces/memory.hpp>

namespace libbitcoin {
namespace database {

/// Sink for ios::stream, copies bytes to/from memory_ptr.
class map_sink
  : public system::device<memory>
{
public:
    typedef system::device<memory> base;
    typedef const memory_ptr& container;
    struct category
      : system::ios::seekable, system::ios::direct_tag
    {
    };

    /// data.ptr must not be nullptr and data.ptr->begin() must be non-null.
    map_sink(const memory_ptr& data) NOEXCEPT
      : base(system::limit<typename base::size_type>(data->size())),
        container_(data),
        next_(data->begin())
    {
    }

    map_sink(map_sink&&) = default;
    map_sink(const map_sink&) = default;
    map_sink& operator=(map_sink&&) = delete;
    map_sink& operator=(const map_sink&) = delete;
    ~map_sink() override = default;

protected:
    typename base::sequence do_sequence() const NOEXCEPT override
    {
        using char_type = typename base::char_type;
        return std::make_pair(
            system::pointer_cast<char_type>(container_->begin()),
            system::pointer_cast<char_type>(container_->end()));
    }

private:
    const memory::ptr container_;
    typename memory::iterator next_;
};

/// A byte flipper with custom flush, that accepts an iostream.
template <typename IOStream = std::iostream>
class finalizing_flipper
  : public system::byte_flipper<IOStream>
{
public:
    DEFAULT5(finalizing_flipper);

    using finalizer = std::function<bool()>;

    finalizing_flipper(IOStream& stream) NOEXCEPT
      : system::byte_flipper<IOStream>(stream)
    {
    }

    void set_finalizer(finalizer&& functor) NOEXCEPT
    {
        finalize_ = std::move(functor);
    }

    // This is expected to have side effect on the stream buffer, specifically
    // setting the "next" pointer into beginning of the address space.
    bool finalize() NOEXCEPT
    {
        // std::function does not allow for noexcept.
        BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
        return finalize_();
        BC_POP_WARNING()
    }

private:
    finalizer finalize_;
};

/// A finalizing byte reader/writer that copies data from/to a memory_ptr.
using writer = system::make_streamer<map_sink, finalizing_flipper>;
typedef std::shared_ptr<writer> writer_ptr;

} // namespace database
} // namespace libbitcoin

#endif
