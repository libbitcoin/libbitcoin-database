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
#ifndef LIBBITCOIN_DATABASE_QUERY_EXTENT_IPP
#define LIBBITCOIN_DATABASE_QUERY_EXTENT_IPP

#include <numeric>
#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>

#define DEFINE_SIZES(name) \
TEMPLATE \
size_t CLASS::name##_size() const NOEXCEPT \
{ \
return name##_head_size() + name##_body_size(); \
} \
TEMPLATE \
size_t CLASS::name##_head_size() const NOEXCEPT \
{ \
return store_.name.head_size(); \
} \
TEMPLATE \
size_t CLASS::name##_body_size() const NOEXCEPT \
{ \
    return store_.name.body_size(); \
}

#define DEFINE_BUCKETS(name) \
TEMPLATE \
size_t CLASS::name##_buckets() const NOEXCEPT \
{ \
    return store_.name.buckets(); \
}

#define DEFINE_RECORDS(name) \
TEMPLATE \
size_t CLASS::name##_records() const NOEXCEPT \
{ \
    return store_.name.count(); \
}

namespace libbitcoin {
namespace database {

// Store extent.
// ----------------------------------------------------------------------------

TEMPLATE
size_t CLASS::store_size() const NOEXCEPT
{
    return store_body_size() + store_head_size();
}

TEMPLATE
size_t CLASS::archive_size() const NOEXCEPT
{
    return archive_body_size() + archive_head_size();
}

TEMPLATE
size_t CLASS::store_body_size() const NOEXCEPT
{
    return archive_body_size()
        + candidate_body_size()
        + confirmed_body_size()
        + strong_tx_body_size()
        + prevout_body_size()
        + validated_tx_body_size()
        + validated_bk_body_size()
        + address_body_size()
        + neutrino_body_size();
}

TEMPLATE
size_t CLASS::archive_body_size() const NOEXCEPT
{
    return header_body_size()
        + output_body_size()
        + input_body_size()
        + point_body_size()
        + puts_body_size()
        + spend_body_size()
        + txs_body_size()
        + tx_body_size();
}

TEMPLATE
size_t CLASS::store_head_size() const NOEXCEPT
{
    return archive_head_size()
        + candidate_head_size()
        + confirmed_head_size()
        + strong_tx_head_size()
        + prevout_head_size()
        + validated_tx_head_size()
        + validated_bk_head_size()
        + address_head_size()
        + neutrino_head_size();
}

TEMPLATE
size_t CLASS::archive_head_size() const NOEXCEPT
{
    return header_head_size()
        + output_head_size()
        + input_head_size()
        + point_head_size()
        + puts_head_size()
        + spend_head_size()
        + txs_head_size()
        + tx_head_size();
}

// Sizes.
// ----------------------------------------------------------------------------

DEFINE_SIZES(header)
DEFINE_SIZES(output)
DEFINE_SIZES(input)
DEFINE_SIZES(point)
DEFINE_SIZES(puts)
DEFINE_SIZES(spend)
DEFINE_SIZES(txs)
DEFINE_SIZES(tx)

DEFINE_SIZES(candidate)
DEFINE_SIZES(confirmed)
DEFINE_SIZES(strong_tx)
DEFINE_SIZES(prevout)
DEFINE_SIZES(validated_tx)
DEFINE_SIZES(validated_bk)
DEFINE_SIZES(address)
DEFINE_SIZES(neutrino)

// Buckets.
// ----------------------------------------------------------------------------

DEFINE_BUCKETS(header)
DEFINE_BUCKETS(point)
DEFINE_BUCKETS(spend)
DEFINE_BUCKETS(txs)
DEFINE_BUCKETS(tx)

DEFINE_BUCKETS(strong_tx)
DEFINE_BUCKETS(prevout)
DEFINE_BUCKETS(validated_tx)
DEFINE_BUCKETS(validated_bk)
DEFINE_BUCKETS(address)
DEFINE_BUCKETS(neutrino)

// Records.
// ----------------------------------------------------------------------------

DEFINE_RECORDS(header)
DEFINE_RECORDS(point)
DEFINE_RECORDS(spend)
DEFINE_RECORDS(tx)

DEFINE_RECORDS(candidate)
DEFINE_RECORDS(confirmed)
DEFINE_RECORDS(strong_tx)
DEFINE_RECORDS(prevout)
DEFINE_RECORDS(address)

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

TEMPLATE
size_t CLASS::input_count(const tx_links& txs) const NOEXCEPT
{
    constexpr auto parallel = poolstl::execution::par;
    const auto fn = [this](auto tx) NOEXCEPT { return input_count(tx); };
    return std::reduce(parallel, txs.begin(), txs.end(), zero, fn);
}

TEMPLATE
size_t CLASS::output_count(const tx_links& txs) const NOEXCEPT
{
    constexpr auto parallel = poolstl::execution::par;
    const auto fn = [this](auto tx) NOEXCEPT { return output_count(tx); };
    return std::reduce(parallel, txs.begin(), txs.end(), zero, fn);
}

TEMPLATE
two_counts CLASS::put_counts(const tx_links& txs) const NOEXCEPT
{
    return { input_count(txs), output_count(txs) };
}

TEMPLATE
bool CLASS::address_enabled() const NOEXCEPT
{
    return store_.address.enabled();
}

TEMPLATE
bool CLASS::neutrino_enabled() const NOEXCEPT
{
    return store_.neutrino.enabled();
}

TEMPLATE
bool CLASS::prevout_enabled() const NOEXCEPT
{
    return store_.prevout.enabled();
}

} // namespace database
} // namespace libbitcoin

#undef DEFINE_SIZES
#undef DEFINE_BUCKETS
#undef DEFINE_RECORDS

#endif
