/**
 * Copyright (c) 2011-2026 libbitcoin developers (see AUTHORS)
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
#ifndef LIBBITCOIN_DATABASE_PRIMITIVES_NOHEAD_IPP
#define LIBBITCOIN_DATABASE_PRIMITIVES_NOHEAD_IPP

#include <algorithm>
#include <bitcoin/database/define.hpp>

// TODO: xcode clang++16 does not support C++20 std::atomic_ref.
////const std::atomic_ref<link> head(unsafe_byte_cast<link>(raw));
namespace libbitcoin {
namespace database {

TEMPLATE
CLASS::nohead(storage& head, const Link& buckets) NOEXCEPT
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
    using namespace system;
    return possible_narrow_cast<size_t>(position_to_link(size()).value);
}

TEMPLATE
bool CLASS::enabled() const NOEXCEPT
{
    return !is_zero(initial_buckets_);
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

    // Body count is written as the first value in link size, but since
    // offsetting is a multiple of cell size, a full cell is consumed for it.
    // In case of nomap or disabled there are no cells, so file is link size.
    count = to_array<Link::size>(ptr->data());
    return true;
}

TEMPLATE
bool CLASS::set_body_count(const Link& count) NOEXCEPT
{
    const auto ptr = file_.get();
    if (!ptr || Link::size > size())
        return false;

    // Body count is written as the first value in link size, but since
    // offsetting is a multiple of cell size, a full cell is consumed for it.
    // In case of nomap or disabled there are no cells, so file is link size.
    to_array<Link::size>(ptr->data()) = count;
    return true;
}

} // namespace database
} // namespace libbitcoin

#endif
