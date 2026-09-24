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
#ifndef LIBBITCOIN_DATABASE_QUERY_CONSENSUS_STATES_IPP
#define LIBBITCOIN_DATABASE_QUERY_CONSENSUS_STATES_IPP

#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

// State machine for block and tx.
// ----------------------------------------------------------------------------

 TEMPLATE
inline bool CLASS::is_confirmable(const header_link& link) const NOEXCEPT
{
    return get_header_state(link) == error::block_confirmable;
}

TEMPLATE
bool CLASS::is_validateable(size_t height) const NOEXCEPT
{
    const auto ec = get_block_state(to_candidate(height));

    // First block state should be unvalidated, valid, or confirmable.
    return
        (ec == database::error::unvalidated) ||
        (ec == database::error::block_valid) ||
        (ec == database::error::unknown_state) ||
        (ec == database::error::block_confirmable);
}

// protected
TEMPLATE
inline code CLASS::to_block_code(
    linkage<schema::code>::integer value) const NOEXCEPT
{
    switch (value)
    {
        // Transitional: Satisfies validation rules (prevouts unverified).
        case block_state::valid:
            return error::block_valid;

        // Final: Satisfies confirmation rules (prevouts confirmable).
        case block_state::confirmable:
            return error::block_confirmable;

        // Final: Does not satisfy either validation or confirmation rules.
        case block_state::unconfirmable:
            return error::block_unconfirmable;

        // Fault: Has no state, should not happen when read from store.
        // block_unknown also used to reset a state (debugging).
        case block_state::block_unknown:
        default:
            return error::unknown_state;
    }
}

// protected
TEMPLATE
inline bool CLASS::is_sufficient(const context& current,
    const context& evaluated) const NOEXCEPT
{
    // Past evaluation at a lesser height and mtp is sufficient, provided that
    // absolute locktime is evaluated against mtp (not block timestamp).
    return evaluated.flags == current.flags
        && evaluated.is_enabled(system::chain::flags::bip113_rule)
        && evaluated.height <= current.height
        && evaluated.mtp <= current.mtp;
}

TEMPLATE
bool CLASS::is_unconfirmable(const header_link& link) const NOEXCEPT
{
    return get_header_state(link) == error::block_unconfirmable;
}

// unvalidated
// block_valid
// block_confirmable
// block_unconfirmable
TEMPLATE
code CLASS::get_header_state(const header_link& link) const NOEXCEPT
{
    table::validated_bk::record valid{};
    if (!store_.validated_bk.at(to_validated_bk(link), valid))
        return error::unvalidated;

    return to_block_code(valid.code);
}

// unassociated
// unvalidated
// block_valid
// block_confirmable
// block_unconfirmable
TEMPLATE
code CLASS::get_block_state(const header_link& link) const NOEXCEPT
{
    table::validated_bk::record valid{};
    if (!store_.validated_bk.at(to_validated_bk(link), valid))
        return is_associated(link) ? error::unvalidated : error::unassociated;

    return to_block_code(valid.code);
}

TEMPLATE
inline bool CLASS::is_validated(const header_link& link) const NOEXCEPT
{
    // Validated and not invalid (checkpoint/milestone shows false).
    const auto state = get_header_state(link);
    return state == error::block_valid || state == error::block_confirmable;
}

TEMPLATE
bool CLASS::is_block_validated(code& state, const header_link& link,
    size_t height, size_t checkpoint) const NOEXCEPT
{
    if (height <= checkpoint || is_milestone(link))
    {
        if (is_associated(link))
        {
            state = error::bypassed;
            return true;
        }
        else
        {
            state = error::unassociated;
            return false;
        }
    }
    else
    {
        state = get_header_state(link);
        return state == error::block_valid
            || state == error::block_confirmable;
    }
}

TEMPLATE
code CLASS::get_tx_state(tx_state& state, const tx_link& link,
    const context& ctx) const NOEXCEPT
{
    using prevout = table::prevout::slab_get;
    if (!store_.validated_tx.enabled())
        return error::unvalidated;

    const auto fk = store_.validated_tx.first(link);
    if (fk.is_terminal())
        return error::unvalidated;

    table::validated_tx::slab valid{};
    valid.prevouts.resize(state.prevouts.size());
    if (!store_.validated_tx.get(fk, valid))
        return error::integrity;

    if (!is_sufficient(ctx, valid.ctx))
        return error::unvalidated;

    state.fee = valid.fee;
    state.sigops = valid.sigops;
    std::ranges::transform(valid.prevouts, state.prevouts.begin(),
        [](auto merged) NOEXCEPT
        {
            return tx_state::prevout
            {
                prevout::output_tx_fk(merged),
                prevout::coinbase(merged)
            };
        });

    return error::success;
}

// writers
// ----------------------------------------------------------------------------

TEMPLATE
bool CLASS::set_block_valid(const header_link& link) NOEXCEPT
{
    return set_block_state(link, block_state::valid);
}

TEMPLATE
bool CLASS::set_block_confirmable(const header_link& link) NOEXCEPT
{
    return set_block_state(link, block_state::confirmable);
}

TEMPLATE
bool CLASS::set_block_unconfirmable(const header_link& link) NOEXCEPT
{
    return !store_.mark_unconfirmable() ||
        set_block_state(link, block_state::unconfirmable);
}

TEMPLATE
bool CLASS::set_block_unknown(const header_link& link) NOEXCEPT
{
    return set_block_state(link, block_state::block_unknown);
}

// private
TEMPLATE
bool CLASS::set_block_state(const header_link& link,
    block_state state) NOEXCEPT
{
    const auto record = to_validated_bk(link);

    // ========================================================================
    const auto scope = get_transactor();

    // Clean single allocation failure (e.g. disk full).
    return store_.validated_bk.put(record,
        table::validated_bk::record{ {}, state });
    // ========================================================================
}

TEMPLATE
bool CLASS::set_tx_state(const tx_link& link, const transaction& tx,
    const chain_context& ctx) NOEXCEPT
{
    return set_tx_state(link, tx, context::from(ctx));
}

TEMPLATE
bool CLASS::set_tx_state(const tx_link& link, const transaction& tx,
    const context& ctx) NOEXCEPT
{
    if (!store_.validated_tx.enabled())
        return true;

    using namespace system;
    using sigs = linkage<schema::sigops>;
    const auto& ins = *tx.inputs_ptr();
    tx_links prevouts(ins.size());

    auto it = prevouts.begin();
    for (const auto& in: ins)
    {
        tx_link parent{ in->metadata.parent_tx };
        auto coinbase = in->metadata.coinbase;

        // Prevouts populated from the tx's own package are not yet linked.
        if (in->metadata.parent_tx == max_uint32)
        {
            if ((parent = to_tx(in->point().hash())).is_terminal())
                return false;

            coinbase = is_coinbase(parent);
        }

        *it++ = table::prevout::merge(coinbase, parent);
    }

    const auto bip16 = ctx.is_enabled(chain::flags::bip16_rule);
    const auto bip141 = ctx.is_enabled(chain::flags::bip141_rule);
    const auto sigops = tx.signature_operations(bip16, bip141);

    // ========================================================================
    const auto scope = get_transactor();

    // Clean single allocation failure (e.g. disk full).
    return store_.validated_tx.put(link, table::validated_tx::slab
    {
        {},
        ctx,
        tx.fee(),
        possible_narrow_cast<sigs::integer>(sigops),
        std::move(prevouts)
    });
    // ========================================================================
}

} // namespace database
} // namespace libbitcoin

#endif
