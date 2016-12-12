/**
 * Copyright (c) 2011-2016 libbitcoin developers (see AUTHORS)
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
#ifndef LIBBITCOIN_DATABASE_OUTPUTS_CACHE_HPP
#define LIBBITCOIN_DATABASE_OUTPUTS_CACHE_HPP

#include <memory>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

/// This class is thread safe.
/// A circular-by-age hash table of [point, output].
/// Coinbase outputs are not cached, this precludes need for height.
/// Outputs store value and unparsed script (for load speed and compactness).
class BCD_API outputs_cache
{
public:
    typedef std::shared_ptr<outputs_cache> ptr;

    outputs_cache();

    /// Add an output to the cache (purges older entries).
    void add(const chain::output_point& key, const chain::output& value);

    /// Remove an output from the cache (has been spent).
    void remove(const chain::output_point& output);

    /// Determine if the output is unspent (otherwise fall back to the store).
    bool find(const chain::output_point& output);

private:
    mutable upgrade_mutex mutex_;
};

} // namespace database
} // namespace libbitcoin

#endif
