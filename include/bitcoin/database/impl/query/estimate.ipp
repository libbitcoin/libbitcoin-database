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
#ifndef LIBBITCOIN_DATABASE_QUERY_ESTIMATE_IPP
#define LIBBITCOIN_DATABASE_QUERY_ESTIMATE_IPP

#include <atomic>
#include <cmath>
#include <iterator>
#include <memory>
#include <numeric>
#include <ranges>
#include <utility>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {
    
// fee estimate
// ----------------------------------------------------------------------------

// protected
TEMPLATE
code CLASS::get_block_fees(fee_states& out,
    const header_link& link) const NOEXCEPT
{
    out.clear();
    const auto block = get_block(link, false);
    if (!block)
        return error::not_found;

    block->populate();
    if (!populate_without_metadata(*block))
        return error::missing_prevouts;

    const auto& txs = *block->transactions_ptr();
    if (txs.empty())
        return error::empty_block;

    out.reserve(txs.size());
    for (auto tx = std::next(txs.begin()); tx != txs.end(); ++tx)
        out.emplace_back((*tx)->virtual_size(), (*tx)->fee());

    return error::success;
}

// public
TEMPLATE
code CLASS::get_block_fees(std::atomic_bool& cancel, fee_state_sets& out,
    size_t top, size_t count) const NOEXCEPT
{
    out.clear();
    if (is_zero(count))
        return {};

    if (top > get_top_confirmed())
        return error::not_found;

    const auto start = top - sub1(count);
    if (system::is_subtract_overflow(top, sub1(count)))
        return error::invalid_argument;

    out.resize(count);
    std::vector<size_t> offsets(count);
    std::iota(offsets.begin(), offsets.end(), zero);
    std::atomic<error::error_t> failure{ error::success };
    constexpr auto relaxed = std::memory_order_relaxed;

    std::for_each(poolstl::execution::par, offsets.begin(), offsets.end(),
        [&](const size_t& offset) NOEXCEPT
        {
            if (failure.load(relaxed) != error::success)
                return;

            if (cancel.load(relaxed))
            {
                failure.store(error::query_canceled, relaxed);
                return;
            }

            const auto link = to_confirmed(start + offset);
            if (const auto ec = get_block_fees(out.at(offset), link))
            {
                failure.store(ec.value(), relaxed);
                return;
            }
        });

    code ec{ failure.load(relaxed) };
    if (ec) out.clear();
    return ec;
}

// TODO: move to server as its own class, fee::accumulator as member.
// ----------------------------------------------------------------------------
// static

TEMPLATE
uint64_t CLASS::estimate_fee(const fee::accumulator& fees, size_t target,
    fee::mode mode) NOEXCEPT
{
    auto estimate = max_uint64;
    constexpr auto large = fee::horizon::large;
    if (target >= large)
        return estimate;

    // Valid results are effectively limited to at least 1 sat/vb.
    // threshold_fee is thread safe but values are affected during update. 
    using namespace fee::confidence;
    switch (mode)
    {
        case fee::mode::basic:
        {
            estimate = compute_fee(fees, target, high);
            break;
        }
        case fee::mode::markov:
        {
            estimate = compute_fee(fees, target, high, true);
            break;
        }
        case fee::mode::economical:
        {
            const auto target1 = to_half(target);
            const auto target2 = std::min(one, target);
            const auto target3 = std::min(large, two * target);
            const auto fee1 = compute_fee(fees, target1, low);
            const auto fee2 = compute_fee(fees, target2, mid);
            const auto fee3 = compute_fee(fees, target3, high);
            estimate = std::max({ fee1, fee2, fee3 });
            break;
        }
        case fee::mode::conservative:
        {
            const auto target1 = to_half(target);
            const auto target2 = std::min(one, target);
            const auto target3 = std::min(large, two * target);
            const auto fee1 = compute_fee(fees, target1, low);
            const auto fee2 = compute_fee(fees, target2, mid);
            const auto fee3 = compute_fee(fees, target3, high);
            estimate = std::max({ fee1, fee2, fee3 });
            break;
        }
    }

    // max_uint64 is failure sentinel (and unachievable/invalid as a fee).
    return estimate;
}

TEMPLATE
bool CLASS::create_fees(fee::accumulator& out,
    const fee_state_sets& blocks) NOEXCEPT
{
    const auto count = blocks.size();
    if (is_zero(count))
        return true;

    const auto top = out.top_height;
    auto height = top - sub1(count);
    if (system::is_subtract_overflow(top, sub1(count)))
        return error::invalid_argument;

    // TODO: could be parallel.
    for (const auto& block: blocks)
        for (const auto& tx: block)
            if (!update_fees(out, tx, height++, true))
                return false;

    return true;
}

TEMPLATE
bool CLASS::pop_fees(fee::accumulator& fees, const fee_states& states) NOEXCEPT
{
    // Blocks must be pushed in order (but independent of chain index).
    const auto result = update_fees(fees, states, fees.top_height--, false);
    decay_fees(fees, false);
    return result;
}

