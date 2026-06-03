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
#ifndef LIBBITCOIN_DATABASE_QUERY_SILENT_IPP
#define LIBBITCOIN_DATABASE_QUERY_SILENT_IPP

#include <utility>
#include <bitcoin/database/define.hpp>
#include <bitcoin/system/wallet/addresses/silent_payment.hpp>

namespace libbitcoin {
namespace database {

// silent
// ----------------------------------------------------------------------------

namespace payment = system::wallet::silent_payment;

static inline bool to_silent_record(silent_record& out,
    const tx_link& tx, const system::chain::transaction& transaction) NOEXCEPT
{
    payment::scan_record record{};
    if (!payment::compute_scan_record(record, transaction))
        return false;

    out = { tx, record.prevouts_summary, std::move(record.outputs) };
    return true;
}

TEMPLATE
bool CLASS::is_silent_indexed(const header_link& link) const NOEXCEPT
{
    return store_.silent.exists(to_silent(link));
}

TEMPLATE
bool CLASS::get_silent(silent& out, const header_link& link) const NOEXCEPT
{
    table::silent::get_records records{};
    if (!store_.silent.at(to_silent(link), records))
        return false;

    out = std::move(records.value);
    return true;
}

TEMPLATE
bool CLASS::set_silent(silent& out, const tx_link& link,
    const transaction& tx) const NOEXCEPT
{
    if (link.is_terminal())
        return false;

    if (tx.is_coinbase())
        return true;

    if (!populate_without_metadata(tx))
        return false;

    silent_record record{};
    if (!to_silent_record(record, link, tx))
        return true;

    out.records.push_back(std::move(record));
    return true;
}

// node/validator
TEMPLATE
bool CLASS::set_silent(const header_link& link,
    const block& block) NOEXCEPT
{
    if (!silent_enabled())
        return true;

    const auto height = get_height(link);
    if (height.is_terminal())
        return false;

    if (height.value < silent_start_height())
        return true;

    const auto txs = to_transactions(link);
    const auto& transactions = *block.transactions_ptr();
    if (txs.size() != transactions.size())
        return false;

    database::silent value{};
    value.records.reserve(transactions.size());

    auto tx = txs.begin();
    for (const auto& transaction: transactions)
        if (!set_silent(value, *tx++, *transaction))
            return false;

    return set_silent(link, value);
}

TEMPLATE
bool CLASS::set_silent(const header_link& link,
    const silent& value) NOEXCEPT
{
    if (!silent_enabled())
        return true;

    const auto height = get_height(link);
    if (height.is_terminal())
        return false;

    if (height.value < silent_start_height())
        return true;

    // ========================================================================
    const auto scope = store_.get_transactor();

    // Clean single allocation failure (e.g. disk full).
    const table::silent::put_ref put{ {}, value };
    return store_.silent.put(to_silent(link), put);
    // ========================================================================
}

} // namespace database
} // namespace libbitcoin

#endif
