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
#ifndef LIBBITCOIN_DATABASE_QUERY_CONTEXT_IPP
#define LIBBITCOIN_DATABASE_QUERY_CONTEXT_IPP

#include <ranges>
#include <memory>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

// This value should never be read, but may be useful in debugging.
static constexpr uint32_t unspecified_timestamp = max_uint32;

// Chain state (any blocks).
// ----------------------------------------------------------------------------

TEMPLATE
bool CLASS::populate_bits(chain_state::data& data,
    const chain_state::map& map, header_link link) const NOEXCEPT
{
    if (!get_bits(data.bits.self, link))
        return false;

    data.bits.ordered.resize(map.bits.count);
    for (auto& bit: std::views::reverse(data.bits.ordered))
    {
        if (!get_bits(bit, link)) return false;
        link = to_parent(link);
    }

    return true;
}

TEMPLATE
bool CLASS::populate_versions(chain_state::data& data,
    const chain_state::map& map, header_link link) const NOEXCEPT
{
    if (!get_version(data.version.self, link))
        return false;

    data.version.ordered.resize(map.version.count);
    for (auto& version: std::views::reverse(data.version.ordered))
    {
        if (!get_version(version, link)) return false;
        link = to_parent(link);
    }

    return true;
}

TEMPLATE
bool CLASS::populate_timestamps(chain_state::data& data,
    const chain_state::map& map, header_link link) const NOEXCEPT
{
    if (!get_timestamp(data.timestamp.self, link))
        return false;

    data.timestamp.ordered.resize(map.timestamp.count);
    for (auto& timestamp: std::views::reverse(data.timestamp.ordered))
    {
        if (!get_timestamp(timestamp, link)) return false;
        link = to_parent(link);
    }

    return true;
}

TEMPLATE
bool CLASS::populate_retarget(chain_state::data& data,
    const chain_state::map& map, header_link link) const NOEXCEPT
{
    if (map.timestamp_retarget == chain_state::map::unrequested)
    {
        data.timestamp.retarget = unspecified_timestamp;
        return true;
    }

    if (map.timestamp_retarget > data.height)
        return false;

    for (auto it = map.timestamp_retarget; it < data.height; ++it)
        link = to_parent(link);

    return get_timestamp(data.timestamp.retarget, link);
}

TEMPLATE
bool CLASS::populate_hashes(chain_state::data& data,
    const chain_state::map& map) const NOEXCEPT
{
    if (map.bip30_deactivate_height != chain_state::map::unrequested)
        data.bip30_deactivate_hash = get_header_key(
            to_candidate(map.bip30_deactivate_height));

    if (map.bip9_bit0_height != chain_state::map::unrequested)
        data.bip9_bit0_hash = get_header_key(
            to_candidate(map.bip9_bit0_height));

    if (map.bip9_bit1_height != chain_state::map::unrequested)
        data.bip9_bit1_hash = get_header_key(
            to_candidate(map.bip9_bit1_height));

    if (map.bip9_bit2_height != chain_state::map::unrequested)
        data.bip9_bit2_hash = get_header_key(
            to_candidate(map.bip9_bit2_height));

    return true;
}

TEMPLATE
bool CLASS::populate_work(chain_state::data& data,
    header_link link) const NOEXCEPT
{
    uint256_t work{};
    data.cumulative_work = work;

    // This may scan the entire chain.
    while (get_work(work, link))
    {
        data.cumulative_work += work;
        link = to_parent(link);

        // Genesis parent link is terminal.
        if (link.is_terminal())
            return true;
    }

    return false;
}

TEMPLATE
bool CLASS::populate_all(chain_state::data& data,
    const system::settings& settings, const header_link& link,
    size_t height) const NOEXCEPT
{
    // Construct the map to inform chain state data population.
    const auto map = chain_state::get_map(height, settings);
    data.hash = get_header_key(link);
    data.height = height;

    return !link.is_terminal() &&
        populate_bits(data, map, link) &&
        populate_versions(data, map, link) &&
        populate_timestamps(data, map, link) &&
        populate_retarget(data, map, link) &&
        populate_hashes(data, map) &&
        populate_work(data, link);
}

TEMPLATE
typename CLASS::chain_state_cptr CLASS::get_chain_state(
    const system::settings& settings, const hash_digest& hash) const NOEXCEPT
{
    const auto link = to_header(hash);
    if (link.is_terminal())
        return nullptr;

    size_t height{};
    if (!get_height(height, link))
        return nullptr;

    if (to_candidate(height) == link)
        return get_candidate_chain_state(settings, link, height);

    chain_state::data data{};
    if (!populate_all(data, settings, link, height))
        return nullptr;

    return system::to_shared<chain_state>(std::move(data), settings);
}

