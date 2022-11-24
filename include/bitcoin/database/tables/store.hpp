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
#ifndef LIBBITCOIN_DATABASE_TABLES_STORE_HPP
#define LIBBITCOIN_DATABASE_TABLES_STORE_HPP

#include <filesystem>
#include <shared_mutex>
#include <bitcoin/database/boost.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/settings.hpp>
#include <bitcoin/database/locks/locks.hpp>
#include <bitcoin/database/memory/memory.hpp>
#include <bitcoin/database/tables/schema.hpp>

#include <bitcoin/database/tables/archives/header.hpp>
#include <bitcoin/database/tables/archives/input.hpp>
#include <bitcoin/database/tables/archives/output.hpp>
#include <bitcoin/database/tables/archives/point.hpp>
#include <bitcoin/database/tables/archives/puts.hpp>
#include <bitcoin/database/tables/archives/transaction.hpp>
#include <bitcoin/database/tables/archives/txs.hpp>

////#include <bitcoin/database/tables/caches/bootstrap.hpp>
////#include <bitcoin/database/tables/caches/buffer.hpp>
////#include <bitcoin/database/tables/caches/neutrino.hpp>
////#include <bitcoin/database/tables/caches/validated_block.hpp>
////#include <bitcoin/database/tables/caches/validated_tx.hpp>
////
////#include <bitcoin/database/tables/indexes/address.hpp>
////#include <bitcoin/database/tables/indexes/candidate_height.hpp>
////#include <bitcoin/database/tables/indexes/confirmed_block.hpp>
////#include <bitcoin/database/tables/indexes/confirmed_height.hpp>
////#include <bitcoin/database/tables/indexes/confirmed_tx.hpp>
////#include <bitcoin/database/tables/indexes/input_tx.hpp>
////#include <bitcoin/database/tables/indexes/output_tx.hpp>
////#include <bitcoin/database/tables/indexes/spent_output.hpp>

namespace libbitcoin {
namespace database {

/// The store and query interface are the primary products of database.
/// Store provides implmentation support for the public query interface.
/// Query privides query interface implmentation over the store.
class BCD_API store
{
public:
    using transactor = std::shared_lock<boost::upgrade_mutex>;

    DELETE5(store);

    /// Construct a store from settings.
    store(const settings& config) NOEXCEPT;

    /// Clear store directory and the set of empty files.
    code create() NOEXCEPT;

    /// Open and load the set of tables, set locks.
    code open() NOEXCEPT;

    /// Snapshot the set of tables.
    /// Pause writes, set body sizes, flush files, copy headers, swap backups.
    code snapshot() NOEXCEPT;

    /// Unload and close the set of tables, clear locks.
    code close() NOEXCEPT;

    /// Get a transactor object.
    const transactor get_transactor() NOEXCEPT;

    /// Archives.
    table::header header;
    table::point point;
    table::input input;
    table::output output;
    table::puts puts;
    table::transaction tx;
    table::txs txs;

    /// Indexes.
    ////table::address address;
    ////table::candidate_height candidate_height;
    ////table::confirmed_block confirmed_block;
    ////table::confirmed_height confirmed_height;
    ////table::confirmed_tx confirmed_tx;
    ////table::input_tx input_tx;
    ////table::output_tx output_tx;
    ////table::spent_output spent_output;

    /// Caches.
    ////table::bootstrap bootstrap;
    ////table::buffer buffer;
    ////table::neutrino neutrino;
    ////table::validated_block validated_block;
    ////table::validated_tx validated_tx;

protected:
    /// Open/close.
    code open_load() NOEXCEPT;
    code unload_close() NOEXCEPT;

    /// Backup/restore all indexes.
    code backup() NOEXCEPT;
    code dump(const std::filesystem::path& folder) NOEXCEPT;
    code restore() NOEXCEPT;

    // These are thread safe.
    const settings& configuration_;

    // record hashmap
    map header_head_;
    map header_body_;

    // record hashmap
    map point_head_;
    map point_body_;

    // slab hashmap
    map input_head_;
    map input_body_;

    // blob
    map output_head_;
    map output_body_;

    // array
    map puts_head_;
    map puts_body_;

    // record hashmap
    map tx_head_;
    map tx_body_;

    // slab hashmap
    map txs_head_;
    map txs_body_;

    // These are protected by mutex.
    flush_lock flush_lock_;
    interprocess_lock process_lock_;
    boost::upgrade_mutex transactor_mutex_;
};

} // namespace database
} // namespace libbitcoin

#endif

