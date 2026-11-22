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
#ifndef LIBBITCOIN_DATABASE_UNSPENT_UNSPENT_SERIAL_HPP
#define LIBBITCOIN_DATABASE_UNSPENT_UNSPENT_SERIAL_HPP

#include <bitcoin/database/define.hpp>
#include <bitcoin/database/types/types.hpp>
#include <bitcoin/database/unspent/unspent_writer.hpp>
#include <bitcoin/database/unspent/unspent_reader.hpp>
#include <bitcoin/database/unspent/unspent_spans.hpp>

namespace libbitcoin {
namespace database {

/// Serialized commitment of set, stream in canonical (txid, index) order,
/// folded by bucket of leading txid byte (canonical across buckets).
template <typename Store>
class unspent_serial
{
public:
    unspent_serial(const Store& store, const stopper& cancel,
        bool turbo) NOEXCEPT;

    /// The totals and serialized hash of the set.
    code hash(unspent_totals& out, hash_digest& digest,
        const difference_set& set) const NOEXCEPT;

private:
    using sizes = std_vector<size_t>;
    static constexpr auto buckets = 256_size;

    code partition(unspent_elements& out, sizes& offsets,
        const difference_set& set) const NOEXCEPT;
    template <typename Emit>
    code walk(const difference_set& set, size_t begin, size_t end,
        const Emit& emit) const NOEXCEPT;
    code slots(sizes& out, const unspent_elements& elements) const NOEXCEPT;
    static void starts(sizes& counts, sizes& offsets, size_t chunks) NOEXCEPT;
    code fill(unspent_coins& out, output_links& puts,
        const unspent_elements& elements, size_t begin,
        size_t end) const NOEXCEPT;
    void order(sizes& out, const unspent_coins& coins) const NOEXCEPT;
    void gather(unspent_coins& out, output_links& links, unspent_coins& coins,
        const output_links& puts, const sizes& order) const NOEXCEPT;
    code fold(unspent_totals& out, system::writer& sink,
        hash_digest& previous, unspent_coins& coins,
        const output_links& puts) const NOEXCEPT;

    const unspent_reader<Store> reader_;
    const unspent_spans<Store> spans_;
    const Store& store_;
    const stopper& cancel_;
    const bool turbo_;
};

} // namespace database
} // namespace libbitcoin

#define TEMPLATE template <typename Store>
#define CLASS unspent_serial<Store>

#include <bitcoin/database/impl/unspent/unspent_serial.ipp>

#undef CLASS
#undef TEMPLATE

#endif
