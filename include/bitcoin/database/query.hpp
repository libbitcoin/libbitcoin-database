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
    
/// Setters:
/// False implies error (invalid store or parameter association).
/// Caller should assume invalid store (proper parameterization).
///
/// Getters:
/// Null/empty/false implies not found or error (invalid store).
/// Caller should assume not found, idependently monitor store state.
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

    /// Archives.
    /// -----------------------------------------------------------------------

    bool set_header(const header& header, const context& ctx) NOEXCEPT;
    bool set_block(const block& block, const context& ctx) NOEXCEPT;
    bool set_txs(const hash_digest& key, const hashes& hashes) NOEXCEPT;
    bool set_tx(const transaction& tx) NOEXCEPT;

    // TODO: test.
    /// Archive existence state only.
    bool header_exists(const hash_digest& key) NOEXCEPT;
    bool block_exists(const hash_digest& key) NOEXCEPT;
    bool tx_exists(const hash_digest& key) NOEXCEPT;

    // TODO: test.
    /// Prevout population, false if any are missing/unpopulated.
    bool populate(const block& block) NOEXCEPT;
    bool populate(const transaction& tx) NOEXCEPT;
    bool populate(const input& input) NOEXCEPT;

    header::cptr get_header(const hash_digest& key) NOEXCEPT;
    block::cptr get_block(const hash_digest& key) NOEXCEPT;
    hashes get_txs(const hash_digest& key) NOEXCEPT;
    transaction::cptr get_tx(const hash_digest& key) NOEXCEPT;
    input::cptr get_spender(const hash_digest& tx_hash, uint32_t index) NOEXCEPT;
    input::cptr get_input(const hash_digest& tx_hash, uint32_t index) NOEXCEPT;
    output::cptr get_output(const hash_digest& tx_hash, uint32_t index) NOEXCEPT;
    output::cptr get_prevout(const input& input) NOEXCEPT;

    /// Indexes.
    /// -----------------------------------------------------------------------

    // TODO: test.
    block::cptr get_header(size_t height) NOEXCEPT;
    block::cptr get_block(size_t height) NOEXCEPT;
    block::cptr get_txs(size_t height) NOEXCEPT;

    /// Caches.
    /// -----------------------------------------------------------------------

    // TODO: test.
    /// Validation states (block implies populated).
    code header_state(const hash_digest& key) NOEXCEPT;
    code block_state(const hash_digest& key) NOEXCEPT;
    code tx_state(const hash_digest& key, const context& context) NOEXCEPT;

protected:
    table::transaction::link set_tx_link(const transaction& tx) NOEXCEPT;
    table::header::link set_header_link(const header& header, const context& ctx) NOEXCEPT;
    table::header::link set_block_link(const block& block, const context& ctx) NOEXCEPT;
    bool set_txs(const table::header::link& fk, const table::txs::slab& set) NOEXCEPT;

    header::cptr get_header(const table::header::link& fk) NOEXCEPT;
    block::cptr get_block(const table::header::link& fk) NOEXCEPT;
    hashes get_txs(const table::header::link& fk) NOEXCEPT;
    transaction::cptr get_tx(const table::transaction::link& fk) NOEXCEPT;
    input::cptr get_input(const table::input::link& fk) NOEXCEPT;
    output::cptr get_output(const table::output::link& fk) NOEXCEPT;

    table::header::link get_header_fk(size_t height) NOEXCEPT;

    code header_state(const table::header::link& fk) NOEXCEPT;
    code block_state(const table::header::link& fk) NOEXCEPT;
    code tx_state(const table::header::link& fk, const context& context) NOEXCEPT;

private:
    Store& store_;
};

} // namespace database
} // namespace libbitcoin

#define TEMPLATE template <typename Store>
#define CLASS query<Store>

#include <bitcoin/database/impl/queries/archive.ipp>
#include <bitcoin/database/impl/queries/cache.ipp>
#include <bitcoin/database/impl/queries/index.ipp>
#include <bitcoin/database/impl/query.ipp>

#undef CLASS
#undef TEMPLATE

#endif
