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
#include <mutex>
#include <shared_mutex>
#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/storage.hpp>

namespace libbitcoin {
namespace database {

TEMPLATE
CLASS::hash_table_header(storage& header, Link buckets) NOEXCEPT
  : file_(header), buckets_(buckets)
{
    BC_ASSERT_MSG(!is_zero(buckets), "no buckets");
}

TEMPLATE
Link CLASS::hash(const Key& key) const NOEXCEPT
{
    return system::djb2_hash(key) % buckets_;
}

TEMPLATE
bool CLASS::create() NOEXCEPT
{
    if (!is_zero(file_.size())) return false;

    // Allocate the file size and get map.
    const auto header = file_.get(file_.allocate(offset(buckets_)));
    if (!header) return false;

    // Fill header file with eof (0xff) bytes.
    constexpr auto fill = system::narrow_cast<uint8_t>(Link::eof);
    std::fill(header->begin(), header->end(), fill);

    // Overwrite start of file with initial body "size" (zero).
    return set_body_size(zero);
}

TEMPLATE
bool CLASS::set_body_size(Link size) NOEXCEPT
{
    // This should only be called at checkpoint/close.
    const auto header = file_.get();
    if (!header) return false;

    array_cast<Link::size>(*header) = size;
    return true;
}

TEMPLATE
bool CLASS::get_body_size(Link& size) const NOEXCEPT
{
    // This should only be read at startup (checkpoint recovery).
    const auto header = file_.get();
    if (!header) return false;

    size = array_cast<Link::size>(*header);
    return true;
}

TEMPLATE
Link CLASS::head(const Key& key) const NOEXCEPT
{
    return head(hash(key));
}

TEMPLATE
Link CLASS::head(Link index) const NOEXCEPT
{
    const auto header = file_.get(offset(index));
    if (!header) return Link::eof;

    const auto& head = array_cast<Link::size>(*header);

    mutex_.lock_shared();
    const auto top = head;
    mutex_.unlock_shared();
    return top;
}

TEMPLATE
bool CLASS::push(Link current, Link& next, const Key& key) NOEXCEPT
{
    return push(current, next, hash(key));
}

TEMPLATE
bool CLASS::push(Link current, Link& next, Link index) NOEXCEPT
{
    const auto header = file_.get(offset(index));
    if (!header) return false;

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
