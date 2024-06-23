/**
 * Copyright (c) 2011-2023 libbitcoin developers (see AUTHORS)
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

// Heads are not subject to resize/remap and therefore do not require memory
// smart pointer with shared remap lock. Using get_raw() saves that allocation.

namespace libbitcoin {
namespace database {

TEMPLATE
CLASS::head(storage& head, const Link& buckets) NOEXCEPT
  : file_(head), buckets_(buckets)
{
}

TEMPLATE
size_t CLASS::size() const NOEXCEPT
{
    return offset(buckets_);
}

TEMPLATE
size_t CLASS::buckets() const NOEXCEPT
{
    return buckets_;
}

TEMPLATE
Link CLASS::index(const Key& key) const NOEXCEPT
{
    BC_ASSERT_MSG(is_nonzero(buckets_), "hash table requires buckets");

    // TODO: for greater flexibility, inject hash function through template.
    if constexpr (Hash)
    {
        // djb2_hash exhibits very poor uniqueness result for sequential keys.
        return system::djb2_hash(key) % buckets_;
    }
    else
    {
        // unique_hash assumes sufficient uniqueness in low order key bytes.
        return system::unique_hash(key) % buckets_;
    }
}

TEMPLATE
bool CLASS::create() NOEXCEPT
{
    if (is_nonzero(file_.size()))
        return false;

    const auto allocation = size();
    const auto start = file_.allocate(allocation);

    // This guards addition overflow in file_.get_raw (start must be valid).
    if (start == storage::eof)
        return false;

    const auto raw = file_.get_raw(start);
    if (is_null(raw))
        return false;

    BC_ASSERT_MSG(verify(), "unexpected body size");

    // std::memset/fill_n have identical performance (on win32).
    ////std::memset(raw, system::bit_all<uint8_t>, allocation);
    std::fill_n(raw, allocation, system::bit_all<uint8_t>);
    return set_body_count(zero);
}

TEMPLATE
bool CLASS::verify() const NOEXCEPT
{
    return file_.size() == size();
}

TEMPLATE
bool CLASS::get_body_count(Link& count) const NOEXCEPT
{
    const auto raw = file_.get_raw();
    if (is_null(raw))
        return false;

    count = array_cast<Link::size>(raw);
    return true;
}

TEMPLATE
bool CLASS::set_body_count(const Link& count) NOEXCEPT
{
    const auto raw = file_.get_raw();
    if (is_null(raw))
        return false;

    array_cast<Link::size>(raw) = count;
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
    const auto raw = file_.get_raw(offset(index));
    if (is_null(raw))
        return {};

    const auto& head = array_cast<Link::size>(raw);

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
    const auto raw = file_.get_raw(offset(index));
    if (is_null(raw))
        return false;

    auto& head = array_cast<Link::size>(raw);

    mutex_.lock();
    next = head;
    head = current;
    mutex_.unlock();
    return true;
}

} // namespace database
} // namespace libbitcoin

#endif
