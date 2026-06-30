/**
 * Copyright (c) 2011-2026 libbitcoin developers (see AUTHORS)
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
#ifndef LIBBITCOIN_DATABASE_STORE_PRUNE_IPP
#define LIBBITCOIN_DATABASE_STORE_PRUNE_IPP

#include <chrono>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

// public
TEMPLATE
code CLASS::prune(const event_handler& handler) NOEXCEPT
{
    // Transactor lock generally only covers writes, but in this case prevout
    // reads must also be guarded since the body shrinks and head is cleared.
    while (!transactor_mutex_.try_lock_for(std::chrono::seconds(1)))
    {
        handler(event_t::wait_lock, table_t::store);
    }

    code ec{ error::success };

    // Prevouts resettable if all candidates confirmed (fork is candidate top).
    if (!query<CLASS>{ *this }.is_coalesced())
    {
        ec = error::not_coalesced;
    }
    else
    {
        // nullify table head, set reference body count to zero.
        handler(event_t::prune_table, table_t::prevout_head);
        if (!prevout.clear())
        {
            ec = error::prune_table;
        }
        else
        {
            // Snapshot with nullified head and zero body count.
            // The 'prune' parameter signals to not reset body count.
            ec = snapshot(handler, true);

            // If the pruning fails here the snapshot remains valid.
            if (!ec)
            {
                // zeroize table body, set logical body count to zero.
                handler(event_t::prune_table, table_t::prevout_body);
                if (!prevout_body_.truncate(0))
                {
                    ec = error::prune_table;
                }
                else
                {
                    // TODO: could move unload+load into map as shrink().

                    // unmap body, setting mapped size to logical size (zero).
                    handler(event_t::unload_file, table_t::prevout_body);
                    ec = prevout_body_.unload();

                    if (!ec)
                    {
                        // map body, making table usable again.
                        handler(event_t::load_file, table_t::prevout_body);
                        ec = prevout_body_.load();
                    }
                }
            }
        }
    }

    transactor_mutex_.unlock();
    return ec;
}

} // namespace database
} // namespace libbitcoin

#endif
