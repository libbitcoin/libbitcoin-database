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

#include <bitcoin/database/define.hpp>
#include <bitcoin/database/types/types.hpp>

namespace libbitcoin {
namespace database {

TEMPLATE
bool CLASS::scan_silent(const stopper& cancel, const ec_secret& scan_key,
    const silent_handler& callback) NOEXCEPT
{
    // False return only implies canceled.
    using batch = system::silent::batch;
    const auto count = store_.silent.count().value;
    const auto ptr = store_.silent.get_memory();
    const auto rows = system::pointer_cast<const batch>(ptr->data());
    batch::scan(cancel, { rows, count }, scan_key, callback);
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
bool CLASS::set_silent(const tx_link& link, const transaction& tx) NOEXCEPT
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

    using silent_prefix = table::silent_prefix::put_ref;
    using silent_compressed = table::silent_compressed::put_ref;
    using silent_correlate = table::silent_correlate::records;

    // ========================================================================
    const auto scope = store_.get_transactor();

    silent_link fk{};
    const auto rows = prefixes.size();

    // Allocate contiguous rows at fk and fill with link (x rows).
    store_.silent_correlate.put_link(fk, silent_correlate{ {}, rows, link });

    // Expand subordinate tables to same size, as necessary.
    if (!store_.silent_prefix.expand(fk + rows) ||
        !store_.silent_compressed.expand(fk + rows))
        return false;

    // Write prefixes (rows) into corresponding fk position.
    if (!store_.silent_prefix.put(fk, silent_prefix{ {}, prefixes }))
        return false;

    // Write compressed (x rows) into corresponding fk position.
    if (!store_.silent_compressed.put(fk, silent_compressed{ {}, rows, key }))
        return false;

    return true;
    // ========================================================================
}

} // namespace database
} // namespace libbitcoin

#endif
