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
#ifndef LIBBITCOIN_DATABASE_MEMORY_WRITER_HPP
#define LIBBITCOIN_DATABASE_MEMORY_WRITER_HPP

#include <memory>
#include <utility>
#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/interfaces/memory.hpp>

namespace libbitcoin {
namespace database {
    
/// Sink for ios::stream, copies bytes to/from memory_ptr.
/// MEMBER MEMORY_PTR HOLDS SHARED LOCK ON STORAGE REMAP, DO NOT EXTEND LIFETIME.
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

/// A byte reader/writer that copies data from/to a memory_ptr.
using writer = system::make_streamer<map_sink, system::byte_flipper>;
typedef std::shared_ptr<writer> writer_ptr;

} // namespace database
} // namespace libbitcoin

#endif
