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
#ifndef LIBBITCOIN_DATABASE_STORE_OPEN_IPP
#define LIBBITCOIN_DATABASE_STORE_OPEN_IPP

#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

// public
TEMPLATE
code CLASS::open(const event_handler& handler) NOEXCEPT
{
    if (!file::is_directory(configuration_.path))
        return error::missing_directory;

    if (!transactor_mutex_.try_lock())
        return error::transactor_lock;

    if (!process_lock_.try_lock())
    {
        transactor_mutex_.unlock();
        return error::process_lock;
    }

    if (!flush_lock_.try_lock())
    {
        /* bool */ process_lock_.try_unlock();
        transactor_mutex_.unlock();
        return error::flush_lock;
    }

    const auto verify = [&handler](code& ec, auto& logical,
        table_t table) NOEXCEPT
    {
        if (!ec)
        {
            handler(event_t::verify_table, table);
            if (!logical.verify())
                ec = error::verify_table;
        }
    };

    auto ec = open_load(handler);

    // The stored envelope governs sizing, so it precedes verification.
    if (!ec)
        ec = load_envelope();

    verify(ec, header, table_t::header_table);
    verify(ec, input, table_t::input_table);
    verify(ec, output, table_t::output_table);
    verify(ec, ins, table_t::ins_table);
    verify(ec, outs, table_t::outs_table);
    verify(ec, tx, table_t::tx_table);
    verify(ec, txs, table_t::txs_table);

    verify(ec, candidate, table_t::candidate_table);
    verify(ec, confirmed, table_t::confirmed_table);
    verify(ec, strong_tx, table_t::strong_tx_table);

    verify(ec, ecdsa, table_t::ecdsa_table);
    verify(ec, schnorr, table_t::schnorr_table);
    verify(ec, silent, table_t::silent_table);
    verify(ec, envelope, table_t::envelope_table);
    verify(ec, duplicate, table_t::duplicate_table);
    verify(ec, prevalid, table_t::prevalid_table);
    verify(ec, prevout, table_t::prevout_table);
    verify(ec, validated_bk, table_t::validated_bk_table);
    verify(ec, validated_tx, table_t::validated_tx_table);
    verify(ec, spends, table_t::spends_table);

    verify(ec, filter_bk, table_t::filter_bk_table);
    verify(ec, filter_tx, table_t::filter_tx_table);

    if (ec)
    {
        /* code */ unload_close(handler);

        // unlock errors override ec.
        if (!flush_lock_.try_unlock()) ec = error::flush_unlock;
        if (!process_lock_.try_unlock()) ec = error::process_unlock;
    }

    // process and flush locks remain open until close().
    transactor_mutex_.unlock();
    return ec;
}

// The envelope is fixed width, so it is rewritten at its allocated link.
TEMPLATE
void CLASS::store_envelope(code& ec) NOEXCEPT
{
    if (ec)
        return;

    const table::envelope::record record{ envelope_ };
    if (is_zero(envelope.head_size()) &&
        (!envelope.reserve(record.count()) ||
            envelope.allocate(record.count()).is_terminal()))
    {
        ec = error::create_table;
        return;
    }

    if (!envelope.put(zero, record))
        ec = error::create_table;
}

TEMPLATE
code CLASS::load_envelope() NOEXCEPT
{
    // The stored creation envelope governs, configuration is not read.
    table::envelope::record record{};
    if (!is_zero(envelope.head_size()))
    {
        if (!envelope.get(zero, record))
            return record.envelope.schema == envelope_.schema ?
                error::verify_table : error::schema_version;

        envelope_ = record.envelope;
    }

    // The stored latch governs, it is one-way and survives the session.
    if (envelope_.pooling)
        pooling_.store(true, std::memory_order_relaxed);

    // The stored bucket counts govern the heads.
    if (!header.set_buckets(envelope_.header_buckets) ||
        !ins.set_buckets(envelope_.ins_buckets) ||
        !outs.set_buckets(envelope_.outs_buckets) ||
        !tx.set_buckets(envelope_.tx_buckets) ||
        !strong_tx.set_buckets(envelope_.strong_tx_buckets) ||
        !duplicate.set_buckets(envelope_.duplicate_buckets) ||
        !validated_tx.set_buckets(envelope_.validated_tx_buckets))
    {
        return error::verify_table;
    }

    // The stored filter k values govern filter bit interpretation.
    if (!header.set_filter_k(envelope_.header_k) ||
        !ins.set_filter_k(envelope_.ins_k) ||
        !outs.set_filter_k(envelope_.outs_k) ||
        !tx.set_filter_k(envelope_.tx_k) ||
        !strong_tx.set_filter_k(envelope_.strong_tx_k) ||
        !duplicate.set_filter_k(envelope_.duplicate_k) ||
        !validated_tx.set_filter_k(envelope_.validated_tx_k))
    {
        return error::verify_table;
    }

    return error::success;
}

} // namespace database
} // namespace libbitcoin

#endif
