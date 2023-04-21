/**
 * Copyright (c) 2011-2022 libbitcoin developers (see AUTHORS)
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
#include <bitcoin/database/primitives/primitives.hpp>

namespace libbitcoin {
namespace database {

template <size_t Size>
using search = system::data_array<Size>;
using hashes = system::hashes;
using hash_digest = search<system::hash_size>;
static_assert(is_same_type<hash_digest, system::hash_digest>);

namespace schema
{
    namespace dir
    {
        constexpr auto heads = "heads";
        constexpr auto primary = "primary";
        constexpr auto secondary = "secondary";
    }

    namespace archive
    {
        constexpr auto header = "archive_header";
        constexpr auto point = "archive_point";
        constexpr auto input = "archive_input";
        constexpr auto output = "archive_output";
        constexpr auto puts = "archive_puts";
        constexpr auto txs = "archive_txs";
        constexpr auto tx = "archive_tx";
    }

    namespace indexes
    {
        constexpr auto address = "address";
        constexpr auto candidate = "candidate";
        constexpr auto confirmed = "confirmed";
        constexpr auto spend = "spend";
        constexpr auto strong_tx = "strong_tx";
    }

    namespace caches
    {
        constexpr auto bootstrap = "bootstrap";
        constexpr auto buffer = "buffer";
        constexpr auto neutrino = "neutrino";
        constexpr auto validated_bk = "validated_bk";
        constexpr auto validated_tx = "validated_tx";
    }

    namespace locks
    {
        constexpr auto flush = "flush";
        constexpr auto process = "process";
    }

    namespace ext
    {
        constexpr auto head = ".head";
        constexpr auto data = ".data";
        constexpr auto lock = ".lock";
    }

    enum block_state : uint8_t
    {
        confirmable = 0,    // final
        preconfirmable = 1, // transitional
        unconfirmable = 2   // final
    };

    enum tx_state : uint8_t
    {
        connected = 0,      // final
        preconnected = 1,   // transitional
        disconnected = 2    // final
    };

    /// Values.
    constexpr size_t bit = 1;       // single bit flag.
    constexpr size_t code = 1;      // validation state.
    constexpr size_t size = 3;      // tx/block size/weight.
    constexpr size_t index = 3;     // input/output index.
    constexpr size_t sigops = 3;    // signature op count.
    constexpr size_t flags = 4;     // fork flags.

    /// Primary keys.
    constexpr size_t put = 5;       // ->input/output slab.
    constexpr size_t puts_ = 5;     // ->puts record.
    constexpr size_t spend_ = 4;    // ->spend record.
    constexpr size_t txs_ = 4;      // ->txs slab.
    constexpr size_t tx = 4;        // ->tx record.
    constexpr size_t block = 3;     // ->header record.
    constexpr size_t bk_slab = 3;   // ->validated_bk record.
    constexpr size_t tx_slab = 5;   // ->validated_tk record.
    constexpr size_t buffer_ = 5;   // ->buffer record (guestimate).
    constexpr size_t neutrino_ = 5; // ->neutrino record (guestimate).

    /// Search keys.
    constexpr size_t hash = system::hash_size;

    /// Archive tables.
    /// -----------------------------------------------------------------------

    // record hashmap
    struct header
    {
        static constexpr bool hash_function = false;
        static constexpr size_t pk = schema::block;
        static constexpr size_t sk = schema::hash;
        static constexpr size_t minsize =
            schema::flags +
            schema::block +
            sizeof(uint32_t) +
            pk +
            sizeof(uint32_t) +
            sizeof(uint32_t) +
            sizeof(uint32_t) +
            sizeof(uint32_t) +
            schema::hash;
        static constexpr size_t minrow = pk + sk + minsize;
        static constexpr size_t size = minsize;
        static constexpr linkage<pk> count() NOEXCEPT { return 1; }
        static_assert(minsize == 62u);
        static_assert(minrow == 97u);
    };

    // array
    struct puts
    {
        static constexpr size_t pk = schema::puts_;
        static constexpr size_t sk = zero;
        static constexpr size_t minsize = schema::put;
        static constexpr size_t minrow = minsize;
        static constexpr size_t size = minsize;
        static_assert(minsize == 5u);
        static_assert(minrow == 5u);
    };

    // record hashmap
    struct transaction
    {
        static constexpr bool hash_function = false;
        static constexpr size_t pk = schema::tx;
        static constexpr size_t sk = schema::hash;
        static constexpr size_t minsize =
            schema::bit +
            schema::size +
            schema::size +
            sizeof(uint32_t) +
            sizeof(uint32_t) +
            schema::index +
            schema::index +
            schema::puts::pk;
        static constexpr size_t minrow = pk + sk + minsize;
        static constexpr size_t size = minsize;
        static constexpr linkage<pk> count() NOEXCEPT { return 1; }
        static_assert(minsize == 26u);
        static_assert(minrow == 62u);
    };

    // TODO: change to blob (remove hashmap)
    // moderate (sk:7) slab multimap, with low multiple rate.
    struct input
    {
        static constexpr bool hash_function = false;
        static constexpr size_t pk = schema::put;
        static constexpr size_t sk = transaction::pk + schema::index;
        static constexpr size_t minsize =
            schema::transaction::pk +
            ////1u + // variable_size (average 1)
            sizeof(uint32_t) +
            1u + // variable_size (average 1)
            1u;  // variable_size (average 1)
        static constexpr size_t minrow = pk + sk + minsize;
        static constexpr size_t size = max_size_t;
        static_assert(minsize == 10u);
        static_assert(minrow == 22u);
    };

    // blob
    struct output
    {
        static constexpr size_t pk = schema::put;
        static constexpr size_t sk = zero;
        static constexpr size_t minsize =
            schema::transaction::pk +
            1u + // variable_size (average 1)
            5u + // variable_size (average 5)
            1u;  // variable_size (average 1)
        static constexpr size_t minrow = minsize;
        static constexpr size_t size = max_size_t;
        static_assert(minsize == 11u);
        static_assert(minrow == 11u);
    };

    // moderate (sk:7) record multimap, with low multiple rate.
    struct spend
    {
        static constexpr bool hash_function = false;
        static constexpr size_t pk = schema::spend_;
        static constexpr size_t sk = transaction::pk + schema::index;
        static constexpr size_t minsize =
            ////schema::input::pk +
            schema::transaction::pk;
        static constexpr size_t minrow = pk + sk + minsize;
        static constexpr size_t size = minsize;
        static constexpr linkage<pk> count() NOEXCEPT { return 1; }
        static_assert(minsize == 4u);
        static_assert(minrow == 15u);
    };

    // record hashmap
    struct point
    {
        static constexpr bool hash_function = false;
        static constexpr size_t pk = schema::tx;
        static constexpr size_t sk = schema::hash;
        static constexpr size_t minsize = zero;
        static constexpr size_t minrow = pk + sk + minsize;
        static constexpr size_t size = minsize;
        static constexpr linkage<pk> count() NOEXCEPT { return 1; }
        static_assert(minsize == 0u);
        static_assert(minrow == 36u);
    };

    // slab hashmap
    struct txs
    {
        static constexpr bool hash_function = false;
        static constexpr size_t pk = schema::txs_;
        static constexpr size_t sk = schema::header::pk;
        static constexpr size_t minsize = zero;
        static constexpr size_t minrow = pk + sk + minsize;
        static constexpr size_t size = max_size_t;
        static_assert(minsize == 0u);
        static_assert(minrow == 7u);
    };

    /// Index tables.
    /// -----------------------------------------------------------------------

    // candidate and confirmed arrays
    struct height
    {
        static constexpr size_t pk = schema::block;
        static constexpr size_t sk = zero;
        static constexpr size_t minsize = schema::block;
        static constexpr size_t minrow = minsize;
        static constexpr size_t size = minsize;
        static constexpr linkage<pk> count() NOEXCEPT { return 1; }
        static_assert(minsize == 3u);
        static_assert(minrow == 3u);
    };

    // TODO: modest (sk:4) record multimap, with high multiple rate.
    // large (sk:32) record multimap, with high multiple rate.
    struct address
    {
        static constexpr bool hash_function = false;
        static constexpr size_t pk = schema::puts_;
        ////static constexpr size_t sk = schema::point::pk;
        static constexpr size_t sk = schema::hash;
        static constexpr size_t minsize = schema::put;
        static constexpr size_t minrow = pk + sk + minsize;
        static constexpr size_t size = minsize;
        static constexpr linkage<pk> count() NOEXCEPT { return 1; }
        static_assert(minsize == 5u);
        ////static_assert(minrow == 14u);
        static_assert(minrow == 42u);
    };

    // record hashmap
    struct strong_tx
    {
        static constexpr bool hash_function = false;
        static constexpr size_t pk = schema::tx;
        static constexpr size_t sk = schema::transaction::pk;
        static constexpr size_t minsize =
            schema::header::pk;
        static constexpr size_t minrow = pk + sk + minsize;
        static constexpr size_t size = minsize;
        static constexpr linkage<pk> count() NOEXCEPT { return 1; }
        static_assert(minsize == 3u);
        static_assert(minrow == 11u);
    };

    /// Cache tables.
    /// -----------------------------------------------------------------------

    // array
    struct bootstrap
    {
        static constexpr size_t pk = schema::block;
        static constexpr size_t sk = zero;
        static constexpr size_t minsize = schema::hash;
        static constexpr size_t minrow = minsize;
        static constexpr size_t size = minsize;
        static constexpr linkage<pk> count() NOEXCEPT { return 1; }
        static_assert(minsize == 32u);
        static_assert(minrow == 32u);
    };

    // slab hashmap
    struct buffer
    {
        static constexpr bool hash_function = false;
        static constexpr size_t pk = schema::buffer_;
        static constexpr size_t sk = schema::transaction::pk;
        static constexpr size_t minsize = zero;
        static constexpr size_t minrow = pk + sk + minsize;
        static constexpr size_t size = max_size_t;
        static_assert(minsize == 0u);
        static_assert(minrow == 9u);
    };

    // slab hashmap
    struct neutrino
    {
        static constexpr bool hash_function = false;
        static constexpr size_t pk = schema::neutrino_;
        static constexpr size_t sk = schema::header::pk;
        static constexpr size_t minsize = 
            schema::hash +
            one;
        static constexpr size_t minrow = pk + sk + minsize;
        static constexpr size_t size = max_size_t;
        static_assert(minsize == 33u);
        static_assert(minrow == 41u);
    };

    // slab hashmap
    struct validated_bk
    {
        static constexpr bool hash_function = false;
        static constexpr size_t pk = schema::bk_slab;
        static constexpr size_t sk = schema::header::pk;
        static constexpr size_t minsize =
            schema::code + 
            one;
        static constexpr size_t minrow = pk + sk + minsize;
        static constexpr size_t size = max_size_t;
        static_assert(minsize == 2u);
        static_assert(minrow == 8u);
    };

    // modest (sk:4) slab multimap, with low multiple rate.
    struct validated_tx
    {
        static constexpr bool hash_function = false;
        static constexpr size_t pk = schema::tx_slab;
        static constexpr size_t sk = schema::transaction::pk;
        static constexpr size_t minsize =
            schema::flags +
            schema::block +
            sizeof(uint32_t) +
            schema::code +
            one +
            one;
        static constexpr size_t minrow = pk + sk + minsize;
        static constexpr size_t size = max_size_t;
        static_assert(minsize == 14u);
        static_assert(minrow == 23u);
    };
}

} // namespace database
} // namespace libbitcoin

#endif
