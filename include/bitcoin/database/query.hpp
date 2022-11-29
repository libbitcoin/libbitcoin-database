/**
 * Copyright (c) 2011-2022 libbitcoin developers (see AUTHORS)
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
#ifndef LIBBITCOIN_DATABASE_QUERY_HPP
#define LIBBITCOIN_DATABASE_QUERY_HPP

#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/primitives/primitives.hpp>
#include <bitcoin/database/tables/tables.hpp>

namespace libbitcoin {
namespace database {

// TODO: rationalize with chain::context.
struct context
{
    uint32_t height{};
    uint32_t flags{};
    uint32_t mtp{};
};

template <typename Store>
class query
{
public:
    query(Store& value) NOEXCEPT;

    /// Store system::chain object (idempotent).
    bool set_tx(const system::chain::transaction& tx) NOEXCEPT;
    bool set_header(const system::chain::header& header, const context& context) NOEXCEPT;
    bool set_block(const system::chain::block& block, const context& context) NOEXCEPT;
    bool set_txs(const hash_digest& key, const system::hashes& hashes) NOEXCEPT;

    /// Retrieve system::chain object (may optimize with property getters).
    system::chain::transaction::cptr get_tx(const hash_digest& key) NOEXCEPT;
    system::chain::header::cptr get_header(const hash_digest& key) NOEXCEPT;
    system::chain::block::cptr get_block(const hash_digest& key) NOEXCEPT;
    system::hashes get_txs(const hash_digest& key) NOEXCEPT;

private:
    Store& store_;
};

} // namespace database
} // namespace libbitcoin

#define TEMPLATE template <typename Store>
#define CLASS query<Store>

#include <bitcoin/database/impl/query.ipp>

#undef CLASS
#undef TEMPLATE

#endif
