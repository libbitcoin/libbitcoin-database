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
#include <bitcoin/database/tables/envelope.hpp>

#include <bitcoin/database/define.hpp>
#include <bitcoin/database/settings.hpp>

namespace libbitcoin {
namespace database {

using namespace system;

// The version is fixed width, so it is written as segments, not as text.
const system::config::version envelope::compiled
{
    schema::version.at(0),
    schema::version.at(1),
    schema::version.at(2),
    schema::version.at(3)
};

envelope::envelope() NOEXCEPT
  : schema(compiled)
{
}

void envelope::set(const settings& database) NOEXCEPT
{
    interval_depth = database.interval_depth;
    header_buckets = database.header.buckets;
    ins_buckets = database.ins.buckets;
    outs_buckets = database.outs.buckets;
    tx_buckets = database.tx.buckets;
    strong_tx_buckets = database.strong_tx.buckets;
    duplicate_buckets = database.duplicate.buckets;
    pool_buckets = database.pool.buckets;
    provide_filters = to_bool(database.filter_bk.buckets) &&
        to_bool(database.filter_tx.buckets);
}

envelope::envelope(const system::settings& bitcoin, const settings& database,
    bool limited) NOEXCEPT
  : schema(compiled),
    forks(bitcoin.forks),
    initial_subsidy_bitcoin(bitcoin.initial_subsidy_bitcoin),
    subsidy_interval_blocks(bitcoin.subsidy_interval_blocks),
    timestamp_limit_seconds(bitcoin.timestamp_limit_seconds),
    retargeting_factor(bitcoin.retargeting_factor),
    retargeting_interval_seconds(bitcoin.retargeting_interval_seconds),
    block_spacing_seconds(bitcoin.block_spacing_seconds),
    proof_of_work_limit(bitcoin.proof_of_work_limit),
    first_version(bitcoin.first_version),
    bip34_version(bitcoin.bip34_version),
    bip66_version(bitcoin.bip66_version),
    bip65_version(bitcoin.bip65_version),
    bip9_version_bit0(bitcoin.bip9_version_bit0),
    bip9_version_bit1(bitcoin.bip9_version_bit1),
    bip9_version_bit2(bitcoin.bip9_version_bit2),
    bip9_version_base(bitcoin.bip9_version_base),
    bip16_activation_time(bitcoin.bip16_activation_time),
    bip34_activation_threshold(bitcoin.bip34_activation_threshold),
    bip34_enforcement_threshold(bitcoin.bip34_enforcement_threshold),
    bip34_activation_sample(bitcoin.bip34_activation_sample),
    bip90_bip34_height(bitcoin.bip90_bip34_height),
    bip90_bip65_height(bitcoin.bip90_bip65_height),
    bip90_bip66_height(bitcoin.bip90_bip66_height),
    bip30_reactivate_height(bitcoin.bip30_reactivate_height),
    bip30_deactivate_checkpoint(bitcoin.bip30_deactivate_checkpoint),
    bip9_bit0_active_checkpoint(bitcoin.bip9_bit0_active_checkpoint),
    bip9_bit1_active_checkpoint(bitcoin.bip9_bit1_active_checkpoint),
    bip9_bit2_active_checkpoint(bitcoin.bip9_bit2_active_checkpoint),
    top_checkpoint(bitcoin.top_checkpoint()),
    milestone(bitcoin.milestone),
    limited_blocks(limited)
{
    set(database);
}

bool envelope::from_data(reader& source) NOEXCEPT
{
    schema =
    {
        source.read_little_endian<uint32_t>(),
        source.read_little_endian<uint32_t>(),
        source.read_little_endian<uint32_t>(),
        source.read_little_endian<uint32_t>()
    };

    if (schema != compiled)
    {
        source.invalidate();
        return false;
    }

    forks.bip16 = to_bool(source.read_byte());
    forks.bip90 = to_bool(source.read_byte());
    forks.bip30 = to_bool(source.read_byte());
    forks.bip30_deactivate = to_bool(source.read_byte());
    forks.bip30_reactivate = to_bool(source.read_byte());
    forks.bip42 = to_bool(source.read_byte());
    forks.bip34 = to_bool(source.read_byte());
    forks.bip65 = to_bool(source.read_byte());
    forks.bip66 = to_bool(source.read_byte());
    forks.bip68 = to_bool(source.read_byte());
    forks.bip112 = to_bool(source.read_byte());
    forks.bip113 = to_bool(source.read_byte());
    forks.bip141 = to_bool(source.read_byte());
    forks.bip143 = to_bool(source.read_byte());
    forks.bip147 = to_bool(source.read_byte());
    forks.bip341 = to_bool(source.read_byte());
    forks.bip342 = to_bool(source.read_byte());
    forks.retarget = to_bool(source.read_byte());
    forks.difficult = to_bool(source.read_byte());
    forks.time_warp_patch = to_bool(source.read_byte());
    forks.block_storm_patch = to_bool(source.read_byte());
    forks.ltc_time_warp_patch = to_bool(source.read_byte());
    forks.ltc_retarget_overflow_patch = to_bool(source.read_byte());
    forks.ltc_scrypt_proof_of_work = to_bool(source.read_byte());

    initial_subsidy_bitcoin = source.read_little_endian<uint64_t>();
    subsidy_interval_blocks = source.read_little_endian<uint32_t>();
    timestamp_limit_seconds = source.read_little_endian<uint32_t>();
    retargeting_factor = source.read_little_endian<uint32_t>();
    retargeting_interval_seconds = source.read_little_endian<uint32_t>();
    block_spacing_seconds = source.read_little_endian<uint32_t>();
    proof_of_work_limit = source.read_little_endian<uint32_t>();
    first_version = source.read_little_endian<uint32_t>();
    bip34_version = source.read_little_endian<uint32_t>();
    bip66_version = source.read_little_endian<uint32_t>();
    bip65_version = source.read_little_endian<uint32_t>();
    bip9_version_bit0 = source.read_little_endian<uint32_t>();
    bip9_version_bit1 = source.read_little_endian<uint32_t>();
    bip9_version_bit2 = source.read_little_endian<uint32_t>();
    bip9_version_base = source.read_little_endian<uint32_t>();
    bip16_activation_time = source.read_little_endian<uint32_t>();
    bip34_activation_threshold = source.read_variable();
    bip34_enforcement_threshold = source.read_variable();
    bip34_activation_sample = source.read_variable();
    bip90_bip34_height = source.read_variable();
    bip90_bip65_height = source.read_variable();
    bip90_bip66_height = source.read_variable();
    bip30_reactivate_height = source.read_variable();

    const auto read_checkpoint = [&]() NOEXCEPT
    {
        const auto hash = source.read_hash();
        return chain::checkpoint{ hash, source.read_variable() };
    };

    bip30_deactivate_checkpoint = read_checkpoint();
    bip9_bit0_active_checkpoint = read_checkpoint();
    bip9_bit1_active_checkpoint = read_checkpoint();
    bip9_bit2_active_checkpoint = read_checkpoint();
    top_checkpoint = read_checkpoint();
    milestone = read_checkpoint();

    interval_depth = source.read_little_endian<uint16_t>();
    header_buckets = source.read_little_endian<uint32_t>();
    ins_buckets = source.read_little_endian<uint32_t>();
    outs_buckets = source.read_little_endian<uint32_t>();
    tx_buckets = source.read_little_endian<uint32_t>();
    strong_tx_buckets = source.read_little_endian<uint32_t>();
    duplicate_buckets = source.read_little_endian<uint32_t>();
    pool_buckets = source.read_little_endian<uint32_t>();
    header_k = source.read_byte();
    ins_k = source.read_byte();
    outs_k = source.read_byte();
    tx_k = source.read_byte();
    strong_tx_k = source.read_byte();
    duplicate_k = source.read_byte();
    pool_k = source.read_byte();

    limited_blocks = to_bool(source.read_byte());
    provide_filters = to_bool(source.read_byte());

    pooling = to_bool(source.read_byte());
    return source;
}

bool envelope::to_data(flipper& sink) const NOEXCEPT
{
    for (const auto segment: schema.segments())
        sink.write_little_endian<uint32_t>(segment);

    sink.write_byte(to_int<uint8_t>(forks.bip16));
    sink.write_byte(to_int<uint8_t>(forks.bip90));
    sink.write_byte(to_int<uint8_t>(forks.bip30));
    sink.write_byte(to_int<uint8_t>(forks.bip30_deactivate));
    sink.write_byte(to_int<uint8_t>(forks.bip30_reactivate));
    sink.write_byte(to_int<uint8_t>(forks.bip42));
    sink.write_byte(to_int<uint8_t>(forks.bip34));
    sink.write_byte(to_int<uint8_t>(forks.bip65));
    sink.write_byte(to_int<uint8_t>(forks.bip66));
    sink.write_byte(to_int<uint8_t>(forks.bip68));
    sink.write_byte(to_int<uint8_t>(forks.bip112));
    sink.write_byte(to_int<uint8_t>(forks.bip113));
    sink.write_byte(to_int<uint8_t>(forks.bip141));
    sink.write_byte(to_int<uint8_t>(forks.bip143));
    sink.write_byte(to_int<uint8_t>(forks.bip147));
    sink.write_byte(to_int<uint8_t>(forks.bip341));
    sink.write_byte(to_int<uint8_t>(forks.bip342));
    sink.write_byte(to_int<uint8_t>(forks.retarget));
    sink.write_byte(to_int<uint8_t>(forks.difficult));
    sink.write_byte(to_int<uint8_t>(forks.time_warp_patch));
    sink.write_byte(to_int<uint8_t>(forks.block_storm_patch));
    sink.write_byte(to_int<uint8_t>(forks.ltc_time_warp_patch));
    sink.write_byte(to_int<uint8_t>(forks.ltc_retarget_overflow_patch));
    sink.write_byte(to_int<uint8_t>(forks.ltc_scrypt_proof_of_work));

    sink.write_little_endian<uint64_t>(initial_subsidy_bitcoin);
    sink.write_little_endian<uint32_t>(subsidy_interval_blocks);
    sink.write_little_endian<uint32_t>(timestamp_limit_seconds);
    sink.write_little_endian<uint32_t>(retargeting_factor);
    sink.write_little_endian<uint32_t>(retargeting_interval_seconds);
    sink.write_little_endian<uint32_t>(block_spacing_seconds);
    sink.write_little_endian<uint32_t>(proof_of_work_limit);
    sink.write_little_endian<uint32_t>(first_version);
    sink.write_little_endian<uint32_t>(bip34_version);
    sink.write_little_endian<uint32_t>(bip66_version);
    sink.write_little_endian<uint32_t>(bip65_version);
    sink.write_little_endian<uint32_t>(bip9_version_bit0);
    sink.write_little_endian<uint32_t>(bip9_version_bit1);
    sink.write_little_endian<uint32_t>(bip9_version_bit2);
    sink.write_little_endian<uint32_t>(bip9_version_base);
    sink.write_little_endian<uint32_t>(bip16_activation_time);
    sink.write_variable(bip34_activation_threshold);
    sink.write_variable(bip34_enforcement_threshold);
    sink.write_variable(bip34_activation_sample);
    sink.write_variable(bip90_bip34_height);
    sink.write_variable(bip90_bip65_height);
    sink.write_variable(bip90_bip66_height);
    sink.write_variable(bip30_reactivate_height);

    const auto write_checkpoint = [&](const chain::checkpoint& in) NOEXCEPT
    {
        sink.write_bytes(in.hash());
        sink.write_variable(in.height());
    };

    write_checkpoint(bip30_deactivate_checkpoint);
    write_checkpoint(bip9_bit0_active_checkpoint);
    write_checkpoint(bip9_bit1_active_checkpoint);
    write_checkpoint(bip9_bit2_active_checkpoint);
    write_checkpoint(top_checkpoint);
    write_checkpoint(milestone);

    sink.write_little_endian<uint16_t>(interval_depth);
    sink.write_little_endian<uint32_t>(header_buckets);
    sink.write_little_endian<uint32_t>(ins_buckets);
    sink.write_little_endian<uint32_t>(outs_buckets);
    sink.write_little_endian<uint32_t>(tx_buckets);
    sink.write_little_endian<uint32_t>(strong_tx_buckets);
    sink.write_little_endian<uint32_t>(duplicate_buckets);
    sink.write_little_endian<uint32_t>(pool_buckets);
    sink.write_byte(header_k);
    sink.write_byte(ins_k);
    sink.write_byte(outs_k);
    sink.write_byte(tx_k);
    sink.write_byte(strong_tx_k);
    sink.write_byte(duplicate_k);
    sink.write_byte(pool_k);

    sink.write_byte(to_int<uint8_t>(limited_blocks));
    sink.write_byte(to_int<uint8_t>(provide_filters));

    sink.write_byte(to_int<uint8_t>(pooling));
    return sink;
}

size_t envelope::serialized_size() const NOEXCEPT
{
    constexpr auto forks_size = 24_size;
    constexpr auto fixed = (4 * sizeof(uint32_t)) + sizeof(uint16_t) +
        (7 * sizeof(uint32_t)) + (10 * sizeof(uint8_t)) + forks_size +
        sizeof(uint64_t) + (15 * sizeof(uint32_t));

    const auto checkpoint_size = [](const chain::checkpoint& in) NOEXCEPT
    {
        return hash_size + variable_size(in.height());
    };

    return fixed +
        variable_size(bip34_activation_threshold) +
        variable_size(bip34_enforcement_threshold) +
        variable_size(bip34_activation_sample) +
        variable_size(bip90_bip34_height) +
        variable_size(bip90_bip65_height) +
        variable_size(bip90_bip66_height) +
        variable_size(bip30_reactivate_height) +
        checkpoint_size(bip30_deactivate_checkpoint) +
        checkpoint_size(bip9_bit0_active_checkpoint) +
        checkpoint_size(bip9_bit1_active_checkpoint) +
        checkpoint_size(bip9_bit2_active_checkpoint) +
        checkpoint_size(top_checkpoint) +
        checkpoint_size(milestone);
}

} // namespace database
} // namespace libbitcoin
