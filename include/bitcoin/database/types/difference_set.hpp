/**
 * Copyright (c) 2011-2026 libbitcoin developers
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
#ifndef LIBBITCOIN_DATABASE_TYPES_DIFFERENCE_SET_HPP
#define LIBBITCOIN_DATABASE_TYPES_DIFFERENCE_SET_HPP

#include <atomic>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

/// Multiset symmetric difference over dense element domain. Each presentation
/// of element toggles its membership, so even multiplicities cancel and residue
/// is the set. One bit per element, in domain order.
/// Toggle is thread safe, enumeration (word) is not concurrent with toggle.
class difference_set
{
public:
    DELETE_COPY_MOVE(difference_set);

    using word = uint64_t;
    static constexpr auto word_bits = bits<word>;
    static constexpr auto word_shift = system::floored_log2(word_bits);

    /// The domain is [0..size), all elements initially absent.
    difference_set(size_t size=zero) NOEXCEPT
      : size_(size), words_(system::ceilinged_divide(size, word_bits))
    {
    }

    ~difference_set() = default;

    /// Toggle element membership (thread safe).
    void toggle(size_t element) NOEXCEPT
    {
        BC_ASSERT(element < size_);
        const auto value = system::bit_right<word>(to_offset(element));
        words_[to_word(element)].fetch_xor(value, std::memory_order_relaxed);
    }

    /// Element domain size.
    size_t size() const NOEXCEPT
    {
        return size_;
    }

    /// Number of words spanning the domain.
    size_t words() const NOEXCEPT
    {
        return words_.size();
    }

    /// Membership of the word_bits elements based at index * word_bits.
    word at(size_t index) const NOEXCEPT
    {
        return words_[index].load(std::memory_order_relaxed);
    }

private:
    static constexpr size_t to_word(size_t element) NOEXCEPT
    {
        return system::shift_right(element, word_shift);
    }

    static constexpr size_t to_offset(size_t element) NOEXCEPT
    {
        return system::bit_and(element, sub1(word_bits));
    }

    const size_t size_;
    std_vector<std::atomic<word>> words_;
};

} // namespace database
} // namespace libbitcoin

#endif
