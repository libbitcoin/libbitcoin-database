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

#include <utility>
#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/error.hpp>
#include <bitcoin/database/primitives/primitives.hpp>
#include <bitcoin/database/tables/context.hpp>
#include <bitcoin/database/tables/tables.hpp>

namespace libbitcoin {
namespace database {

/// Database type aliases.
using point_link = table::point::link;
using input_link = table::input::link;
using output_link = table::output::link;
using tx_link = table::transaction::link;
using height_link = table::height::link;
using header_link = table::header::link;
using txs_link = table::txs::link;
using tx_links = std_vector<tx_link::integer>;
using input_links = std_vector<input_link::integer>;
using output_links = std_vector<output_link::integer>;

template <typename Store>
class query
{
public:
    DELETE_COPY_MOVE_DESTRUCT(query);

    /// Query type aliases.
    using block = system::chain::block;
    using point = system::chain::point;
    using input = system::chain::input;
    using output = system::chain::output;
    using header = system::chain::header;
    using transaction = system::chain::transaction;
    using inputs_ptr = system::chain::inputs_ptr;
    using outputs_ptr = system::chain::outputs_ptr;
    using transactions_ptr = system::chain::transactions_ptr;
    using chain_state = system::chain::chain_state;
    using chain_state_ptr = system::chain::chain_state::ptr;
    using chain_context = system::chain::context;
    using sizes = std::pair<size_t, size_t>;
    using heights = std_vector<size_t>;
    using filter = system::data_chunk;

    query(Store& value) NOEXCEPT;

    /// Initialization (natural-keyed).
    /// -----------------------------------------------------------------------
    /// Not reliable during organization.

    inline bool is_initialized() const NOEXCEPT;
    inline size_t get_top_candidate() const NOEXCEPT;
    inline size_t get_top_confirmed() const NOEXCEPT;
    size_t get_fork() const NOEXCEPT;
    size_t get_last_associated_from(size_t height) const NOEXCEPT;
    hashes get_all_unassociated_above(size_t height) const NOEXCEPT;
    hashes get_candidate_hashes(const heights& heights) const NOEXCEPT;
    hashes get_confirmed_hashes(const heights& heights) const NOEXCEPT;

    /// Store sizing.
    /// -----------------------------------------------------------------------

    /// Table logical byte sizes (archive bodies).
    size_t archive_size() const NOEXCEPT;
    size_t header_size() const NOEXCEPT;
    size_t output_size() const NOEXCEPT;
    size_t input_size() const NOEXCEPT;
    size_t point_size() const NOEXCEPT;
    size_t puts_size() const NOEXCEPT;
    size_t txs_size() const NOEXCEPT;
    size_t tx_size() const NOEXCEPT;

    /// Table logical byte sizes (metadata bodies).
    size_t candidate_size() const NOEXCEPT;
    size_t confirmed_size() const NOEXCEPT;
    size_t strong_tx_size() const NOEXCEPT;
    size_t validated_tx_size() const NOEXCEPT;
    size_t validated_bk_size() const NOEXCEPT;

    /// Buckets (archive hash tables).
    size_t header_buckets() const NOEXCEPT;
    size_t point_buckets() const NOEXCEPT;
    size_t input_buckets() const NOEXCEPT;
    size_t txs_buckets() const NOEXCEPT;
    size_t tx_buckets() const NOEXCEPT;

    /// Buckets (metadata hash tables).
    size_t strong_tx_buckets() const NOEXCEPT;
    size_t validated_tx_buckets() const NOEXCEPT;
    size_t validated_bk_buckets() const NOEXCEPT;

    /// Counts (archive records).
    size_t header_records() const NOEXCEPT;
    size_t point_records() const NOEXCEPT;
    size_t puts_records() const NOEXCEPT;
    size_t tx_records() const NOEXCEPT;

    /// Counts (metadata records).
    size_t candidate_records() const NOEXCEPT;
    size_t confirmed_records() const NOEXCEPT;
    size_t strong_tx_records() const NOEXCEPT;

    /// Counters (archive slabs).
    /// header_records is upper bound for txs_slabs (in terms of collision).
    size_t input_slabs(const tx_link& link) const NOEXCEPT;
    size_t output_slabs(const tx_link& link) const NOEXCEPT;
    sizes put_slabs(const tx_link& link) const NOEXCEPT;

    /// Translation (key/link to link/s).
    /// -----------------------------------------------------------------------

    /// search key (entry)
    inline header_link to_candidate(size_t height) const NOEXCEPT;
    inline header_link to_confirmed(size_t height) const NOEXCEPT;
    inline header_link to_header(const hash_digest& key) const NOEXCEPT;
    inline point_link to_point(const hash_digest& key) const NOEXCEPT;
    inline tx_link to_tx(const hash_digest& key) const NOEXCEPT;

