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
#ifndef LIBBITCOIN_DATABASE_PRIMITIVES_HEAD_IPP
#define LIBBITCOIN_DATABASE_PRIMITIVES_HEAD_IPP

#include <algorithm>
#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

TEMPLATE
CLASS::head(storage& head, const Link& buckets) NOEXCEPT
  : file_(head), buckets_(buckets)
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
    if (!ptr)
        return false;

    std::fill_n(ptr->begin(), size, system::bit_all<uint8_t>);
    return verify() && set_body_count(zero);
}

TEMPLATE
bool CLASS::verify() const NOEXCEPT
{
    return offset(buckets_) == file_.size();
}

TEMPLATE
bool CLASS::get_body_count(Link& count) const NOEXCEPT
{
    const auto ptr = file_.get();
    if (!ptr)
        return false;

    count = array_cast<Link::size>(*ptr);
    return true;
}

TEMPLATE
bool CLASS::set_body_count(const Link& count) NOEXCEPT
{
    const auto ptr = file_.get();
    if (!ptr)
        return false;

    array_cast<Link::size>(*ptr) = count;
    return true;
}

TEMPLATE
Link CLASS::top(const Key& key) const NOEXCEPT
{
    return top(index(key));
}

TEMPLATE
Link CLASS::top(const Link& index) const NOEXCEPT
{
    const auto ptr = file_.get(offset(index));
    if (!ptr)
        return Link::terminal;

    const auto& head = array_cast<Link::size>(*ptr);

    mutex_.lock_shared();
    const auto top = head;
    mutex_.unlock_shared();
    return top;
}

TEMPLATE
bool CLASS::push(const bytes& current, bytes& next, const Key& key) NOEXCEPT
{
    return push(current, next, index(key));
}

TEMPLATE
bool CLASS::push(const bytes& current, bytes& next, const Link& index) NOEXCEPT
{
    const auto ptr = file_.get(offset(index));
    if (!ptr)
        return false;

    auto& head = array_cast<Link::size>(*ptr);

    mutex_.lock();
    next = head;
    head = current;
    mutex_.unlock();
    return true;
}

} // namespace database
} // namespace libbitcoin

#endif
