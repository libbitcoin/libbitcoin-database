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
#ifndef LIBBITCOIN_DATABASE_QUERY_BATCH_SILENT_IPP
#define LIBBITCOIN_DATABASE_QUERY_BATCH_SILENT_IPP

#include <algorithm>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/tables/tables.hpp>
#include <bitcoin/database/types/types.hpp>

namespace libbitcoin {
namespace database {

TEMPLATE
bool CLASS::scan_silent(const stopper& cancel, const ec_secret& scan_key,
    const silent_handler& callback) NOEXCEPT
{
    const auto prefix_ptr = store_.silent.prefix.get_memory();
    const auto compressed_ptr = store_.silent.compressed.get_memory();
    const auto correlate_ptr = store_.silent.correlate.get_memory();

    using correlate_t = const table::silent_correlate::span;
    using prefix_t = const table::silent_prefix::span;
    using compressed_t = const table::silent_compressed::span;

    using namespace system;
    const auto correlate = pointer_cast<correlate_t>(correlate_ptr.data());
    const auto prefix = pointer_cast<prefix_t>(prefix_ptr.data());
    const auto compressed = pointer_cast<compressed_t>(compressed_ptr.data());

    // Shortest column.
    const auto count = store_.silent.count();
    const silent::batch batch
    {
        .correlates = { correlate, count },
        .prefixes = { prefix, count },
        .points = { compressed, count }
    };

    // False return only implies canceled.
    // Callbacks invoked on caller thread if turbo is false.
    silent::batch::scan(cancel, batch, scan_key, callback, store_.turbo());
    return !cancel;
}

// setters
// ----------------------------------------------------------------------------
// Caller (node) controls which txs are indexed (e.g. by confirmed height).

TEMPLATE
bool CLASS::set_silent(const header_link& link, const block& block) NOEXCEPT
{
    const auto& txs = block.transactions_ptr();
    const auto count = txs->size();
    if (is_one(count))
        return true;

    const auto links = to_transactions(link);
    if (links.size() != count)
        return false;

    stopper fail{};
    std::vector<size_t> it(sub1(count));
    std::iota(it.begin(), it.end(), one);
    constexpr auto parallel = poolstl::execution::par;
    constexpr auto relaxed = std::memory_order_relaxed;

    // TODO: parallel may or may not be optimal.
    // TODO: alternatively could accumulate block results and write once.
    std::for_each(parallel, it.cbegin(), it.cend(), [&](size_t index) NOEXCEPT
    {
        if (fail.load(relaxed))
            return;

        if (!set_silent(links.at(index), *txs->at(index)))
            fail.store(true, relaxed);
    });
    
    return !fail.load(relaxed);
}

TEMPLATE
bool CLASS::set_silent(const tx_link& link,
    const transaction& BC_DEBUG_ONLY(tx)) NOEXCEPT
{
    BC_ASSERT(!tx.is_coinbase());
    if (link.is_terminal())
        return false;

    // Short-circuits with success on empty.
    ////using namespace system::wallet;
    ////silent_payment::scan_record record{};
    ////if (!silent_payment::compute_scan_record(record, tx))
    ////    return true;

    // TODO: aliases for record above;
    const ec_compressed key{};
    const std::vector<uint64_t> prefixes{};

    using correlate_t = table::silent_correlate::records;
    using prefix_t = table::silent_prefix::put_ref;
    using compressed_t = table::silent_compressed::put_ref;

    // TODO: caller must guard reads, this is writing into hot storage.
    // ========================================================================
    const auto scope = get_transactor();

    using namespace system;
    auto rows = possible_narrow_cast<silent_link::integer>(prefixes.size());

    // Allocate rows across all columns.
    // TODO: this could provide a single remap lock for all puts below.
    const auto fk = store_.silent.allocate(rows);
    if (fk.is_terminal())
        return false;

    // Write values to each column in corresponding positions.
    return
        store_.silent.correlate.put(fk, correlate_t{ {}, rows, link }) &&
        store_.silent.prefix.put(fk, prefix_t{ {}, prefixes }) &&
        store_.silent.compressed.put(fk, compressed_t{ {}, rows, key });
    // ========================================================================
}

} // namespace database
} // namespace libbitcoin

#endif