    /// put to tx (reverse navigation)
    tx_link to_input_tx(const input_link& link) const NOEXCEPT;
    tx_link to_output_tx(const output_link& link) const NOEXCEPT;
    tx_link to_prevout_tx(const input_link& link) const NOEXCEPT;

    /// point to put (forward navigation)
    input_link to_input(const tx_link& link, uint32_t input_index) const NOEXCEPT;
    output_link to_output(const tx_link& link, uint32_t output_index) const NOEXCEPT;
    output_link to_prevout(const input_link& link) const NOEXCEPT;

    /// block/tx to block (reverse navigation)
    header_link to_block(const tx_link& link) const NOEXCEPT;
    header_link to_parent(const header_link& link) const NOEXCEPT;

    /// output to spenders (reverse navigation)
    input_links to_spenders(const output_link& link) const NOEXCEPT;
    input_links to_spenders(const point& prevout) const NOEXCEPT;
    input_links to_spenders(const tx_link& link,
        uint32_t output_index) const NOEXCEPT;

    /// tx to puts (forward navigation)
    input_links to_tx_inputs(const tx_link& link) const NOEXCEPT;
    output_links to_tx_outputs(const tx_link& link) const NOEXCEPT;

    /// block to txs/puts (forward navigation)
    tx_links to_txs(const header_link& link) const NOEXCEPT;
    tx_link to_coinbase(const header_link& link) const NOEXCEPT;
    input_links to_non_coinbase_inputs(const header_link& link) const NOEXCEPT;
    output_links to_block_outputs(const header_link& link) const NOEXCEPT;

    /// Archival (mostly natural-keyed).
    /// -----------------------------------------------------------------------

    inline bool is_header(const hash_digest& key) const NOEXCEPT;
    inline bool is_block(const hash_digest& key) const NOEXCEPT;
    inline bool is_tx(const hash_digest& key) const NOEXCEPT;
    inline bool is_coinbase(const tx_link& link) const NOEXCEPT;
    inline bool is_associated(const header_link& link) const NOEXCEPT;

    inline bool set(const header& header, const chain_context& ctx) NOEXCEPT;
    inline bool set(const header& header, const context& ctx) NOEXCEPT;
    inline bool set(const block& block, const chain_context& ctx) NOEXCEPT;
    inline bool set(const block& block, const context& ctx) NOEXCEPT;
    inline bool set(const transaction& tx) NOEXCEPT;

    /// False implies not fully populated.
    bool populate(const input& input) const NOEXCEPT;
    bool populate(const transaction& tx) const NOEXCEPT;
    bool populate(const block& block) const NOEXCEPT;

    /// Archival (surrogate-keyed).
    /// -----------------------------------------------------------------------

    /// Empty/null_hash implies fault.
    hashes get_txs(const header_link& link) const NOEXCEPT;
    inline hash_digest get_header_key(const header_link& link) const NOEXCEPT;
    inline hash_digest get_point_key(const point_link& link) const NOEXCEPT;
    inline hash_digest get_tx_key(const tx_link& link) const NOEXCEPT;

    /// False implies not confirmed, false get_value implies error.
    bool get_height(size_t& out, const header_link& link) const NOEXCEPT;
    bool get_tx_height(size_t& out, const tx_link& link) const NOEXCEPT;
    bool get_tx_position(size_t& out, const tx_link& link) const NOEXCEPT;
    bool get_value(uint64_t& out, const output_link& link) const NOEXCEPT;

    inputs_ptr get_inputs(const tx_link& link) const NOEXCEPT;
    outputs_ptr get_outputs(const tx_link& link) const NOEXCEPT;
    transactions_ptr get_transactions(const header_link& link) const NOEXCEPT;

    header::cptr get_header(const header_link& link) const NOEXCEPT;
    block::cptr get_block(const header_link& link) const NOEXCEPT;
    transaction::cptr get_transaction(const tx_link& link) const NOEXCEPT;
    output::cptr get_output(const output_link& link) const NOEXCEPT;
    input::cptr get_input(const input_link& link) const NOEXCEPT;
    point::cptr get_point(const input_link& link) const NOEXCEPT;
    inputs_ptr get_spenders(const output_link& link) const NOEXCEPT;

    output::cptr get_output(const point& prevout) const NOEXCEPT;
    output::cptr get_output(const tx_link& link, uint32_t output_index) const NOEXCEPT;
    input::cptr get_input(const tx_link& link, uint32_t input_index) const NOEXCEPT;
    inputs_ptr get_spenders(const tx_link& link, uint32_t output_index) const NOEXCEPT;

    header_link set_link(const header& header, const chain_context& ctx) NOEXCEPT;
    header_link set_link(const header& header, const context& ctx) NOEXCEPT;
    header_link set_link(const block& block, const chain_context& ctx) NOEXCEPT;
    header_link set_link(const block& block, const context& ctx) NOEXCEPT;
    tx_link set_link(const transaction& tx) NOEXCEPT;

