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
#ifndef LIBBITCOIN_DATABASE_PRIMITIVES_ARRAYHEAD_IPP
#define LIBBITCOIN_DATABASE_PRIMITIVES_ARRAYHEAD_IPP

#include <algorithm>
#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

TEMPLATE
CLASS::arrayhead(storage& head, const Link& buckets) NOEXCEPT
  : file_(head), initial_buckets_(buckets.value)
{
}

TEMPLATE
inline size_t CLASS::size() const NOEXCEPT
{
    return file_.size();
}

TEMPLATE
inline size_t CLASS::buckets() const NOEXCEPT
{
    const auto count = position_to_link(size()).value;
    BC_ASSERT(count < Link::terminal);
    return system::possible_narrow_cast<size_t>(count);
}

TEMPLATE
bool CLASS::enabled() const NOEXCEPT
{
    return !is_zero(initial_buckets_);
}

TEMPLATE
inline Link CLASS::index(size_t key) const NOEXCEPT
{
    // buckets a table lock via file.size().
    ////if (key >= buckets()) return {};
    ////BC_ASSERT_MSG(key < buckets(), "index overflow");

    // Put index does not validate, allowing for head expansion.
    return putter_index(key);
}

TEMPLATE
inline Link CLASS::putter_index(size_t key) const NOEXCEPT
{
    // Key is the logical bucket index (no-hash).
    return body::cast_link(key);
}

TEMPLATE
bool CLASS::clear() NOEXCEPT
{
    const auto ptr = file_.get();
    if (!ptr)
        return false;

    // Retains head size, since head is array not map, and resets body logical
    // count to zero, which is picked up in arraymap::reset(). Body file size
    // remains unchanged and subject to initialization size at each startup. So
    // there is no reduction until restart, which can include config change.
    std::fill_n(ptr->data(), size(), system::bit_all<uint8_t>);
    return set_body_count(zero);
}

TEMPLATE
bool CLASS::create() NOEXCEPT
{
    if (is_nonzero(size()))
        return false;

    // Guards addition overflow in manager_.get (start must be valid).
    if (file_.allocate(link_to_position(initial_buckets_)) == storage::eof)
        return false;

    BC_ASSERT_MSG(verify(), "unexpected head size");
    return clear();
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
    if (!ptr || Link::size > size())
        return false;

    count = array_cast<Link::size>(ptr->data());
    return true;
}

TEMPLATE
bool CLASS::set_body_count(const Link& count) NOEXCEPT
{
    const auto ptr = file_.get();
    if (!ptr || Link::size > size())
        return false;

    array_cast<Link::size>(ptr->data()) = count;
    return true;
}

TEMPLATE
Link CLASS::at(size_t key) const NOEXCEPT
{
    const auto link = index(key);
    if (link.is_terminal())
        return {};

    const auto ptr = file_.get(link_to_position(link));
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
