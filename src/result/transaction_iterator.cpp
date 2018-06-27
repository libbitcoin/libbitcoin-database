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
#include <bitcoin/database/primitives/list_element.hpp>
#include <bitcoin/database/primitives/slab_manager.hpp>
#include <bitcoin/database/memory/memory.hpp>

namespace libbitcoin {
namespace database {

using namespace bc::chain;

transaction_iterator::transaction_iterator(const manager& records,
    array_index start, size_t count)
  : index_(0)
{
    // Prepopulating the full set of offsets is assumed to be optimal.
    // However this behavior can be modified within this iterator as desired.
    if (count != 0)
    {
        offsets_.resize(count);
        const auto memory = records.get(start);
        auto deserial = make_unsafe_deserializer(memory->buffer());

        for (auto offset = 0u; offset < count; ++offset)
            offsets_[offset] =
                deserial.template read_little_endian<value_type>();
    }
}

transaction_iterator::pointer transaction_iterator::operator->() const
{
    return offsets_[index_];
}

transaction_iterator::reference transaction_iterator::operator*() const
{
    return offsets_[index_];
}

transaction_iterator::iterator& transaction_iterator::operator++()
{
    ++index_;
    return *this;
}

transaction_iterator::iterator transaction_iterator::operator++(int)
{
    auto it = *this;
    ++index_;
    return it;
}

bool transaction_iterator::operator==(const transaction_iterator& other) const
{
    // Cannot compare vectors iterators because they are from different vectors.
    // Cannot combine the vectors without qurying result in block_result.
    const auto left_terminal = (offsets_.size() == index_);
    const auto right_terminal = (other.offsets_.size() == other.index_);
    const auto both_terminal = left_terminal && right_terminal;
    const auto neither_terminal = !left_terminal && !right_terminal;
    return both_terminal || (index_ == other.index_ && neither_terminal);
}

bool transaction_iterator::operator!=(const transaction_iterator& other) const
{
    return !(*this == other);
}

} // namespace database
} // namespace libbitcoin
