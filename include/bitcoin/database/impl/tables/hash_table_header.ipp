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
#ifndef LIBBITCOIN_DATABASE_TABLES_HASH_TABLE_HEADER_IPP
#define LIBBITCOIN_DATABASE_TABLES_HASH_TABLE_HEADER_IPP

#include <algorithm>
#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

TEMPLATE
CLASS::hash_table_header(storage& header, const Link& buckets) NOEXCEPT
  : file_(header), buckets_(buckets)
{
    BC_ASSERT_MSG(!is_zero(buckets), "no buckets");
}

TEMPLATE
Link CLASS::index(const Key& key) const NOEXCEPT
{
    return system::djb2_hash(key) % buckets_;
}

TEMPLATE
bool CLASS::create() NOEXCEPT
{
    if (!is_zero(file_.size()))
        return false;

    // Allocate the file size and get map.
    const auto size = offset(buckets_);
    const auto header = file_.get(file_.allocate(size));
    if (!header)
        return false;

    // Fill header file with terminal (0xff) bytes.
    std::fill_n(header->begin(), size, system::bit_all<uint8_t>);

    // Overwrite start of file with initial body "size" (zero).
    return verify() && set_body_count(zero);
}

TEMPLATE
bool CLASS::verify() const NOEXCEPT
{
    // Byte offset vs. byte size, must be same.
    return offset(buckets_) == file_.size();
}

TEMPLATE
bool CLASS::set_body_count(const Link& count) NOEXCEPT
{
    // This should only be called at checkpoint/close.
    const auto header = file_.get();
    if (!header)
        return false;

    array_cast<Link::size>(*header) = count;
    return true;
}

TEMPLATE
bool CLASS::get_body_count(Link& count) const NOEXCEPT
{
    // This should only be read at startup (checkpoint recovery).
    const auto header = file_.get();
    if (!header)
        return false;

    count = array_cast<Link::size>(*header);
    return true;
}

TEMPLATE
Link CLASS::head(const Key& key) const NOEXCEPT
{
    return head(index(key));
}

TEMPLATE
Link CLASS::head(const Link& index) const NOEXCEPT
{
    const auto header = file_.get(offset(index));
    if (!header)
        return Link::terminal;

    // const head link& as byte array
    const auto& head = array_cast<Link::size>(*header);

    mutex_.lock_shared();
    const auto top = head;
    mutex_.unlock_shared();
    return top;
}

TEMPLATE
bool CLASS::push(const Link& current, Link& next, const Key& key) NOEXCEPT
{
    return push(current, next, index(key));
}

TEMPLATE
bool CLASS::push(const Link& current, Link& next, const Link& index) NOEXCEPT
{
    // This can only return false if file is unmapped.
    const auto header = file_.get(offset(index));
    if (!header)
        return false;

    // head link& as byte array
    auto& head = array_cast<Link::size>(*header);

    mutex_.lock();
    next = head;
    head = current;
    mutex_.unlock();
    return true;
}

} // namespace database
} // namespace libbitcoin

#endif
