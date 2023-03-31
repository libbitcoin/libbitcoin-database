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

#include <memory>
#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

// This value should never be read, but may be useful in debugging.
static constexpr uint32_t unspecified_timestamp = max_uint32;

// Chain state (candidate).
// ----------------------------------------------------------------------------

TEMPLATE
bool CLASS::get_candidate_bits(uint32_t& bits, size_t height,
    const header& header, size_t header_height) const NOEXCEPT
{
    if (height == header_height)
    {
        bits = header.bits();
        return true;
    }

    return get_bits(bits, to_candidate(height));
}

TEMPLATE
bool CLASS::get_candidate_version(uint32_t& version, size_t height,
    const header& header, size_t header_height) const NOEXCEPT
{
    if (height == header_height)
    {
        version = header.version();
        return true;
    }

    return get_version(version, to_candidate(height));
}

TEMPLATE
bool CLASS::get_candidate_timestamp(uint32_t& time, size_t height,
    const header& header, size_t header_height) const NOEXCEPT
{
    if (height == header_height)
    {
        time = header.timestamp();
        return true;
    }

    return get_timestamp(time, to_candidate(height));
}

TEMPLATE
bool CLASS::populate_candidate_bits(chain_state::data& data,
    const chain_state::map& map, const header& header,
    size_t header_height) const NOEXCEPT
{
    auto& bits = data.bits.ordered;
    bits.resize(map.bits.count);
    auto height = map.bits.high - map.bits.count;

    for (auto& bit: bits)
        if (!get_candidate_bits(bit, ++height, header, header_height))
            return false;

    return get_candidate_bits(data.bits.self, map.bits_self, header,
        header_height);
}

TEMPLATE
bool CLASS::populate_candidate_versions(chain_state::data& data,
    const chain_state::map& map, const header& header,
    size_t header_height) const NOEXCEPT
{
    auto& versions = data.version.ordered;
    versions.resize(map.version.count);
    auto height = map.version.high - map.version.count;

    for (auto& version: versions)
        if (!get_candidate_version(version, ++height, header, header_height))
            return false;

    return get_candidate_version(data.version.self, map.version_self, header,
        header_height);
}

TEMPLATE
bool CLASS::populate_candidate_timestamps(chain_state::data& data,
    const chain_state::map& map, const header& header,
    size_t header_height) const NOEXCEPT
{
    data.timestamp.retarget = unspecified_timestamp;
    auto& timestamps = data.timestamp.ordered;
    timestamps.resize(map.timestamp.count);
    auto height = map.timestamp.high - map.timestamp.count;

    for (auto& timestamp: timestamps)
        if (!get_candidate_timestamp(timestamp, ++height, header, header_height))
            return false;

    // Retarget is required if timestamp_retarget is not unrequested.
    if (map.timestamp_retarget != chain_state::map::unrequested &&
        !get_candidate_timestamp(data.timestamp.retarget,
            map.timestamp_retarget, header, header_height))
    {
        return false;
    }

    return get_candidate_timestamp(data.timestamp.self, map.timestamp_self,
        header, header_height);
}

TEMPLATE
bool CLASS::populate_candidate_all(chain_state::data& data,
    const system::settings& settings, const header& header,
    size_t height) const NOEXCEPT
{
    // Construct the map to inform chain state data population, set hash.
    const auto map = chain_state::get_map(data.height, settings);
    const auto link = to_candidate(data.height);
    data.hash = get_header_key(link);

    return !link.is_terminal() &&
        populate_candidate_bits(data, map, header, height) &&
        populate_candidate_versions(data, map, header, height) &&
        populate_candidate_timestamps(data, map, header, height);
}

TEMPLATE
typename CLASS::chain_state_ptr CLASS::get_candidate_chain_state(
    const system::settings& settings) const NOEXCEPT
{
    return get_candidate_chain_state(settings, get_top_candidate());
}

TEMPLATE
typename CLASS::chain_state_ptr CLASS::get_candidate_chain_state(
    const system::settings& settings, size_t height) const NOEXCEPT
{
    const auto header = get_header(to_candidate(height));
    return header ? get_candidate_chain_state(settings, *header, height) :
        nullptr;
}

TEMPLATE
typename CLASS::chain_state_ptr CLASS::get_candidate_chain_state(
    const system::settings& settings, const header& header,
    size_t height) const NOEXCEPT
{
    chain_state::data data{ height };
    if (!populate_candidate_all(data, settings, header, height))
        return nullptr;

    return std::make_shared<chain_state>(std::move(data), settings);
}

