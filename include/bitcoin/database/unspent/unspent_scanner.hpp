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
#ifndef LIBBITCOIN_DATABASE_UNSPENT_UNSPENT_SCANNER_HPP
#define LIBBITCOIN_DATABASE_UNSPENT_UNSPENT_SCANNER_HPP

#include <bitcoin/database/define.hpp>
#include <bitcoin/database/tables/tables.hpp>
#include <bitcoin/database/types/types.hpp>

namespace libbitcoin {
namespace database {

template <typename Store>
class query;

/// Toggles the creates and spends of a branch into a set, the residue of
/// which is the unspent set (as outs table links).
template <typename Store>
class unspent_scanner
{
public:
    unspent_scanner(const query<Store>& query, const Store& store,
        const stopper& cancel, bool turbo) NOEXCEPT;

    /// Scan the branch into the set, with the unspendable and bip30 totals.
    code scan(difference_set& set, unspent_totals& out,
        const header_links& branch) const NOEXCEPT;

private:
    using gets = std_vector<table::transaction::get_puts>;

    static constexpr auto per_thread = 32_size;

    struct buffers
    {
        gets txs{};
        output_links exclusions{};
        system::chain::points spends{};
    };

    code bounds(std_vector<size_t>& out,
        const header_links& branch) const NOEXCEPT;
    code span(difference_set& set, uint64_t& unspendable,
        uint64_t& overwritten, const header_links& branch, size_t begin,
        size_t end) const NOEXCEPT;
    code block(difference_set& set, uint64_t& unspendable,
        uint64_t& overwritten, buffers& buffer,
        const header_link& link) const NOEXCEPT;
    code overwritten(uint64_t& out,
        const table::transaction::get_puts& coinbase) const NOEXCEPT;
    code read_transactions(gets& out, const header_link& link) const NOEXCEPT;
    code toggle_creates(difference_set& set, output_links& exclusions,
        const gets& txs) const NOEXCEPT;
    code read_exclusions(uint64_t& out,
        const output_links& exclusions) const NOEXCEPT;
    code read_points(system::chain::points& out,
        const gets& txs) const NOEXCEPT;
    code toggle_spends(difference_set& set,
        const system::chain::points& spends) const NOEXCEPT;

    const query<Store>& query_;
    const Store& store_;
    const stopper& cancel_;
    const bool turbo_;
};

} // namespace database
} // namespace libbitcoin

#define TEMPLATE template <typename Store>
#define CLASS unspent_scanner<Store>

#include <bitcoin/database/impl/unspent/unspent_scanner.ipp>

#undef CLASS
#undef TEMPLATE

#endif
