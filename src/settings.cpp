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
#include <bitcoin/database/settings.hpp>

#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

using namespace bc::system;

settings::settings() NOEXCEPT
  : dir{ "bitcoin" },

    // Archives.

    header_buckets{ 100 },
    header_size{ 1 },
    header_rate{ 50 },

    point_buckets{ 100 },
    point_size{ 1 },
    point_rate{ 50 },

    input_buckets{ 100 },
    input_size{ 1 },
    input_rate{ 50 },

    output_size{ 1 },
    output_rate{ 50 },

    puts_size{ 1 },
    puts_rate{ 50 },

    tx_buckets{ 100 },
    tx_size{ 1 },
    tx_rate{ 50 },

    txs_buckets{ 100 },
    txs_size{ 1 },
    txs_rate{ 50 },

    // Indexes.

    address_buckets{ 100 },
    address_size{ 1 },
    address_rate{ 50 },

    candidate_size{ 1 },
    candidate_rate{ 50 },

    confirmed_size{ 1 },
    confirmed_rate{ 50 },

    strong_tx_buckets{ 100 },
    strong_tx_size{ 1 },
    strong_tx_rate{ 50 },

    // Caches.

    bootstrap_size{ 1 },
    bootstrap_rate{ 50 },

    buffer_buckets{ 100 },
    buffer_size{ 1 },
    buffer_rate{ 50 },

    neutrino_buckets{ 100 },
    neutrino_size{ 1 },
    neutrino_rate{ 50 },

    validated_bk_buckets{ 100 },
    validated_bk_size{ 1 },
    validated_bk_rate{ 50 },

    validated_tx_buckets{ 100 },
    validated_tx_size{ 1 },
    validated_tx_rate{ 50 }
{
}

settings::settings(chain::selection context) NOEXCEPT
  : settings()
{
    switch (context)
    {
        case chain::selection::mainnet:
        {
            break;
        }

        case chain::selection::testnet:
        {
            break;
        }

        case chain::selection::regtest:
        {
            break;
        }

        default:
        case chain::selection::none:
        {
        }
    }
}

} // namespace database
} // namespace libbitcoin
