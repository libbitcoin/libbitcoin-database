/**
 * Copyright (c) 2011-2025 libbitcoin developers (see AUTHORS)
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

#include <mutex>
#include <utility>
#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/error.hpp>
#include <bitcoin/database/primitives/primitives.hpp>
#include <bitcoin/database/tables/tables.hpp>

namespace libbitcoin {
namespace database {

/// Database type aliases.
using height_link = table::height::link;
using header_link = table::header::link;
using output_link = table::output::link;
using input_link = table::input::link;
using outs_link = table::outs::link;
using ins_link = table::ins::link;
using point_link = table::point::link;
using tx_link = table::transaction::link;
using filter_link = table::filter_tx::link;
using strong_link = table::strong_tx::link;

using header_links = std::vector<header_link::integer>;
using tx_links = std::vector<tx_link::integer>;
using input_links = std::vector<input_link::integer>;
using output_links = std::vector<output_link::integer>;
using point_links = std::vector<point_link::integer>;
using two_counts = std::pair<size_t, size_t>;
using point_key = table::point::key;

struct header_state{ header_link link; code ec; };
using header_states = std::vector<header_state>;

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
    using headers = system::chain::header_cptrs;
    using transaction = system::chain::transaction;
    using transactions = system::chain::transaction_cptrs;
    using inputs_ptr = system::chain::inputs_ptr;
    using outputs_ptr = system::chain::outputs_ptr;
    using transactions_ptr = system::chain::transactions_ptr;
    using chain_state = system::chain::chain_state;
    using chain_state_cptr = system::chain::chain_state::cptr;
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

    /// Count of puts resulting in table body search to detect duplication.
    size_t positive_search_count() const NOEXCEPT;

    /// Count of puts not resulting in table body search to detect duplication.
    size_t negative_search_count() const NOEXCEPT;

    /// Store extent.
    /// -----------------------------------------------------------------------

    /// Full store (all data) logical byte sizes.
    size_t store_size() const NOEXCEPT;
    size_t store_head_size() const NOEXCEPT;
    size_t store_body_size() const NOEXCEPT;

    /// Full archive (primary data) logical byte sizes.
    size_t archive_size() const NOEXCEPT;
    size_t archive_head_size() const NOEXCEPT;
    size_t archive_body_size() const NOEXCEPT;

    /// Table head logical byte sizes.
    size_t header_head_size() const NOEXCEPT;
    size_t output_head_size() const NOEXCEPT;
    size_t input_head_size() const NOEXCEPT;
    size_t point_head_size() const NOEXCEPT;
    size_t ins_head_size() const NOEXCEPT;
    size_t outs_head_size() const NOEXCEPT;
    size_t txs_head_size() const NOEXCEPT;
    size_t tx_head_size() const NOEXCEPT;

    size_t candidate_head_size() const NOEXCEPT;
    size_t confirmed_head_size() const NOEXCEPT;
    size_t strong_tx_head_size() const NOEXCEPT;
    size_t duplicate_head_size() const NOEXCEPT;
    size_t prevout_head_size() const NOEXCEPT;
    size_t validated_bk_head_size() const NOEXCEPT;
    size_t validated_tx_head_size() const NOEXCEPT;
    size_t filter_bk_head_size() const NOEXCEPT;
    size_t filter_tx_head_size() const NOEXCEPT;
    size_t address_head_size() const NOEXCEPT;

    /// Table body logical byte sizes.
    size_t header_body_size() const NOEXCEPT;
    size_t output_body_size() const NOEXCEPT;
    size_t input_body_size() const NOEXCEPT;
    size_t point_body_size() const NOEXCEPT;
    size_t ins_body_size() const NOEXCEPT;
    size_t outs_body_size() const NOEXCEPT;
    size_t txs_body_size() const NOEXCEPT;
    size_t tx_body_size() const NOEXCEPT;

    size_t candidate_body_size() const NOEXCEPT;
    size_t confirmed_body_size() const NOEXCEPT;
    size_t strong_tx_body_size() const NOEXCEPT;
    size_t duplicate_body_size() const NOEXCEPT;
    size_t prevout_body_size() const NOEXCEPT;
    size_t validated_bk_body_size() const NOEXCEPT;
    size_t validated_tx_body_size() const NOEXCEPT;
    size_t filter_bk_body_size() const NOEXCEPT;
    size_t filter_tx_body_size() const NOEXCEPT;
    size_t address_body_size() const NOEXCEPT;

    /// Table (head + body) logical byte sizes.
    size_t header_size() const NOEXCEPT;
    size_t output_size() const NOEXCEPT;
    size_t input_size() const NOEXCEPT;
    size_t point_size() const NOEXCEPT;
    size_t ins_size() const NOEXCEPT;
    size_t outs_size() const NOEXCEPT;
    size_t txs_size() const NOEXCEPT;
    size_t tx_size() const NOEXCEPT;

    size_t candidate_size() const NOEXCEPT;
    size_t confirmed_size() const NOEXCEPT;
    size_t strong_tx_size() const NOEXCEPT;
    size_t duplicate_size() const NOEXCEPT;
    size_t prevout_size() const NOEXCEPT;
    size_t validated_bk_size() const NOEXCEPT;
    size_t validated_tx_size() const NOEXCEPT;
    size_t filter_bk_size() const NOEXCEPT;
    size_t filter_tx_size() const NOEXCEPT;
    size_t address_size() const NOEXCEPT;

    /// Buckets (hashmap + arraymap).
    size_t header_buckets() const NOEXCEPT;
    size_t point_buckets() const NOEXCEPT;
    size_t txs_buckets() const NOEXCEPT;
    size_t tx_buckets() const NOEXCEPT;

    size_t strong_tx_buckets() const NOEXCEPT;
    size_t duplicate_buckets() const NOEXCEPT;
    size_t prevout_buckets() const NOEXCEPT;
    size_t validated_bk_buckets() const NOEXCEPT;
    size_t validated_tx_buckets() const NOEXCEPT;
    size_t filter_bk_buckets() const NOEXCEPT;
    size_t filter_tx_buckets() const NOEXCEPT;
    size_t address_buckets() const NOEXCEPT;

    /// Records.
    size_t header_records() const NOEXCEPT;
    size_t point_records() const NOEXCEPT;
    size_t ins_records() const NOEXCEPT;
    size_t outs_records() const NOEXCEPT;
    size_t tx_records() const NOEXCEPT;

    size_t candidate_records() const NOEXCEPT;
    size_t confirmed_records() const NOEXCEPT;
    size_t strong_tx_records() const NOEXCEPT;
    size_t duplicate_records() const NOEXCEPT;
    size_t filter_bk_records() const NOEXCEPT;
    size_t address_records() const NOEXCEPT;

    /// Counters (archive slabs - txs/puts/filter_tx can be derived).
    size_t input_count(const tx_link& link) const NOEXCEPT;
    size_t output_count(const tx_link& link) const NOEXCEPT;
    two_counts put_counts(const tx_link& link) const NOEXCEPT;
    size_t input_count(const tx_links& txs) const NOEXCEPT;
    size_t output_count(const tx_links& txs) const NOEXCEPT;
    two_counts put_counts(const tx_links& txs) const NOEXCEPT;

    /// Optional table state.
    bool address_enabled() const NOEXCEPT;
    bool filter_enabled() const NOEXCEPT;

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
    associations get_unassociated_above(size_t height, size_t count,
        size_t last) const NOEXCEPT;
    size_t get_unassociated_count() const NOEXCEPT;
    size_t get_unassociated_count_above(size_t height) const NOEXCEPT;
    size_t get_unassociated_count_above(size_t height,
        size_t last) const NOEXCEPT;
    hashes get_candidate_hashes(const heights& heights) const NOEXCEPT;
    hashes get_confirmed_hashes(const heights& heights) const NOEXCEPT;

    /// Translation (key/link to link/s).
    /// -----------------------------------------------------------------------
    /// to_input not provided as input_link cannot produce chain::input.

    /// search key (entry)
    inline header_link to_candidate(size_t height) const NOEXCEPT;
    inline header_link to_confirmed(size_t height) const NOEXCEPT;
    inline header_link to_header(const hash_digest& key) const NOEXCEPT;
    inline tx_link to_tx(const hash_digest& key) const NOEXCEPT;
    inline filter_link to_filter(const header_link& key) const NOEXCEPT;
    inline output_link to_output(const point& prevout) const NOEXCEPT;
    inline output_link to_output(const hash_digest& key,
        uint32_t output_index) const NOEXCEPT;

    /// put to tx (reverse navigation)
    tx_link to_output_tx(const output_link& link) const NOEXCEPT;
    tx_link to_prevout_tx(const point_link& link) const NOEXCEPT;
    tx_link to_spending_tx(const point_link& link) const NOEXCEPT;

    /// point to put (forward navigation)
    point_link to_point(const tx_link& link,
        uint32_t input_index) const NOEXCEPT;
    output_link to_output(const tx_link& link,
        uint32_t output_index) const NOEXCEPT;
    output_link to_prevout(const point_link& link) const NOEXCEPT;

    /// block/tx to block (reverse navigation)
    header_link to_strong(const hash_digest& tx_hash) const NOEXCEPT;
    header_link to_parent(const header_link& link) const NOEXCEPT;
    header_link to_block(const tx_link& key) const NOEXCEPT;

    /// output to spenders (reverse navigation)
    point_links to_spenders(const point& prevout) const NOEXCEPT;
    point_links to_spenders(const output_link& link) const NOEXCEPT;
    point_links to_spenders(const hash_digest& point_hash,
        uint32_t output_index) const NOEXCEPT;
    point_links to_spenders(const tx_link& output_tx,
        uint32_t output_index) const NOEXCEPT;

    /// tx to puts (forward navigation)
    point_links to_points(const tx_link& link) const NOEXCEPT;
    output_links to_outputs(const tx_link& link) const NOEXCEPT;
    output_links to_prevouts(const tx_link& link) const NOEXCEPT;

    /// txs to puts (forward navigation)
    point_links to_points(const tx_links& txs) const NOEXCEPT;
    output_links to_outputs(const tx_links& txs) const NOEXCEPT;
    output_links to_prevouts(const tx_links& txs) const NOEXCEPT;

    /// block to puts (forward navigation)
    point_links to_block_points(const header_link& link) const NOEXCEPT;
    output_links to_block_outputs(const header_link& link) const NOEXCEPT;
    output_links to_block_prevouts(const header_link& link) const NOEXCEPT;

    /// block to txs (forward navigation)
    tx_link to_coinbase(const header_link& link) const NOEXCEPT;
    tx_links to_transactions(const header_link& link) const NOEXCEPT;
    tx_links to_spending_txs(const header_link& link) const NOEXCEPT;

    /// header to arraymap tables (guard domain transitions)
    constexpr size_t to_validated_bk(const header_link& link) const NOEXCEPT;
    constexpr size_t to_filter_bk(const header_link& link) const NOEXCEPT;
    constexpr size_t to_filter_tx(const header_link& link) const NOEXCEPT;
    constexpr size_t to_prevout(const header_link& link) const NOEXCEPT;
    constexpr size_t to_txs(const header_link& link) const NOEXCEPT;

    /// hashmap enumeration
    header_link top_header(size_t bucket) const NOEXCEPT;
    point_link top_point(size_t bucket) const NOEXCEPT;
    tx_link top_tx(size_t bucket) const NOEXCEPT;

    /// Archive reads.
    /// -----------------------------------------------------------------------

    // Bools.
    inline bool is_header(const hash_digest& key) const NOEXCEPT;
    inline bool is_block(const hash_digest& key) const NOEXCEPT;
    inline bool is_tx(const hash_digest& key) const NOEXCEPT;
    inline bool is_coinbase(const tx_link& link) const NOEXCEPT;
    inline bool is_milestone(const header_link& link) const NOEXCEPT;
    inline bool is_associated(const header_link& link) const NOEXCEPT;
    inline bool is_confirmable(const header_link& link) const NOEXCEPT;
    inline bool is_valid(const header_link& link) const NOEXCEPT;

    /// Empty/null_hash implies fault, zero count implies unassociated.
    hashes get_tx_keys(const header_link& link) const NOEXCEPT;
    size_t get_tx_count(const header_link& link) const NOEXCEPT;
    inline hash_digest get_header_key(const header_link& link) const NOEXCEPT;
    inline hash_digest get_tx_key(const tx_link& link) const NOEXCEPT;
    inline point_key get_point_key(const point_link& link) const NOEXCEPT;
    inline hash_digest get_point_hash(const point_link& link) const NOEXCEPT;

    /// False implies not confirmed.
    bool get_tx_height(size_t& out, const tx_link& link) const NOEXCEPT;
    bool get_tx_position(size_t& out, const tx_link& link) const NOEXCEPT;
    bool get_tx_sizes(size_t& light, size_t& is_locked,
        const tx_link& link) const NOEXCEPT;

    /// Terminal implies not found, false implies fault.
    size_t get_block_size(const header_link& link) const NOEXCEPT;
    height_link get_height(const hash_digest& key) const NOEXCEPT;
    height_link get_height(const header_link& link) const NOEXCEPT;
    bool get_height(size_t& out, const hash_digest& key) const NOEXCEPT;
    bool get_height(size_t& out, const header_link& link) const NOEXCEPT;
    bool get_value(uint64_t& out, const output_link& link) const NOEXCEPT;
    bool get_unassociated(association& out,
        const header_link& link) const NOEXCEPT;

    /// Objects.
    /// -----------------------------------------------------------------------

    inputs_ptr get_inputs(const tx_link& link) const NOEXCEPT;
    outputs_ptr get_outputs(const tx_link& link) const NOEXCEPT;
    transactions_ptr get_transactions(const header_link& link) const NOEXCEPT;

    header::cptr get_header(const header_link& link) const NOEXCEPT;
    block::cptr get_block(const header_link& link) const NOEXCEPT;
    transaction::cptr get_transaction(const tx_link& link) const NOEXCEPT;
    output::cptr get_output(const output_link& link) const NOEXCEPT;
    input::cptr get_input(const point_link& link) const NOEXCEPT;
    inputs_ptr get_spenders(const output_link& link) const NOEXCEPT;

    input::cptr get_input(const tx_link& link,
        uint32_t input_index) const NOEXCEPT;
    output::cptr get_output(const tx_link& link,
        uint32_t output_index) const NOEXCEPT;
    point get_point(const point_link& link) const NOEXCEPT;
    inputs_ptr get_spenders(const tx_link& link,
        uint32_t output_index) const NOEXCEPT;

    /// False implies missing prevouts, node input.metadata is populated.
    bool populate_with_metadata(const input& input) const NOEXCEPT;
    bool populate_with_metadata(const block& block) const NOEXCEPT;
    bool populate_with_metadata(const transaction& tx) const NOEXCEPT;

    /// False implies missing prevouts, input.metadata is not populated.
    bool populate_without_metadata(const input& input) const NOEXCEPT;
    bool populate_without_metadata(const block& block) const NOEXCEPT;
    bool populate_without_metadata(const transaction& tx) const NOEXCEPT;

    /// Archive writes.
    /// -----------------------------------------------------------------------

    /// Bool returns.
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

    /// Set transaction.
    code set_code(const transaction& tx) NOEXCEPT;

    /// Set header (headers-first).
    code set_code(const header& header, const context& ctx,
        bool milestone) NOEXCEPT;
    code set_code(const header& header, const chain_context& ctx,
        bool milestone) NOEXCEPT;
    code set_code(header_link& out_fk, const header& header,
        const context& ctx, bool milestone, bool=false) NOEXCEPT;
    code set_code(header_link& out_fk, const header& header,
        const chain_context& ctx, bool milestone, bool=false) NOEXCEPT;

    /// Set full block (blocks-first).
    code set_code(const block& block, const context& ctx, bool milestone,
        bool strong) NOEXCEPT;
    code set_code(const block& block, const chain_context& ctx, bool milestone,
        bool strong) NOEXCEPT;
    code set_code(header_link& out_fk, const block& block, const context& ctx,
        bool milestone, bool strong) NOEXCEPT;
    code set_code(header_link& out_fk, const block& block,
        const chain_context& ctx, bool milestone, bool strong) NOEXCEPT;

    /// Set block.txs (headers-first).
    code set_code(const block& block, bool strong) NOEXCEPT;
    code set_code(header_link& out_fk, const block& block, bool strong) NOEXCEPT;
    code set_code(const block& block, const header_link& key, bool strong) NOEXCEPT;

    /// Context.
    /// -----------------------------------------------------------------------

    chain_state_cptr get_chain_state(const system::settings& settings,
        const hash_digest& hash) const NOEXCEPT;
    chain_state_cptr get_candidate_chain_state(
        const system::settings& settings) const NOEXCEPT;
    chain_state_cptr get_candidate_chain_state(const system::settings& settings,
        size_t height) const NOEXCEPT;
    chain_state_cptr get_candidate_chain_state(const system::settings& settings,
        const header_link& link, size_t height) const NOEXCEPT;
    chain_state_cptr get_candidate_chain_state(const system::settings& settings,
        const header& header, const header_link& link,
        size_t height) const NOEXCEPT;

    /// Validation.
    /// -----------------------------------------------------------------------

    /// States.
    code get_header_state(const header_link& link) const NOEXCEPT;
    code get_block_state(const header_link& link) const NOEXCEPT;
    code get_block_state(uint64_t& fees, const header_link& link) const NOEXCEPT;
    code get_tx_state(const tx_link& link, const context& ctx) const NOEXCEPT;
    code get_tx_state(uint64_t& fee, size_t& sigops, const tx_link& link,
        const context& ctx) const NOEXCEPT;

    /// Values.
    // get_context(chain::context) sets only flags, median_time_past, height.
    bool get_timestamp(uint32_t& timestamp, const header_link& link) const NOEXCEPT;
    bool get_version(uint32_t& version, const header_link& link) const NOEXCEPT;
    bool get_work(uint256_t& work, const header_link& link) const NOEXCEPT;
    bool get_bits(uint32_t& bits, const header_link& link) const NOEXCEPT;
    bool get_context(context& ctx, const header_link& link) const NOEXCEPT;
    bool get_context(system::chain::context& ctx,
        const header_link& link) const NOEXCEPT;

    /// Setters.
    bool set_block_valid(const header_link& link, uint64_t fees) NOEXCEPT;
    bool set_block_unconfirmable(const header_link& link) NOEXCEPT;
    bool set_block_confirmable(const header_link& link) NOEXCEPT;
    bool set_block_unknown(const header_link& link) NOEXCEPT;
    bool set_tx_unknown(const tx_link& link) NOEXCEPT;
    bool set_tx_disconnected(const tx_link& link, const context& ctx) NOEXCEPT;
    bool set_tx_connected(const tx_link& link, const context& ctx,
        uint64_t fee, size_t sigops) NOEXCEPT;

    /// Confirmation.
    /// -----------------------------------------------------------------------
    /// These are not used in consensus confirmation.

    /// These compare strong with height index.
    bool is_candidate_header(const header_link& link) const NOEXCEPT;
    bool is_confirmed_block(const header_link& link) const NOEXCEPT;
    bool is_confirmed_tx(const tx_link& link) const NOEXCEPT;
    bool is_confirmed_input(const point_link& link) const NOEXCEPT;
    bool is_confirmed_output(const output_link& link) const NOEXCEPT;
    bool is_spent_output(const output_link& link) const NOEXCEPT;

    /// Height index not used by these.
    bool is_strong_tx(const tx_link& link) const NOEXCEPT;
    bool is_strong_block(const header_link& link) const NOEXCEPT;
    bool is_unconfirmable(const header_link& link) const NOEXCEPT;

    /// Consensus.
    /// -----------------------------------------------------------------------
    /// These are used in consensus confirmation.

    code block_confirmable(const header_link& link) const NOEXCEPT;
    bool is_prevouts_cached(const header_link& link) const NOEXCEPT;

    bool set_strong(const header_link& link) NOEXCEPT;
    bool set_unstrong(const header_link& link) NOEXCEPT;
    bool set_prevouts(const header_link& link, const block& block) NOEXCEPT;
    bool get_branch(header_states& branch, const hash_digest& hash) const NOEXCEPT;
    bool get_work(uint256_t& work, const header_states& states) const NOEXCEPT;
    bool get_strong_branch(bool& strong, const uint256_t& branch_work,
        size_t branch_point) const NOEXCEPT;
    bool get_strong_fork(bool& strong, const uint256_t& fork_work,
        size_t fork_point) const NOEXCEPT;

    /// Height indexation.
    /// -----------------------------------------------------------------------

    size_t get_candidate_size() const NOEXCEPT;
    size_t get_candidate_size(size_t top) const NOEXCEPT;
    size_t get_confirmed_size() const NOEXCEPT;
    size_t get_confirmed_size(size_t top) const NOEXCEPT;

    header_links get_confirmed_fork(const header_link& fork) const NOEXCEPT;
    header_links get_candidate_fork(size_t& fork_point) const NOEXCEPT;
    header_states get_validated_fork(size_t& fork_point,
        size_t top_checkpoint=zero, bool filter=false) const NOEXCEPT;

    bool initialize(const block& genesis) NOEXCEPT;
    bool push_candidate(const header_link& link) NOEXCEPT;
    bool push_confirmed(const header_link& link, bool strong) NOEXCEPT;
    bool pop_candidate() NOEXCEPT;
    bool pop_confirmed() NOEXCEPT;

    /// Populate message payloads from locator.
    headers get_headers(const hashes& locator, const hash_digest& stop,
        size_t limit) const NOEXCEPT;
    hashes get_blocks(const hashes& locator, const hash_digest& stop,
        size_t limit) const NOEXCEPT;

    /// Optional Tables.
    /// -----------------------------------------------------------------------

    /// Address, set internal to tx (natural-keyed).
    bool to_address_outputs(output_links& out,
        const hash_digest& key) const NOEXCEPT;
    bool to_confirmed_unspent_outputs(output_links& out,
        const hash_digest& key) const NOEXCEPT;
    bool to_minimum_unspent_outputs(output_links& out, const hash_digest& key,
        uint64_t value) const NOEXCEPT;
    bool get_confirmed_balance(uint64_t& out,
        const hash_digest& key) const NOEXCEPT;

    bool is_filtered(const header_link& link) const NOEXCEPT;
    bool get_filter_body(filter& out, const header_link& link) const NOEXCEPT;
    bool get_filter_head(hash_digest& out, const header_link& link) const NOEXCEPT;
    bool set_filter_body(const header_link& link, const block& block) NOEXCEPT;
    bool set_filter_body(const header_link& link, const filter& body) NOEXCEPT;
    bool set_filter_head(const header_link& link) NOEXCEPT;
    bool set_filter_head(const header_link& link,
        const hash_digest& head) NOEXCEPT;

protected:
    struct span
    {
        size_t size() const NOEXCEPT { return end - begin; }
        size_t begin;
        size_t end;
    };

    /// Network
    /// -----------------------------------------------------------------------

    /// Height of highest confirmed block (assumes locator descending).
    size_t get_fork(const hashes& locator) const NOEXCEPT;

    /// Height of highest confirmed block (assumes locator descending).
    span get_locator_span(const hashes& locator, const hash_digest& stop,
        size_t limit) const NOEXCEPT;

    /// Translate.
    /// -----------------------------------------------------------------------
    uint32_t to_input_index(const tx_link& parent_fk,
        const point_link& point_fk) const NOEXCEPT;
    uint32_t to_output_index(const tx_link& parent_fk,
        const output_link& output_fk) const NOEXCEPT;

    /// Objects.
    /// -----------------------------------------------------------------------
    static inline point::cptr make_point(hash_digest&& hash,
        uint32_t index) NOEXCEPT;

    /// Validate.
    /// -----------------------------------------------------------------------
    inline code to_block_code(linkage<schema::code>::integer value) const NOEXCEPT;
    inline code to_tx_code(linkage<schema::code>::integer value) const NOEXCEPT;
    inline bool is_sufficient(const context& current,
        const context& evaluated) const NOEXCEPT;

    /// Called by confirmation chaser.
    bool is_block_validated(code& state, const header_link& link,
        size_t height, size_t checkpoint) const NOEXCEPT;

    /// Confirm.
    /// -----------------------------------------------------------------------
    bool is_confirmed_unspent(const output_link& link) const NOEXCEPT;

    /// Consensus.
    /// -----------------------------------------------------------------------

    /// Called by block_confirmable (check bip30)
    bool is_spent_coinbase(const tx_link& link) const NOEXCEPT;
    code unspent_duplicates(const header_link& coinbase,
        const context& ctx) const NOEXCEPT;

    /// Called by block_confirmable (populate and check double spends).
    error::error_t unspendable(uint32_t sequence, bool coinbase,
        const tx_link& prevout_tx, uint32_t version,
        const context& ctx) const NOEXCEPT;

    /// Called by block_confirmable (populate and check double spends).
    code populate_prevouts(point_sets& sets, size_t points,
        const header_link& link) const NOEXCEPT;

    /// TODO: apply these to compact block confirmation, as the block will
    /// TODO: associate existing txs, making it impossible to rely on the
    /// TODO: duplicates table. The full query approach is used instead.
    bool get_double_spenders(tx_links& out, const block& block) const NOEXCEPT;
    bool get_double_spenders(tx_links& out, const point& point,
        const point_link& self) const NOEXCEPT;

    /// Support set_strong and set_unstrong writers.
    bool set_strong(const header_link& link, size_t count,
        const tx_link& first_fk, bool positive) NOEXCEPT;

    /// Get all tx links for any point of block that is also in duplicate table.
    bool get_doubles(tx_links& out, const block& block) const NOEXCEPT;
    bool get_doubles(tx_links& out, const point& point) const NOEXCEPT;

    /// Context.
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

    /// tx_fk must be allocated.
    /// -----------------------------------------------------------------------
    code set_code(const tx_link& tx_fk, const transaction& tx) NOEXCEPT;

private:
    // Chain objects.
    template <typename Bool>
    static inline bool push_bool(std_vector<Bool>& stack,
        const Bool& element) NOEXCEPT;

    // Not thread safe.
    size_t get_fork_() const NOEXCEPT;

    // These are thread safe.
    mutable std::shared_mutex candidate_reorganization_mutex_{};
    mutable std::shared_mutex confirmed_reorganization_mutex_{};
    Store& store_;
};

} // namespace database
} // namespace libbitcoin

#define TEMPLATE template <typename Store>
#define CLASS query<Store>

BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

#include <bitcoin/database/impl/query/query.ipp>
#include <bitcoin/database/impl/query/archive_read.ipp>
#include <bitcoin/database/impl/query/archive_write.ipp>
#include <bitcoin/database/impl/query/confirm.ipp>
#include <bitcoin/database/impl/query/consensus.ipp>
#include <bitcoin/database/impl/query/context.ipp>
#include <bitcoin/database/impl/query/extent.ipp>
#include <bitcoin/database/impl/query/height.ipp>
#include <bitcoin/database/impl/query/initialize.ipp>
#include <bitcoin/database/impl/query/network.ipp>
#include <bitcoin/database/impl/query/objects.ipp>
#include <bitcoin/database/impl/query/optional.ipp>
#include <bitcoin/database/impl/query/translate.ipp>
#include <bitcoin/database/impl/query/validate.ipp>

BC_POP_WARNING()

#undef CLASS
#undef TEMPLATE

#endif
