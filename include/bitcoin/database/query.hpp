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
#include <bitcoin/system.hpp>
#include <bitcoin/database/association.hpp>
#include <bitcoin/database/associations.hpp>
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
using filter_link = table::neutrino::link;

using header_links = std_vector<header_link::integer>;
using tx_links = std_vector<tx_link::integer>;
using input_links = std_vector<input_link::integer>;
using output_links = std_vector<output_link::integer>;
using spend_links = std_vector<spend_link::integer>;

struct strong_pair { header_link block{}; tx_link tx{}; };
using foreign_point = table::spend::search_key;
using two_counts = std::pair<size_t, size_t>;

struct spend_set
{
    struct spend
    {
        inline table::spend::search_key prevout() const NOEXCEPT
        {
            return table::spend::compose(point_fk, point_index);
        }

        inline bool is_null() const NOEXCEPT
        {
            return point_fk == table::spend::pt::terminal;
        }

        table::spend::pt::integer point_fk{};
        table::spend::ix::integer point_index{};
        uint32_t sequence{};
    };

    tx_link tx{};
    uint32_t version{};
    std_vector<spend> spends{};
};
using spend_sets = std_vector<spend_set>;

// Writers (non-const) are only: push_, pop_, set_ and initialize.
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
    using index = table::transaction::ix::integer;
    using sizes = std::pair<size_t, size_t>;
    using heights = std_vector<size_t>;
    using filter = system::data_chunk;

    query(Store& store) NOEXCEPT;

    /// Store management from query-holder (not store owner) context.
    /// -----------------------------------------------------------------------

    /// Get first fault code, or disk_full if none and full, or success.
    code get_code() const NOEXCEPT;

    /// Get first fault code.
    code get_fault() const NOEXCEPT;

    /// True if there is a fault condition (disk full is not a fault).
    bool is_fault() const NOEXCEPT;

    /// True if there is a disk full condition.
    bool is_full() const NOEXCEPT;

    /// Get the space required to clear the disk full condition.
    size_t get_space() const NOEXCEPT;

    /// Resume from disk full condition.
    code reload(const typename Store::event_handler& handler) const NOEXCEPT;

    /// Snapshot the store while running.
    code snapshot(const typename Store::event_handler& handler) const NOEXCEPT;

    /// Store extent.
    /// -----------------------------------------------------------------------

    /// Body + head logical byte sizes.
    size_t store_size() const NOEXCEPT;
    size_t archive_size() const NOEXCEPT;
    size_t header_size() const NOEXCEPT;
    size_t output_size() const NOEXCEPT;
    size_t input_size() const NOEXCEPT;
    size_t point_size() const NOEXCEPT;
    size_t puts_size() const NOEXCEPT;
    size_t spend_size() const NOEXCEPT;
    size_t txs_size() const NOEXCEPT;
    size_t tx_size() const NOEXCEPT;
    size_t candidate_size() const NOEXCEPT;
    size_t confirmed_size() const NOEXCEPT;
    size_t strong_tx_size() const NOEXCEPT;
    size_t validated_tx_size() const NOEXCEPT;
    size_t validated_bk_size() const NOEXCEPT;
    size_t address_size() const NOEXCEPT;
    size_t neutrino_size() const NOEXCEPT;

    /// Body logical byte sizes.
    size_t store_body_size() const NOEXCEPT;
    size_t archive_body_size() const NOEXCEPT;
    size_t header_body_size() const NOEXCEPT;
    size_t output_body_size() const NOEXCEPT;
    size_t input_body_size() const NOEXCEPT;
    size_t point_body_size() const NOEXCEPT;
    size_t puts_body_size() const NOEXCEPT;
    size_t spend_body_size() const NOEXCEPT;
    size_t txs_body_size() const NOEXCEPT;
    size_t tx_body_size() const NOEXCEPT;
    size_t candidate_body_size() const NOEXCEPT;
    size_t confirmed_body_size() const NOEXCEPT;
    size_t strong_tx_body_size() const NOEXCEPT;
    size_t validated_tx_body_size() const NOEXCEPT;
    size_t validated_bk_body_size() const NOEXCEPT;
    size_t address_body_size() const NOEXCEPT;
    size_t neutrino_body_size() const NOEXCEPT;

    /// Head logical byte sizes.
    size_t store_head_size() const NOEXCEPT;
    size_t archive_head_size() const NOEXCEPT;
    size_t header_head_size() const NOEXCEPT;
    size_t output_head_size() const NOEXCEPT;
    size_t input_head_size() const NOEXCEPT;
    size_t point_head_size() const NOEXCEPT;
    size_t puts_head_size() const NOEXCEPT;
    size_t spend_head_size() const NOEXCEPT;
    size_t txs_head_size() const NOEXCEPT;
    size_t tx_head_size() const NOEXCEPT;
    size_t candidate_head_size() const NOEXCEPT;
    size_t confirmed_head_size() const NOEXCEPT;
    size_t strong_tx_head_size() const NOEXCEPT;
    size_t validated_tx_head_size() const NOEXCEPT;
    size_t validated_bk_head_size() const NOEXCEPT;
    size_t address_head_size() const NOEXCEPT;
    size_t neutrino_head_size() const NOEXCEPT;

    /// Buckets.
    size_t header_buckets() const NOEXCEPT;
    size_t point_buckets() const NOEXCEPT;
    size_t spend_buckets() const NOEXCEPT;
    size_t txs_buckets() const NOEXCEPT;
    size_t tx_buckets() const NOEXCEPT;
    size_t strong_tx_buckets() const NOEXCEPT;
    size_t validated_tx_buckets() const NOEXCEPT;
    size_t validated_bk_buckets() const NOEXCEPT;
    size_t address_buckets() const NOEXCEPT;
    size_t neutrino_buckets() const NOEXCEPT;

    /// Records.
    size_t header_records() const NOEXCEPT;
    size_t point_records() const NOEXCEPT;
    size_t spend_records() const NOEXCEPT;
    size_t tx_records() const NOEXCEPT;
    size_t candidate_records() const NOEXCEPT;
    size_t confirmed_records() const NOEXCEPT;
    size_t strong_tx_records() const NOEXCEPT;
    size_t address_records() const NOEXCEPT;

    /// Counters (archive slabs - txs/puts/neutrino can be derived).
    size_t input_count(const tx_link& link) const NOEXCEPT;
    size_t output_count(const tx_link& link) const NOEXCEPT;
    two_counts put_counts(const tx_link& link) const NOEXCEPT;

    /// Optional table state.
    bool address_enabled() const NOEXCEPT;
    bool neutrino_enabled() const NOEXCEPT;

    /// Initialization (natural-keyed).
    /// -----------------------------------------------------------------------
    /// Not reliable during organization.

    inline bool is_initialized() const NOEXCEPT;
    inline size_t get_top_candidate() const NOEXCEPT;
    inline size_t get_top_confirmed() const NOEXCEPT;
    size_t get_fork() const NOEXCEPT;
    size_t get_top_associated() const NOEXCEPT;
    size_t get_top_associated_from(size_t height) const NOEXCEPT;
    associations get_all_unassociated() const NOEXCEPT;
    associations get_unassociated_above(size_t height) const NOEXCEPT;
    associations get_unassociated_above(size_t height,
        size_t count) const NOEXCEPT;
    associations get_unassociated_above(size_t height,
        size_t count, size_t last) const NOEXCEPT;
    size_t get_unassociated_count() const NOEXCEPT;
    size_t get_unassociated_count_above(size_t height) const NOEXCEPT;
    size_t get_unassociated_count_above(size_t height,
        size_t last) const NOEXCEPT;
    hashes get_candidate_hashes(const heights& heights) const NOEXCEPT;
    hashes get_confirmed_hashes(const heights& heights) const NOEXCEPT;

    /// Translation (key/link to link/s).
    /// -----------------------------------------------------------------------

    /// search key (entry)
    inline header_link to_candidate(size_t height) const NOEXCEPT;
    inline header_link to_confirmed(size_t height) const NOEXCEPT;
    inline header_link to_header(const hash_digest& key) const NOEXCEPT;
    inline point_link to_point(const hash_digest& key) const NOEXCEPT;
    inline tx_link to_tx(const hash_digest& key) const NOEXCEPT;
    inline txs_link to_txs(const header_link& key) const NOEXCEPT;
    inline filter_link to_filter(const header_link& key) const NOEXCEPT;

    /// put to tx (reverse navigation)
    tx_link to_spend_tx(const spend_link& link) const NOEXCEPT;
    tx_link to_output_tx(const output_link& link) const NOEXCEPT;
    tx_link to_prevout_tx(const spend_link& link) const NOEXCEPT;
    foreign_point to_spend_key(const spend_link& link) const NOEXCEPT;

    /// point to put (forward navigation)
    spend_link to_spend(const tx_link& link,
        uint32_t input_index) const NOEXCEPT;
    output_link to_output(const tx_link& link,
        uint32_t output_index) const NOEXCEPT;
    output_link to_prevout(const spend_link& link) const NOEXCEPT;

    /// block/tx to block/s (reverse navigation)
    header_link to_parent(const header_link& link) const NOEXCEPT;
    header_link to_block(const tx_link& key) const NOEXCEPT;

    /// output to spenders (reverse navigation)
    spend_links to_spenders(const point& prevout) const NOEXCEPT;
    spend_links to_spenders(const output_link& link) const NOEXCEPT;
    spend_links to_spenders(const foreign_point& point) const NOEXCEPT;
    spend_links to_spenders(const tx_link& link,
        uint32_t output_index) const NOEXCEPT;

    /// tx to puts (forward navigation)
    output_links to_tx_outputs(const tx_link& link) const NOEXCEPT;
    spend_links to_tx_spends(const tx_link& link) const NOEXCEPT;

    /// block to txs/puts (forward navigation)
    tx_link to_coinbase(const header_link& link) const NOEXCEPT;
    tx_links to_transactions(const header_link& link) const NOEXCEPT;
    output_links to_block_outputs(const header_link& link) const NOEXCEPT;
    spend_links to_block_spends(const header_link& link) const NOEXCEPT;

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
    inline bool is_milestone(const header_link& link) const NOEXCEPT;

    bool set(const header& header, const chain_context& ctx,
        bool milestone) NOEXCEPT;
    bool set(const header& header, const context& ctx,
        bool milestone) NOEXCEPT;
    bool set(const block& block, const chain_context& ctx,
        bool milestone, bool strong) NOEXCEPT;
    bool set(const block& block, const context& ctx,
        bool milestone, bool strong) NOEXCEPT;
    bool set(const transaction& tx) NOEXCEPT;
    bool set(const block& block, bool strong) NOEXCEPT;

    /// False implies not fully populated, input.metadata is not populated.
    bool populate(const input& input) const NOEXCEPT;
    bool populate(const transaction& tx) const NOEXCEPT;
    bool populate(const block& block) const NOEXCEPT;

    /// For testing only.
    /// False implies not fully populated, input.metadata is populated.
    bool populate_with_metadata(const input& input) const NOEXCEPT;
    bool populate_with_metadata(const transaction& tx) const NOEXCEPT;
    bool populate_with_metadata(const block& block) const NOEXCEPT;

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

    /// Terminal implies not found, false implies fault.
    height_link get_height(const hash_digest& key) const NOEXCEPT;
    height_link get_height(const header_link& link) const NOEXCEPT;
    bool get_height(size_t& out, const hash_digest& key) const NOEXCEPT;
    bool get_height(size_t& out, const header_link& link) const NOEXCEPT;
    bool get_value(uint64_t& out, const output_link& link) const NOEXCEPT;
    bool get_unassociated(association& out,
        const header_link& link) const NOEXCEPT;

    inputs_ptr get_inputs(const tx_link& link) const NOEXCEPT;
    outputs_ptr get_outputs(const tx_link& link) const NOEXCEPT;
    transactions_ptr get_transactions(const header_link& link) const NOEXCEPT;

    size_t get_candidate_size() const NOEXCEPT;
    size_t get_candidate_size(size_t top) const NOEXCEPT;
    size_t get_confirmed_size() const NOEXCEPT;
    size_t get_confirmed_size(size_t top) const NOEXCEPT;
    size_t get_block_size(const header_link& link) const NOEXCEPT;
    header::cptr get_header(const header_link& link) const NOEXCEPT;
    block::cptr get_block(const header_link& link) const NOEXCEPT;
    transaction::cptr get_transaction(const tx_link& link) const NOEXCEPT;
    output::cptr get_output(const output_link& link) const NOEXCEPT;
    input::cptr get_input(const spend_link& link) const NOEXCEPT;
    point::cptr get_point(const spend_link& link) const NOEXCEPT;
    inputs_ptr get_spenders(const output_link& link) const NOEXCEPT;

    output::cptr get_output(const point& prevout) const NOEXCEPT;
    output::cptr get_output(const tx_link& link,
        uint32_t output_index) const NOEXCEPT;
    input::cptr get_input(const tx_link& link,
        uint32_t input_index) const NOEXCEPT;
    inputs_ptr get_spenders(const tx_link& link,
        uint32_t output_index) const NOEXCEPT;

    /// Set transaction.
    code set_code(const transaction& tx) NOEXCEPT;
    code set_code(tx_link& out_fk, const transaction& tx) NOEXCEPT;
    tx_link set_link(const transaction& tx) NOEXCEPT;

    /// Set header (headers-first).
    code set_code(const header& header, const context& ctx,
        bool milestone) NOEXCEPT;
    code set_code(const header& header, const chain_context& ctx,
        bool milestone) NOEXCEPT;
    code set_code(header_link& out_fk, const header& header,
        const context& ctx, bool milestone, bool=false) NOEXCEPT;
    code set_code(header_link& out_fk, const header& header,
        const chain_context& ctx, bool milestone, bool=false) NOEXCEPT;
    header_link set_link(const header& header, const auto& ctx,
        bool milestone) NOEXCEPT;

    /// Set full block (blocks-first).
    code set_code(const block& block, const context& ctx, bool milestone,
        bool strong) NOEXCEPT;
    code set_code(const block& block, const chain_context& ctx, bool milestone,
        bool strong) NOEXCEPT;
    code set_code(header_link& out_fk, const block& block, const context& ctx,
        bool milestone, bool strong) NOEXCEPT;
    code set_code(header_link& out_fk, const block& block,
        const chain_context& ctx, bool milestone, bool strong) NOEXCEPT;
    header_link set_link(const block& block, const auto& ctx, bool milestone,
        bool strong) NOEXCEPT;

    /// Set txs (headers-first).
    code set_code(const transactions& txs, const header_link& key,
        size_t block_size, bool strong) NOEXCEPT;
    code set_code(txs_link& out_fk, const transactions& txs,
        const header_link& key, size_t block_size, bool strong) NOEXCEPT;
    txs_link set_link(const transactions& txs, const header_link& key,
        size_t block_size, bool strong) NOEXCEPT;

    /// Set block.txs (headers-first).
    code set_code(const block& block, bool strong) NOEXCEPT;
    code set_code(header_link& out_fk, const block& block,
        bool strong) NOEXCEPT;
    header_link set_link(const block& block, bool strong) NOEXCEPT;

    /// Chain state.
    /// -----------------------------------------------------------------------
    chain_state_ptr get_chain_state(const system::settings& settings,
        const hash_digest& hash) const NOEXCEPT;
    chain_state_ptr get_candidate_chain_state(
        const system::settings& settings) const NOEXCEPT;
    chain_state_ptr get_candidate_chain_state(const system::settings& settings,
        size_t height) const NOEXCEPT;
    chain_state_ptr get_candidate_chain_state(const system::settings& settings,
        const header_link& link, size_t height) const NOEXCEPT;
    chain_state_ptr get_candidate_chain_state(const system::settings& settings,
        const header& header, const header_link& link,
        size_t height) const NOEXCEPT;

    /// Validation (surrogate-keyed).
    /// -----------------------------------------------------------------------

    code get_header_state(const header_link& link) const NOEXCEPT;
    code get_block_state(const header_link& link) const NOEXCEPT;
    code get_block_state(uint64_t& fees, const header_link& link) const NOEXCEPT;
    code get_tx_state(const tx_link& link, const context& ctx) const NOEXCEPT;
    code get_tx_state(uint64_t& fee, size_t& sigops, const tx_link& link,
        const context& ctx) const NOEXCEPT;

    bool get_bits(uint32_t& bits, const header_link& link) const NOEXCEPT;
    bool get_work(uint256_t& work, const header_link& link) const NOEXCEPT;
    bool get_context(context& ctx, const header_link& link) const NOEXCEPT;
    bool get_version(uint32_t& version, const header_link& link) const NOEXCEPT;
    bool get_timestamp(uint32_t& timestamp,
        const header_link& link) const NOEXCEPT;

    bool set_block_valid(const header_link& link) NOEXCEPT;
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
    bool is_candidate_header(const header_link& link) const NOEXCEPT;
    bool is_confirmed_block(const header_link& link) const NOEXCEPT;
    bool is_confirmed_tx(const tx_link& link) const NOEXCEPT;
    bool is_confirmed_input(const spend_link& link) const NOEXCEPT;
    bool is_confirmed_output(const output_link& link) const NOEXCEPT;
    bool is_spent_output(const output_link& link) const NOEXCEPT;

    /// These are not used in confirmation.
    /// These rely on strong (use only for confirmation process).
    bool is_spent(const spend_link& link) const NOEXCEPT;
    bool is_strong_tx(const tx_link& link) const NOEXCEPT;
    bool is_strong_block(const header_link& link) const NOEXCEPT;
    bool is_strong_spend(const spend_link& link) const NOEXCEPT;
    bool is_mature(const spend_link& link, size_t height) const NOEXCEPT;
    bool is_locked(const spend_link& link, uint32_t sequence,
        const context& ctx) const NOEXCEPT;

    /// These are used in confirmation.
    /// Block association relies on strong (confirmed or pending).
    bool set_strong(const header_link& link) NOEXCEPT;
    bool set_unstrong(const header_link& link) NOEXCEPT;
    code block_confirmable(const header_link& link) const NOEXCEPT;
    code tx_confirmable(const tx_link& link, const context& ctx) const NOEXCEPT;
    code unspent_duplicates(const tx_link& coinbase,
        const context& ctx) const NOEXCEPT;

    /// Height indexation.
    bool initialize(const block& genesis) NOEXCEPT;
    bool push_candidate(const header_link& link) NOEXCEPT;
    bool push_confirmed(const header_link& link) NOEXCEPT;
    bool pop_candidate() NOEXCEPT;
    bool pop_confirmed() NOEXCEPT;

    /// Optional Tables.
    /// -----------------------------------------------------------------------

    /// Address, set internal to tx (natural-keyed).
    bool get_confirmed_balance(uint64_t& out,
        const hash_digest& key) const NOEXCEPT;
    bool to_address_outputs(output_links& out,
        const hash_digest& key) const NOEXCEPT;
    bool to_unspent_outputs(output_links& out,
        const hash_digest& key) const NOEXCEPT;
    bool to_minimum_unspent_outputs(output_links& out, const hash_digest& key,
        uint64_t value) const NOEXCEPT;

    /// Neutrino, set during validation with prevouts (surrogate-keyed).
    bool get_filter(filter& out, const header_link& link) const NOEXCEPT;
    bool get_filter_head(hash_digest& out,
        const header_link& link) const NOEXCEPT;
    bool set_filter(const header_link& link, const hash_digest& head,
        const filter& body) NOEXCEPT;

