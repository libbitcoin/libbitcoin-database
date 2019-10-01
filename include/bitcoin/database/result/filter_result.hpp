/**
 * Copyright (c) 2011-2019 libbitcoin developers (see AUTHORS)
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

// Sponsored in part by Digital Contract Design, LLC

#ifndef LIBBITCOIN_DATABASE_FILTER_RESULT_HPP
#define LIBBITCOIN_DATABASE_FILTER_RESULT_HPP

#include <cstddef>
#include <cstdint>
#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/primitives/list_element.hpp>
#include <bitcoin/database/primitives/slab_manager.hpp>

namespace libbitcoin {
namespace database {

/// Partially-deferred read filter result.
class BCD_API filter_result
{
public:
    typedef system::hash_digest key_type;
    typedef file_offset link_type;
    typedef slab_manager<link_type> manager;
    typedef list_element<const manager, link_type, key_type>
        const_element_type;

    filter_result(const const_element_type& element,
        system::shared_mutex& metadata_mutex,
        uint8_t filter_type);

    /// True if this filter result is valid (found).
    operator bool() const;

    /// The link for the transaction slab.
    file_offset link() const;

    /// The filter type
    uint8_t filter_type() const;

    /// The block hash
    // system::hash_digest block_hash() const;

    /// The filter header
    system::hash_digest header() const;

    /// The filter
    system::data_chunk filter() const;

    /// The block_filter
    system::chain::block_filter block_filter() const;

private:
    uint8_t filter_type_;

    // This class is thread safe.
    const const_element_type element_;

    // Metadata values are kept consistent by mutex.
    system::shared_mutex& metadata_mutex_;
};

} // namespace database
} // namespace libbitcoin

#endif
