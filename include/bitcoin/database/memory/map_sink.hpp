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

#include <functional>
#include <memory>
#include <utility>
#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/interfaces/memory.hpp>

namespace libbitcoin {
namespace database {

struct sink
{
    const memory_ptr ptr;
    const std::function<void(uint8_t*)> finalize;
};

/// Sink for ios::stream, copies bytes to/from memory_ptr.
class map_sink
  : public system::device<memory>
{
public:
    DEFAULT4(map_sink);

    typedef system::device<memory> base;
    typedef const sink& container;
    struct category
      : system::ios::seekable, system::ios::direct_tag
    {
    };

    map_sink(const sink& data) NOEXCEPT
      : base(system::limit<typename base::size_type>(data.ptr->size())),
        record_(data.ptr),
        next_(data.ptr->begin()),
        finalize_(data.finalize)
    {
    }

    /// Add the record to the hash table.
    ~map_sink() NOEXCEPT override
    {
        // std::function does not allow noexcept qualifier.
        BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
        finalize_(record_->begin());
        BC_POP_WARNING()
    }

protected:
    typename base::sequence do_sequence() const NOEXCEPT override
    {
        using char_type = typename base::char_type;
        return std::make_pair(
            system::pointer_cast<char_type>(record_->begin()),
            system::pointer_cast<char_type>(record_->end()));
    }

private:
    const memory::ptr record_;
    typename memory::iterator next_;
    const std::function<void(uint8_t*)> finalize_;
};

/// A byte reader/writer that copies data from/to a memory_ptr.
using writer = system::make_streamer<map_sink, system::byte_flipper>;
typedef std::shared_ptr<writer> writer_ptr;

} // namespace database
} // namespace libbitcoin

#endif
