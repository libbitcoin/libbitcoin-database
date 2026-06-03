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
#ifndef LIBBITCOIN_DATABASE_TYPES_SILENT_HPP
#define LIBBITCOIN_DATABASE_TYPES_SILENT_HPP

#include <bitcoin/database/define.hpp>
#include <bitcoin/database/tables/archives/transaction.hpp>
#include <bitcoin/system/wallet/addresses/silent_payment.hpp>

namespace libbitcoin {
namespace database {

using pay_witness_taproot_output =
    system::wallet::silent_payment::pay_witness_taproot_output;

struct BCD_API silent_record
{
    table::transaction::link tx{};
    system::ec_compressed prevouts_summary{};
    std_vector<pay_witness_taproot_output> outputs{};
};

struct BCD_API silent
{
    std_vector<silent_record> records{};
};

} // namespace database
} // namespace libbitcoin

#endif
