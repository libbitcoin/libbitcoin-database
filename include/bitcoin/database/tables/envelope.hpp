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
#ifndef LIBBITCOIN_DATABASE_TABLES_ENVELOPE_HPP
#define LIBBITCOIN_DATABASE_TABLES_ENVELOPE_HPP

#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/memory.hpp>
#include <bitcoin/database/tables/schema.hpp>

namespace libbitcoin {
namespace database {

struct settings;

/// Store creation settings, written at create and read at load. Stored values
/// govern store, configuration consumed only at creation.
struct BCD_API envelope
{
    /// Flags activated together, and the header that activated them.
    struct activation
    {
        uint32_t flags{};
        schema::header::link::integer link{ schema::header::link::terminal };
        bool operator==(const activation& other) const NOEXCEPT = default;
    };

    /// The schema version compiled into this build (see schema::version).
    static const system::config::version compiled;

    /// The oldest schema version migrated to compiled (see schema::migratable).
    static const system::config::version migratable;

    /// True if the schema is compiled or migratable to compiled.
    static bool is_readable(const system::config::version& schema) NOEXCEPT;

    /// The schema of a serialized envelope (default if too short).
    static system::config::version get_schema(
        const system::data_chunk& data) NOEXCEPT;

    /// Migrate a serialized 4.0.30 envelope to 4.0.31.
    static bool migrate_4_0_30_to_4_0_31(system::data_chunk& data) NOEXCEPT;

    /// Migrate a serialized 4.0.31 envelope to 4.0.32.
    static bool migrate_4_0_31_to_4_0_32(system::data_chunk& data,
        const envelope& configured) NOEXCEPT;

    /// Migrate a serialized 4.0.32 envelope to 4.0.33.
    static bool migrate_4_0_32_to_4_0_33(system::data_chunk& data) NOEXCEPT;

    /// Migrate serialized migratable envelope to next schema, taking values
    /// prior schema does not store from configured.
    static bool migrate_step(system::data_chunk& data,
        const envelope& configured) NOEXCEPT;

    /// Migrate a serialized readable envelope to the compiled schema.
    static bool migrate(system::data_chunk& data,
        const envelope& configured) NOEXCEPT;
    static constexpr size_t forks_count = 28;
    static constexpr auto activation_count = to_bits(sizeof(uint32_t));
    static constexpr size_t activation_size = sizeof(uint32_t) +
        schema::header::pk;
    static constexpr size_t pooling_size = sizeof(uint8_t) +
        (2 * sizeof(uint32_t));

    envelope() NOEXCEPT;
    envelope(const system::settings& bitcoin, const settings& database,
        bool limited, bool filters, bool witness, uint32_t silent) NOEXCEPT;

    void set(const settings& database) NOEXCEPT;

    /// Assign stored values to configuration, returning names of those that
    /// differed. Differing top checkpoint cannot be assigned to configuration
    /// and is only returned. Filters are not assigned, but their tables are
    /// unbucketed when not provided.
    system::string_list apply(system::settings& bitcoin, settings& database,
        bool& limited, bool& witness, uint32_t& silent) const NOEXCEPT;

    bool from_data(reader& source) NOEXCEPT;
    bool to_data(flipper& sink) const NOEXCEPT;
    size_t serialized_size() const NOEXCEPT;

    size_t pooling_offset() const NOEXCEPT;
    size_t activation_offset(size_t index) const NOEXCEPT;
    bool operator==(const envelope& other) const NOEXCEPT = default;

    /// Store schema version, fixed width (see schema::version).
    system::config::version schema{};

    /// System settings.
    system::chain::selection network{};
    system::forks forks{};
    uint64_t initial_subsidy_bitcoin{};
    uint32_t subsidy_interval_blocks{};
    uint32_t timestamp_limit_seconds{};
    uint32_t retargeting_factor{};
    uint32_t retargeting_interval_seconds{};
    uint32_t block_spacing_seconds{};
    uint32_t proof_of_work_limit{};
    uint32_t first_version{};
    uint32_t bip34_version{};
    uint32_t bip66_version{};
    uint32_t bip65_version{};
    uint32_t bip9_version_bit0{};
    uint32_t bip9_version_bit1{};
    uint32_t bip9_version_bit2{};
    uint32_t bip9_version_base{};
    uint32_t bip16_activation_time{};
    size_t bip34_activation_threshold{};
    size_t bip34_enforcement_threshold{};
    size_t bip34_activation_sample{};
    size_t bip90_bip34_height{};
    size_t bip90_bip65_height{};
    size_t bip90_bip66_height{};
    size_t bip9_activation_period{};
    system::chain::chain_state::deployment bip9_bit0_deployment{};
    system::chain::chain_state::deployment bip9_bit1_deployment{};
    system::chain::chain_state::deployment bip9_bit2_deployment{};
    system::chain::chain_state::deployment bip54_timewarp_deployment{};
    system::chain::chain_state::deployment bip54_timespan_deployment{};
    system::chain::chain_state::deployment bip54_sigops_deployment{};
    system::chain::chain_state::deployment bip54_txsize_deployment{};
    system::chain::chain_state::deployment bip54_coinbase_deployment{};
    system::chain::checkpoint top_checkpoint{};
    system::chain::checkpoint milestone{};
    system::chain::script signet_challenge{};

    /// Database settings.
    uint8_t interval_depth{};
    uint32_t header_buckets{};
    uint32_t ins_buckets{};
    uint32_t outs_buckets{};
    uint32_t tx_buckets{};
    uint32_t duplicate_buckets{};
    uint32_t pool_buckets{};
    uint32_t wtxid_buckets{};
    uint8_t header_k{};
    uint8_t ins_k{};
    uint8_t outs_k{};
    uint8_t tx_k{};
    uint8_t duplicate_k{};
    uint8_t pool_k{};
    uint8_t wtxid_k{};

    /// Network settings.
    bool limited_blocks{};
    bool provide_filters{};

    /// Node settings.
    bool require_witness{};
    uint32_t silent_start_height{ max_uint32 };

    /// Node state.
    bool pooling{};
    uint32_t scan_frontier{};
    uint32_t latch_tx{};
    std::array<activation, activation_count> activations{};
};

} // namespace database
} // namespace libbitcoin

#endif
