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
#ifndef LIBBITCOIN_DATABASE_QUERY_WIRE_IPP
#define LIBBITCOIN_DATABASE_QUERY_WIRE_IPP

#include <algorithm>
#include <utility>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

// Wire serialized objects.
// ----------------------------------------------------------------------------
// Due to the idiotic segwit serialization of witness after output there is are
// duplicated navigations to store_.ins and store_.input by the witness reader.
// This normalized approach is also the most efficient.

TEMPLATE
bool CLASS::get_wire_header(byteflipper& flipper,
    const header_link& link) const NOEXCEPT
{
    const auto start = flipper.get_write_position();
    table::header::wire_header header{ {}, flipper };
    if (!store_.header.get(link, header))
    {
        flipper.invalidate();
        return false;
    }

    // Genesis header parent is defaulted.
    if (header.parent_fk == schema::header::link::terminal)
        return true;

    flipper.set_position(start);
    table::header::wire_key key{ {}, flipper };
    if (!store_.header.get(header.parent_fk, key))
    {
        flipper.invalidate();
        return false;
    }

    return true;
}

TEMPLATE
bool CLASS::get_wire_input(byteflipper& flipper,
    const point_link& link) const NOEXCEPT
{
    // [point]
    table::point::wire_point point{ {}, flipper };
    if (!store_.point.get(link, point))
    {
        flipper.invalidate();
        return false;
    }

    // [[size]script]
    table::ins::get_input ins{};
    table::input::wire_script script{ {}, flipper };
    if (!store_.ins.get(link, ins) ||
        !store_.input.get(ins.input_fk, script))
    {
        flipper.invalidate();
        return false;
    }

    // [sequence]
    flipper.write_4_bytes_little_endian(ins.sequence);
    return true;
}

TEMPLATE
bool CLASS::get_wire_output(byteflipper& flipper,
    const output_link& link) const NOEXCEPT
{
    // [value][[[size]script]]
    table::output::wire_script out{ {}, flipper };
    if (!store_.output.get(link, out))
    {
        flipper.invalidate();
        return false;
    }

    return true;
}

TEMPLATE
bool CLASS::get_wire_witness(byteflipper& flipper,
    const point_link& link) const NOEXCEPT
{
    // [count][[[size]element]]
    table::ins::get_input ins{};
    table::input::wire_witness wire{ {}, flipper };
    if (!store_.ins.get(link, ins) ||
        !store_.input.get(ins.input_fk, wire))
    {
        flipper.invalidate();
        return false;
    }

    return true;
}

TEMPLATE
bool CLASS::get_wire_tx(byteflipper& flipper, const tx_link& link,
    bool witness) const NOEXCEPT
{
    table::transaction::record tx{};
    if (!store_.tx.get(link, tx))
    {
        flipper.invalidate();
        return false;
    }

    table::outs::record outs{};
    outs.out_fks.resize(tx.outs_count);
    if (!store_.outs.get(tx.outs_fk, outs))
    {
        flipper.invalidate();
        return false;
    }

    // Point links are contiguous (computed).
    const auto ins_begin = tx.point_fk;
    const auto ins_count = tx.ins_count;
    const auto ins_final = ins_begin + ins_count;
    const auto witnessed = witness && (tx.heavy != tx.light);

    flipper.write_4_bytes_little_endian(tx.version);

    if (witnessed)
    {
        flipper.write_byte(system::chain::witness_marker);
        flipper.write_byte(system::chain::witness_enabled);
    }

    flipper.write_variable(ins_count);
    for (auto fk = ins_begin; fk < ins_final; ++fk)
        if (!get_wire_input(flipper, fk))
            return false;

    flipper.write_variable(outs.out_fks.size());
    for (const auto& fk: outs.out_fks)
        if (!get_wire_output(flipper, fk))
            return false;

    if (witnessed)
    {
        for (auto fk = ins_begin; fk < ins_final; ++fk)
            if (!get_wire_witness(flipper, fk))
                return false;
    }

    flipper.write_4_bytes_little_endian(tx.locktime);
    return true;
}

TEMPLATE
bool CLASS::get_wire_block(byteflipper& flipper, const header_link& link,
    bool witness) const NOEXCEPT
{
    if (!get_wire_header(flipper, link))
        return false;

    const auto txs = to_transactions(link);
    if (txs.empty())
    {
        flipper.invalidate();
        return false;
    }

    flipper.write_variable(txs.size());
    for (const auto& tx_link: txs)
        if (!get_wire_tx(flipper, tx_link, witness))
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
