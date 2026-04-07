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
#ifndef LIBBITCOIN_DATABASE_QUERY_CONSENSUS_STATES_IPP
#define LIBBITCOIN_DATABASE_QUERY_CONSENSUS_STATES_IPP

#include <utility>
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
        case schema::block_state::valid:
            return error::block_valid;

        // Final: Satisfies confirmation rules (prevouts confirmable).
        case schema::block_state::confirmable:
            return error::block_confirmable;

        // Final: Does not satisfy either validation or confirmation rules.
        case schema::block_state::unconfirmable:
            return error::block_unconfirmable;

        // Fault: Has no state, should not happen when read from store.
        // block_unknown also used to reset a state (debugging).
        case schema::block_state::block_unknown:
        default:
            return error::unknown_state;
    }
}

// protected
TEMPLATE
inline code CLASS::to_tx_code(
    linkage<schema::code>::integer value) const NOEXCEPT
{
    // Validation states are unrelated to confirmation rules.
    // All stored transactions are presumed valid in some possible context.
    // All states below are relevant only to the associated validation context.
    switch (value)
    {
        // Final: Is valid (passed check, accept, and connect).
        case schema::tx_state::connected:
            return error::tx_connected;

        // Final: Is not valid (failed check, accept, or connect).
        case schema::tx_state::disconnected:
            return error::tx_disconnected;

        // Fault: Has no state, should not happen when read from store.
        // tx_unknown also used to reset a state (debugging).
        case schema::tx_state::tx_unknown:
        default:
            return error::unknown_state;
    }
}

// protected
TEMPLATE
inline bool CLASS::is_sufficient(const context& current,
    const context& evaluated) const NOEXCEPT
{
    // Past evaluation at a lesser height and/or mtp is sufficient.
    return evaluated.flags == current.flags
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
code CLASS::get_tx_state(const tx_link& link,
    const context& ctx) const NOEXCEPT
{
    table::validated_tx::slab_get_code valid{};
    for (auto it = store_.validated_tx.it(link); it; ++it)
    {
        if (!store_.validated_tx.get(it, valid))
            return error::integrity;

        if (is_sufficient(ctx, valid.ctx))
            return to_tx_code(valid.code);
    }

    return error::unvalidated;
}

TEMPLATE
code CLASS::get_tx_state(uint64_t& fee, size_t& sigops, const tx_link& link,
    const context& ctx) const NOEXCEPT
{

    table::validated_tx::slab valid{};
    for (auto it = store_.validated_tx.it(link); it; ++it)
    {
        if (!store_.validated_tx.get(it, valid))
            return error::integrity;

        if (is_sufficient(ctx, valid.ctx))
        {
            fee = valid.fee;
            sigops = valid.sigops;
            return to_tx_code(valid.code);
        }
    }

    return error::unvalidated;
}

// writers
// ----------------------------------------------------------------------------

TEMPLATE
bool CLASS::set_block_valid(const header_link& link) NOEXCEPT
{
    return set_block_state(link, schema::block_state::valid);
}

TEMPLATE
bool CLASS::set_block_confirmable(const header_link& link) NOEXCEPT
{
    return set_block_state(link, schema::block_state::confirmable);
}

TEMPLATE
bool CLASS::set_block_unconfirmable(const header_link& link) NOEXCEPT
{
    return set_block_state(link, schema::block_state::unconfirmable);
}

TEMPLATE
bool CLASS::set_block_unknown(const header_link& link) NOEXCEPT
{
    return set_block_state(link, schema::block_state::block_unknown);
}

// private
TEMPLATE
bool CLASS::set_block_state(const header_link& link,
    schema::block_state state) NOEXCEPT
{
    const auto record = to_validated_bk(link);

    // ========================================================================
    const auto scope = store_.get_transactor();

    // Clean single allocation failure (e.g. disk full).
    return store_.validated_bk.put(record,
        table::validated_bk::record{ {}, state });
    // ========================================================================
}

TEMPLATE
bool CLASS::set_tx_unknown(const tx_link& link) NOEXCEPT
{
    return set_tx_state(link, {}, {}, {}, schema::tx_state::tx_unknown);
}

TEMPLATE
bool CLASS::set_tx_disconnected(const tx_link& link,
    const context& ctx) NOEXCEPT
{
    return set_tx_state(link, ctx, {}, {}, schema::tx_state::disconnected);
}

TEMPLATE
bool CLASS::set_tx_connected(const tx_link& link, const context& ctx,
    uint64_t fee, size_t sigops) NOEXCEPT
{
    return set_tx_state(link, ctx, fee, sigops, schema::tx_state::connected);
}

// private
TEMPLATE
bool CLASS::set_tx_state(const tx_link& link, const context& ctx,
    uint64_t fee, size_t sigops, schema::tx_state state) NOEXCEPT
{
    using sigs = linkage<schema::sigops>;

    // ========================================================================
    const auto scope = store_.get_transactor();
    using namespace system;

    // Clean single allocation failure (e.g. disk full).
    return store_.validated_tx.put(link, table::validated_tx::slab
    {
        {}, ctx, state, fee, possible_narrow_cast<sigs::integer>(sigops)
    });
    // ========================================================================
}

} // namespace database
} // namespace libbitcoin

#endif
