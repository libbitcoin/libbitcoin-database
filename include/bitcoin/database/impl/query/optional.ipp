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
bool CLASS::get_confirmed_balance(uint64_t& out,
    const hash_digest& key) const NOEXCEPT
{
    auto it = store_.address.it(key);
    if (!it)
        return false;

    out = zero;
    do
    {
        table::address::record address{};
        if (!store_.address.get(it, address))
            return false;

        // Failure or overflow returns maximum value.
        if (is_confirmed_unspent(address.output_fk))
        {
            uint64_t value{};
            if (!get_value(value, address.output_fk))
                return false;

            out = system::ceilinged_add(value, out);
        }
    }
    while (it.advance());
    return true;
}

// TODO: test more.
TEMPLATE
bool CLASS::to_address_outputs(output_links& out,
    const hash_digest& key) const NOEXCEPT
{
    auto it = store_.address.it(key);
    if (!it)
        return false;

    out.clear();
    do
    {
        table::address::record address{};
        if (!store_.address.get(it, address))
        {
            out.clear();
            return false;
        }

        out.push_back(address.output_fk);
    }
    while (it.advance());
    return true;
}

// TODO: test more.
TEMPLATE
bool CLASS::to_unspent_outputs(output_links& out,
    const hash_digest& key) const NOEXCEPT
{
    auto it = store_.address.it(key);
    if (!it)
        return false;

    out.clear();
    do
    {
        table::address::record address{};
        if (!store_.address.get(it, address))
        {
            out.clear();
            return false;
        }

        if (is_confirmed_unspent(address.output_fk))
            out.push_back(address.output_fk);
    }
    while (it.advance());
    return true;
}

// TODO: test more.
TEMPLATE
bool CLASS::to_minimum_unspent_outputs(output_links& out,
    const hash_digest& key, uint64_t minimum) const NOEXCEPT
{
    auto it = store_.address.it(key);
    if (!it)
        return false;

    out.clear();
    do
    {
        table::address::record address{};
        if (!store_.address.get(it, address))
            return false;

        // Confirmed and not spent, but possibly immature.
        if (is_confirmed_output(address.output_fk) &&
            !is_spent_output(address.output_fk))
        {
            uint64_t value{};
            if (!get_value(value, address.output_fk))
            {
                out.clear();
                return false;
            }

            if (value >= minimum)
                out.push_back(address.output_fk);
        }
    }
    while (it.advance());
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

// Neutrino (surrogate-keyed).
// ----------------------------------------------------------------------------

TEMPLATE
bool CLASS::get_filter_body(filter& out, const header_link& link) const NOEXCEPT
{
    table::neutrino::get_filter neutrino{};
    if (!store_.neutrino.find(link, neutrino))
        return false;

    out = std::move(neutrino.filter);
    return true;
}

TEMPLATE
bool CLASS::get_filter_head(hash_digest& out,
    const header_link& link) const NOEXCEPT
{
    table::neutrino::get_head neutrino{};
    if (!store_.neutrino.find(link, neutrino))
        return false;

    out = std::move(neutrino.filter_head);
    return true;
}

TEMPLATE
bool CLASS::set_filter_body(const header_link& link,
    const block& block) NOEXCEPT
{
    filter bytes{};

    // compute_filter is only false if prevouts are missing.
    return system::neutrino::compute_filter(bytes, block) &&
        set_filter_body(link, bytes);
}

TEMPLATE
bool CLASS::set_filter_body(const header_link&,
    const filter&) NOEXCEPT
{
    // ========================================================================
    const auto scope = store_.get_transactor();

    ////// Clean single allocation failure (e.g. disk full).
    ////return store_.neutrino.put(link, table::neutrino::put_ref
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
    // ========================================================================
    const auto scope = store_.get_transactor();

    ////// Clean single allocation failure (e.g. disk full).
    ////return store_.neutrino.put(link, table::neutrino::put_ref
    ////{
    ////    {},
    ////    filter_head,
    ////    filter
    ////});
    // ========================================================================

    return false;
}

////// Buffer (surrogate-keyed).
////// ----------------------------------------------------------------------------
////// TODO: serialize prevouts, compare deserialization time to native storage.
////
////TEMPLATE
////typename CLASS::transaction::cptr CLASS::get_buffered_tx(
////    const tx_link& link) const NOEXCEPT
////{
////    table::buffer::slab_ptr buffer{};
////    if (!store_.buffer.find(link, buffer))
////        return {};
////
////    return buffer.tx;
////}
////
////TEMPLATE
////bool CLASS::set_buffered_tx(const tx_link& link,
////    const transaction& tx) NOEXCEPT
////{
////    // ========================================================================
////    const auto scope = store_.get_transactor();
////
////    return store_.buffer.put(link, table::buffer::put_ref
////    {
////        {},
////        tx
////    });
////    // ========================================================================
////}
////
////// Bootstrap (array).
////// ----------------------------------------------------------------------------
////
////TEMPLATE
////bool CLASS::get_bootstrap(hashes& out) const NOEXCEPT
////{
////    table::bootstrap::record boot{};
////    boot.block_hashes.resize(store_.bootstrap.count());
////    if (!store_.bootstrap.get(0, boot))
////        return false;
////
////    out = std::move(boot.block_hashes);
////    return true;
////}
////
////TEMPLATE
////bool CLASS::set_bootstrap(size_t height) NOEXCEPT
////{
////    if (height > get_top_confirmed())
////        return false;
////
////    table::bootstrap::record boot{};
////    boot.block_hashes.reserve(add1(height));
////    for (auto index = zero; index <= height; ++index)
////    {
////        const auto header_fk = to_confirmed(index);
////        if (header_fk.is_terminal())
////            return false;
////
////        boot.block_hashes.push_back(get_header_key(header_fk));
////    }
////
////    // ========================================================================
////    const auto scope = store_.get_transactor();
////
////    return store_.bootstrap.truncate(0) && store_.bootstrap.put(boot);
////    // ========================================================================
////}

} // namespace database
} // namespace libbitcoin

#endif
