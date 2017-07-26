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
#ifndef LIBBITCOIN_DATABASE_UNSPENT_TRANSACTION_HPP
#define LIBBITCOIN_DATABASE_UNSPENT_TRANSACTION_HPP

#include <cstddef>
#include <cstdint>
#include <memory>
#include <unordered_map>
#include <boost/functional/hash_fwd.hpp>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

/// This class is not thread safe.
class BCD_API unspent_transaction
{
public:
    typedef std::unordered_map<uint32_t, chain::output> output_map;
    typedef std::shared_ptr<output_map> output_map_ptr;

    // Move/copy constructors.
    unspent_transaction(unspent_transaction&& other);
    unspent_transaction(const unspent_transaction& other);

    /// Constructors.
    explicit unspent_transaction(const hash_digest& hash);
    explicit unspent_transaction(const chain::output_point& point);
    explicit unspent_transaction(const chain::transaction& tx, size_t height,
        uint32_t median_time_past, bool confirmed);

    /// Properties.
    size_t height() const;
    uint32_t median_time_past() const;
    bool is_coinbase() const;
    bool is_confirmed() const;
    const hash_digest& hash() const;

    /// Access to outputs is mutable and unprotected (not thread safe).
    output_map_ptr outputs() const;

    /// Operators.
    bool operator==(const unspent_transaction& other) const;
    unspent_transaction& operator=(unspent_transaction&& other);
    unspent_transaction& operator=(const unspent_transaction& other);

private:

    // These are thread safe (non-const only for assignment operator).
    size_t height_;
    uint32_t median_time_past_;
    bool is_coinbase_;
    bool is_confirmed_;
    hash_digest hash_;

    // This is not thead safe and is publicly reachable.
    // The outputs can be changed without affecting the bimapping.
    mutable output_map_ptr outputs_;
};

} // namespace database
} // namespace libbitcoin

// Standard (boost) hash.
//-----------------------------------------------------------------------------

namespace boost
{

// Extend boost namespace with our unspent output wrapper hash function.
template <>
struct hash<bc::database::unspent_transaction>
{
    size_t operator()(const bc::database::unspent_transaction& unspent) const
    {
        return boost::hash<bc::hash_digest>()(unspent.hash());
    }
};

} // namespace boost

#endif
