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

    const auto size = offset(buckets_);
    const auto start = file_.allocate(size);
    if (start == storage::eof)
        return false;

    const auto ptr = file_.get(start);
    std::fill_n(ptr->begin(), size, system::bit_all<uint8_t>);

    const auto valid = verify();
    if (valid)
        set_body_count(zero);

    return valid;
}

TEMPLATE
bool CLASS::verify() const NOEXCEPT
{
    return offset(buckets_) == file_.size();
}

TEMPLATE
Link CLASS::get_body_count() const NOEXCEPT
{
    return array_cast<Link::size>(*file_.get());
}

TEMPLATE
void CLASS::set_body_count(const Link& count) NOEXCEPT
{
    array_cast<Link::size>(*file_.get()) = count;
}

TEMPLATE
Link CLASS::head(const Key& key) const NOEXCEPT
{
    return head(index(key));
}

TEMPLATE
Link CLASS::head(const Link& index) const NOEXCEPT
{
    const auto& head = array_cast<Link::size>(*file_.get(offset(index)));

    mutex_.lock_shared();
    const auto top = head;
    mutex_.unlock_shared();
    return top;
}

TEMPLATE
void CLASS::push(const Link& current, Link& next, const Key& key) NOEXCEPT
{
    push(current, next, index(key));
}

TEMPLATE
void CLASS::push(const Link& current, Link& next, const Link& index) NOEXCEPT
{
    auto& head = array_cast<Link::size>(*file_.get(offset(index)));

    mutex_.lock();
    next = head;
    head = current;
    mutex_.unlock();
}

} // namespace database
} // namespace libbitcoin

#endif
