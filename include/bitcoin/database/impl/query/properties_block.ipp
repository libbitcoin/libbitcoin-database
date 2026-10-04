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
#ifndef LIBBITCOIN_DATABASE_QUERY_PROPERTIES_BLOCK_IPP
#define LIBBITCOIN_DATABASE_QUERY_PROPERTIES_BLOCK_IPP

#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {
    
// boolean
// ----------------------------------------------------------------------------

TEMPLATE
inline bool CLASS::is_header(const hash_digest& key) const NOEXCEPT
{
    return store_.header.exists(key);
}

TEMPLATE
inline bool CLASS::is_block(const hash_digest& key) const NOEXCEPT
{
    return is_associated(to_header(key));
}

TEMPLATE
inline bool CLASS::is_block_segregated(const header_link& link) const NOEXCEPT
{
    size_t light{}, heavy{};
    return get_block_sizes(light, heavy, link) && heavy != light;
}

TEMPLATE
inline bool CLASS::is_milestone(const header_link& link) const NOEXCEPT
{
    table::header::get_milestone header{};
    return store_.header.get(link, header) && header.milestone;
}

TEMPLATE
inline bool CLASS::is_compact(const header_link& link) const NOEXCEPT
{
    table::header::get_compact header{};
    return store_.header.get(link, header) && header.compact;
}

TEMPLATE
inline bool CLASS::is_associated(const header_link& link) const NOEXCEPT
{
    table::txs::get_associated txs{};
    return store_.txs.at(to_txs(link), txs) && txs.associated;
}

// properties
// ----------------------------------------------------------------------------

// node/is_current
TEMPLATE
uint32_t CLASS::get_top_timestamp(bool confirmed) const NOEXCEPT
{
    const auto top = confirmed ? to_confirmed(get_top_confirmed()) :
        to_candidate(get_top_candidate());

    // returns zero if read fails.
    uint32_t timestamp{};
    /* bool */ get_timestamp(timestamp, top);
    return timestamp;
}

TEMPLATE
bool CLASS::get_timestamp(uint32_t& timestamp,
    const header_link& link) const NOEXCEPT
{
    table::header::get_timestamp header{};
    if (!store_.header.get(link, header))
        return false;

    timestamp = header.timestamp;
    return true;
}

TEMPLATE
bool CLASS::get_version(uint32_t& version,
    const header_link& link) const NOEXCEPT
{
    table::header::get_version header{};
    if (!store_.header.get(link, header))
        return false;

    version = header.version;
    return true;
}

TEMPLATE
bool CLASS::get_work(uint256_t& work, const header_link& link) const NOEXCEPT
{
    uint32_t bits{};
    const auto result = get_bits(bits, link);
    work = header::proof(bits);
    return result;
}

TEMPLATE
bool CLASS::get_branch_work(uint256_t& work, const header_link& link) const NOEXCEPT
{
    table::header::get_work header{};
    if (!store_.header.get(link, header))
        return false;

    work = header.work;
    return true;
}

TEMPLATE
bool CLASS::get_bits(uint32_t& bits, const header_link& link) const NOEXCEPT
{
    table::header::get_bits header{};
    if (!store_.header.get(link, header))
        return false;

    bits = header.bits;
    return true;
}

// context
// ----------------------------------------------------------------------------

TEMPLATE
bool CLASS::get_context(context& ctx, const header_link& link) const NOEXCEPT
{
    table::header::record_context header{};
    if (!store_.header.get(link, header))
        return false;

    ctx = std::move(header.ctx);
    return true;
}

TEMPLATE
bool CLASS::get_context(system::chain::context& ctx,
    const header_link& link) const NOEXCEPT
{
    table::header::record_context_timestamp header{};
    if (!store_.header.get(link, header))
        return false;

    // Context for header.check and header.accept are filled from chain_state.
    // So these are not populated here as they are not expected to be used.
    ctx =
    {
        // [block.check, .accept, .connect]
        .flags = header.ctx.flags,

        // [block.check]
        .timestamp = header.timestamp,

        // [block.check, header.accept]
        .median_time_past = header.ctx.mtp,

        // [block.check & block.accept]
        .height = header.ctx.height

        // [header.accept]
        // .minimum_block_version

        // [header.accept]
        // .work_required
    };

    return true;
}

// height
// ----------------------------------------------------------------------------

TEMPLATE
height_link CLASS::get_height(const hash_digest& key) const NOEXCEPT
{
    table::header::get_height header{};
    if (!store_.header.find(key, header))
        return {};

    return header.height;
}

TEMPLATE
height_link CLASS::get_height(const header_link& link) const NOEXCEPT
{
    table::header::get_height header{};
    if (!store_.header.get(link, header))
        return {};

    return header.height;
}

