/**
 * Copyright (c) 2011-2025 libbitcoin developers (see AUTHORS)
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
#ifndef LIBBITCOIN_DATABASE_PRIMITIVES_HEAD2_IPP
#define LIBBITCOIN_DATABASE_PRIMITIVES_HEAD2_IPP

#include <algorithm>
#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

TEMPLATE
CLASS::head2(storage& head, const Link& buckets) NOEXCEPT
  : file_(head), initial_buckets_(buckets)
{
}

TEMPLATE
size_t CLASS::size() const NOEXCEPT
{
    return file_.size();
}

TEMPLATE
size_t CLASS::buckets() const NOEXCEPT
{
    const auto count = position_to_link(size()).value;
    BC_ASSERT(count < Link::terminal);
    return system::possible_narrow_cast<size_t>(count);
}

TEMPLATE
bool CLASS::enabled() const NOEXCEPT
{
    return initial_buckets_ > one;
}

TEMPLATE
Link CLASS::index(const Key& key) const NOEXCEPT
{
    // Key is the logical bucket index (no-hash).
    if (key < buckets())
        return manager<Link, system::data_array<zero>, Link::size>::
            cast_link(key);

    return {};
}

TEMPLATE
bool CLASS::create() NOEXCEPT
{
    if (is_nonzero(file_.size()))
        return false;

    const auto allocation = link_to_position(initial_buckets_);
    const auto start = file_.allocate(allocation);

    // Guards addition overflow in manager_.get (start must be valid).
    if (start == storage::eof)
        return false;

    const auto ptr = file_.get(start);
    if (!ptr)
        return false;

    BC_ASSERT_MSG(verify(), "unexpected body size");

    // std::memset/fill_n have identical performance (on win32).
    ////std::memset(ptr->data(), system::bit_all<uint8_t>, size());
    std::fill_n(ptr->data(), size(), system::bit_all<uint8_t>);
    return set_body_count(zero);
}

TEMPLATE
bool CLASS::verify() const NOEXCEPT
{
    return buckets() >= initial_buckets_;
}

TEMPLATE
bool CLASS::get_body_count(Link& count) const NOEXCEPT
{
    const auto ptr = file_.get();
    if (!ptr)
        return false;

    count = array_cast<Link::size>(ptr->data());
    return true;
}

TEMPLATE
bool CLASS::set_body_count(const Link& count) NOEXCEPT
{
    const auto ptr = file_.get();
    if (!ptr)
        return false;

    array_cast<Link::size>(ptr->data()) = count;
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
    const auto ptr = file_.get(link_to_position(index));
    if (is_null(ptr))
        return {};

    const auto& head = array_cast<Link::size>(ptr->data());

    mutex_.lock_shared();
    const auto top = head;
    mutex_.unlock_shared();
    return top;
}

TEMPLATE
bool CLASS::push(const bytes& current, const Link& index) NOEXCEPT
{
    constexpr auto fill = system::bit_all<uint8_t>;

    // Allocate as necessary and fill allocations.
    const auto ptr = file_.set(link_to_position(index), Link::size, fill);

    if (is_null(ptr))
        return false;

    auto& head = array_cast<Link::size>(ptr->data());

    mutex_.lock();
    head = current;
    mutex_.unlock();
    return true;
}

} // namespace database
} // namespace libbitcoin

#endif
