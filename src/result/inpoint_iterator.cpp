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
#include <bitcoin/database/result/inpoint_iterator.hpp>

#include <bitcoin/bitcoin.hpp>

namespace libbitcoin {
namespace database {

using namespace bc::chain;

static constexpr auto height_size = sizeof(uint32_t);
static constexpr auto position_size = sizeof(uint16_t);
static constexpr auto state_size = sizeof(uint8_t);
static constexpr auto median_time_past_size = sizeof(uint32_t);

static constexpr auto index_spend_size = sizeof(uint8_t);
////static constexpr auto height_size = sizeof(uint32_t);
static constexpr auto value_size = sizeof(uint64_t);

static constexpr auto spend_size = index_spend_size + height_size + value_size;
static constexpr auto metadata_size = height_size + position_size +
    state_size + median_time_past_size;

static constexpr auto sequence_size = sizeof(uint32_t);

inpoint_iterator::inpoint_iterator(const const_element& element)
  : index_(0)
{
    // Prepopulating the full set of points is assumed to be optimal.
    // However this behavior can be modified within this iterator as desired.
    if (!element.terminal())
    {
        element.read([&](byte_deserializer& deserial)
        {
            deserial.skip(metadata_size);
            const auto outputs = deserial.read_size_little_endian();

            // Skip outputs.
            for (auto output = 0u; output < outputs; ++output)
            {
                deserial.skip(spend_size);
                deserial.skip(deserial.read_size_little_endian());
            }

            const auto inputs = deserial.read_size_little_endian();
            inpoints_.resize(inputs);

            for (auto input = 0u; input < inputs; ++input)
            {
                // Read input point.
                inpoints_[input].from_data(deserial, false);

                // Skip script.
                deserial.skip(deserial.read_size_little_endian());

                // Skip witnesses.
                for (auto count = deserial.read_size_little_endian();
                    count > 0; --count)
                    deserial.skip(deserial.read_size_little_endian());

                // Skip sequence.
                deserial.skip(sequence_size);
            }
        });
    }
}

inpoint_iterator::pointer inpoint_iterator::operator->() const
{
    return inpoints_[index_];
}

inpoint_iterator::reference inpoint_iterator::operator*() const
{
    return inpoints_[index_];
}

inpoint_iterator::iterator& inpoint_iterator::operator++()
{
    ++index_;
    return *this;
}

inpoint_iterator::iterator inpoint_iterator::operator++(int)
{
    auto it = *this;
    ++index_;
    return it;
}

bool inpoint_iterator::operator==(const inpoint_iterator& other) const
{
    // Cannot compare vectors iterators because they are from different vectors.
    // Cannot combine the vectors without querying result in transaction_result.
    const auto left_terminal = (inpoints_.size() == index_);
    const auto right_terminal = (other.inpoints_.size() == other.index_);
    const auto both_terminal = left_terminal && right_terminal;
    const auto neither_terminal = !left_terminal && !right_terminal;
    return both_terminal || (index_ == other.index_ && neither_terminal);
}

bool inpoint_iterator::operator!=(const inpoint_iterator& other) const
{
    return !(*this == other);
}

} // namespace database
} // namespace libbitcoin