    bool set(const header_link& link, const hashes& hashes) NOEXCEPT;
    bool set(const header_link& link, const tx_links& links) NOEXCEPT;

    /// Chain state.
    /// -----------------------------------------------------------------------

    chain_state_ptr get_candidate_chain_state(
        const system::settings& settings) const NOEXCEPT;
    chain_state_ptr get_candidate_chain_state(const system::settings& settings,
        size_t height) const NOEXCEPT;
    chain_state_ptr get_candidate_chain_state(const system::settings& settings,
        const header& header, size_t height) const NOEXCEPT;

    chain_state_ptr get_confirmed_chain_state(
        const system::settings& settings) const NOEXCEPT;
    chain_state_ptr get_confirmed_chain_state(const system::settings& settings,
        size_t height) const NOEXCEPT;
    chain_state_ptr get_confirmed_chain_state(const system::settings& settings,
        const header& header, size_t height) const NOEXCEPT;

    /// Validation (surrogate-keyed).
    /// -----------------------------------------------------------------------

    code get_block_state(const header_link& link) const NOEXCEPT;
    code get_block_state(uint64_t& fees, const header_link& link) const NOEXCEPT;
    code get_tx_state(const tx_link& link, const context& ctx) const NOEXCEPT;
    code get_tx_state(uint64_t& fee, size_t& sigops, const tx_link& link,
        const context& ctx) const NOEXCEPT;

    bool get_timestamp(uint32_t& timestamp, const header_link& link) const NOEXCEPT;
    bool get_version(uint32_t& version, const header_link& link) const NOEXCEPT;
    bool get_context(context& ctx, const header_link& link) const NOEXCEPT;
    bool get_bits(uint32_t& bits, const header_link& link) const NOEXCEPT;

    bool set_block_preconfirmable(const header_link& link) NOEXCEPT;
    bool set_block_unconfirmable(const header_link& link) NOEXCEPT;
    bool set_block_confirmable(const header_link& link, uint64_t fees) NOEXCEPT;

    // set_txs_connected is FOR PERFORMANCE EVALUATION ONLY.
    bool set_txs_connected(const header_link& link) NOEXCEPT;
    bool set_tx_preconnected(const tx_link& link, const context& ctx) NOEXCEPT;
    bool set_tx_disconnected(const tx_link& link, const context& ctx) NOEXCEPT;
    bool set_tx_connected(const tx_link& link, const context& ctx,
        uint64_t fee, size_t sigops) NOEXCEPT;

    /// Confirmation (surrogate-keyed).
    /// -----------------------------------------------------------------------

    /// These compare strong with height index (not for confirmation process).
    bool is_candidate_block(const header_link& link) const NOEXCEPT;
    bool is_confirmed_block(const header_link& link) const NOEXCEPT;
    bool is_confirmed_tx(const tx_link& link) const NOEXCEPT;
    bool is_confirmed_input(const input_link& link) const NOEXCEPT;
    bool is_confirmed_output(const output_link& link) const NOEXCEPT;
    bool is_spent_output(const output_link& link) const NOEXCEPT;

    /// These rely on strong (use only for confirmation process).
    bool is_strong(const input_link& link) const NOEXCEPT;
    bool is_spent(const input_link& link) const NOEXCEPT;
    bool is_mature(const input_link& link, size_t height) const NOEXCEPT;
    ////bool is_exhausted(const tx_link& link) const NOEXCEPT;
    code block_confirmable(const header_link& link) const NOEXCEPT;

    /// Block association relies on strong (confirmed or pending).
    bool set_strong(const header_link& link) NOEXCEPT;
    bool set_unstrong(const header_link& link) NOEXCEPT;

    bool initialize(const block& genesis) NOEXCEPT;
    bool push_candidate(const header_link& link) NOEXCEPT;
    bool push_confirmed(const header_link& link) NOEXCEPT;
    bool pop_candidate() NOEXCEPT;
    bool pop_confirmed() NOEXCEPT;

    /// Optional Tables.
    /// -----------------------------------------------------------------------

    /// Address hash function.
    static hash_digest address_hash(const output& output) NOEXCEPT;

    /// Address (natural-keyed).
    bool get_confirmed_balance(uint64_t& out, const hash_digest& key) const NOEXCEPT;
    bool to_address_outputs(output_links& out, const hash_digest& key) const NOEXCEPT;
    bool to_unspent_outputs(output_links& out, const hash_digest& key) const NOEXCEPT;
    bool to_minimum_unspent_outputs(output_links& out, const hash_digest& key,
        uint64_t value) const NOEXCEPT;
    bool set_address_output(const hash_digest& key,
        const output_link& link) NOEXCEPT;

