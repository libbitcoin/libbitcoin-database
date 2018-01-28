/**
 * Copyright (c) 2011-2017 libbitcoin developers (see AUTHORS)
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
#include <bitcoin/database/result/transaction_iterator.hpp>

#include <cstddef>
#include <bitcoin/database/memory/memory.hpp>

namespace libbitcoin {
namespace database {

using namespace bc::chain;

transaction_iterator::transaction_iterator(const manager& records,
    size_t start, size_t count)
  : offset_(0),
    index_(0),
    start_(start),
    count_(count),
    manager_(records)
{
    populate();
}

void transaction_iterator::populate()
{
    if (index_ < count_)
    {
        const auto memory = manager_.get(start_);
        memory->increment(index_ * sizeof(offset_));
        auto deserial = make_unsafe_deserializer(memory->buffer());
        offset_ = deserial.read_8_bytes_little_endian();
        ++index_;
    }
}

transaction_iterator::pointer transaction_iterator::operator->() const
{
    return offset_;
}

transaction_iterator::reference transaction_iterator::operator*() const
{
    return offset_;
}

transaction_iterator::iterator& transaction_iterator::operator++()
{
    populate();
    return *this;
}

transaction_iterator::iterator transaction_iterator::operator++(int)
{
    auto it = *this;
    populate();
    return it;
}

bool transaction_iterator::operator==(const transaction_iterator& other) const
{
    return start_ == other.start_ && index_ == other.index_;
}

bool transaction_iterator::operator!=(const transaction_iterator& other) const
{
    return !(*this == other);
}

} // namespace database
} // namespace libbitcoin
