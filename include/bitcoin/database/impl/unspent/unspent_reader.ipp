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
#ifndef LIBBITCOIN_DATABASE_UNSPENT_UNSPENT_READER_IPP
#define LIBBITCOIN_DATABASE_UNSPENT_UNSPENT_READER_IPP

#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

TEMPLATE
CLASS::unspent_reader(const Store& store,
    const stopper& cancel) NOEXCEPT
  : store_(store), cancel_(cancel)
{
}

TEMPLATE
template <typename Flush>
code CLASS::elements(const difference_set& set, size_t begin, size_t end,
    const Flush& flush) const NOEXCEPT
{
    constexpr auto width = difference_set::word_bits;
    unspent_elements elements{};

    using namespace system;
    using word = difference_set::word;
    const auto last = ceilinged_divide(end, width);
    for (auto index = floored_divide(begin, width); index < last; ++index)
    {
        if (cancel_)
            return error::query_canceled;

        const auto base = index * width;
        auto bits = set.at(index);
        if (base < begin)
            bits = bit_and(bits, mask_right<word>(begin - base));

        if (base + width > end)
            bits = bit_and(bits, unmask_right<word>(end - base));

        for (; !is_zero(bits); bits = bit_and(bits, sub1(bits)))
        {
            const auto link = base + right_zeros(bits);
            elements.push_back(possible_narrow_cast<outs_link::integer>(link));
        }

        if (elements.size() >= batch_size)
        {
            if (const auto ec = flush(elements))
                return ec;

            elements.clear();
        }
    }

    return flush(elements);
}

TEMPLATE
template <typename Handler>
code CLASS::batch(const difference_set& set, size_t begin, size_t end,
    const Handler& handle) const NOEXCEPT
{
    output_links puts{};
    unspent_coins coins{};
    auto previous = tx_link::terminal;

    return elements(set, begin, end, [&](const unspent_elements& at) NOEXCEPT
    {
        puts.resize(at.size());
        coins.resize(at.size());
        auto ec = fill(coins, puts, zero, previous, at, zero, at.size());
        if (!ec)
            ec = read_scripts(coins, zero, puts, at.size());

        if (!ec)
            for (const auto& coin: coins)
                handle(coin);

        return ec;
    });
}

TEMPLATE
code CLASS::fill(unspent_coins& out, output_links& puts, size_t offset,
    tx_link::integer& previous, const unspent_elements& elements,
    size_t begin, size_t end) const NOEXCEPT
{
    if (const auto ec = read_puts(puts, offset, elements, begin, end))
        return ec;

    tx_links parents{};
    if (const auto ec = read_parents(parents, puts, offset, end - begin))
        return ec;

    if (const auto ec = read_transactions(out, offset, previous, parents,
        elements, begin))
        return ec;

    header_links blocks{};
    if (const auto ec = read_blocks(blocks, parents))
        return ec;

    std_vector<size_t> heights{};
    if (const auto ec = read_heights(heights, blocks))
        return ec;

    return confirm(out, offset, blocks, heights);
}

TEMPLATE
code CLASS::read_scripts(unspent_coins& out, size_t offset,
    const output_links& puts, size_t count) const NOEXCEPT
{
    const auto ptr = store_.output.get_memory();
    for (size_t at{}; at < count; ++at)
    {
        table::output::get_coin output{};
        if (!store_.output.get(ptr, puts.at(offset + at), output))
            return error::integrity;

        auto& coin = out.at(offset + at);
        coin.out = { coin.out.point(), output.value };
        std::swap(coin.script, output.script);
    }

    return error::success;
}

TEMPLATE
code CLASS::read_puts(output_links& out, size_t offset,
    const unspent_elements& elements, size_t begin, size_t end) const NOEXCEPT
{
    const auto count = end - begin;

    const auto ptr = store_.outs.puts.get_memory();
    for (size_t at{}; at < count; ++at)
    {
        table::outs::get_output output{};
        if (!store_.outs.puts.get(ptr, elements.at(begin + at), output))
            return error::integrity;

        out.at(offset + at) = output.out_fk;
    }

    return error::success;
}

