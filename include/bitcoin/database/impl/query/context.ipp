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
#ifndef LIBBITCOIN_DATABASE_QUERY_CONTEXT_IPP
#define LIBBITCOIN_DATABASE_QUERY_CONTEXT_IPP

#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

// This value should never be read, but may be useful in debugging.
static constexpr uint32_t unspecified_timestamp = max_uint32;

// Chain state.
// ----------------------------------------------------------------------------

TEMPLATE
CLASS::chain_state_ptr CLASS::get_chain_state(size_t height) const NOEXCEPT
{
    // TODO:
    return {};
}

TEMPLATE
bool CLASS::get_bits(uint32_t& bits, size_t height, const header& header,
    size_t header_height, bool candidate) const NOEXCEPT
{
    if (height == header_height)
    {
        bits = header.bits();
        return true;
    }

    return {}; ////fast_chain_.get_bits(bits, height, candidate);
}

TEMPLATE
bool CLASS::get_version(uint32_t& version, size_t height,
    const header& header, size_t header_height, bool candidate) const NOEXCEPT
{
    if (height == header_height)
    {
        version = header.version();
        return true;
    }

    return {}; ////fast_chain_.get_version(version, height, candidate);
}

TEMPLATE
bool CLASS::get_timestamp(uint32_t& time, size_t height,
    const header& header, size_t header_height, bool candidate) const NOEXCEPT
{
    if (height == header_height)
    {
        time = header.timestamp();
        return true;
    }

    return {}; ////fast_chain_.get_timestamp(time, height, candidate);
}

TEMPLATE
bool CLASS::get_block_hash(hash_digest& hash, size_t height,
    const header& header, size_t header_height, bool candidate) const
{
    if (height == header_height)
    {
        hash = header.hash();
        return true;
    }

    return {}; ////fast_chain_.get_block_hash(hash, height, candidate);
}

TEMPLATE
bool CLASS::populate_bits(chain_state::data& data, const chain_state::map& map,
    const header& header, size_t header_height, bool confirmed) const NOEXCEPT
{
    auto& bits = data.bits.ordered;
    bits.resize(map.bits.count);
    auto height = map.bits.high - map.bits.count;

    for (auto& bit: bits)
        if (!get_bits(bit, ++height, header, header_height, confirmed))
            return false;

    return get_bits(data.bits.self, map.bits_self,
        header, header_height, confirmed);
}

TEMPLATE
bool CLASS::populate_versions(chain_state::data& data,
    const chain_state::map& map, const header& header, size_t header_height,
    bool candidate) const NOEXCEPT
{
    auto& versions = data.version.ordered;
    versions.resize(map.version.count);
    auto height = map.version.high - map.version.count;

    for (auto& version: versions)
        if (!get_version(version, ++height, header, header_height, candidate))
            return false;

    return get_version(data.version.self, map.version_self, header,
        header_height, candidate);
}

TEMPLATE
bool CLASS::populate_timestamps(chain_state::data& data,
    const chain_state::map& map, const header& header,
    size_t header_height, bool candidate) const NOEXCEPT
{
    data.timestamp.retarget = unspecified_timestamp;
    auto& timestamps = data.timestamp.ordered;
    timestamps.resize(map.timestamp.count);
    auto height = map.timestamp.high - map.timestamp.count;

    for (auto& timestamp: timestamps)
        if (!get_timestamp(timestamp, ++height,
            header, header_height, candidate))
            return false;

    // Retarget is required if timestamp_retarget is not unrequested.
    if (map.timestamp_retarget != chain_state::map::unrequested &&
        !get_timestamp(data.timestamp.retarget, map.timestamp_retarget,
            header, header_height, candidate))
    {
        return false;
    }

    return get_timestamp(data.timestamp.self, map.timestamp_self,
        header, header_height, candidate);
}

TEMPLATE
bool CLASS::populate_all(chain_state::data& data,
    const system::settings& settings, const header& header, size_t height,
    bool candidate) const NOEXCEPT
{
    // Construct the map to inform chain state data population.
    const auto map = chain_state::get_map(data.height, settings);

    return
        populate_bits(data, map, header, height, candidate) &&
        populate_versions(data, map, header, height, candidate) &&
        populate_timestamps(data, map, header, height, candidate);
}

// Populate chain state for the top block|header.
TEMPLATE
CLASS::chain_state_ptr CLASS::populate(bool candidate) const NOEXCEPT
{
    header header{};
    size_t header_height{};

    return {};
    ////return fast_chain_.get_top(header, header_height, candidate) ?
    ////    populate(header, header_height, candidate) : nullptr;
}

// Get chain state for the given block|header by height.
TEMPLATE
CLASS::chain_state_ptr CLASS::populate(size_t header_height,
    bool candidate) const NOEXCEPT
{
    header header{};

    return {};
    ////return fast_chain_.get_header(header, header_height, candidate) ?
    ////    populate(header, header_height, candidate) : nullptr;
}

// Get chain state for the given block|header.
// Only hash and height are queried from the current block/header.
TEMPLATE
CLASS::chain_state_ptr CLASS::populate(const header& header,
    size_t header_height, bool candidate) const NOEXCEPT
{
    chain_state::data data{};
    data.height = header_height;

    return {};
    ////return populate_all(data, header, header_height, candidate) ?
    ////    std::make_shared<chain_state>(std::move(data), settings_) :
    ////    nullptr;
}

} // namespace database
} // namespace libbitcoin

#endif
