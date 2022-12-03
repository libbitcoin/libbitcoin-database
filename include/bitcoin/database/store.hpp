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

////#include <bitcoin/database/tables/indexes/address.hpp>
////#include <bitcoin/database/tables/indexes/confirmed_block.hpp>
////#include <bitcoin/database/tables/indexes/confirmed_tx.hpp>
#include <bitcoin/database/tables/indexes/height.hpp>
////#include <bitcoin/database/tables/indexes/input_tx.hpp>
////#include <bitcoin/database/tables/indexes/output_tx.hpp>
////#include <bitcoin/database/tables/indexes/spent_output.hpp>

namespace libbitcoin {
namespace database {

/// The store and query interface are the primary products of database.
/// Store provides implmentation support for the public query interface.
/// Query privides query interface implmentation over the store.
template <typename Storage, if_base_of<storage, Storage> = true>
class store
{
public:
    using transactor = std::shared_lock<boost::upgrade_mutex>;

    DELETE5(store);

    /// Construct a store from settings.
    store(const settings& config) NOEXCEPT;

    /// Create the set of empty files (from unloaded).
    code create() NOEXCEPT;

    /// Open and load the set of tables, set locks.
    code open() NOEXCEPT;

    /// Snapshot the set of tables (from loaded).
    code snapshot() NOEXCEPT;

    /// Restore the most recent snapshot (from unloaded).
    code restore() NOEXCEPT;

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
    table::height candidate_height;
    table::height confirmed_height;
    ////table::confirmed_block confirmed_block;
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
    code open_load() NOEXCEPT;
    code unload_close() NOEXCEPT;
    code backup() NOEXCEPT;
    code dump(const std::filesystem::path& folder) NOEXCEPT;

    // These are thread safe.
    const settings& configuration_;

    /// Archives.
    /// -----------------------------------------------------------------------

    // record hashmap
    Storage header_head_;
    Storage header_body_;

    // record hashmap
    Storage point_head_;
    Storage point_body_;

    // slab hashmap
    Storage input_head_;
    Storage input_body_;

    // blob
    Storage output_head_;
    Storage output_body_;

    // array
    Storage puts_head_;
    Storage puts_body_;

    // record hashmap
    Storage tx_head_;
    Storage tx_body_;

    // slab hashmap
    Storage txs_head_;
    Storage txs_body_;

    /// Indexes.
    /// -----------------------------------------------------------------------

    // array
    Storage candidate_height_head_;
    Storage candidate_height_body_;

    // array
    Storage confirmed_height_head_;
    Storage confirmed_height_body_;

    /// Locks.
    /// -----------------------------------------------------------------------

    // These are protected by mutex.
    flush_lock flush_lock_;
    interprocess_lock process_lock_;
    boost::upgrade_mutex transactor_mutex_;

private:
    using path = std::filesystem::path;

    static inline path head(const path& folder, const std::string& name) NOEXCEPT
    {
        return folder / (name + schema::ext::head);
    }

    static inline path body(const path& folder, const std::string& name) NOEXCEPT
    {
        return folder / (name + schema::ext::data);
    }

    static inline path lock(const path& folder, const std::string& name) NOEXCEPT
    {
        return folder / (name + schema::ext::lock);
    }
};

} // namespace database
} // namespace libbitcoin

#define TEMPLATE template <typename Storage, if_base_of<storage, Storage> If>
#define CLASS store<Storage, If>

#include <bitcoin/database/impl/store.ipp>

#undef CLASS
#undef TEMPLATE

#endif

