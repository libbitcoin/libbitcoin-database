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
#ifndef LIBBITCOIN_DATABASE_QUERY_ARCHIVE_WIRE_READER_IPP
#define LIBBITCOIN_DATABASE_QUERY_ARCHIVE_WIRE_READER_IPP

#include <atomic>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

// Reader directly from store to wire-encoded buffer (store to network).
// ----------------------------------------------------------------------------
// Due to the stoopid segwit serialization of witness after output there is are
// duplicated navigations to store_.ins and store_.input by the witness reader.
// This normalized approach is also the most efficient.

// The stored tx is presumed to have the txid of the tx, so only the witness
// may differ. Stored witnesses are wire encoded, so are compared in place.
TEMPLATE
bool CLASS::is_witness_match(const tx_link& link,
    const transaction& tx) const NOEXCEPT
{
    table::transaction::record record{};
    if (!is_size_match(record, link, tx.inputs(), tx.serialized_size(false),
        tx.serialized_size(true)))
        return false;

    if (!tx.is_segregated())
        return true;

    // Point links are contiguous (computed).
    auto fk = record.point_fk;
    table::input::match_stack match{};
    const auto guard = store_.ins.get_memory();
    const auto ptr = store_.input.get_memory();
    for (const auto& in: *tx.inputs_ptr())
    {
        table::ins_sequence::get_input ins{};
        match.expected = &in->witness().stack();
        if (!store_.ins.sequence.get_raw(fk++, ins) ||
            !store_.input.raw(ptr, ins.input_fk, match) || !match.match)
            return false;
    }

    return true;
}

TEMPLATE
bool CLASS::is_witness_match(const tx_link& link,
    const transaction_view& tx) const NOEXCEPT
{
    table::transaction::record record{};
    if (!is_size_match(record, link, tx.inputs(), tx.serialized_size(false),
        tx.serialized_size(true)))
        return false;

    if (!tx.is_segregated())
        return true;

    const auto witnesses = tx.witnesses();
    table::input::match_witness match{ {}, witnesses.data(),
        witnesses.size() };

    // Point links are contiguous (computed).
    const auto ins_begin = record.point_fk;
    const auto ins_final = ins_begin + record.ins_count;
    const auto guard = store_.ins.get_memory();
    const auto ptr = store_.input.get_memory();
    for (auto fk = ins_begin; fk < ins_final; ++fk)
    {
        table::ins_sequence::get_input ins{};
        if (!store_.ins.sequence.get_raw(fk, ins) ||
            !store_.input.raw(ptr, ins.input_fk, match) || !match.match)
            return false;
    }

    return is_zero(match.remaining);
}

// protected
TEMPLATE
bool CLASS::is_size_match(table::transaction::record& out,
    const tx_link& link, size_t inputs, size_t light,
    size_t heavy) const NOEXCEPT
{
    return store_.tx.get(link, out)
        && (out.ins_count == inputs)
        && (out.light == light)
        && (out.heavy == heavy);
}

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
    const ins_link& link) const NOEXCEPT
{
    const auto ins = store_.ins.get_memory();
    return get_wire_input(sink, ins, store_.input.get_memory(), link);
}

TEMPLATE
bool CLASS::get_wire_output(bytewriter& sink,
    const output_link& link) const NOEXCEPT
{
    return get_wire_output(sink, store_.output.get_memory(), link);
}

TEMPLATE
bool CLASS::get_wire_witness(bytewriter& sink,
    const ins_link& link) const NOEXCEPT
{
    const auto guard = store_.ins.get_memory();
    return get_wire_witness(sink, store_.input.get_memory(), link);
}

TEMPLATE
bool CLASS::get_wire_tx(bytewriter& sink, const tx_link& link,
    bool witness) const NOEXCEPT
{
    return get_wire_tx(sink, get_wire_accessors(), link, witness);
}

// protected
TEMPLATE
CLASS::wire_accessors CLASS::get_wire_accessors() const NOEXCEPT
{
    return
    {
        store_.tx.get_memory(),
        store_.ins.get_memory(),
        store_.outs.puts.get_memory(),
        store_.input.get_memory(),
        store_.output.get_memory()
    };
}

// protected
TEMPLATE
bool CLASS::get_wire_input(bytewriter& sink, const memory& ins,
    const memory& input, const ins_link& link) const NOEXCEPT
{
    table::ins_point::wire_point point{ {}, sink };
    if (!store_.ins.get(ins, link, point))
        return false;

    // The ins accessor guards its sequence column.
    table::ins_sequence::get_input in{};
    table::input::wire_script script{ {}, sink };
    if (!store_.ins.sequence.get_raw(link, in) ||
        !store_.input.get(input, in.input_fk, script))
        return false;

    sink.write_4_bytes_little_endian(in.sequence);
    return true;
}

// protected
TEMPLATE
bool CLASS::get_wire_output(bytewriter& sink, const memory& output,
    const output_link& link) const NOEXCEPT
{
    table::output::wire_script out{ {}, sink };
    return store_.output.get(output, link, out);
}

// protected
TEMPLATE
bool CLASS::get_wire_witness(bytewriter& sink, const memory& input,
    const ins_link& link) const NOEXCEPT
{
    // Caller holds an ins accessor, guarding its sequence column.
    table::ins_sequence::get_input in{};
    table::input::wire_witness wire{ {}, sink };
    return store_.ins.sequence.get_raw(link, in)
        && store_.input.get(input, in.input_fk, wire);
}

