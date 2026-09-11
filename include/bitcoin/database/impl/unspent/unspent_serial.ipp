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
#ifndef LIBBITCOIN_DATABASE_UNSPENT_UNSPENT_SERIAL_IPP
#define LIBBITCOIN_DATABASE_UNSPENT_UNSPENT_SERIAL_IPP

#include <atomic>
#include <semaphore>
#include <thread>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

TEMPLATE
CLASS::unspent_serial(const Store& store, const stopper& cancel,
    bool turbo) NOEXCEPT
  : reader_(store, cancel),
    spans_(store, cancel, turbo),
    store_(store),
    cancel_(cancel),
    turbo_(turbo)
{
}

// One folder thread for the run, each bucket handed off through 'ordered'
// (ready) and returned (done), so a fold overlaps the fill of the next bucket.
TEMPLATE
code CLASS::hash(unspent_totals& out, hash_digest& digest,
    const difference_set& set) const NOEXCEPT
{
    sizes offsets{};
    unspent_elements elements{};
    if (const auto ec = partition(elements, offsets, set))
        return ec;

    code folded{};
    std::atomic_bool stop{};
    auto previous = system::null_hash;
    system::stream::out::fast stream{ digest };
    system::hash::sha256x2::fast sink{ stream };
    unspent_coins coins{}, ordered{};
    output_links puts{}, links{};
    std::binary_semaphore ready{ 0 };
    std::binary_semaphore done{ 1 };
    std::thread folder
    {
        [&]() NOEXCEPT
        {
            for (ready.acquire(); !stop; ready.acquire())
            {
                folded = fold(out, sink, previous, ordered, links);
                done.release();
            }
        }
    };

    code ec{};
    sizes order{};
    for (size_t bucket{}; bucket < buckets; ++bucket)
    {
        const auto begin = offsets.at(bucket);
        const auto end = offsets.at(add1(bucket));
        ec = fill(coins, puts, elements, begin, end);
        done.acquire();
        if (!ec)
            ec = folded;

        if (ec)
            break;

        this->order(order, coins);
        gather(ordered, links, coins, puts, order);
        ready.release();
    }

    if (!ec)
    {
        done.acquire();
        ec = folded;
    }

    stop = true;
    ready.release();
    folder.join();
    if (ec)
        return ec;

    sink.flush();
    return error::success;
}

// Two passes over the set: count per chunk and bucket, then scatter.
TEMPLATE
code CLASS::partition(unspent_elements& out, sizes& offsets,
    const difference_set& set) const NOEXCEPT
{
    const auto chunks = std::max(one, std::min(set.words(), spans_.count()));
    const auto span = system::ceilinged_divide(set.words(), chunks);
    sizes counts(chunks * buckets, zero);
    sizes index(chunks);
    std::iota(index.begin(), index.end(), zero);
    const auto parallel = poolstl::execution::par_if(turbo_);
    std::atomic_bool fail{};

    std::for_each(parallel, index.begin(), index.end(),
        [&](size_t chunk) NOEXCEPT
        {
            if (fail)
                return;

            const auto begin = chunk * span;
            const auto end = std::min(set.words(), begin + span);
            if (walk(set, begin, end,
                [&](size_t slot, outs_link::integer) NOEXCEPT
                {
                    ++counts.at(chunk * buckets + slot);
                })) fail = true;
        });

    if (fail)
        return cancel_ ? error::query_canceled : error::integrity;

    starts(counts, offsets, chunks);
    out.resize(offsets.back());

    std::for_each(parallel, index.begin(), index.end(),
        [&](size_t chunk) NOEXCEPT
        {
            if (fail)
                return;

            const auto begin = chunk * span;
            const auto end = std::min(set.words(), begin + span);
            if (walk(set, begin, end,
                [&](size_t slot, outs_link::integer element) NOEXCEPT
                {
                    out.at(counts.at(chunk * buckets + slot)++) = element;
                })) fail = true;
        });

    if (fail)
        return cancel_ ? error::query_canceled : error::integrity;

    return error::success;
}

// emit(slot, element) for the set bits of the word range, in element order.
TEMPLATE
template <typename Emit>
code CLASS::walk(const difference_set& set, size_t begin, size_t end,
    const Emit& emit) const NOEXCEPT
{
    constexpr auto width = difference_set::word_bits;
    unspent_elements elements{};

    for (auto word = begin; word < end; ++word)
    {
        if (cancel_)
            return error::query_canceled;

        using namespace system;
        const auto base = word * width;
        for (auto bits = set.at(word); !is_zero(bits);
            bits = bit_and(bits, sub1(bits)))
        {
            const auto link = base + right_zeros(bits);
            elements.push_back(possible_narrow_cast<outs_link::integer>(link));
        }
    }

    sizes slots{};
    if (const auto ec = this->slots(slots, elements))
        return ec;

    for (size_t at{}; at < elements.size(); ++at)
        emit(slots.at(at), elements.at(at));

    return error::success;
}

