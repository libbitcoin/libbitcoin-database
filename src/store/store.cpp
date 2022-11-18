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
#include <bitcoin/database/store/store.hpp>

#include <memory>
#include <shared_mutex>
#include <bitcoin/database/define.hpp>

// Any call to arraymap/hashmap put (or allocate/set/commit) requires a shared
// lock on the transactor mutex. The store is in a consistent state when all
// shares are released (no open transactions). Store create, startup, shutdown
// and snapshotting all require a unique lock on the transactor. Try-locks are
// used by all but snapshot, as thread coalescence implies all shares released.
// Snapshot blocks all transaction requests for the store, just as remap blocks
// all memory access requests for a given memory map (file). Snapshot may be
// completed with ongoing reads, though threads will be blocked at the p2p and 
// client-server network interfaces.

// The query inteface can access a store transactor factory, with shared mutex
// RAII semantics, just as with the memory accessor (though with no methods). A
// transactor must never exceed its creation scope and must have a provably
// non-reentrant execution path, just as memory accessors. So transactor issue
// is protected and lifetime is within the scope of a query call. A transactor
// is limited to the smallest scope where database integrity is assured.

// The transactor mutex would allow write detection for open/close operations,
// however it cannot detect read operations. Read/write is guarded at the
// file level within the tables. However the transactor is overloaded to order
// calls to the thread-unsafe process_lock_ and flush_lock_.

namespace libbitcoin {
namespace database {

store::store(const settings& config) NOEXCEPT
  : configuration_(config),
    flush_lock_(config.dir / "flush.lck"),
    process_lock_(config.dir / "process.lck"),

    header_head_(config.dir / "archive_header.idx"),
    header_body_(config.dir / "archive_header.dat", config.size, config.rate),
    header(header_head_, header_body_, config.buckets),

    point_head_(config.dir / "archive_point.idx"),
    point_body_(config.dir / "archive_point.dat", config.size, config.rate),
    point(point_head_, point_body_, config.buckets),

    input_head_(config.dir / "archive_input.idx"),
    input_body_(config.dir / "archive_input.dat", config.size, config.rate),
    input(input_head_, input_body_, config.buckets),

    output_body_(config.dir / "archive_output.dat", config.size, config.rate),
    output(output_body_),

    puts_body_(config.dir / "archive_puts.dat", config.size, config.rate),
    puts(puts_body_),

    transaction_head_(config.dir / "archive_tx.idx"),
    transaction_body_(config.dir / "archive_tx.dat", config.size, config.rate),
    transaction(transaction_head_, transaction_body_, config.buckets),

    txs_head_(config.dir / "archive_txs.idx"),
    txs_body_(config.dir / "archive_txs.dat", config.size, config.rate),
    txs(txs_head_, txs_body_, config.buckets)
{
}

bool store::create() NOEXCEPT
{
    return file::clear(configuration_.dir)
        && file::create(configuration_.dir / "archive_header.idx")
        && file::create(configuration_.dir / "archive_header.dat")
        && file::create(configuration_.dir / "archive_point.idx")
        && file::create(configuration_.dir / "archive_point.dat")
        && file::create(configuration_.dir / "archive_input.idx")
        && file::create(configuration_.dir / "archive_input.dat")
        && file::create(configuration_.dir / "archive_output.dat")
        && file::create(configuration_.dir / "archive_puts.dat")
        && file::create(configuration_.dir / "archive_tx.idx")
        && file::create(configuration_.dir / "archive_tx.dat")
        && file::create(configuration_.dir / "archive_txs.idx")
        && file::create(configuration_.dir / "archive_txs.dat");
}

bool store::open() NOEXCEPT
{
    // TODO: log/report transactor/process/flush/open failure code.
    if (transactor_mutex_.try_lock())
    {
        const auto result = process_lock_.try_lock() && flush_lock_.try_lock();

        // TODO: if (result) open tables.

        transactor_mutex_.unlock();
        return result;
    }

    return false;
}

bool store::snapshot() NOEXCEPT
{
    while (!transactor_mutex_.try_lock_for(boost::chrono::seconds(1)))
    {
        // TODO: log deadlock_hint
    }

    transactor_mutex_.unlock();
    return true;
}

bool store::close() NOEXCEPT
{
    // TODO: log/report transactor/close/process/flush failure code.
    if (transactor_mutex_.try_lock())
    {
        // TODO: close tables.

        const auto result = process_lock_.try_unlock() && flush_lock_.try_unlock();
        transactor_mutex_.unlock();
        return result;
    }

    return false;
}

void store::open_transaction() NOEXCEPT
{
    transactor_mutex_.lock_shared();
}

void store::close_transaction() NOEXCEPT
{
    transactor_mutex_.unlock_shared();
}

} // namespace database
} // namespace libbitcoin
