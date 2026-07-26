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
#ifndef LIBBITCOIN_DATABASE_SETTINGS_HPP
#define LIBBITCOIN_DATABASE_SETTINGS_HPP

#include <bitcoin/database/define.hpp>

#include <filesystem>

namespace libbitcoin {
namespace database {

/// 32 bit builds will warn on size downcasts.
/// TODO: helper methods to embed/construct full paths (see store()).
/// Common database configuration settings, properties not thread safe.
struct BCD_API settings
{
    DEFAULT_COPY_MOVE_DESTRUCT(settings);

    settings() NOEXCEPT;
    settings(system::chain::selection context) NOEXCEPT;

    /// Table settings.
    /// -----------------------------------------------------------------------

    /// Body storage sizing (all tables).
    struct simple_table
    {
        /// Minimum body allocation (bytes).
        uint64_t size{ 1 };

        /// Body expansion rate (percentage).
        uint16_t rate{ 50 };
    };

    /// Adds the head bucket count (hashmap hash space or arraymap length).
    struct bucket_table
      : public simple_table
    {
        /// Head bucket count.
        uint32_t buckets{ 128 };
    };

    /// Properties.
    /// -----------------------------------------------------------------------

    /// Enable concurrency in individual client-server queries.
    bool turbo{ false };

    /// Mark blocks as unconfirmable (disorganize vs. stall).
    bool mark_unconfirmable{ true };

    /// Depth of merkle tree interval caching (used at store create only).
    uint16_t interval_depth{ max_uint8 };

    /// Fork flags upon store creation (used at store create only).
    uint32_t fork_flags{};

    /// Path to the database directory.
    std::filesystem::path path{ "bitcoin" };

    /// Archives.
    /// -----------------------------------------------------------------------

    bucket_table header{};
    simple_table input{};
    simple_table output{};
    bucket_table ins{};
    simple_table outs{};
    bucket_table tx{};
    bucket_table txs{};

    /// Indexes.
    /// -----------------------------------------------------------------------

    simple_table candidate{};
    simple_table confirmed{};
    bucket_table strong_tx{};

    /// Caches.
    /// -----------------------------------------------------------------------

    simple_table ecdsa{};
    simple_table schnorr{};
    simple_table silent{};
    bucket_table duplicate{};
    simple_table prevalid{};
    bucket_table prevout{};
    bucket_table validated_bk{};
    bucket_table validated_tx{};

    /// Optionals.
    /// -----------------------------------------------------------------------

    bucket_table address{};
    bucket_table filter_bk{};
    bucket_table filter_tx{};
};

} // namespace database
} // namespace libbitcoin

#endif
