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
#ifndef LIBBITCOIN_DATABASE_QUERY_ARCHIVE_WIRE_READER_IPP
#define LIBBITCOIN_DATABASE_QUERY_ARCHIVE_WIRE_READER_IPP

#include <algorithm>
#include <utility>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

// Reader directly from store to wire-encoded buffer (store to network).
// ----------------------------------------------------------------------------
// Due to the stoopid segwit serialization of witness after output there is are
// duplicated navigations to store_.ins and store_.input by the witness reader.
// This normalized approach is also the most efficient.

TEMPLATE
bool CLASS::get_wire_header(bytewriter& sink,
    const header_link& link) const NOEXCEPT
{
    // Double read of header table is small, prevents need for writer rewind.
    const auto parent = to_parent(link);
    table::header::wire_header header{ {}, sink, get_header_key(parent) };
    return store_.header.get(link, header);
}

TEMPLATE
bool CLASS::get_wire_input(bytewriter& sink,
    const point_link& link) const NOEXCEPT
{
    table::point::wire_point point{ {}, sink };
    if (!store_.point.get(link, point))
        return false;

    table::ins::get_input ins{};
    table::input::wire_script script{ {}, sink };
    if (!store_.ins.get(link, ins) ||
        !store_.input.get(ins.input_fk, script))
        return false;

    sink.write_4_bytes_little_endian(ins.sequence);
    return true;
}

TEMPLATE
bool CLASS::get_wire_output(bytewriter& sink,
    const output_link& link) const NOEXCEPT
{
    table::output::wire_script out{ {}, sink };
    return store_.output.get(link, out);
}

TEMPLATE
bool CLASS::get_wire_witness(bytewriter& sink,
    const point_link& link) const NOEXCEPT
{
    table::ins::get_input ins{};
    table::input::wire_witness wire{ {}, sink };
    return store_.ins.get(link, ins)
        && store_.input.get(ins.input_fk, wire);
}

TEMPLATE
bool CLASS::get_wire_tx(bytewriter& sink, const tx_link& link,
    bool witness) const NOEXCEPT
{
    table::transaction::record tx{};
    if (!store_.tx.get(link, tx))
        return false;

    table::outs::record outs{};
    outs.out_fks.resize(tx.outs_count);
    if (!store_.outs.get(tx.outs_fk, outs))
        return false;

    // Point links are contiguous (computed).
    const auto ins_begin = tx.point_fk;
    const auto ins_count = tx.ins_count;
    const auto ins_final = ins_begin + ins_count;
    const auto witnessed = witness && (tx.heavy != tx.light);

    sink.write_4_bytes_little_endian(tx.version);

    if (witnessed)
    {
        sink.write_byte(system::chain::witness_marker);
        sink.write_byte(system::chain::witness_enabled);
    }

    sink.write_variable(ins_count);
    for (auto fk = ins_begin; fk < ins_final; ++fk)
        if (!get_wire_input(sink, fk))
            return false;

    sink.write_variable(outs.out_fks.size());
    for (const auto& fk: outs.out_fks)
        if (!get_wire_output(sink, fk))
            return false;

    if (witnessed)
    {
        for (auto fk = ins_begin; fk < ins_final; ++fk)
            if (!get_wire_witness(sink, fk))
                return false;
    }

    sink.write_4_bytes_little_endian(tx.locktime);
    return true;
}

TEMPLATE
bool CLASS::get_wire_block(bytewriter& sink, const header_link& link,
    bool witness) const NOEXCEPT
{
    if (!get_wire_header(sink, link))
        return false;

    const auto txs = to_transactions(link);
    if (txs.empty())
        return false;

    sink.write_variable(txs.size());
    for (const auto& tx_link: txs)
        if (!get_wire_tx(sink, tx_link, witness))
            return false;

    return true;
}

// These convenience wrappers are made practical by size caching for block and
// tx for both nominal and witness wire encodings (and fixed size headers).
// Intermediate objects (input, output, witness) have a size prefix 

TEMPLATE
data_chunk CLASS::get_wire_header(const header_link& link) const NOEXCEPT
{
    using namespace system;
    data_chunk data(chain::header::serialized_size());

    stream::flip::fast ostream(data);
    flip::bytes::fast out(ostream);
    if (!get_wire_header(out, link) || !out)
        return {};

    return data;
}

TEMPLATE
data_chunk CLASS::get_wire_tx(const tx_link& link, bool witness) const NOEXCEPT
{
    using namespace system;
    size_t size{};
    if (!get_tx_size(size, link, witness))
        return {};

    data_chunk data(size);
    stream::flip::fast ostream(data);
    flip::bytes::fast out(ostream);
    if (!get_wire_tx(out, link, witness) || !out)
        return {};

    return data;
}

TEMPLATE
data_chunk CLASS::get_wire_block(const header_link& link,
    bool witness) const NOEXCEPT
{
    using namespace system;
    size_t size{};
    if (!get_block_size(size, link, witness))
        return {};

    data_chunk data(size);
    stream::flip::fast ostream(data);
    flip::bytes::fast out(ostream);
    if (!get_wire_block(out, link, witness) || !out)
        return {};

    return data;
}

} // namespace database
} // namespace libbitcoin

#endif
