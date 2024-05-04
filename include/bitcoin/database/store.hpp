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
#ifndef LIBBITCOIN_DATABASE_STORE_HPP
#define LIBBITCOIN_DATABASE_STORE_HPP

#include <filesystem>
#include <functional>
#include <shared_mutex>
#include <bitcoin/database/boost.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/settings.hpp>
#include <bitcoin/database/locks/locks.hpp>
#include <bitcoin/database/memory/memory.hpp>
#include <bitcoin/database/tables/tables.hpp>

namespace libbitcoin {
namespace database {

/// The store and query interface are the primary products of database.
/// Store provides implmentation support for the public query interface.
/// Query privides query interface implmentation over the store.
/// Event handlers are invoked synchronously, providing progress.
template <typename Storage, if_base_of<storage, Storage> = true>
class store
{
public:
    DELETE_COPY_MOVE_DESTRUCT(store);

    typedef std::function<void(event_t, table_t)> event_handler;
    typedef std::shared_lock<std::shared_timed_mutex> transactor;

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

    /// Detect the specified exclusive error condition.
    bool get_error(const code& ec) const NOEXCEPT;

    /// Clear all error conditions.
    void clear_error() NOEXCEPT;

    /// Archives.
    table::header header;
    table::input input;
    table::output output;
    table::point point;
    table::puts puts;
    table::spend spend;
    table::transaction tx;
    table::txs txs;

    /// Indexes.
    table::height candidate;
    table::height confirmed;
    table::strong_tx strong_tx;

    /// Caches.
    table::validated_bk validated_bk;
    table::validated_tx validated_tx;

    /// Optionals.
    table::address address;
    table::neutrino neutrino;
    ////table::bootstrap bootstrap;
    ////table::buffer buffer;

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

    // slab hashmap
    Storage input_head_;
    Storage input_body_;

    // blob
    Storage output_head_;
    Storage output_body_;

    // record hashmap
    Storage point_head_;
    Storage point_body_;

    // array
    Storage puts_head_;
    Storage puts_body_;

    // record hashmap
    Storage spend_head_;
    Storage spend_body_;

    // record hashmap
    Storage tx_head_;
    Storage tx_body_;

    // slab hashmap
    Storage txs_head_;
    Storage txs_body_;

    /// Indexes.
    /// -----------------------------------------------------------------------

    // array
    Storage candidate_head_;
    Storage candidate_body_;

    // array
    Storage confirmed_head_;
    Storage confirmed_body_;

    // record hashmap
    Storage strong_tx_head_;
    Storage strong_tx_body_;

    /// Caches.
    /// -----------------------------------------------------------------------

    // record hashmap
    Storage validated_bk_head_;
    Storage validated_bk_body_;

    // record multimap
    Storage validated_tx_head_;
    Storage validated_tx_body_;

    /// Optionals.
    /// -----------------------------------------------------------------------

    // record hashmap
    Storage address_head_;
    Storage address_body_;

    // slab hashmap
    Storage neutrino_head_;
    Storage neutrino_body_;

    ////// array
    ////Storage bootstrap_head_;
    ////Storage bootstrap_body_;

    ////// slab hashmap
    ////Storage buffer_head_;
    ////Storage buffer_body_;

    /// Locks.
    /// -----------------------------------------------------------------------

    // These are protected by mutex.
    flush_lock flush_lock_;
    interprocess_lock process_lock_;
    std::shared_timed_mutex transactor_mutex_;

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