TEMPLATE
bool CLASS::push_fees(fee::accumulator& fees, const fee_states& states) NOEXCEPT
{
    // Blocks must be pushed in order (but independent of chain index).
    decay_fees(fees, true);
    return update_fees(fees, states, ++fees.top_height, true);
}

// protected
// ----------------------------------------------------------------------------
// static

TEMPLATE
static double CLASS::to_scale_term(size_t age) NOEXCEPT
{
    const auto scale = std::pow(decay_rate(), age);
}

TEMPLATE
static double CLASS::to_scale_factor(bool push) NOEXCEPT
{
    const auto scale = std::pow(decay_rate(), push ? +1.0 : -1.0);
}

TEMPLATE
void CLASS::decay_fees(fee::accumulator& fees, bool push) NOEXCEPT
{
    // Not thread safe (use sequentially by block).
    const auto factor = to_scale_factor(push);
    decay_fees(fees.large, factor);
    decay_fees(fees.medium, factor);
    decay_fees(fees.small, factor);
}

TEMPLATE
void CLASS::decay_fees(auto& buckets, double factor) NOEXCEPT
{
    // Not thread safe (apply sequentially by block).
    const auto apply_decay = [factor](auto& count) NOEXCEPT
    {
        const auto relaxed = std::memory_order_relaxed;
        const auto value = count.load(relaxed);
        count.store(system::to_floored_integer(value * factor), relaxed);
    };

    for (auto& bucket: buckets)
    {
        apply_decay(bucket.total);
        for (auto& count: bucket.confirmed)
            apply_decay(count);
    }
}

TEMPLATE
uint64_t CLASS::compute_fee(const fee::accumulator& fees, size_t target,
    double confidence, bool use_markov) NOEXCEPT
{
    const auto threshold = [&](double part, double total) NOEXCEPT
    {
        return part / total;
    };

    // Geometric distribution approximation, not a full Markov process.
    const auto markov = [&](double part, double total) NOEXCEPT
    {
        return system::power(part / total, target);
    };

    const auto call = [&](const auto& buckets) NOEXCEPT
    {
        using namespace system;
        constexpr auto magic_number = 2u;
        constexpr auto relaxed = std::memory_order_relaxed;
        const auto at_least_four = magic_number * add1(target);
        const auto& contribution = use_markov ? markov : threshold;

        double total{}, part{};
        auto index = buckets.size();
        auto found = index;
        for (const auto& bucket: std::views::reverse(buckets))
        {
            --index;
            total += to_floating(bucket.total.load(relaxed));
            part += to_floating(bucket.confirmed[target].load(relaxed));
            if (total < at_least_four)
                continue;

            if (contribution(part, total) > (1.0 - confidence))
                break;

            found = index;
        }

        if (found == buckets.size())
            return max_uint64;

        const auto minimum = fee::size::min * std::pow(fee::size::step, found);
        return to_ceilinged_integer<uint64_t>(minimum);
    };

    if (target < fee::horizon::small)  return call(fees.small);
    if (target < fee::horizon::medium) return call(fees.medium);
    if (target < fee::horizon::large)  return call(fees.large);
    return max_uint64;
}

TEMPLATE
bool CLASS::update_fees(fee::accumulator& fees, const fee_states& states,
    size_t height, bool push) NOEXCEPT
{
    using namespace system;
    if (( push && (height == max_size_t)) ||
        (!push && is_zero(height)) ||
        (height > fees.top_height))
        return false;

    // std::log (replace static with constexpr in c++26).
    static const auto growth = std::log(fee::size::step);
    std::array<size_t, fee::size::count> counts{};

    for (const auto& state: states)
    {
        if (is_zero(state.bytes))
            return false;

        if (is_zero(state.fee))
            continue;

        const auto rate = to_floating(state.fee) / state.bytes;
        if (rate < fee::size::min)
            continue;

        // Clamp overflow to last bin.
        const auto bin = std::log(rate / fee::size::min) / growth;
        ++counts[std::min(to_floored_integer(bin), sub1(fee::size::count))];
    }

    const auto age = fees.top_height - height;
    const auto scale = to_scale_term(age);
    const auto update = [age, scale](const auto& buckets) NOEXCEPT
    {
        // The array count of the buckets element type.
        const auto depth = buckets.front().confirmed.size();

        size_t bin{};
        for (const auto count: counts)
        {
            if (is_zero(count))
            {
                ++bin;
                continue;
            }

            auto& bucket = buckets[bin++];
            const auto scaled = to_floored_integer(count * scale);
            const auto signed_term = push ? scaled : twos_complement(scaled);
            constexpr auto relaxed = std::memory_order_relaxed;
            bucket.total.fetch_add(signed_term, relaxed);
            for (auto target = age; target < depth; ++target)
                bucket.confirmed[target].fetch_add(signed_term, relaxed);
        }
    };

    update(fees.large);
    update(fees.medium);
    update(fees.small);
    return true;
}

} // namespace database
} // namespace libbitcoin

#endif