TEMPLATE
bool CLASS::get_height(size_t& out, const hash_digest& key) const NOEXCEPT
{
    const auto height = get_height(key);
    if (height >= height_link::terminal)
        return false;

    out = system::possible_narrow_cast<size_t>(height.value);
    return true;
}

TEMPLATE
bool CLASS::get_height(size_t& out, const header_link& link) const NOEXCEPT
{
    const auto height = get_height(link);
    if (height >= height_link::terminal)
        return false;

    out = system::possible_narrow_cast<size_t>(height.value);
    return true;
}

// association
// ----------------------------------------------------------------------------
// Empty/null_hash implies fault, zero count implies unassociated.

TEMPLATE
hashes CLASS::get_tx_keys(const header_link& link) const NOEXCEPT
{
    const auto tx_fks = to_transactions(link);
    if (tx_fks.empty())
        return {};

    // Overallocate as required for the common merkle scenario.
    const auto count = tx_fks.size();
    const auto size = is_odd(count) && !is_one(count) ? add1(count) : count;

    system::hashes hashes{};
    hashes.reserve(size);
    for (const auto& tx_fk: tx_fks)
        hashes.push_back(get_tx_key(tx_fk));

    // Return of any null_hash implies failure.
    return hashes;
}

TEMPLATE
hashes CLASS::get_wtxids(const header_link& link) const NOEXCEPT
{
    const auto tx_fks = to_transactions(link);
    if (tx_fks.empty())
        return {};

    system::hashes hashes(tx_fks.size());
    std::transform(tx_fks.begin(), tx_fks.end(), hashes.begin(),
        [this](const auto& tx_fk) NOEXCEPT
        {
            return this->get_wtxid(tx_fk);
        });

    // Return of any null_hash implies failure.
    return hashes;
}

// The coinbase commits to the witness root, otherwise the block is
// unsegregated and its witness root is the header merkle root.
TEMPLATE
bool CLASS::is_witness_committed(const hash_digest& witness_root,
    const header_link& link) const NOEXCEPT
{
    using namespace system;
    const auto coinbase = get_transaction(to_coinbase(link), true);
    if (!coinbase)
        return false;

    hash_cref commitment{ null_hash };
    hash_cref reservation{ null_hash };
    if (!coinbase->get_witness_commitment(commitment))
    {
        const auto header = get_header(link);
        return header && (header->merkle_root() == witness_root);
    }

    return coinbase->get_witness_reservation(reservation) &&
        (bitcoin_hash(witness_root, reservation) == commitment.get());
}

// The pool id columns hold the witness hash of a pooled tx, otherwise (such as
// for any coinbase) the witness hash is computed from the stored tx.
TEMPLATE
hash_digest CLASS::get_wtxid(const tx_link& link) const NOEXCEPT
{
    using namespace system;
    if (store_.pool.enabled())
    {
        if (const auto row = store_.pool.first(link); !row.is_terminal())
        {
            table::pool_word id0{}, id1{}, id2{}, id3{};
            if (!store_.pool.id0.get(row, id0) ||
                !store_.pool.id1.get(row, id1) ||
                !store_.pool.id2.get(row, id2) ||
                !store_.pool.id3.get(row, id3))
                return {};

            using lane_t = schema::pool::witness_lane;
            using lanes_t = std_array<lane_t, schema::pool::witness_lanes>;
            const auto words = to_little_endians(lanes_t
            {
                id0.word, id1.word, id2.word, id3.word
            });

            return array_cast<uint8_t>(words);
        }
    }

    const auto tx = get_transaction(link, true);
    return tx ? tx->hash(true) : null_hash;
}

TEMPLATE
size_t CLASS::get_tx_count(const header_link& link) const NOEXCEPT
{
    table::txs::get_tx_count txs{};
    if (!store_.txs.at(to_txs(link), txs))
        return {};

    return txs.number;
}

// TODO: optimize with stored aggregate.
TEMPLATE
size_t CLASS::get_branch_tx_count(const header_link& link) const NOEXCEPT
{
    size_t count{};
    auto parent = link;

    while (!parent.is_terminal())
    {
        count += get_tx_count(parent);
        parent = to_parent(parent);
    }

    return count;
}

TEMPLATE
tx_link CLASS::get_position_tx(const header_link& link,
    size_t position) const NOEXCEPT
{
    table::txs::get_at_position txs{ {}, position };
    if (!store_.txs.at(to_txs(link), txs))
        return {};

    return txs.tx_fk;
}

} // namespace database
} // namespace libbitcoin

#endif
