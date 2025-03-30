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
constexpr size_t count_ = 3;    // txs count.
constexpr size_t index = 3;     // input/output index.
constexpr size_t sigops = 3;    // signature op count.
constexpr size_t flags = 4;     // fork flags.
constexpr size_t hash = system::hash_size;

/// Primary keys.
/// -----------------------------------------------------------------------
constexpr size_t put = 5;       // ->input/output slab.
constexpr size_t ins_ = 4;      // ->ins|point record.
constexpr size_t outs_ = 4;     // ->outs (puts) record.
constexpr size_t spend_ = 4;    // ->spend record.
constexpr size_t prevout_ = 5;  // ->prevout slab.
constexpr size_t txs_ = 5;      // ->txs slab.
constexpr size_t tx = 4;        // ->tx record.
constexpr size_t block = 3;     // ->header record.
constexpr size_t bk_slab = 3;   // ->validated_bk record.
constexpr size_t tx_slab = 5;   // ->validated_tk record.
constexpr size_t neutrino_ = 5; // ->neutrino record.
constexpr size_t doubles_ = 4;  // doubles bucket (no actual keys).

/// Archive tables.
/// -----------------------------------------------------------------------

// record hashmap
struct header
{
    static constexpr size_t sk = schema::hash;
    using key = system::data_array<sk>;
    static constexpr size_t pk = schema::block;
    static constexpr size_t minsize =
        schema::flags +
        schema::block +
        sizeof(uint32_t) +
        bit +
        pk +
        sizeof(uint32_t) +
        sizeof(uint32_t) +
        sizeof(uint32_t) +
        sizeof(uint32_t) +
        schema::hash;
    static constexpr size_t minrow = pk + sk + minsize;
    static constexpr size_t size = minsize;
    static constexpr size_t cell = sizeof(unsigned_type<linkage<pk>::size>);
    static constexpr linkage<pk> count() NOEXCEPT { return 1; }
    static_assert(minsize == 63u);
    static_assert(minrow == 98u);
};

// record hashmap
struct transaction
{
    static constexpr size_t sk = schema::hash;
    using key = system::data_array<sk>;
    static constexpr size_t pk = schema::tx;
    static constexpr size_t minsize =
        schema::bit +
        schema::size +
        schema::size +
        sizeof(uint32_t) +
        sizeof(uint32_t) +
        schema::index +     // inputs count
        schema::index +     // outputs count
        schema::ins_ +      // first contiguous input (point)
        schema::outs_;      // first contiguous output (put)
    static constexpr size_t minrow = pk + sk + minsize;
    static constexpr size_t size = minsize;
    static constexpr size_t cell = linkage<pk>::size;
    static constexpr linkage<pk> count() NOEXCEPT { return 1; }
    static_assert(minsize == 29u);
    static_assert(minrow == 65u);
};

// blob
struct input
{
    static constexpr size_t sk = zero;
    static constexpr size_t pk = schema::put;
    static constexpr size_t minsize =
        1u + // variable_size (minimum 1, average 1)
        1u;  // variable_size (minimum 1, average 1)
    static constexpr size_t minrow = minsize;
    static constexpr size_t size = max_size_t;
    static_assert(minsize == 2u);
    static_assert(minrow == 2u);
};

// blob
struct output
{
    static constexpr size_t sk = zero;
    static constexpr size_t pk = schema::put;
    static constexpr size_t minsize =
        schema::transaction::pk +   // parent->tx (address navigation)
        5u + // variable_size (minimum 1, average 5)
        1u;  // variable_size (minimum 1, average 1)
    static constexpr size_t minrow = minsize;
    static constexpr size_t size = max_size_t;
    static_assert(minsize == 10u);
    static_assert(minrow == 10u);
};

// record multimap
struct point
{
    using key = system::chain::point;
    static constexpr size_t pk = schema::ins_;
    static constexpr size_t sk = schema::hash + schema::index;
    static constexpr size_t minsize = zero;
    static constexpr size_t minrow = pk + sk + minsize;
    static constexpr size_t size = minsize;
    static constexpr size_t cell = sizeof(uint64_t); //// linkage<pk>::size;
    static constexpr linkage<pk> count() NOEXCEPT { return 1; }
    static_assert(minsize == 0u);
    static_assert(minrow == 39u);
};

// array
struct ins
{
    static constexpr size_t pk = schema::ins_;
    static constexpr size_t minsize =
        sizeof(uint32_t) +          // sequence
        schema::input::pk +         // input->script|witness
        schema::transaction::pk;    // parent->tx (spend navigation)
    static constexpr size_t minrow = minsize;
    static constexpr size_t size = minsize;
    static constexpr linkage<pk> count() NOEXCEPT { return 1; }
    static_assert(minsize == 13u);
    static_assert(minrow == 13u);
};

// array
struct outs
{
    static constexpr size_t pk = schema::outs_;
    static constexpr size_t minsize =
        schema::output::pk;
    static constexpr size_t minrow = minsize;
    static constexpr size_t size = minsize;
    ////static constexpr linkage<pk> count() NOEXCEPT { return 1; }
    static_assert(minsize == 5u);
    static_assert(minrow == 5u);
};

