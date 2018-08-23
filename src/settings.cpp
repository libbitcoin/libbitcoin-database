/**
 * Copyright (c) 2011-2017 libbitcoin developers (see AUTHORS)
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
#include <bitcoin/database/settings.hpp>

#include <boost/filesystem.hpp>

namespace libbitcoin {
namespace database {

using namespace boost::filesystem;

settings::settings()
  : directory("blockchain"),
  
    index_addresses(true),
    flush_writes(false),
    file_growth_rate(5),

    // Hash table sizes (must be configured).
    block_table_buckets(0),
    transaction_table_buckets(0),
    address_table_buckets(0),
    cache_capacity(0)
{
}

settings::settings(config::settings context)
  : settings()
{
    switch (context)
    {
        case config::settings::mainnet:
        {
            block_table_buckets = 650000;
            transaction_table_buckets = 110000000;
            address_table_buckets = 107000000;
            break;
        }

        case config::settings::testnet:
        {
            // TODO: optimize for testnet.
            block_table_buckets = 650000;
            transaction_table_buckets = 110000000;
            address_table_buckets = 107000000;
            break;
        }

        case config::settings::regtest:
        {
            // TODO: optimize for regtest.
            block_table_buckets = 650000;
            transaction_table_buckets = 110000000;
            address_table_buckets = 107000000;
            break;
        }

        default:
        case config::settings::none:
        {
        }
    }
}

} // namespace database
} // namespace libbitcoin