TEMPLATE
code CLASS::slots(sizes& out,
    const unspent_elements& elements) const NOEXCEPT
{
    output_links puts(elements.size());
    if (const auto ec = reader_.read_puts(puts, zero, elements, zero,
        elements.size()))
        return ec;

    tx_links parents{};
    if (const auto ec = reader_.read_parents(parents, puts, zero, puts.size()))
        return ec;

    system::hashes hashes{};
    if (const auto ec = reader_.read_hashes(hashes, parents))
        return ec;

    out.resize(hashes.size());
    for (auto at = zero; at < hashes.size(); ++at)
        out.at(at) = hashes.at(at).front();

    return error::success;
}

// Bucket offsets, then chunk starts within each bucket (counts in place).
TEMPLATE
void CLASS::starts(sizes& counts, sizes& offsets, size_t chunks) NOEXCEPT
{
    offsets.assign(add1(buckets), zero);
    for (size_t chunk{}; chunk < chunks; ++chunk)
        for (size_t bucket{}; bucket < buckets; ++bucket)
            offsets.at(add1(bucket)) += counts.at(chunk * buckets + bucket);

    for (size_t bucket{}; bucket < buckets; ++bucket)
        offsets.at(add1(bucket)) += offsets.at(bucket);

    for (size_t bucket{}; bucket < buckets; ++bucket)
    {
        auto start = offsets.at(bucket);
        for (size_t chunk{}; chunk < chunks; ++chunk)
        {
            auto& slot = counts.at(chunk * buckets + bucket);
            const auto count = slot;
            slot = start;
            start += count;
        }
    }
}

TEMPLATE
code CLASS::fill(unspent_coins& out, output_links& puts,
    const unspent_elements& elements, size_t begin, size_t end) const NOEXCEPT
{
    const auto size = end - begin;
    out.resize(size);
    puts.resize(size);
    if (is_zero(size))
        return error::success;

    using namespace system;
    const auto parallel = poolstl::execution::par_if(turbo_);
    const auto chunks = std::max(one, std::min(size, spans_.count()));
    const auto span = ceilinged_divide(size, chunks);
    sizes index(ceilinged_divide(size, span));
    std::iota(index.begin(), index.end(), zero);
    std::atomic_bool fail{};

    std::for_each(parallel, index.begin(), index.end(),
        [&](size_t chunk) NOEXCEPT
        {
            if (fail)
                return;

            auto previous = tx_link::terminal;
            const auto first = chunk * span;
            const auto last = std::min(size, first + span);
            if (reader_.fill(out, puts, first, previous, elements,
                begin + first, begin + last))
                fail = true;
        });

    if (fail)
        return cancel_ ? error::query_canceled : error::integrity;

    return error::success;
}

TEMPLATE
void CLASS::order(sizes& out, const unspent_coins& coins) const NOEXCEPT
{
    out.resize(coins.size());
    std::iota(out.begin(), out.end(), zero);
    const auto parallel = poolstl::execution::par_if(turbo_);

    std::sort(parallel, out.begin(), out.end(),
        [&](size_t left, size_t right) NOEXCEPT
        {
            const auto& one = coins.at(left).out.point();
            const auto& two = coins.at(right).out.point();
            return (one.hash() == two.hash()) ?
                (one.index() < two.index()) :
                (one.hash() < two.hash());
        });
}

TEMPLATE
void CLASS::gather(unspent_coins& out, output_links& links,
    unspent_coins& coins, const output_links& puts,
    const sizes& order) const NOEXCEPT
{
    out.resize(coins.size());
    links.resize(coins.size());
    sizes index(coins.size());
    std::iota(index.begin(), index.end(), zero);
    const auto parallel = poolstl::execution::par_if(turbo_);

    std::for_each(parallel, index.begin(), index.end(),
        [&](size_t at) NOEXCEPT
        {
            out.at(at) = std::move(coins.at(order.at(at)));
            links.at(at) = puts.at(order.at(at));
        });
}

TEMPLATE
code CLASS::fold(unspent_totals& out, system::writer& sink,
    hash_digest& previous, unspent_coins& coins,
    const output_links& puts) const NOEXCEPT
{
    const auto ptr = store_.output.get_memory();
    for (size_t at{}; at < coins.size(); ++at)
    {
        if (cancel_)
            return error::query_canceled;

        auto& coin = coins.at(at);
        const auto& hash = coin.out.point().hash();
        coin.first = (hash != previous);
        previous = hash;

        unspent_writer::write(sink, coin);
        table::output::write_script tail{ {}, sink };
        if (!store_.output.raw(ptr, puts.at(at), tail))
            return error::integrity;

        unspent_writer::add(out, coin.first, tail.value, tail.length);
    }

    return error::success;
}

} // namespace database
} // namespace libbitcoin

#endif
