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
  : history_start_height(0),
    stealth_start_height(0),
    directory("database")
{
}

settings::settings(bc::settings context)
  : settings()
{
    switch (context)
    {
        case bc::settings::mainnet:
        {
            stealth_start_height = 350000;
            directory = { "mainnet" };
            break;
        }

        case bc::settings::testnet:
        {
            stealth_start_height = 500000;
            directory = { "testnet" };
            break;
        }

        default:
        case bc::settings::none:
        {
        }
    }
}

} // namespace database
} // namespace libbitcoin