    /// Neutrino (surrogate-keyed).
    bool get_filter(filter& out, const header_link& link) const NOEXCEPT;
    bool get_filter_head(hash_digest& out, const header_link& link) const NOEXCEPT;
    bool set_filter(const header_link& link, const hash_digest& head,
        const filter& body) NOEXCEPT;

    /// Buffer (surrogate-keyed).
    transaction::cptr get_buffered_tx(const tx_link& link) const NOEXCEPT;
    bool set_buffered_tx(const tx_link& link, const transaction& tx) NOEXCEPT;

    /// Bootstrap (natural-keyed).
    bool get_bootstrap(hashes& out) const NOEXCEPT;
    bool set_bootstrap(size_t height) NOEXCEPT;

protected:
    using input_key = table::input::search_key;
    using puts_link = table::puts::link;

    inline txs_link to_txs_link(const header_link& link) const NOEXCEPT;
    inline input_key make_foreign_point(const point& prevout) const NOEXCEPT;
    inline code to_block_code(linkage<schema::code>::integer value) const NOEXCEPT;
    inline code to_tx_code(linkage<schema::code>::integer value) const NOEXCEPT;
    inline bool is_sufficient(const context& current,
        const context& evaluated) const NOEXCEPT;

    height_link get_height(const header_link& link) const NOEXCEPT;
    input_links to_spenders(const table::input::search_key& key) const NOEXCEPT;
    error::error_t mature_prevout(const point_link& link,
        size_t height) const NOEXCEPT;
    error::error_t locked_input(const input_link& link, uint32_t sequence,
        const context& put) const NOEXCEPT;
    bool is_confirmed_unspent(const output_link& link) const NOEXCEPT;
    bool is_spent_prevout(const table::input::search_key& key,
        const input_link& self) const NOEXCEPT;

    bool get_candidate_bits(uint32_t& bits, size_t height,
        const header& header, size_t header_height) const NOEXCEPT;
    bool get_candidate_version(uint32_t& version, size_t height,
        const header& header, size_t header_height) const NOEXCEPT;
    bool get_candidate_timestamp(uint32_t& time, size_t height,
        const header& header, size_t header_height) const NOEXCEPT;

    bool populate_candidate_bits(chain_state::data& data,
        const chain_state::map& map, const header& header,
        size_t header_height) const NOEXCEPT;
    bool populate_candidate_versions(chain_state::data& data,
        const chain_state::map& map, const header& header,
        size_t header_height) const NOEXCEPT;
    bool populate_candidate_timestamps(chain_state::data& data,
        const chain_state::map& map, const header& header,
        size_t header_height) const NOEXCEPT;
    bool populate_candidate_all(chain_state::data& data,
        const system::settings& settings, const header& header,
        size_t header_height) const NOEXCEPT;

    bool get_confirmed_bits(uint32_t& bits, size_t height,
        const header& header, size_t header_height) const NOEXCEPT;
    bool get_confirmed_version(uint32_t& version, size_t height,
        const header& header, size_t header_height) const NOEXCEPT;
    bool get_confirmed_timestamp(uint32_t& time, size_t height,
        const header& header, size_t header_height) const NOEXCEPT;

    bool populate_confirmed_bits(chain_state::data& data,
        const chain_state::map& map, const header& header,
        size_t header_height) const NOEXCEPT;
    bool populate_confirmed_versions(chain_state::data& data,
        const chain_state::map& map, const header& header,
        size_t header_height) const NOEXCEPT;
    bool populate_confirmed_timestamps(chain_state::data& data,
        const chain_state::map& map, const header& header,
        size_t header_height) const NOEXCEPT;
    bool populate_confirmed_all(chain_state::data& data,
        const system::settings& settings, const header& header,
        size_t header_height) const NOEXCEPT;

private:
    static size_t nested_count(const auto& outer) NOEXCEPT;

    Store& store_;
};

} // namespace database
} // namespace libbitcoin

#define TEMPLATE template <typename Store>
#define CLASS query<Store>

BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
BC_PUSH_WARNING(NO_USE_OF_MOVED_OBJECT)

#include <bitcoin/database/impl/query/query.ipp>
#include <bitcoin/database/impl/query/archive.ipp>
#include <bitcoin/database/impl/query/confirm.ipp>
#include <bitcoin/database/impl/query/context.ipp>
#include <bitcoin/database/impl/query/initialize.ipp>
#include <bitcoin/database/impl/query/optional.ipp>
#include <bitcoin/database/impl/query/extent.ipp>
#include <bitcoin/database/impl/query/translate.ipp>
#include <bitcoin/database/impl/query/validate.ipp>

BC_POP_WARNING()
BC_POP_WARNING()

#undef CLASS
#undef TEMPLATE

#endif
