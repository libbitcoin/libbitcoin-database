/**
 * Copyright (c) 2011-2023 libbitcoin developers (see AUTHORS)
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
#include <unordered_map>
#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/error.hpp>
#include <bitcoin/database/primitives/primitives.hpp>
#include <bitcoin/database/tables/context.hpp>
#include <bitcoin/database/tables/tables.hpp>

namespace libbitcoin {
namespace database {

/// Database type aliases.
using height_link = table::height::link;
using header_link = table::header::link;
using output_link = table::output::link;
using input_link = table::input::link;
using point_link = table::point::link;
using spend_link = table::spend::link;
using txs_link = table::txs::link;
using tx_link = table::transaction::link;
using tx_links = std_vector<tx_link::integer>;
using spend_links = std_vector<spend_link::integer>;
using input_links = std_vector<input_link::integer>;
using output_links = std_vector<output_link::integer>;
using foreign_point = table::spend::search_key;
using two_counts = std::pair<size_t, size_t>;
using context_map = std::unordered_map<system::hash_digest,
    system::chain::context>;

template <typename Store>
class query
{
public:
    DELETE_COPY_MOVE_DESTRUCT(query);

    /// Query type aliases.
    using block = system::chain::block;
    using point = system::chain::point;
    using input = system::chain::input;
    using script = system::chain::script;
    using output = system::chain::output;
    using header = system::chain::header;
    using transaction = system::chain::transaction;
    using transactions = system::chain::transaction_cptrs;
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
    context_map get_all_unassociated_above(size_t height) const NOEXCEPT;
    hashes get_candidate_hashes(const heights& heights) const NOEXCEPT;
    hashes get_confirmed_hashes(const heights& heights) const NOEXCEPT;

    /// Store extent.
    /// -----------------------------------------------------------------------

    /// Table logical byte sizes (archive bodies).
    size_t archive_size() const NOEXCEPT;
    size_t header_size() const NOEXCEPT;
    size_t output_size() const NOEXCEPT;
    size_t input_size() const NOEXCEPT;
    size_t point_size() const NOEXCEPT;
    size_t puts_size() const NOEXCEPT;
    size_t spend_size() const NOEXCEPT;
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
    size_t spend_buckets() const NOEXCEPT;
    size_t txs_buckets() const NOEXCEPT;
    size_t tx_buckets() const NOEXCEPT;

    /// Buckets (metadata hash tables).
    size_t strong_tx_buckets() const NOEXCEPT;
    size_t validated_tx_buckets() const NOEXCEPT;
    size_t validated_bk_buckets() const NOEXCEPT;

    /// Counts (archive records).
    size_t header_records() const NOEXCEPT;
    size_t point_records() const NOEXCEPT;
    size_t spend_records() const NOEXCEPT;
    size_t tx_records() const NOEXCEPT;

    /// Counts (metadata records).
    size_t candidate_records() const NOEXCEPT;
    size_t confirmed_records() const NOEXCEPT;
    size_t strong_tx_records() const NOEXCEPT;

    /// Counters (archive slabs - txs/puts can be derived).
    size_t input_count(const tx_link& link) const NOEXCEPT;
    size_t output_count(const tx_link& link) const NOEXCEPT;
    two_counts put_counts(const tx_link& link) const NOEXCEPT;

    /// Translation (key/link to link/s).
    /// -----------------------------------------------------------------------

    /// search key (entry)
    inline header_link to_candidate(size_t height) const NOEXCEPT;
    inline header_link to_confirmed(size_t height) const NOEXCEPT;
    inline header_link to_header(const hash_digest& key) const NOEXCEPT;
    inline point_link to_point(const hash_digest& key) const NOEXCEPT;
    inline tx_link to_tx(const hash_digest& key) const NOEXCEPT;
    inline txs_link to_txs_link(const header_link& link) const NOEXCEPT;

    /// put to tx (reverse navigation)
    tx_link to_spend_tx(const spend_link& link) const NOEXCEPT;
    tx_link to_output_tx(const output_link& link) const NOEXCEPT;
    tx_link to_prevout_tx(const spend_link& link) const NOEXCEPT;
    foreign_point to_spend_key(const spend_link& link) const NOEXCEPT;

    /// point to put (forward navigation)
    spend_link to_spend(const tx_link& link, uint32_t input_index) const NOEXCEPT;
    output_link to_output(const tx_link& link, uint32_t output_index) const NOEXCEPT;
    output_link to_prevout(const spend_link& link) const NOEXCEPT;

    /// block/tx to block (reverse navigation)
    header_link to_block(const tx_link& link) const NOEXCEPT;
    header_link to_parent(const header_link& link) const NOEXCEPT;

    /// output to spenders (reverse navigation)
    spend_links to_spenders(const point& prevout) const NOEXCEPT;
    spend_links to_spenders(const output_link& link) const NOEXCEPT;
    spend_links to_spenders(const foreign_point& point) const NOEXCEPT;
    spend_links to_spenders(const tx_link& link,
        uint32_t output_index) const NOEXCEPT;

    /// tx to puts (forward navigation)
    spend_links to_tx_spends(const tx_link& link) const NOEXCEPT;
    output_links to_tx_outputs(const tx_link& link) const NOEXCEPT;

    /// block to txs/puts (forward navigation)
    tx_links to_txs(const header_link& link) const NOEXCEPT;
    tx_link to_coinbase(const header_link& link) const NOEXCEPT;
    spend_links to_non_coinbase_spends(const header_link& link) const NOEXCEPT;
    spend_links to_block_spends(const header_link& link) const NOEXCEPT;
    output_links to_block_outputs(const header_link& link) const NOEXCEPT;

    /// hashmap enumeration
    header_link top_header(size_t bucket) const NOEXCEPT;
    point_link top_point(size_t bucket) const NOEXCEPT;
    spend_link top_spend(size_t bucket) const NOEXCEPT;
    txs_link top_txs(size_t bucket) const NOEXCEPT;
    tx_link top_tx(size_t bucket) const NOEXCEPT;

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
    inline bool set(const block& block) NOEXCEPT;
    inline bool set(const hash_digest& point_hash) NOEXCEPT;
    inline bool set(const transaction& tx) NOEXCEPT;

    /// False implies not fully populated, input.metadata is not populated.
    bool populate(const input& input) const NOEXCEPT;
    bool populate(const transaction& tx) const NOEXCEPT;
    bool populate(const block& block) const NOEXCEPT;

    /// Archival (surrogate-keyed).
    /// -----------------------------------------------------------------------

    /// Empty/null_hash implies fault.
    hashes get_tx_keys(const header_link& link) const NOEXCEPT;
    inline hash_digest get_header_key(const header_link& link) const NOEXCEPT;
    inline hash_digest get_point_key(const point_link& link) const NOEXCEPT;
    inline hash_digest get_tx_key(const tx_link& link) const NOEXCEPT;

    /// False implies not confirmed.
    bool get_tx_height(size_t& out, const tx_link& link) const NOEXCEPT;
    bool get_tx_position(size_t& out, const tx_link& link) const NOEXCEPT;

    /// False implies fault.
    bool get_height(size_t& out, const header_link& link) const NOEXCEPT;
    bool get_value(uint64_t& out, const output_link& link) const NOEXCEPT;

    inputs_ptr get_inputs(const tx_link& link) const NOEXCEPT;
    outputs_ptr get_outputs(const tx_link& link) const NOEXCEPT;
    transactions_ptr get_transactions(const header_link& link) const NOEXCEPT;

    header::cptr get_header(const header_link& link) const NOEXCEPT;
    block::cptr get_block(const header_link& link) const NOEXCEPT;
    transaction::cptr get_transaction(const tx_link& link) const NOEXCEPT;
    output::cptr get_output(const output_link& link) const NOEXCEPT;
    input::cptr get_input(const spend_link& link) const NOEXCEPT;
    point::cptr get_point(const spend_link& link) const NOEXCEPT;
    inputs_ptr get_spenders(const output_link& link) const NOEXCEPT;

    output::cptr get_output(const point& prevout) const NOEXCEPT;
    output::cptr get_output(const tx_link& link, uint32_t output_index) const NOEXCEPT;
    input::cptr get_input(const tx_link& link, uint32_t input_index) const NOEXCEPT;
    inputs_ptr get_spenders(const tx_link& link, uint32_t output_index) const NOEXCEPT;

    // TODO: all except point expose idempotency guard option.
    header_link set_link(const header& header, const chain_context& ctx) NOEXCEPT;
    header_link set_link(const header& header, const context& ctx) NOEXCEPT;
    header_link set_link(const block& block, const chain_context& ctx) NOEXCEPT;
    header_link set_link(const block& block, const context& ctx) NOEXCEPT;
    header_link set_link(const block& block) NOEXCEPT;
    point_link set_link(const hash_digest& point_hash) NOEXCEPT;
    tx_link set_link(const transaction& tx) NOEXCEPT;

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

    code get_header_state(const header_link& link) const NOEXCEPT;
    code get_block_state(uint64_t& fees, const header_link& link) const NOEXCEPT;
    code get_tx_state(const tx_link& link, const context& ctx) const NOEXCEPT;
    code get_tx_state(uint64_t& fee, size_t& sigops, const tx_link& link,
        const context& ctx) const NOEXCEPT;

    bool get_timestamp(uint32_t& timestamp, const header_link& link) const NOEXCEPT;
    bool get_version(uint32_t& version, const header_link& link) const NOEXCEPT;
    bool get_bits(uint32_t& bits, const header_link& link) const NOEXCEPT;
    bool get_context(context& ctx, const header_link& link) const NOEXCEPT;
    bool get_check_context(context& ctx, hash_digest& hash, uint32_t& timestamp,
        const header_link& link) const NOEXCEPT;

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
    bool is_confirmed_input(const spend_link& link) const NOEXCEPT;
    bool is_confirmed_output(const output_link& link) const NOEXCEPT;
    bool is_spent_output(const output_link& link) const NOEXCEPT;

    /// These are not used in confirmation.
    /// These rely on strong (use only for confirmation process).
    bool is_spent(const spend_link& link) const NOEXCEPT;
    bool is_strong(const spend_link& link) const NOEXCEPT;
    bool is_mature(const spend_link& link, size_t height) const NOEXCEPT;
    bool is_locked(const spend_link& link, uint32_t sequence,
        const context& ctx) const NOEXCEPT;

    /// These are used in confirmation.
    /// Block association relies on strong (confirmed or pending).
    code block_confirmable(const header_link& link) const NOEXCEPT;
    bool set_strong(const header_link& link) NOEXCEPT;
    bool set_unstrong(const header_link& link) NOEXCEPT;

    /// Height indexation.
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
    /// Translate.
    /// -----------------------------------------------------------------------
    uint32_t to_spend_index(const tx_link& parent_fk,
        const spend_link& input_fk) const NOEXCEPT;
    uint32_t to_output_index(const tx_link& parent_fk,
        const output_link& output_fk) const NOEXCEPT;
    spend_link to_spender(const tx_link& link,
        const foreign_point& point) const NOEXCEPT;

    /// Validate.
    /// -----------------------------------------------------------------------
    inline code to_block_code(linkage<schema::code>::integer value) const NOEXCEPT;
    inline code to_tx_code(linkage<schema::code>::integer value) const NOEXCEPT;
    inline bool is_sufficient(const context& current,
        const context& evaluated) const NOEXCEPT;

    /// Confirm.
    /// -----------------------------------------------------------------------

    height_link get_height(const header_link& link) const NOEXCEPT;
    bool is_confirmed_unspent(const output_link& link) const NOEXCEPT;
    error::error_t mature_prevout(const point_link& link,
        size_t height) const NOEXCEPT;
    error::error_t locked_prevout(const point_link& link, uint32_t sequence,
        const context& ctx) const NOEXCEPT;

    // Critical path
    inline bool is_spent_prevout(const foreign_point& point,
        const tx_link& self) const NOEXCEPT;
    inline error::error_t spendable_prevout(const tx_link& link,
        uint32_t sequence, const context& ctx) const NOEXCEPT;

    /// context
    /// -----------------------------------------------------------------------

    bool get_candidate_work(uint256_t& work, size_t height) const NOEXCEPT;
    bool get_candidate_bits(uint32_t& bits, size_t height,
        const header& header, size_t header_height) const NOEXCEPT;
    bool get_candidate_version(uint32_t& version, size_t height,
        const header& header, size_t header_height) const NOEXCEPT;
    bool get_candidate_timestamp(uint32_t& time, size_t height,
        const header& header, size_t header_height) const NOEXCEPT;

    bool populate_candidate_work(chain_state::data& data, const header& header,
        size_t header_height) const NOEXCEPT;
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

    bool get_confirmed_work(uint256_t& work, size_t height) const NOEXCEPT;
    bool get_confirmed_bits(uint32_t& bits, size_t height,
        const header& header, size_t header_height) const NOEXCEPT;
    bool get_confirmed_version(uint32_t& version, size_t height,
        const header& header, size_t header_height) const NOEXCEPT;
    bool get_confirmed_timestamp(uint32_t& time, size_t height,
        const header& header, size_t header_height) const NOEXCEPT;

    bool populate_confirmed_work(chain_state::data& data, const header& header,
        size_t header_height) const NOEXCEPT;
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
