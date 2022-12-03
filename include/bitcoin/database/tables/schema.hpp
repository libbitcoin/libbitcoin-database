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
        constexpr auto strong_bk = "strong_bk";
        constexpr auto strong_tx = "strong_tx";
    }

    namespace caches
    {
        ////constexpr auto bootstrap = "bootstrap";
        ////constexpr auto buffer = "buffer";
        ////constexpr auto neutrino = "neutrino";
        ////constexpr auto validated_bk = "validated_bk";
        ////constexpr auto validated_tx = "validated_tx";
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

    /// Values.
    constexpr size_t bit = 1;       // Single bit flag.
    constexpr size_t code = 1;      // State flag.
    constexpr size_t size = 3;      // tx/block size/weight.
    constexpr size_t index = 3;     // input/output index.
    constexpr size_t sigops = 3;    // signature operation count.
    constexpr size_t flags = 4;     // validation flags.

    /// Primary keys.
    constexpr size_t put = 5;       // ->input/output slab.
    constexpr size_t puts_ = 4;     // ->puts record.
    constexpr size_t txs_ = 4;      // ->txs slab.
    constexpr size_t tx = 4;        // ->tx record.
    constexpr size_t block = 3;     // ->header record.
    constexpr size_t address_ = 4;  // ->address record.

    /// Search keys.
    constexpr size_t tx_fp = tx + index;
    constexpr size_t hash = system::hash_size;

    /// Archive tables.
    /// -----------------------------------------------------------------------

    struct header
    {
        static constexpr size_t pk = schema::block;
        static constexpr size_t sk = schema::hash;
        static constexpr size_t minsize =
            schema::block +
            schema::flags +
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

    // Moderate (sk:7) multimap.
    struct input
    {
        static constexpr size_t pk = schema::put;
        static constexpr size_t sk = schema::tx_fp;
        static constexpr size_t minsize =
            schema::tx +
            1u + // variable_size (average 1)
            sizeof(uint32_t) +
            1u + // variable_size (average 1)
            1u;  // variable_size (average 1)
        static constexpr size_t minrow = pk + sk + minsize;
        static constexpr size_t size = max_size_t;
        static_assert(minsize == 11u);
        static_assert(minrow == 23u);
    };

    struct output
    {
        static constexpr size_t pk = schema::put;
        static constexpr size_t sk = zero;
        static constexpr size_t minsize =
            schema::tx +
            1u + // variable_size (average 1)
            5u + // variable_size (average 5)
            1u;  // variable_size (average 1)
        static constexpr size_t minrow = minsize;
        static constexpr size_t size = max_size_t;
        static_assert(minsize == 11u);
        static_assert(minrow == 11u);
    };

    struct point
    {
        static constexpr size_t pk = schema::tx;
        static constexpr size_t sk = schema::hash;
        static constexpr size_t minsize = zero;
        static constexpr size_t minrow = pk + sk + minsize;
        static constexpr size_t size = minsize;
        static constexpr linkage<pk> count() NOEXCEPT { return 1; }
        static_assert(minsize == 0u);
        static_assert(minrow == 36u);
    };

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

    struct transaction
    {
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
            schema::puts_;
        static constexpr size_t minrow = pk + sk + minsize;
        static constexpr size_t size = minsize;
        static constexpr linkage<pk> count() NOEXCEPT { return 1; }
        static_assert(minsize == 25u);
        static_assert(minrow == 61u);
    };

    struct txs
    {
        static constexpr size_t pk = schema::txs_;
        static constexpr size_t sk = schema::block;
        static constexpr size_t minsize = zero;
        static constexpr size_t minrow = pk + sk + minsize;
        static constexpr size_t size = max_size_t;
        static_assert(minsize == 0u);
        static_assert(minrow == 7u);
    };

    /// Index tables.
    /// -----------------------------------------------------------------------

    // candidate and confirmed
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

    // Expensive (sk:32) multimap.
    struct address
    {
        static constexpr size_t pk = schema::address_;
        static constexpr size_t sk = schema::hash;
        static constexpr size_t minsize = schema::put;
        static constexpr size_t minrow = pk + sk + minsize;
        static constexpr size_t size = minsize;
        static constexpr linkage<pk> count() NOEXCEPT { return 1; }
        static_assert(minsize == 5u);
        static_assert(minrow == 41u);
    };

    struct strong_bk
    {
        static constexpr size_t pk = schema::block;
        static constexpr size_t sk = schema::block;
        static constexpr size_t minsize = schema::code;
        static constexpr size_t minrow = pk + sk + minsize;
        static constexpr size_t size = minsize;
        static constexpr linkage<pk> count() NOEXCEPT { return 1; }
        static_assert(minsize == 1u);
        static_assert(minrow == 7u);
    };

    struct strong_tx
    {
        static constexpr size_t pk = schema::tx;
        static constexpr size_t sk = schema::tx;
        static constexpr size_t minsize =
            schema::block +
            schema::block;
        static constexpr size_t minrow = pk + sk + minsize;
        static constexpr size_t size = minsize;
        static constexpr linkage<pk> count() NOEXCEPT { return 1; }
        static_assert(minsize == 6u);
        static_assert(minrow == 14u);
    };

    /// Cache tables.
    /// -----------------------------------------------------------------------
}

} // namespace database
} // namespace libbitcoin

#endif
