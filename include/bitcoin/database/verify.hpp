/**
 * Copyright (c) 2011-2018 libbitcoin developers (see AUTHORS)
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
#ifndef LIBBITCOIN_DATABASE_VERIFY_HPP
#define LIBBITCOIN_DATABASE_VERIFY_HPP

#include <cstddef>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/databases/block_database.hpp>
#include <bitcoin/database/databases/transaction_database.hpp>

namespace libbitcoin {
namespace database {

code verify(const block_database& blocks, const config::checkpoint& fork_point,
    bool candidate);

code verify_top(const block_database& blocks, size_t height, bool candidate);

code verify_exists(const block_database& blocks, const chain::header& header);

code verify_exists(const transaction_database& transactions,
    const chain::transaction& tx);

code verify_missing(const transaction_database& transactions,
    const chain::transaction& tx);

code verify_push(const block_database& blocks, const chain::header& header,
    size_t height);

code verify_push(const block_database& blocks, const chain::block& block,
    size_t height);

code verify_update(const block_database& blocks, const chain::block& block,
    size_t height);

code verify_not_failed(const block_database& blocks, const chain::block& block);

} // namespace database
} // namespace libbitcoin

#endif
