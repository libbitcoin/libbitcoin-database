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
#ifndef LIBBITCOIN_DATABASE_UNSPENT_UNSPENT_SPANS_IPP
#define LIBBITCOIN_DATABASE_UNSPENT_UNSPENT_SPANS_IPP

#include <atomic>
#include <thread>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/memory.hpp>

namespace libbitcoin {
namespace database {

TEMPLATE
CLASS::unspent_spans(const Store& store, const stopper& cancel,
    bool turbo) NOEXCEPT
  : store_(store), cancel_(cancel), turbo_(turbo)
{
}

TEMPLATE
size_t CLASS::count() const NOEXCEPT
{
    return cores() * per_thread;
}

TEMPLATE
template <typename Span>
code CLASS::for_each(const difference_set& set, const Span& span) const NOEXCEPT
{
    using namespace system;
    std_vector<size_t> bounds{};
    if (const auto ec = this->bounds(bounds, set))
        return ec;

    // Static chunking is contiguous, so spans are interleaved across threads
    // (unspent density rises with height, contiguous lanes are unbalanced).
    const auto cuts = sub1(bounds.size());
    const auto width = cores();
    std_vector<size_t> index{};
    index.reserve(cuts);
    for (size_t lane{}; lane < width; ++lane)
        for (auto at = lane; at < cuts; at += width)
            index.push_back(at);

    std::atomic_bool fail{};
    const auto parallel = poolstl::execution::par_if(turbo_);

    std::for_each(parallel, index.cbegin(), index.cend(),
        [&](size_t chunk) NOEXCEPT
        {
            if (fail)
                return;

            if (span(chunk, bounds.at(chunk), bounds.at(add1(chunk))))
                fail = true;
        });

    if (fail)
        return cancel_ ? error::query_canceled : error::integrity;

    return error::success;
}

TEMPLATE
bool CLASS::to_tx_base(size_t& out, size_t element) const NOEXCEPT
{
    using namespace system;
    const outs_link link{ possible_narrow_cast<outs_link::integer>(element) };

    table::outs::get_output outs{};
    if (!store_.outs.puts.get(link, outs))
        return false;

    table::output::get_parent output{};
    if (!store_.output.get(outs.out_fk, output))
        return false;

    table::transaction::get_output tx{};
    if (!store_.tx.get(output.parent_fk, tx))
        return false;

    out = tx.outs_fk;
    return true;
}

// Bounds are snapped to tx boundaries, as tx count is by distinct tx.
TEMPLATE
code CLASS::bounds(std_vector<size_t>& out,
    const difference_set& set) const NOEXCEPT
{
    BC_ASSERT(out.empty());
    out.push_back(zero);
    if (is_zero(set.size()))
    {
        out.push_back(zero);
        return error::success;
    }

    using namespace system;
    const auto chunks = std::min(set.size(), count());
    const auto stride = ceilinged_divide(set.size(), chunks);
    out.reserve(add1(chunks));

    for (auto chunk = one; chunk < chunks; ++chunk)
    {
        const auto element = chunk * stride;
        if (element >= set.size())
            break;

        size_t at{};
        if (!to_tx_base(at, element))
            return error::integrity;

        if (at > out.back())
            out.push_back(at);
    }

    out.push_back(set.size());
    return error::success;
}

} // namespace database
} // namespace libbitcoin

#endif