// protected
TEMPLATE
bool CLASS::get_wire_tx(bytewriter& sink, const wire_accessors& ptrs,
    const tx_link& link, bool witness) const NOEXCEPT
{
    table::transaction::record tx{};
    if (!store_.tx.get(ptrs.tx, link, tx))
        return false;

    // Point and put links are contiguous (computed).
    const auto ins_begin = tx.point_fk;
    const auto ins_count = tx.ins_count;
    const auto ins_final = ins_begin + ins_count;
    const auto witnessed = witness && (tx.heavy != tx.light);

    // Writers may encode, so the version write gives positions per byte.
    const auto start = sink.get_write_position();
    sink.write_4_bytes_little_endian(tx.version);
    const auto scale = (sink.get_write_position() - start) / sizeof(uint32_t);

    if (witnessed)
    {
        sink.write_byte(system::chain::witness_marker);
        sink.write_byte(system::chain::witness_enabled);
    }

    sink.write_variable(ins_count);
    for (auto fk = ins_begin; fk < ins_final; ++fk)
        if (!get_wire_input(sink, ptrs.ins, ptrs.input, fk))
            return false;

    const auto outs_begin = tx.outs_fk;
    const auto outs_final = outs_begin + tx.outs_count;
    sink.write_variable(tx.outs_count);
    for (auto fk = outs_begin; fk < outs_final; ++fk)
    {
        table::outs::get_output put{};
        if (!store_.outs.puts.get(ptrs.puts, fk, put) ||
            !get_wire_output(sink, ptrs.output, put.out_fk))
            return false;
    }

    if (witnessed)
    {
        for (auto fk = ins_begin; fk < ins_final; ++fk)
            if (!get_wire_witness(sink, ptrs.input, fk))
                return false;
    }

    sink.write_4_bytes_little_endian(tx.locktime);

    // A pruned tx (emptied input scripts) is short of its archived size.
    const auto size = witnessed ? tx.heavy : tx.light;
    return sink.get_write_position() - start == scale * size;
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
    const auto ptrs = get_wire_accessors();
    for (const auto& tx_link: txs)
        if (!get_wire_tx(sink, ptrs, tx_link, witness))
            return false;

    return true;
}

// These convenience wrappers are made practical by size caching for block and
// tx for both nominal and witness wire encodings (and fixed size headers).
// Intermediate objects (input, output, witness) have a size prefix.

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
data_chunk CLASS::get_wire_block(const header_link& link, bool witness,
    bool turbo) const NOEXCEPT
{
    using namespace system;
    size_t size{};
    if (!get_block_size(size, link, witness))
        return {};

    data_chunk data(size);
    if (turbo)
        return get_wire_block(data, link, witness, turbo) ? data : data_chunk{};

    stream::flip::fast ostream(data);
    flip::bytes::fast out(ostream);
    if (!get_wire_block(out, link, witness) || !out)
        return {};

    return data;
}

TEMPLATE
template <typename Writer>
bool CLASS::get_wire_block(const system::data_slab& out,
    const header_link& link, bool witness, bool turbo) const NOEXCEPT
{
    using namespace system;
    if (!turbo)
    {
        stream::out::fast sink{ out };
        Writer writer{ sink };
        return get_wire_block(writer, link, witness) && writer;
    }

    const auto txs = to_transactions(link);
    if (txs.empty())
        return false;

    // Tx sizes, converted in place to offsets. Tasks read through the held
    // accessors, as a task taking a held lock could wait behind a remap.
    const auto ptrs = get_wire_accessors();
    std::vector<size_t> offsets(txs.size());
    for (size_t index{}; index < txs.size(); ++index)
    {
        table::transaction::get_sizes sizes{};
        if (!store_.tx.get(ptrs.tx, txs.at(index), sizes))
            return false;

        offsets.at(index) = witness ? sizes.heavy : sizes.light;
    }

    const auto last = offsets.back();
    const auto prefix = chain::header::serialized_size() +
        variable_size(txs.size());
    std::exclusive_scan(offsets.cbegin(), offsets.cend(), offsets.begin(),
        prefix);

    const auto bytes = offsets.back() + last;
    const auto scale = out.size() / bytes;
    if (is_zero(scale) || (scale * bytes != out.size()))
        return false;

    // Txs are written in ranges, one task each, as each task costs a wakeup,
    // and a block of two ranges or fewer is written without tasks.
    constexpr size_t range_txs = 256;
    const auto ranges = ceilinged_divide(txs.size(), range_txs);
    if (ranges <= two)
    {
        stream::out::fast sink{ out };
        Writer writer{ sink };
        return get_wire_block(writer, link, witness) && writer;
    }

    const auto region = [&](size_t offset, size_t size) NOEXCEPT
    {
        const auto begin = std::next(out.data(), offset * scale);
        return data_slab{ begin, std::next(begin, size * scale) };
    };

    stream::out::fast head_sink{ region(zero, prefix) };
    Writer head{ head_sink };
    if (!get_wire_header(head, link))
        return false;

    head.write_variable(txs.size());
    if (!head)
        return false;

    std::atomic_bool fault{};
    std::vector<size_t> starts(ranges);
    std::iota(starts.begin(), starts.end(), zero);
    std::for_each(poolstl::execution::par, starts.cbegin(), starts.cend(),
        [&](size_t range) NOEXCEPT
        {
            const auto first = range * range_txs;
            const auto stop = std::min(txs.size(), first + range_txs);
            for (auto index = first; index < stop; ++index)
            {
                const auto offset = offsets.at(index);
                const auto next = add1(index);
                const auto end = next == offsets.size() ? bytes :
                    offsets.at(next);

                stream::out::fast sink{ region(offset, end - offset) };
                Writer writer{ sink };
                if (!get_wire_tx(writer, ptrs, txs.at(index), witness) ||
                    !writer)
                {
                    fault.store(true, std::memory_order_relaxed);
                    return;
                }
            }
        });

    return !fault.load(std::memory_order_relaxed);
}

} // namespace database
} // namespace libbitcoin

#endif
