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
#include <bitcoin/database/settings.hpp>

#include <boost/filesystem.hpp>

namespace libbitcoin {
namespace database {

using namespace boost::filesystem;

settings::settings()
  : file_growth_rate(50),
    index_start_height(0),

    // Hash table sizes (must be configured).
    block_table_buckets(0),
    transaction_table_buckets(0),
    spend_table_buckets(0),
    history_table_buckets(0),

    directory("blockchain")
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
            spend_table_buckets = 250000000;
            history_table_buckets = 107000000;
            break;
        }

        case config::settings::testnet:
        {
            // TODO: optimize for testnet.
            block_table_buckets = 650000;
            transaction_table_buckets = 110000000;
            spend_table_buckets = 250000000;
            history_table_buckets = 107000000;
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
