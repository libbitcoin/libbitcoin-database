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
#ifndef LIBBITCOIN_DATABASE_QUERY_EXTENT_IPP
#define LIBBITCOIN_DATABASE_QUERY_EXTENT_IPP

#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

// Table logical byte sizes (archive bodies).
// ----------------------------------------------------------------------------

TEMPLATE
size_t CLASS::archive_size() const NOEXCEPT
{
    return
        header_size() +
        output_size() +
        input_size() +
        point_size() +
        puts_size() +
        spend_size() +
        txs_size() +
        tx_size();
}

TEMPLATE
size_t CLASS::header_size() const NOEXCEPT
{
    return store_.header.body_size();
}

TEMPLATE
size_t CLASS::output_size() const NOEXCEPT
{
    return store_.output.body_size();
}

TEMPLATE
size_t CLASS::input_size() const NOEXCEPT
{
    return store_.input.body_size();
}

TEMPLATE
size_t CLASS::point_size() const NOEXCEPT
{
    return store_.point.body_size();
}

TEMPLATE
size_t CLASS::puts_size() const NOEXCEPT
{
    return store_.puts.body_size();
}

TEMPLATE
size_t CLASS::spend_size() const NOEXCEPT
{
    return store_.spend.body_size();
}

TEMPLATE
size_t CLASS::txs_size() const NOEXCEPT
{
    return store_.txs.body_size();
}

TEMPLATE
size_t CLASS::tx_size() const NOEXCEPT
{
    return store_.tx.body_size();
}

// Table logical byte sizes (metadata bodies).
// ----------------------------------------------------------------------------

TEMPLATE
size_t CLASS::candidate_size() const NOEXCEPT
{
    return store_.candidate.body_size();
}

TEMPLATE
size_t CLASS::confirmed_size() const NOEXCEPT
{
    return store_.confirmed.body_size();
}

TEMPLATE
size_t CLASS::strong_tx_size() const NOEXCEPT
{
    return store_.strong_tx.body_size();
}

TEMPLATE
size_t CLASS::validated_tx_size() const NOEXCEPT
{
    return store_.validated_tx.body_size();
}

TEMPLATE
size_t CLASS::validated_bk_size() const NOEXCEPT
{
    return store_.validated_bk.body_size();
}

// Buckets (archive hash tables).
// ----------------------------------------------------------------------------

TEMPLATE
size_t CLASS::header_buckets() const NOEXCEPT
{
    return store_.header.buckets();
}

TEMPLATE
size_t CLASS::point_buckets() const NOEXCEPT
{
    return store_.point.buckets();
}

TEMPLATE
size_t CLASS::spend_buckets() const NOEXCEPT
{
    return store_.spend.buckets();
}

TEMPLATE
size_t CLASS::txs_buckets() const NOEXCEPT
{
    return store_.txs.buckets();
}

TEMPLATE
size_t CLASS::tx_buckets() const NOEXCEPT
{
    return store_.tx.buckets();
}

// Buckets (metadata hash tables).
// ----------------------------------------------------------------------------

TEMPLATE
size_t CLASS::strong_tx_buckets() const NOEXCEPT
{
    return store_.strong_tx.buckets();
}

TEMPLATE
size_t CLASS::validated_tx_buckets() const NOEXCEPT
{
    return store_.validated_tx.buckets();
}

TEMPLATE
size_t CLASS::validated_bk_buckets() const NOEXCEPT
{
    return store_.validated_bk.buckets();
}

//  Counts (archive records).
// ----------------------------------------------------------------------------

TEMPLATE
size_t CLASS::header_records() const NOEXCEPT
{
    return store_.header.count();
}

TEMPLATE
size_t CLASS::point_records() const NOEXCEPT
{
    return store_.point.count();
}

TEMPLATE
size_t CLASS::spend_records() const NOEXCEPT
{
    return store_.spend.count();
}

TEMPLATE
size_t CLASS::tx_records() const NOEXCEPT
{
    return store_.tx.count();
}

//  Counts (metadata records).
// ----------------------------------------------------------------------------

TEMPLATE
size_t CLASS::candidate_records() const NOEXCEPT
{
    return store_.candidate.count();
}

TEMPLATE
size_t CLASS::confirmed_records() const NOEXCEPT
{
    return store_.confirmed.count();
}

TEMPLATE
size_t CLASS::strong_tx_records() const NOEXCEPT
{
    return store_.strong_tx.count();
}

// Counters (archive slabs).
// ----------------------------------------------------------------------------
// Also txs and puts, but these can be derived instead of counting.
// txs elements is equivalent to header_records and puts elements is the sum of
// spends or inputs and outputs.

TEMPLATE
size_t CLASS::input_count(const tx_link& link) const NOEXCEPT
{
    table::transaction::get_put_counts tx{};
    if (!store_.tx.get(link, tx))
        return {};

    return tx.ins_count;
}

TEMPLATE
size_t CLASS::output_count(const tx_link& link) const NOEXCEPT
{
    table::transaction::get_put_counts tx{};
    if (!store_.tx.get(link, tx))
        return {};

    return tx.outs_count;
}

TEMPLATE
two_counts CLASS::put_counts(const tx_link& link) const NOEXCEPT
{
    table::transaction::get_put_counts tx{};
    if (!store_.tx.get(link, tx))
        return {};

    return { tx.ins_count, tx.outs_count };
}

} // namespace database
} // namespace libbitcoin

#endif