// Chain state (candidate blocks).
// ----------------------------------------------------------------------------

TEMPLATE
bool CLASS::populate_candidate_bits(chain_state::data& data,
    const chain_state::map& map, const header& header) const NOEXCEPT
{
    data.bits.ordered.resize(map.bits.count);
    auto height = map.bits.high - map.bits.count;

    for (auto& bit: data.bits.ordered)
        if (!get_bits(bit, to_candidate(++height)))
            return false;

    data.bits.self = header.bits();
    return true;
}

TEMPLATE
bool CLASS::populate_candidate_versions(chain_state::data& data,
    const chain_state::map& map, const header& header) const NOEXCEPT
{
    data.version.ordered.resize(map.version.count);
    auto height = map.version.high - map.version.count;

    for (auto& version: data.version.ordered)
        if (!get_version(version, to_candidate(++height)))
            return false;

    data.version.self = header.version();
    return true;
}

TEMPLATE
bool CLASS::populate_candidate_timestamps(chain_state::data& data,
    const chain_state::map& map, const header& header) const NOEXCEPT
{
    data.timestamp.ordered.resize(map.timestamp.count);
    auto height = map.timestamp.high - map.timestamp.count;

    for (auto& timestamp: data.timestamp.ordered)
        if (!get_timestamp(timestamp, to_candidate(++height)))
            return false;

    data.timestamp.self = header.timestamp();
    return true;
}

TEMPLATE
bool CLASS::populate_candidate_retarget(chain_state::data& data,
    const chain_state::map& map, const header& header) const NOEXCEPT
{
    if (map.timestamp_retarget == chain_state::map::unrequested)
    {
        data.timestamp.retarget = unspecified_timestamp;
        return true;
    }

    if (map.timestamp_retarget == data.height)
    {
        data.timestamp.retarget = header.timestamp();
        return true;
    }

    return get_timestamp(data.timestamp.retarget,
        to_candidate(map.timestamp_retarget));
}

TEMPLATE
bool CLASS::populate_candidate_work(chain_state::data& data,
    const header& header) const NOEXCEPT
{
    uint256_t work{};
    data.cumulative_work = work;

    // This may scan the entire chain.
    for (auto height = zero; height < data.height; ++height)
        if (get_work(work, to_candidate(height)))
            data.cumulative_work += work;
        else
            return false;

    data.cumulative_work += header.proof();
    return true;
}

TEMPLATE
bool CLASS::populate_candidate_all(chain_state::data& data,
    const system::settings& settings, const header& header,
    const header_link& link, size_t height) const NOEXCEPT
{
    // Construct the map to inform chain state data population.
    const auto map = chain_state::get_map(height, settings);
    data.hash = get_header_key(link);
    data.height = height;

    return !link.is_terminal() &&
        populate_candidate_bits(data, map, header) &&
        populate_candidate_versions(data, map, header) &&
        populate_candidate_timestamps(data, map, header) &&
        populate_candidate_retarget(data, map, header) &&
        populate_candidate_work(data, header) &&
        populate_hashes(data, map);
}

TEMPLATE
typename CLASS::chain_state_cptr CLASS::get_candidate_chain_state(
    const system::settings& settings) const NOEXCEPT
{
    return get_candidate_chain_state(settings, get_top_candidate());
}

TEMPLATE
typename CLASS::chain_state_cptr CLASS::get_candidate_chain_state(
    const system::settings& settings, size_t height) const NOEXCEPT
{
    return get_candidate_chain_state(settings, to_candidate(height), height);
}

TEMPLATE
typename CLASS::chain_state_cptr CLASS::get_candidate_chain_state(
    const system::settings& settings, const header_link& link,
    size_t height) const NOEXCEPT
{
    const auto header = get_header(link);
    return header ? get_candidate_chain_state(settings, *header, link, height) :
        nullptr;
}

TEMPLATE
typename CLASS::chain_state_cptr CLASS::get_candidate_chain_state(
    const system::settings& settings, const header& header,
    const header_link& link, size_t height) const NOEXCEPT
{
    chain_state::data data{};
    if (!populate_candidate_all(data, settings, header, link, height))
        return nullptr;

    return std::make_shared<chain_state>(std::move(data), settings);
}

} // namespace database
} // namespace libbitcoin

#endif