// Chain state (confirmed).
// ----------------------------------------------------------------------------

TEMPLATE
bool CLASS::get_confirmed_bits(uint32_t& bits, size_t height,
    const header& header, size_t header_height) const NOEXCEPT
{
    if (height == header_height)
    {
        bits = header.bits();
        return true;
    }

    return get_bits(bits, to_confirmed(height));
}

TEMPLATE
bool CLASS::get_confirmed_version(uint32_t& version, size_t height,
    const header& header, size_t header_height) const NOEXCEPT
{
    if (height == header_height)
    {
        version = header.version();
        return true;
    }

    return get_version(version, to_confirmed(height));
}

TEMPLATE
bool CLASS::get_confirmed_timestamp(uint32_t& time, size_t height,
    const header& header, size_t header_height) const NOEXCEPT
{
    if (height == header_height)
    {
        time = header.timestamp();
        return true;
    }

    return get_timestamp(time, to_confirmed(height));
}

TEMPLATE
bool CLASS::populate_confirmed_bits(chain_state::data& data,
    const chain_state::map& map, const header& header,
    size_t header_height) const NOEXCEPT
{
    auto& bits = data.bits.ordered;
    bits.resize(map.bits.count);
    auto height = map.bits.high - map.bits.count;

    for (auto& bit: bits)
        if (!get_confirmed_bits(bit, ++height, header, header_height))
            return false;

    return get_confirmed_bits(data.bits.self, map.bits_self, header,
        header_height);
}

TEMPLATE
bool CLASS::populate_confirmed_versions(chain_state::data& data,
    const chain_state::map& map, const header& header,
    size_t header_height) const NOEXCEPT
{
    auto& versions = data.version.ordered;
    versions.resize(map.version.count);
    auto height = map.version.high - map.version.count;

    for (auto& version: versions)
        if (!get_confirmed_version(version, ++height, header, header_height))
            return false;

    return get_confirmed_version(data.version.self, map.version_self, header,
        header_height);
}

TEMPLATE
bool CLASS::populate_confirmed_timestamps(chain_state::data& data,
    const chain_state::map& map, const header& header,
    size_t header_height) const NOEXCEPT
{
    data.timestamp.retarget = unspecified_timestamp;
    auto& timestamps = data.timestamp.ordered;
    timestamps.resize(map.timestamp.count);
    auto height = map.timestamp.high - map.timestamp.count;

    for (auto& timestamp: timestamps)
        if (!get_confirmed_timestamp(timestamp, ++height, header,
            header_height))
            return false;

    // Retarget is required if timestamp_retarget is not unrequested.
    if (map.timestamp_retarget != chain_state::map::unrequested &&
        !get_confirmed_timestamp(data.timestamp.retarget,
            map.timestamp_retarget, header, header_height))
    {
        return false;
    }

    return get_confirmed_timestamp(data.timestamp.self, map.timestamp_self,
        header, header_height);
}

TEMPLATE
bool CLASS::populate_confirmed_all(chain_state::data& data,
    const system::settings& settings, const header& header,
    size_t height) const NOEXCEPT
{
    // Construct the map to inform chain state data population, set hash.
    const auto map = chain_state::get_map(data.height, settings);
    const auto link = to_confirmed(data.height);
    data.hash = get_header_key(link);

    return !link.is_terminal() &&
        populate_confirmed_bits(data, map, header, height) &&
        populate_confirmed_versions(data, map, header, height) &&
        populate_confirmed_timestamps(data, map, header, height);
}

TEMPLATE
typename CLASS::chain_state_ptr CLASS::get_confirmed_chain_state(
    const system::settings& settings) const NOEXCEPT
{
    return get_confirmed_chain_state(settings, get_top_confirmed());
}

TEMPLATE
typename CLASS::chain_state_ptr CLASS::get_confirmed_chain_state(
    const system::settings& settings, size_t height) const NOEXCEPT
{
    const auto header = get_header(to_confirmed(height));
    return header ? get_confirmed_chain_state(settings, *header, height) :
        nullptr;
}

TEMPLATE
typename CLASS::chain_state_ptr CLASS::get_confirmed_chain_state(
    const system::settings& settings, const header& header,
    size_t height) const NOEXCEPT
{
    chain_state::data data{ height };
    if (!populate_confirmed_all(data, settings, header, height))
        return nullptr;

    return std::make_shared<chain_state>(std::move(data), settings);
}

} // namespace database
} // namespace libbitcoin

#endif
