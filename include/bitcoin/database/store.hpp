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
#ifndef LIBBITCOIN_DATABASE_TABLES_STORE_HPP
#define LIBBITCOIN_DATABASE_TABLES_STORE_HPP

#include <filesystem>
#include <functional>
#include <shared_mutex>
#include <bitcoin/database/boost.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/settings.hpp>
#include <bitcoin/database/locks/locks.hpp>
#include <bitcoin/database/memory/memory.hpp>
#include <bitcoin/database/tables/schema.hpp>
#include <bitcoin/database/tables/tables.hpp>

namespace libbitcoin {
namespace database {

enum class event_t
{
    create_file,
    open_file,
    load_file,
    unload_file,
    close_file,

    create_table,
    verify_table,
    close_table
};

enum class table_t
{
    header_table,
    header_head,
    header_body,
    point_table,
    point_head,
    point_body,
    input_table,
    input_head,
    input_body,
    output_table,
    output_head,
    output_body,
    puts_table,
    puts_head,
    puts_body,
    tx_table,
    tx_head,
    txs_table,
    tx_body,
    txs_head,
    txs_body,

    address_table,
    address_head,
    address_body,
    candidate_table,
    candidate_head,
    candidate_body,
    confirmed_table,
    confirmed_head,
    confirmed_body,
    spend_table,
    spend_head,
    spend_body,
    strong_tx_table,
    strong_tx_head,
    strong_tx_body,

    bootstrap_table,
    bootstrap_head,
    bootstrap_body,
    buffer_table,
    buffer_head,
    buffer_body,
    neutrino_table,
    neutrino_head,
    neutrino_body,
    validated_bk_table,
    validated_bk_head,
    validated_bk_body,
    validated_tx_table,
    validated_tx_head,
    validated_tx_body
};

/// The store and query interface are the primary products of database.
/// Store provides implmentation support for the public query interface.
/// Query privides query interface implmentation over the store.
/// Event handlers are invoked synchronously, providing progress.
template <typename Storage, if_base_of<storage, Storage> = true>
class store
{
public:
    DELETE_COPY_MOVE_DESTRUCT(store);

    typedef std::shared_lock<boost::upgrade_mutex> transactor;
    typedef std::function<void(event_t, table_t)> event_handler;

    /// Construct a store from settings.
    store(const settings& config) NOEXCEPT;

    /// Create the set of empty files (from unloaded).
    code create(const event_handler& handler) NOEXCEPT;

    /// Open and load the set of tables, set locks.
    code open(const event_handler& handler) NOEXCEPT;

    /// Snapshot the set of tables (from loaded).
    code snapshot(const event_handler& handler) NOEXCEPT;

    /// Restore the most recent snapshot (from unloaded).
    code restore(const event_handler& handler) NOEXCEPT;

    /// Unload and close the set of tables, clear locks.
    code close(const event_handler& handler) NOEXCEPT;

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
    table::address address;
    table::height candidate;
    table::height confirmed;
    table::spend spend;
    table::strong_tx strong_tx;

    /// Caches.
    table::bootstrap bootstrap;
    table::buffer buffer;
    table::neutrino neutrino;
    table::validated_bk validated_bk;
    table::validated_tx validated_tx;

protected:
    code open_load(const event_handler& handler) NOEXCEPT;
    code unload_close(const event_handler& handler) NOEXCEPT;
    code backup(const event_handler& handler) NOEXCEPT;
    code dump(const std::filesystem::path& folder,
        const event_handler& handler) NOEXCEPT;

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

    // record hashmap
    Storage address_head_;
    Storage address_body_;

    // array
    Storage candidate_head_;
    Storage candidate_body_;

    // array
    Storage confirmed_head_;
    Storage confirmed_body_;

    // record hashmap
    Storage spend_head_;
    Storage spend_body_;

    // record hashmap
    Storage strong_tx_head_;
    Storage strong_tx_body_;

    /// Caches.
    /// -----------------------------------------------------------------------

    // array
    Storage bootstrap_head_;
    Storage bootstrap_body_;

    // slab hashmap
    Storage buffer_head_;
    Storage buffer_body_;

    // slab hashmap
    Storage neutrino_head_;
    Storage neutrino_body_;

    // record hashmap
    Storage validated_bk_head_;
    Storage validated_bk_body_;

    // record multimap
    Storage validated_tx_head_;
    Storage validated_tx_body_;

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