// slab hashmap
struct txs
{
    static constexpr size_t sk = schema::header::pk;
    using key = system::data_array<sk>;
    static constexpr size_t pk = schema::txs_;
    static constexpr size_t minsize =
        count_ +         // txs
        schema::size +   // block.serialized_size(true)
        transaction::pk; // coinbase
    static constexpr size_t minrow = pk + sk + minsize;
    static constexpr size_t size = max_size_t;
    ////static constexpr size_t cell = sizeof(unsigned_type<linkage<pk>::size>);
    static constexpr size_t cell = linkage<pk>::size;
    static_assert(minsize == 10u);
    static_assert(minrow == 18u);
};

/// Index tables.
/// -----------------------------------------------------------------------

// candidate and confirmed arrays
struct height
{
    static constexpr size_t sk = zero;
    static constexpr size_t pk = schema::block;
    static constexpr size_t minsize = schema::block;
    static constexpr size_t minrow = minsize;
    static constexpr size_t size = minsize;
    static constexpr linkage<pk> count() NOEXCEPT { return 1; }
    static_assert(minsize == 3u);
    static_assert(minrow == 3u);
};

// TODO: modest (sk:4) record multimap, with high multiple rate.
// large (sk:32) record multimap, with high multiple rate.
// address record count is output count.
struct address
{
    static constexpr size_t sk = schema::hash;
    using key = system::data_array<sk>;
    static constexpr size_t pk = schema::outs_;
    static constexpr size_t minsize = schema::put;
    static constexpr size_t minrow = pk + sk + minsize;
    static constexpr size_t size = minsize;
    static constexpr size_t cell = linkage<pk>::size;
    static constexpr linkage<pk> count() NOEXCEPT { return 1; }
    static_assert(minsize == 5u);
    static_assert(minrow == 41u);
};

// record hashmap
struct strong_tx
{
    static constexpr size_t sk = schema::transaction::pk;
    using key = system::data_array<sk>;
    static constexpr size_t pk = schema::tx;
    static constexpr size_t minsize =
        schema::header::pk + bit;
    static constexpr size_t minrow = pk + sk + minsize;
    static constexpr size_t size = minsize;
    static constexpr size_t cell = linkage<pk>::size;
    static constexpr linkage<pk> count() NOEXCEPT { return 1; }
    static_assert(minsize == 4u);
    static_assert(minrow == 12u);
};

/// Cache tables.
/// -----------------------------------------------------------------------

// slab arraymap, one slab per block
struct prevout
{
    static constexpr size_t align = true;
    static constexpr size_t pk = schema::prevout_;
    static constexpr size_t minsize =
        ////schema::bit + // merged bit into tx.
        one +                       // varint(conflict-count)
        schema::transaction::pk +   // prevout_tx
        one;                        // varint(sequence)
    static constexpr size_t minrow = minsize;
    static constexpr size_t size = max_size_t;
    static_assert(minsize == 6u);
    static_assert(minrow == 6u);
};

// hashmap array
// The hashmap header is a cuckoo filter, which does not link the body.
// The filter data is contained within the buckets. The body is unindexed
// array of duplicated ponts (at least two in the main table). This body
// itself is neither de-deduplicated nor indexed.
struct doubles
{
    using key = system::chain::point;
    static constexpr size_t pk = schema::doubles_;
    static constexpr size_t sk = schema::hash + schema::index;
    static constexpr size_t minsize =
        schema::hash +
        schema::index;
    static constexpr size_t minrow = minsize;
    static constexpr size_t size = minsize;
    static constexpr size_t cell = linkage<pk>::size;
    static constexpr linkage<pk> count() NOEXCEPT { return 1; }
    static_assert(minsize == 35u);
    static_assert(minrow == 35u);
};

// slab hashmap
struct validated_bk
{
    static constexpr size_t sk = schema::header::pk;
    using key = system::data_array<sk>;
    static constexpr size_t pk = schema::bk_slab;
    static constexpr size_t minsize =
        schema::code + 
        one;
    static constexpr size_t minrow = pk + sk + minsize;
    static constexpr size_t size = max_size_t;
    static constexpr size_t cell = sizeof(unsigned_type<linkage<pk>::size>);
    static_assert(minsize == 2u);
    static_assert(minrow == 8u);
};

// slab modest (sk:4) multimap, with low multiple rate.
struct validated_tx
{
    static constexpr size_t sk = schema::transaction::pk;
    using key = system::data_array<sk>;
    static constexpr size_t pk = schema::tx_slab;
    static constexpr size_t minsize =
        schema::flags +
        schema::block +
        sizeof(uint32_t) +
        schema::code +
        one +
        one;
    static constexpr size_t minrow = pk + sk + minsize;
    static constexpr size_t size = max_size_t;
    static constexpr size_t cell = linkage<pk>::size;
    static inline linkage<pk> count() NOEXCEPT;
    static_assert(minsize == 14u);
    static_assert(minrow == 23u);
};

// slab hashmap
struct neutrino
{
    static constexpr size_t sk = schema::header::pk;
    using key = system::data_array<sk>;
    static constexpr size_t pk = schema::neutrino_;
    static constexpr size_t minsize =
        schema::hash +
        one;
    static constexpr size_t minrow = pk + sk + minsize;
    static constexpr size_t size = max_size_t;
    static constexpr size_t cell = linkage<pk>::size;
    static_assert(minsize == 33u);
    static_assert(minrow == 41u);
};

} // namespace schema
} // namespace database
} // namespace libbitcoin

#endif
