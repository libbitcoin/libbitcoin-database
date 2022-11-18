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
#ifndef LIBBITCOIN_DATABASE_MEMORY_READER_HPP
#define LIBBITCOIN_DATABASE_MEMORY_READER_HPP

#include <memory>
#include <utility>
#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/interfaces/memory.hpp>

namespace libbitcoin {
namespace database {
    
/// Source for ios::stream, copies bytes from memory_ptr.
class map_source
  : public system::device<memory>
{
public:
    typedef system::device<memory> base;
    typedef const memory_ptr& container;
    struct category
      : system::ios::input_seekable, system::ios::direct_tag
    {
    };

    map_source(const memory_ptr& data) NOEXCEPT
      : base(system::limit<typename base::size_type>(data->size())),
        container_(data),
        next_(data->begin())
    {
    }

    map_source(map_source&&) = default;
    map_source(const map_source&) = default;
    map_source& operator=(map_source&&) = delete;
    map_source& operator=(const map_source&) = delete;
    ~map_source() override = default;

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
    typename memory::const_iterator next_;
};

/// A byte reader that copies data from a memory_ptr.
using reader = system::make_streamer<map_source, system::byte_reader>;
typedef std::shared_ptr<reader> reader_ptr;

} // namespace database
} // namespace libbitcoin

#endif
