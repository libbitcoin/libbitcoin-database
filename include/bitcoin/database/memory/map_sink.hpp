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

/// Sink for ios::stream, copies bytes to memory_ptr.
class map_sink
  : public system::device<memory>
{
public:
    DEFAULT5(map_sink);

    typedef system::device<memory> base;
    typedef const memory_ptr& container;
    struct category
      : system::ios::output_seekable, system::ios::direct_tag
    {
    };

    map_sink(const memory_ptr& data) NOEXCEPT
      : base(system::limit<typename base::size_type>(data->size())),
        container_(data),
        next_(data->begin())
    {
    }

protected:
    typename base::sequence do_sequence() const NOEXCEPT override
    {
        using char_type = typename base::char_type;
        const auto first = system::pointer_cast<char_type>(&(*container_->begin()));
        return std::make_pair(first, std::next(first, container_->size()));
    }

private:
    const memory::ptr container_;
    typename memory::iterator next_;
};

namespace write
{
    namespace bytes
    {
        /// A byte writer that copies data to a memory_ptr.
        using copy = system::make_streamer<map_sink, system::byte_writer>;
    }
}

typedef std::shared_ptr<write::bytes::copy> map_sink_ptr;

} // namespace database
} // namespace libbitcoin

#endif
