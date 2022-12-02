/**
 * Copyright (c) 2011-2022 libbitcoin developers (see AUTHORS)
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
#ifndef LIBBITCOIN_DATABASE_QUERY_HPP
#define LIBBITCOIN_DATABASE_QUERY_HPP

#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/primitives/primitives.hpp>
#include <bitcoin/database/tables/context.hpp>
#include <bitcoin/database/tables/tables.hpp>

namespace libbitcoin {
namespace database {

template <typename Store>
class query
{
public:
    using block = system::chain::block;
    using point = system::chain::point;
    using input = system::chain::input;
    using output = system::chain::output;
    using header = system::chain::header;
    using transaction = system::chain::transaction;

    query(Store& value) NOEXCEPT;

    bool set_tx(const transaction& tx) NOEXCEPT;
    bool set_header(const header& header, const context& context) NOEXCEPT;
    bool set_block(const block& block, const context& context) NOEXCEPT;
    bool set_txs(const hash_digest& key, const system::hashes& hashes) NOEXCEPT;

    // TODO: test input/output/spender getters.
    input::cptr get_input(const hash_digest& tx_hash, uint32_t index) NOEXCEPT;
    output::cptr get_output(const hash_digest& tx_hash, uint32_t index) NOEXCEPT;
    input::cptr get_spender(const point::cptr& prevout) NOEXCEPT;
    input::cptr get_spender(const point& prevout) NOEXCEPT;
    transaction::cptr get_tx(const hash_digest& key) NOEXCEPT;
    header::cptr get_header(const hash_digest& key) NOEXCEPT;
    block::cptr get_block(const hash_digest& key) NOEXCEPT;
    system::hashes get_txs(const hash_digest& key) NOEXCEPT;

protected:
    table::transaction::link set_tx_link(const transaction& tx) NOEXCEPT;
    table::header::link set_header_link(const header& header, const context& context) NOEXCEPT;
    table::header::link set_block_link(const block& block, const context& context) NOEXCEPT;
    bool set_txs(const table::header::link& key, const table::txs::slab& txs) NOEXCEPT;

    input::cptr get_input(const table::input::link& fk) NOEXCEPT;
    output::cptr get_output(const table::output::link& fk) NOEXCEPT;
    transaction::cptr get_tx(const table::transaction::link& fk) NOEXCEPT;
    header::cptr get_header(const table::header::link& fk) NOEXCEPT;
    block::cptr get_block(const table::header::link& fk) NOEXCEPT;
    system::hashes get_txs(const table::header::link& fk) NOEXCEPT;

private:
    Store& store_;
};

} // namespace database
} // namespace libbitcoin

#define TEMPLATE template <typename Store>
#define CLASS query<Store>

#include <bitcoin/database/impl/query.ipp>

#undef CLASS
#undef TEMPLATE

#endif
