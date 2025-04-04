/**
 * Copyright (c) 2011-2025 libbitcoin developers (see AUTHORS)
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
#ifndef LIBBITCOIN_DATABASE_QUERY_OPTIONAL_IPP
#define LIBBITCOIN_DATABASE_QUERY_OPTIONAL_IPP

#include <utility>
#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

// Address (natural-keyed).
// ----------------------------------------------------------------------------
// TODO: use point keys (for multimap compression).

// TODO: test more.
TEMPLATE
bool CLASS::to_address_outputs(output_links& out,
    const hash_digest& key) const NOEXCEPT
{
    out.clear();
    for (auto it = store_.address.it(key); it; ++it)
    {
        table::address::record address{};
        if (!store_.address.get(it, address))
        {
            out.clear();
            out.shrink_to_fit();
            return false;
        }

        out.push_back(address.output_fk);
    }

    return true;
}

// TODO: test more.
TEMPLATE
bool CLASS::to_confirmed_unspent_outputs(output_links& out,
    const hash_digest& key) const NOEXCEPT
{
    output_links output_fks{};
    if (!to_address_outputs(output_fks, key))
        return false;

    out.clear();
    out.reserve(output_fks.size());
    for (auto output_fk: output_fks)
        if (is_confirmed_unspent(output_fk))
            out.push_back(output_fk);

    out.shrink_to_fit();
    return true;
}

// TODO: test more.
TEMPLATE
bool CLASS::to_minimum_unspent_outputs(output_links& out,
    const hash_digest& key, uint64_t minimum) const NOEXCEPT
{
    output_links unspent_fks{};
    if (!to_confirmed_unspent_outputs(unspent_fks, key))
        return false;

    out.clear();
    out.reserve(unspent_fks.size());
    for (auto unspent_fk: unspent_fks)
    {
        uint64_t value{};
        if (!get_value(value, unspent_fk))
        {
            out.clear();
            out.shrink_to_fit();
            return false;
        }

        if (value >= minimum)
            out.push_back(unspent_fk);
    }

    out.shrink_to_fit();
    return true;
}

// TODO: test more.
TEMPLATE
bool CLASS::get_confirmed_balance(uint64_t& out,
    const hash_digest& key) const NOEXCEPT
{
    out = zero;
    output_links unspent_fks{};
    if (!to_confirmed_unspent_outputs(unspent_fks, key))
        return false;

    for (auto unspent_fk: unspent_fks)
    {
        uint64_t value{};
        if (!get_value(value, unspent_fk))
            return false;

        // max if overflowed.
        out = system::ceilinged_add(value, out);
    }

    return true;
}

////TEMPLATE
////bool CLASS::set_address_output(const output& output,
////    const output_link& link) NOEXCEPT
////{
////    return set_address_output(output.script().hash(), link);
////}
////
////TEMPLATE
////bool CLASS::set_address_output(const hash_digest& key,
////    const output_link& link) NOEXCEPT
////{
////    if (link.is_terminal())
////        return false;
////
////    // ========================================================================
////    const auto scope = store_.get_transactor();
////
////    // Clean single allocation failure (e.g. disk full).
////    return store_.address.put(key, table::address::record
////    {
////        {},
////        link
////    });
////    // ========================================================================
////}

// filter_tx (surrogate-keyed).
// ----------------------------------------------------------------------------

TEMPLATE
bool CLASS::get_filter_body(filter& out, const header_link& link) const NOEXCEPT
{
    table::filter_tx::get_filter filter_tx{};
    if (!store_.filter_tx.find(link, filter_tx))
        return false;

    out = std::move(filter_tx.filter);
    return true;
}

TEMPLATE
bool CLASS::get_filter_head(hash_digest& out,
    const header_link& link) const NOEXCEPT
{
    table::filter_bk::get_head filter_bk{};
    if (!store_.filter_bk.at(link, filter_bk))
        return false;
    
    out = std::move(filter_bk.filter_head);
    return true;
}

TEMPLATE
bool CLASS::set_filter_head(const header_link& link) NOEXCEPT
{
    if (!filter_enabled())
        return true;

    // The filter body must have been previously stored under the block link.
    filter bytes{};
    if (!get_filter_body(bytes, link))
        return false;

    // If genesis then previous is null_hash otherwise get by confirmed height.
    hash_digest previous{};
    const auto height = get_height(link);
    if (!is_zero(height))
        if (!get_filter_head(previous, to_confirmed(sub1(height))))
            return false;

    // Use the previous head and current body to compute the current head.
    return set_filter_head(link,
        system::neutrino::compute_filter_header(previous, bytes));
}

TEMPLATE
bool CLASS::set_filter_body(const header_link& link,
    const block& block) NOEXCEPT
{
    if (!filter_enabled())
        return true;

    // Compute the current filter from the block and store under the link.
    filter bytes{};
    return system::neutrino::compute_filter(bytes, block) &&
        set_filter_body(link, bytes);
}

TEMPLATE
bool CLASS::set_filter_body(const header_link&,
    const filter&) NOEXCEPT
{
    if (!filter_enabled())
        return true;

    // ========================================================================
    const auto scope = store_.get_transactor();

    ////// Clean single allocation failure (e.g. disk full).
    ////return store_.filter_tx.put(link, table::filter_tx::put_ref
    ////{
    ////    {},
    ////    filter_head,
    ////    filter
    ////});
    // ========================================================================

    return false;
}

TEMPLATE
bool CLASS::set_filter_head(const header_link&,
    const hash_digest&) NOEXCEPT
{
    if (!filter_enabled())
        return true;

    // ========================================================================
    const auto scope = store_.get_transactor();

    ////// Clean single allocation failure (e.g. disk full).
    ////return store_.filter_tx.put(link, table::filter_tx::put_ref
    ////{
    ////    {},
    ////    filter_head,
    ////    filter
    ////});
    // ========================================================================

    return false;
}

} // namespace database
} // namespace libbitcoin

#endif
