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

namespace libbitcoin {
namespace database {

struct settings;

/// Store creation settings, written at create and read at load. The stored
/// values govern the store, configuration is consumed only at creation.
struct BCD_API envelope
{
    /// The schema version compiled into this build (see schema::version).
    static const system::config::version compiled;

    envelope() NOEXCEPT;
    envelope(const system::settings& bitcoin,
        const settings& database) NOEXCEPT;

    void set(const settings& database) NOEXCEPT;

    bool from_data(reader& source) NOEXCEPT;
    bool to_data(flipper& sink) const NOEXCEPT;
    size_t serialized_size() const NOEXCEPT;
    bool operator==(const envelope& other) const NOEXCEPT = default;

    /// Store schema version, fixed width (see schema::version).
    system::config::version schema{};

    /// Database settings.
    uint16_t interval_depth{};
    uint32_t header_buckets{};
    uint32_t ins_buckets{};
    uint32_t outs_buckets{};
    uint32_t tx_buckets{};
    uint32_t strong_tx_buckets{};
    uint32_t duplicate_buckets{};
    uint32_t validated_tx_buckets{};
    uint8_t header_k{};
    uint8_t ins_k{};
    uint8_t outs_k{};
    uint8_t tx_k{};
    uint8_t strong_tx_k{};
    uint8_t duplicate_k{};
    uint8_t validated_tx_k{};
    bool filter{};

    /// System settings.
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
    size_t bip30_reactivate_height{};
    system::chain::checkpoint bip30_deactivate_checkpoint{};
    system::chain::checkpoint bip9_bit0_active_checkpoint{};
    system::chain::checkpoint bip9_bit1_active_checkpoint{};
    system::chain::checkpoint bip9_bit2_active_checkpoint{};

    /// Store state.
    bool pooling{};
};

} // namespace database
} // namespace libbitcoin

#endif
