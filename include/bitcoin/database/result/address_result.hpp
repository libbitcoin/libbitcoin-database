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
#ifndef LIBBITCOIN_DATABASE_ADDRESS_RESULT_HPP
#define LIBBITCOIN_DATABASE_ADDRESS_RESULT_HPP

#include <cstddef>
#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/primitives/list_element.hpp>
#include <bitcoin/database/primitives/record_manager.hpp>
#include <bitcoin/database/result/address_iterator.hpp>

namespace libbitcoin {
namespace database {

/// Partially-deferred address result.
class BCD_API address_result
{
public:
    typedef empty_key key_type;
    typedef array_index link_type;
    typedef record_manager<link_type> manager;
    typedef list_element<const manager, link_type, key_type> const_value_type;

    address_result(const const_value_type& element,
        const system::short_hash& hash);

    /// True if the requested block exists.
    operator bool() const;

    /// The address hash of the query.
    const system::short_hash& hash() const;

    /// Iterate over the address metadata set.
    address_iterator begin() const;
    address_iterator end() const;

private:
    system::short_hash hash_;

    // This class is thread safe.
    const_value_type element_;
};

} // namespace database
} // namespace libbitcoin

#endif