protected:
    /// Translate.
    /// -----------------------------------------------------------------------

    spend_set to_spend_set(const tx_link& link) const NOEXCEPT;
    spend_sets to_spend_sets(const header_link& link) const NOEXCEPT;

    uint32_t to_spend_index(const tx_link& parent_fk,
        const spend_link& input_fk) const NOEXCEPT;
    uint32_t to_output_index(const tx_link& parent_fk,
        const output_link& output_fk) const NOEXCEPT;
    spend_link to_spender(const tx_link& link,
        const foreign_point& point) const NOEXCEPT;

    // Critical path
    inline tx_links to_strong_txs(const tx_link& link) const NOEXCEPT;
    inline tx_links to_strong_txs(const hash_digest& tx_hash) const NOEXCEPT;
    inline strong_pair to_strong(const hash_digest& tx_hash) const NOEXCEPT;

    /// Validate.
    /// -----------------------------------------------------------------------
    inline code to_block_code(linkage<schema::code>::integer value) const NOEXCEPT;
    inline code to_tx_code(linkage<schema::code>::integer value) const NOEXCEPT;
    inline bool is_sufficient(const context& current,
        const context& evaluated) const NOEXCEPT;

    /// Confirm.
    /// -----------------------------------------------------------------------

    bool is_confirmed_unspent(const output_link& link) const NOEXCEPT;
    code mature_prevout(const point_link& link,
        size_t height) const NOEXCEPT;
    code locked_prevout(const point_link& link, uint32_t sequence,
        const context& ctx) const NOEXCEPT;

    // Critical path
    bool is_spent_prevout(const tx_link& link, index index) const NOEXCEPT;
    bool is_spent_prevout(const foreign_point& point,
        const tx_link& self) const NOEXCEPT;
    error::error_t spent_prevout(const foreign_point& point,
        const tx_link& self) const NOEXCEPT;
    error::error_t unspendable_prevout(const point_link& link,
        uint32_t sequence, uint32_t version,
        const context& ctx) const NOEXCEPT;
    bool set_strong(const header_link& link, const tx_links& txs,
        bool positive) NOEXCEPT;

    /// context
    /// -----------------------------------------------------------------------

    /// any blocks

    bool populate_bits(chain_state::data& data,
        const chain_state::map& map, header_link link) const NOEXCEPT;
    bool populate_versions(chain_state::data& data,
        const chain_state::map& map, header_link link) const NOEXCEPT;
    bool populate_timestamps(chain_state::data& data,
        const chain_state::map& map, header_link link) const NOEXCEPT;
    bool populate_retarget(chain_state::data& data,
        const chain_state::map& map, header_link link) const NOEXCEPT;
    bool populate_hashes(chain_state::data& data,
        const chain_state::map& map) const NOEXCEPT;
    bool populate_work(chain_state::data& data,
        header_link link) const NOEXCEPT;
    bool populate_all(chain_state::data& data,
        const system::settings& settings, const header_link& link,
        size_t height) const NOEXCEPT;

    /// candidate blocks

    bool populate_candidate_bits(chain_state::data& data,
        const chain_state::map& map, const header& header) const NOEXCEPT;
    bool populate_candidate_versions(chain_state::data& data,
        const chain_state::map& map, const header& header) const NOEXCEPT;
    bool populate_candidate_timestamps(chain_state::data& data,
        const chain_state::map& map, const header& header) const NOEXCEPT;
    bool populate_candidate_retarget(chain_state::data& data,
        const chain_state::map& map, const header& header) const NOEXCEPT;
    bool populate_candidate_work(chain_state::data& data,
        const header& header) const NOEXCEPT;
    bool populate_candidate_all(chain_state::data& data,
        const system::settings& settings, const header& header,
        const header_link& link, size_t height) const NOEXCEPT;

private:
    struct maybe_strong { header_link block{}; tx_link tx{}; bool strong{}; };
    using maybe_strongs = std_vector<maybe_strong>;

    static inline tx_links strong_only(const maybe_strongs& pairs) NOEXCEPT;
    static inline bool contains(const maybe_strongs& pairs,
        const header_link& block) NOEXCEPT;

    // These are thread safe.
    Store& store_;
    bool minimize_;
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
