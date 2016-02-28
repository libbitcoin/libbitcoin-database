/**
 * Copyright (c) 2011-2015 libbitcoin developers (see AUTHORS)
 *
 * This file is part of libbitcoin.
 *
 * libbitcoin is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License with
 * additional permissions to the one published by the Free Software
 * Foundation, either version 3 of the License, or (at your option)
 * any later version. For more information see LICENSE.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef LIBBITCOIN_DATABASE_TRANSACTION_RESULT_HPP
#define LIBBITCOIN_DATABASE_TRANSACTION_RESULT_HPP

#include <cstddef>
#include <cstdint>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

class BCD_API transaction_result
{
public:
    transaction_result(const uint8_t* slab);

    /// Test whether the result exists, return false otherwise.
    operator bool() const;

    /// Height of the block which includes this transaction.
    size_t height() const;

    /// Index of transaction within a block.
    size_t index() const;

    /// Actual transaction itself.
    chain::transaction transaction() const;

private:
    const uint8_t* slab_;
};

} // namespace database
} // namespace libbitcoin

#endif