TEMPLATE
code CLASS::read_parents(tx_links& out, const output_links& puts,
    size_t offset, size_t count) const NOEXCEPT
{
    out.resize(count);

    const auto ptr = store_.output.get_memory();
    for (size_t at{}; at < count; ++at)
    {
        table::output::get_parent output{};
        if (!store_.output.get(ptr, puts.at(offset + at), output))
            return error::integrity;

        out.at(at) = output.parent_fk;
    }

    return error::success;
}

TEMPLATE
code CLASS::read_hashes(system::hashes& out,
    const tx_links& parents) const NOEXCEPT
{
    out.resize(parents.size());

    const auto ptr = store_.tx.get_memory();
    for (size_t at{}; at < parents.size(); ++at)
    {
        table::transaction::get_hash tx{};
        if (!store_.tx.get(ptr, parents.at(at), tx))
            return error::integrity;

        out.at(at) = tx.hash;
    }

    return error::success;
}

TEMPLATE
code CLASS::read_transactions(unspent_coins& out, size_t offset,
    tx_link::integer& previous, const tx_links& parents,
    const unspent_elements& elements, size_t begin) const NOEXCEPT
{
    auto parent = tx_link::terminal;
    table::transaction::get_puts tx{};
    table::transaction::get_hash hash{};

    const auto ptr = store_.tx.get_memory();
    for (size_t at{}; at < parents.size(); ++at)
    {
        if (parents.at(at) != parent)
        {
            parent = parents.at(at);
            if (!store_.tx.get(ptr, parent, tx) ||
                !store_.tx.get(ptr, parent, hash))
                return error::integrity;
        }

        auto& coin = out.at(offset + at);
        const auto index = elements.at(begin + at) - tx.outs_fk;
        coin.first = (parent != previous);
        coin.coinbase = tx.coinbase;
        coin.out = { { hash.hash, index }, coin.out.value() };
        previous = parent;
    }

    return error::success;
}

// The branch is inactive if a tx of it is no longer strong (reorganized).
TEMPLATE
code CLASS::read_blocks(header_links& out,
    const tx_links& parents) const NOEXCEPT
{
    tx_link parent{};
    auto block = header_link::terminal;
    out.resize(parents.size());

    const auto ptr = store_.strong_tx.get_memory();
    for (size_t at{}; at < parents.size(); ++at)
    {
        if (parents.at(at) != parent)
        {
            parent = parents.at(at);
            const auto fk = store_.strong_tx.first(ptr, parent);

            table::strong_tx::record strong{};
            if (!store_.strong_tx.get(ptr, fk, strong) || !strong.positive())
                return error::branch_inactive;

            block = strong.header_fk();
        }

        out.at(at) = block;
    }

    return error::success;
}

TEMPLATE
code CLASS::read_heights(std_vector<size_t>& out,
    const header_links& blocks) const NOEXCEPT
{
    size_t height{};
    header_link block{};
    out.resize(blocks.size());

    const auto ptr = store_.header.get_memory();
    for (size_t at{}; at < blocks.size(); ++at)
    {
        if (blocks.at(at) != block)
        {
            block = blocks.at(at);
            table::header::get_height get_height{};
            if (!store_.header.get(ptr, block, get_height))
                return error::integrity;

            height = get_height.height;
        }

        out.at(at) = height;
    }

    return error::success;
}

// bitcoind retains duplicated coinbases at the overwriting heights (bip30
// exceptions), the store at the originals.
TEMPLATE
code CLASS::confirm(unspent_coins& out, size_t offset,
    const header_links& blocks, const std_vector<size_t>& heights) const NOEXCEPT
{
    header_link block{};
    const auto fork = store_.get_envelope().forks.bip30;

    const auto ptr = store_.confirmed.get_memory();
    for (size_t at{}; at < blocks.size(); ++at)
    {
        if (blocks.at(at) != block)
        {
            block = blocks.at(at);
            if (store_.confirmed.at(ptr, heights.at(at)) != block)
                return error::integrity;
        }

        auto& coin = out.at(offset + at);
        coin.height = heights.at(at);
        if (!fork || !coin.coinbase)
            continue;

        if (coin.height == bip30::first_original)
            coin.height = bip30::first_exception;
        else if (coin.height == bip30::second_original)
            coin.height = bip30::second_exception;
    }

    return error::success;
}

} // namespace database
} // namespace libbitcoin

#endif
