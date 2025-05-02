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
#ifndef LIBBITCOIN_DATABASE_TABLES_SCHEMA_HPP
#define LIBBITCOIN_DATABASE_TABLES_SCHEMA_HPP

#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {
namespace schema {

/// Values.
/// -----------------------------------------------------------------------
constexpr size_t bit = 1;       // single bit flag.
constexpr size_t code = 1;      // validation state.
constexpr size_t size = 3;      // tx/block size/weight.
constexpr size_t height_ = 3;   // height record.
constexpr size_t count_ = 3;    // txs count.
constexpr size_t index = 3;     // input/output index.
constexpr size_t sigops = 3;    // signature op count.
constexpr size_t flags = 4;     // fork flags.
constexpr size_t hash = system::hash_size;

/// Primary keys.
/// -----------------------------------------------------------------------
constexpr size_t dup = 2;       // ->duplicate
constexpr size_t put = 5;       // ->input/output slab.
constexpr size_t ins_ = 4;      // ->point|ins record.
constexpr size_t outs_ = 4;     // ->outs (puts) record.
constexpr size_t spend_ = 4;    // ->spend record.
constexpr size_t prevout_ = 5;  // ->prevout slab.
constexpr size_t txs_ = 5;      // ->txs slab.
constexpr size_t tx = 4;        // ->tx record.
constexpr size_t block = 3;     // ->header record.
constexpr size_t bk_slab = 4;   // ->validated_bk record.
constexpr size_t tx_slab = 5;   // ->validated_tx record.
constexpr size_t filter_ = 5;   // ->filter record.
constexpr size_t doubles_ = 4;  // doubles bucket (no actual keys).

/// Archive tables.
/// -----------------------------------------------------------------------

// record hashmap
struct header
{
    // TODO: merge milestone with parent.pk.
    static constexpr size_t sk = schema::hash;
    static constexpr size_t pk = schema::block;
    using link = linkage<pk, sub1(to_bits(pk))>; // reduced for strong_tx merge.
    using key = system::data_array<sk>;
    static constexpr size_t minsize =
        schema::flags +         // context.flags
        schema::height_ +       // context.height
        sizeof(uint32_t) +      // context.mtp
        schema::bit +           // milestone
        pk +                    // parent.pk
        sizeof(uint32_t) +      // version
        sizeof(uint32_t) +      // timestamp
        sizeof(uint32_t) +      // bits
        sizeof(uint32_t) +      // nonce
        schema::hash;           // merkle root
    static constexpr size_t minrow = pk + sk + minsize;
    static constexpr size_t size = minsize;
    static constexpr size_t cell = sizeof(unsigned_type<link::size>);
    static constexpr link count() NOEXCEPT { return 1; }
    static_assert(minsize == 63u);
    static_assert(minrow == 98u);
    static_assert(link::size == 3u);
    static_assert(cell == 4u);
};

// record hashmap
struct transaction
{
    // TODO: merge coinbase with something.
    static constexpr size_t sk = schema::hash;
    static constexpr size_t pk = schema::tx;
    using link = linkage<pk, sub1(to_bits(pk))>; // reduced for prevout merge.
    using key = system::data_array<sk>;
    static constexpr size_t minsize =
        schema::bit +           // coinbase
        schema::size +          // light
        schema::size +          // heavy
        sizeof(uint32_t) +      // locktime
        sizeof(uint32_t) +      // version
        schema::index +         // inputs count
        schema::index +         // outputs count
        schema::ins_ +          // first contiguous input (same link as point)
        schema::outs_;          // first contiguous output
    static constexpr size_t minrow = pk + sk + minsize;
    static constexpr size_t size = minsize;
    static constexpr size_t cell = link::size;
    static constexpr link count() NOEXCEPT { return 1; }
    static_assert(minsize == 29u);
    static_assert(minrow == 65u);
    static_assert(link::size == 4u);
    static_assert(cell == 4u);
};

// blob
struct input
{
    static constexpr size_t sk = zero;
    static constexpr size_t pk = schema::put;
    using link = linkage<pk, to_bits(pk)>;
    static constexpr size_t minsize =
        1u +                    // variable_size (minimum 1, average 1)
        1u;                     // variable_size (minimum 1, average 1)
    static constexpr size_t minrow = minsize;
    static constexpr size_t size = max_size_t;
    static_assert(minsize == 2u);
    static_assert(minrow == 2u);
    static_assert(link::size == 5u);
};

// blob
struct output
{
    static constexpr size_t sk = zero;
    static constexpr size_t pk = schema::put;
    using link = linkage<pk, to_bits(pk)>;
    static constexpr size_t minsize =
        schema::transaction::pk +   // parent->tx (address navigation)
        5u +                    // variable_size (minimum 1, average 5)
        1u;                     // variable_size (minimum 1, average 1)
    static constexpr size_t minrow = minsize;
    static constexpr size_t size = max_size_t;
    static_assert(minsize == 10u);
    static_assert(minrow == 10u);
    static_assert(link::size == 5u);
};

// record multimap
struct point
{
    static constexpr size_t pk = schema::ins_;
    using link = linkage<pk, to_bits(pk)>;
    using key = system::chain::point;
    static constexpr size_t sk = schema::hash + schema::index;
    static constexpr size_t minsize =
        zero;                   // empty row
    static constexpr size_t minrow = pk + sk + minsize;
    static constexpr size_t size = minsize;
    static constexpr size_t cell = sizeof(uint64_t);
    static constexpr link count() NOEXCEPT { return 1; }
    static_assert(minsize == 0u);
    static_assert(minrow == 39u);
    static_assert(link::size == 4u);
    static_assert(cell == 8u);
};

// array
struct ins
{
    static constexpr size_t pk = point::pk;
    using link = point::link;   // aligned with point.
    static constexpr size_t minsize =
        sizeof(uint32_t) +      // sequence
        schema::input::pk +     // input->script|witness
        schema::transaction::pk;// parent->tx (spend navigation)
    static constexpr size_t minrow = minsize;
    static constexpr size_t size = minsize;
    static constexpr link count() NOEXCEPT { return 1; }
    static_assert(minsize == 13u);
    static_assert(minrow == 13u);
    static_assert(link::size == 4u);
};

// array
struct outs
{
    static constexpr size_t pk = schema::outs_;
    using link = linkage<pk, to_bits(pk)>;
    static constexpr size_t minsize =
        schema::output::pk;
    static constexpr size_t minrow = minsize;
    static constexpr size_t size = minsize;
    ////static constexpr link count() NOEXCEPT { return 1; }
    static_assert(minsize == 5u);
    static_assert(minrow == 5u);
    static_assert(link::size == 4u);
};

// slab arraymap
struct txs
{
    static constexpr size_t align = false;
    static constexpr size_t pk = schema::txs_;
    using link = linkage<pk, to_bits(pk)>;
    static constexpr size_t minsize =
        count_ +                // txs
        schema::size +          // block.serialized_size(true)
        schema::transaction::pk;// coinbase tx
    static constexpr size_t minrow = minsize;
    static constexpr size_t size = max_size_t;
    static_assert(minsize == 10u);
    static_assert(minrow == 10u);
    static_assert(link::size == 5u);
};

/// Index tables.
/// -----------------------------------------------------------------------

// candidate and confirmed arrays
struct height
{
    static constexpr size_t sk = zero;
    static constexpr size_t pk = schema::header::pk;
    using link = schema::header::link;
    static constexpr size_t minsize =
        schema::header::pk;
    static constexpr size_t minrow = minsize;
    static constexpr size_t size = minsize;
    static constexpr link count() NOEXCEPT { return 1; }
    static_assert(minsize == 3u);
    static_assert(minrow == 3u);
    static_assert(link::size == 3u);
};

// record hashmap
struct strong_tx
{
    static constexpr size_t sk = schema::transaction::pk;
    static constexpr size_t pk = schema::transaction::pk;
    using link = linkage<pk, to_bits(pk)>;
    using key = system::data_array<sk>;
    static constexpr size_t minsize =
        ////schema::bit +           // merged bit into header::pk.
        schema::header::pk;
    static constexpr size_t minrow = pk + sk + minsize;
    static constexpr size_t size = minsize;
    static constexpr size_t cell = link::size;
    static constexpr link count() NOEXCEPT { return 1; }
    static_assert(minsize == 3u);
    static_assert(minrow == 11u);
    static_assert(link::size == 4u);
    static_assert(cell == 4u);
};

/// Cache tables.
/// -----------------------------------------------------------------------

// record hashmap
struct duplicate
{
    // This could be compressed to store fks once we start tx relay.
    static constexpr size_t pk = schema::dup;
    using link = linkage<pk, to_bits(pk)>;
    using key = system::chain::point;
    static constexpr size_t sk = schema::hash + schema::index;
    static constexpr size_t minsize =
        zero;
    static constexpr size_t minrow = pk + sk + minsize;
    static constexpr size_t size = minsize;
    static constexpr size_t cell = sizeof(uint32_t);
    static constexpr link count() NOEXCEPT { return 1; }
    static_assert(minsize == 0u);
    static_assert(minrow == 37u);
    static_assert(link::size == 2u);
    static_assert(cell == 4u);
};

// slab arraymap
struct prevout
{
    static constexpr size_t align = false;
    static constexpr size_t pk = schema::prevout_;
    using link = linkage<pk, to_bits(pk)>;
    static constexpr size_t minsize =
        ////schema::bit +           // merged bit into transaction::pk.
        one +                       // varint(conflict-count)
        schema::transaction::pk +   // prevout_tx
        one;                        // varint(sequence)
    static constexpr size_t minrow = minsize;
    static constexpr size_t size = max_size_t;
    static_assert(minsize == 6u);
    static_assert(minrow == 6u);
    static_assert(link::size == 5u);
};

// slab arraymap
struct validated_bk
{
    static constexpr size_t align = false;
    static constexpr size_t pk = schema::bk_slab;
    using link = linkage<pk, to_bits(pk)>;
    static constexpr size_t minsize =
        schema::code +  // TODO: change code to variable.
        one;
    static constexpr size_t minrow = minsize;
    static constexpr size_t size = max_size_t;
    static_assert(minsize == 2u);
    static_assert(minrow == 2u);
    static_assert(link::size == 4u);
};

// slab modest (sk:4) multimap, with low multiple rate.
struct validated_tx
{
    static constexpr size_t sk = schema::transaction::pk;
    static constexpr size_t pk = schema::tx_slab;
    using link = linkage<pk, to_bits(pk)>;
    using key = system::data_array<sk>;
    static constexpr size_t minsize =
        schema::flags +
        schema::header::pk +
        sizeof(uint32_t) +
        schema::code +  // TODO: change code to variable.
        one +
        one;
    static constexpr size_t minrow = pk + sk + minsize;
    static constexpr size_t size = max_size_t;
    static constexpr size_t cell = link::size;
    static inline link count() NOEXCEPT;
    static_assert(minsize == 14u);
    static_assert(minrow == 23u);
};

/// Optional tables.
/// -----------------------------------------------------------------------

// TODO: modest (sk:4) record multimap, with high multiple rate.
// large (sk:32) record multimap, with high multiple rate.
// address record count is output count.
struct address
{
    static constexpr size_t sk = schema::hash;
    static constexpr size_t pk = schema::outs::pk;
    using link = linkage<pk, to_bits(pk)>;
    using key = system::data_array<sk>;
    static constexpr size_t minsize =
        schema::output::pk;
    static constexpr size_t minrow = pk + sk + minsize;
    static constexpr size_t size = minsize;
    static constexpr size_t cell = link::size;
    static constexpr link count() NOEXCEPT { return 1; }
    static_assert(minsize == 5u);
    static_assert(minrow == 41u);
    static_assert(link::size == 4u);
    static_assert(cell == 4u);
};

// record arraymap
struct filter_bk
{
    static constexpr size_t align = false;
    static constexpr size_t pk = schema::header::pk;
    using link = linkage<pk, to_bits(pk)>;
    static constexpr size_t minsize =
        schema::hash;
    static constexpr size_t minrow = minsize;
    static constexpr size_t size = minsize;
    static constexpr link count() NOEXCEPT { return 1; }
    static_assert(minsize == 32u);
    static_assert(minrow == 32u);
    static_assert(link::size == 3u);
};

// slab arraymap
struct filter_tx
{
    static constexpr size_t align = false;
    static constexpr size_t pk = schema::filter_;
    using link = linkage<pk, to_bits(pk)>;
    static constexpr size_t minsize =
        one;
    static constexpr size_t minrow = minsize;
    static constexpr size_t size = max_size_t;
    static inline link count() NOEXCEPT;
    static_assert(minsize == 1u);
    static_assert(minrow == 1u);
    static_assert(link::size == 5u);
};

} // namespace schema
} // namespace database
} // namespace libbitcoin

#endif
