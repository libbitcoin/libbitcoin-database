/**
 * Copyright (c) 2011-2019 libbitcoin developers (see AUTHORS)
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

// Sponsored in part by Digital Contract Design, LLC

#include <bitcoin/database/result/filter_result.hpp>

#include <cstddef>
#include <cstdint>
#include <utility>
#include <bitcoin/system.hpp>
#include <bitcoin/database/databases/filter_database.hpp>
#include <bitcoin/database/memory/memory.hpp>

namespace libbitcoin {
namespace database {

using namespace bc::system;
using namespace bc::system::chain;

filter_result::filter_result(const const_element_type& element,
    shared_mutex& metadata_mutex, uint8_t filter_type)
  : filter_type_(filter_type),
    element_(element),
    metadata_mutex_(metadata_mutex)
{
    if (!element_)
        return;
}

filter_result::operator bool() const
{
    return element_;
}

file_offset filter_result::link() const
{
    return element_.link();
}

uint8_t filter_result::filter_type() const
{
    return filter_type_;
}

//hash_digest filter_result::block_hash() const
//{
//    // This is read each time it is invoked, so caller should cache.
//    return element_ ? element_.key() : null_hash;
//}

hash_digest filter_result::header() const
{
    // This is read each time it is invoked, so caller should cache.
    return element_ ? element_.key() : null_hash;
}

system::data_chunk filter_result::filter() const
{
    // This is read each time it is invoked, so caller should cache.
    if (!element_)
        return {};

    data_chunk filter;

    const auto reader = [&](byte_deserializer& deserial)
    {
        size_t size = deserial.read_size_little_endian();
        filter = deserial.read_bytes(size);
    };

    element_.read(reader);
    return filter;
}

chain::block_filter filter_result::block_filter() const
{
    BITCOIN_ASSERT(element_);
    chain::block_filter result;
    result.set_filter(filter());
    result.set_header(header());
    result.metadata.link = element_.link();
    return result;
}

} // namespace database
} // namespace libbitcoin
