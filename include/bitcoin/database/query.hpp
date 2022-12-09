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

using header_link = table::header::link;
using input_link = table::input::link;
using output_link = table::output::link;
using tx_link = table::transaction::link;
using tx_links = std_vector<tx_link>;
////using txs_link = table::txs::link;
    
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
    using filter = system::data_chunk;

    query(Store& value) NOEXCEPT;

    /// Initialization.
    /// -----------------------------------------------------------------------

    size_t get_top() NOEXCEPT;
    size_t get_top_candidate() NOEXCEPT;
    size_t get_fork() NOEXCEPT;
    size_t get_first_unpopulated_from(size_t height) NOEXCEPT;
    hashes get_all_unpopulated_from(size_t height) NOEXCEPT;
    hashes get_locator(size_t height) NOEXCEPT;

    /// Key conversion. 
    /// -----------------------------------------------------------------------

    header_link to_header(size_t height) NOEXCEPT;
    header_link to_header(const hash_digest& key) NOEXCEPT;
    tx_links to_txs(const header_link& link) NOEXCEPT;
    tx_link to_tx(const hash_digest& key) NOEXCEPT;

    /// Archive (natural-keyed).
    /// -----------------------------------------------------------------------
    bool is_header(const hash_digest& key) NOEXCEPT;
    bool is_block(const hash_digest& key) NOEXCEPT;
    bool is_tx(const hash_digest& key) NOEXCEPT;

    bool set(const header& header, const context& ctx) NOEXCEPT;
    bool set(const block& block, const context& ctx) NOEXCEPT;
    bool set(const transaction& tx) NOEXCEPT;

    bool populate(const block& block) NOEXCEPT;
    bool populate(const transaction& tx) NOEXCEPT;
    bool populate(const input& input) NOEXCEPT;

    /// Archive (foreign-keyed).
    /// -----------------------------------------------------------------------

    header::cptr get_header(const header_link& link) NOEXCEPT;
    block::cptr get_block(const header_link& link) NOEXCEPT;
    hashes get_txs(const header_link& link) NOEXCEPT;
    transaction::cptr get_tx(const tx_link& link) NOEXCEPT;
    output::cptr get_output(const tx_link& link, uint32_t output_index) NOEXCEPT;
    input::cptr get_input(const tx_link& link, uint32_t input_index) NOEXCEPT;
    input::cptr get_spender(const tx_link& link, uint32_t output_index) NOEXCEPT;

    header_link set_link(const header& header, const context& ctx) NOEXCEPT;
    header_link set_link(const block& block, const context& ctx) NOEXCEPT;
    tx_link set_link(const transaction& tx) NOEXCEPT;
    bool set(const header_link& link, const hashes& hashes) NOEXCEPT;
    bool set(const header_link& link, const tx_links& links) NOEXCEPT;

    /// Validation (foreign-keyed).
    /// -----------------------------------------------------------------------

    code get_header_state(const header_link& link) NOEXCEPT;
    code get_block_state(const header_link& link) NOEXCEPT;
    code get_tx_state(const tx_link& link, const context& ctx) NOEXCEPT;

    bool set_block_connected(const header_link& link) NOEXCEPT;
    bool set_block_valid(const header_link& link) NOEXCEPT;
    bool set_block_invalid(const header_link& link, const code& code) NOEXCEPT;

    bool set_tx_preconnected(const tx_link& link, const context& ctx) NOEXCEPT;
    bool set_tx_connected(const tx_link& link, const context& ctx) NOEXCEPT;
    bool set_tx_invalid(const tx_link& link, const context& ctx,
        const code& code) NOEXCEPT;

    /// Confirmation (foreign-keyed).
    /// -----------------------------------------------------------------------

    bool is_confirmed_block(const header_link& link) NOEXCEPT;
    bool is_candidate_block(const header_link& link) NOEXCEPT;
    bool is_confirmed_tx(const tx_link& link) NOEXCEPT;
    bool is_confirmed_prevout(const input_link& link) NOEXCEPT;
    bool is_unspent_prevout(const input_link& link) NOEXCEPT;

    bool push(const header_link& link) NOEXCEPT;
    bool push_candidate(const header_link& link) NOEXCEPT;

    bool pop() NOEXCEPT;
    bool pop_candidate() NOEXCEPT;

    /// Buffer (foreign-keyed).
    /// -----------------------------------------------------------------------

    transaction::cptr get_buffered_tx(const tx_link& link) NOEXCEPT;
    bool set_buffered_tx(const transaction& tx) NOEXCEPT;

    /// Neutrino (foreign-keyed).
    /// -----------------------------------------------------------------------

    filter get_filter(const header_link& link) NOEXCEPT;
    hash_digest get_filter_head(const header_link& link) NOEXCEPT;
    bool set_filter(const header_link& link, const hash_digest& head,
        const filter& body) NOEXCEPT;

    /// Bootstrap (array).
    /// -----------------------------------------------------------------------

    hashes get_bootstrap(size_t from, size_t to) NOEXCEPT;
    bool set_bootstrap(size_t height) NOEXCEPT;

private:
    Store& store_;
};

} // namespace database
} // namespace libbitcoin

#define TEMPLATE template <typename Store>
#define CLASS query<Store>

////#include <bitcoin/database/impl/queries/archive.ipp>
////#include <bitcoin/database/impl/queries/cache.ipp>
////#include <bitcoin/database/impl/queries/index.ipp>
#include <bitcoin/database/impl/query.ipp>

#undef CLASS
#undef TEMPLATE

#endif
