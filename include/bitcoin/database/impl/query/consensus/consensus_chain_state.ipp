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
#ifndef LIBBITCOIN_DATABASE_CONSENSUS_CHAIN_STATE_IPP
#define LIBBITCOIN_DATABASE_CONSENSUS_CHAIN_STATE_IPP

#include <ranges>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

// Chain state (any blocks).
// ----------------------------------------------------------------------------

TEMPLATE
bool CLASS::populate_bits(chain_state::data& data,
    const chain_state::map& map, header_link link) const NOEXCEPT
{
    if (!get_bits(data.bits.self, link))
        return false;

    // Navigate from the subject (data.height) to the map's high block.
    for (auto height = data.height; height > map.bits.high; --height)
        link = to_parent(link);

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

    // Navigate from the subject (data.height) to the map's high block.
    for (auto height = data.height; height > map.version.high; --height)
        link = to_parent(link);

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

    // Navigate from the subject (data.height) to the map's high block.
    for (auto height = data.height; height > map.timestamp.high; --height)
        link = to_parent(link);

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
bool CLASS::populate_signal(chain_state::data::signal& signal,
    const chain_state::map::signal& map, size_t height,
    const header_links& branch) const NOEXCEPT
{
    if (map.high == chain_state::map::unrequested)
        return true;

    // Branch links are above the candidate chain, by descending height.
    const auto fork = height - branch.size();
    const auto to_link = [&](size_t at) NOEXCEPT
    {
        BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
        return at > fork ? header_link{ branch.at(height - at) } :
            to_candidate(at);
        BC_POP_WARNING()
    };

    // Median time past of a period boundary (zero at genesis).
    const auto median = [&](uint32_t& out, size_t boundary) NOEXCEPT
    {
        chain_state::timestamps times(std::min(boundary,
            system::chain::median_time_past_interval));

        for (auto& time: times)
            if (!get_timestamp(time, to_link(--boundary)))
                return false;

        out = chain_state::median(times);
        return true;
    };

    // Signaling blocks of heights [from, to).
    const auto count = [&](size_t& out, size_t from, size_t to) NOEXCEPT
    {
        uint32_t version{};
        for (out = zero; from < to; ++from)
        {
            if (!get_version(version, to_link(from)))
                return false;

            if (chain_state::is_signal(version, map.base, map.bit))
                ++out;
        }

        return true;
    };

    // Periods below a boundary with median time past below start are defined.
    // A period is counted only if its opening boundary is at/above start and
    // the boundary below that is below timeout, as otherwise it is not started.
    auto boundary = map.high;
    uint32_t current{};
    if (!median(current, boundary))
        return false;

    if (is_zero(boundary) || current < map.start)
        return true;

    uint32_t below{};
    if (!median(below, boundary - map.period))
        return false;

    if (below < map.timeout && !count(signal.count, boundary, height))
        return false;

    for (;;)
    {
        const auto opening = boundary - map.period;

        size_t value{};
        uint32_t lower{};
        if (!is_zero(opening) && below >= map.start)
        {
            if (!median(lower, opening - map.period))
                return false;

            if (lower < map.timeout && !count(value, opening, boundary))
                return false;
        }

        BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
        signal.ordered.push_back({ current, value });
        BC_POP_WARNING()

        if (is_zero(opening) || below < map.start)
            return true;

        boundary = opening;
        current = below;
        below = lower;
    }
}

TEMPLATE
bool CLASS::populate_signals(chain_state::data& data,
    const chain_state::map& map, header_link link) const NOEXCEPT
{
    constexpr auto unrequested = chain_state::map::unrequested;
    const auto requested = [](const auto& signal) NOEXCEPT
    {
        return signal.high != unrequested;
    };

    if (std::none_of(map.signals.begin(), map.signals.end(), requested))
        return true;

    // Branch links above the candidate chain, by descending height.
    header_links branch{};
    while (!is_candidate_header(link))
    {
        if (link.is_terminal())
            return false;

        BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
        branch.push_back(link);
        BC_POP_WARNING()
        link = to_parent(link);
    }

    for (size_t index{}; index < map.signals.size(); ++index)
        if (!populate_signal(data.signals.at(index), map.signals.at(index),
            data.height, branch))
            return false;

    return true;
}

TEMPLATE
bool CLASS::populate_work(chain_state::data& data,
    header_link link) const NOEXCEPT
{
    return get_branch_work(data.cumulative_work, link);
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
        populate_signals(data, map, link) &&
        populate_work(data, link);
}

TEMPLATE
typename CLASS::chain_state_cptr CLASS::get_confirmed_chain_state(
    const system::settings& settings, const hash_digest& hash) const NOEXCEPT
{
    const auto link = to_header(hash);
    if (link.is_terminal())
        return nullptr;

    size_t height{};
    if (!get_height(height, link))
        return nullptr;

    return get_confirmed_chain_state(settings, link, height);
}

TEMPLATE
typename CLASS::chain_state_cptr CLASS::get_confirmed_chain_state(
    const system::settings& settings, const header_link& link,
    size_t height) const NOEXCEPT
{
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
    data.cumulative_work = zero;
    if (!is_zero(data.height) &&
        !get_branch_work(data.cumulative_work, to_candidate(sub1(data.height))))
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
        populate_hashes(data, map) &&
        populate_signals(data, map, link);
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
