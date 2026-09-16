/**
 * Copyright (c) 2011-2026 libbitcoin developers
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
#ifndef LIBBITCOIN_DATABASE_TABLES_CACHES_ENVELOPE_HPP
#define LIBBITCOIN_DATABASE_TABLES_CACHES_ENVELOPE_HPP

#include <bitcoin/database/define.hpp>
#include <bitcoin/database/primitives/primitives.hpp>
#include <bitcoin/database/tables/envelope.hpp>
#include <bitcoin/database/tables/schema.hpp>

namespace libbitcoin {
namespace database {
namespace table {

/// envelope is a head-only slab of one element (no body), holding the store
/// envelope. The element is rewritten in place as settings change.
struct envelope
  : public head_map<schema::envelope>
{
    using head_map<schema::envelope>::headmap;

    struct record
    {
        static constexpr size_t size = max_size_t;

        inline link count() const NOEXCEPT
        {
            return system::possible_narrow_cast<link::integer>(
                envelope.serialized_size());
        }

        inline bool from_data(reader& source) NOEXCEPT
        {
            return envelope.from_data(source);
        }

        inline bool to_data(flipper& sink) const NOEXCEPT
        {
            return envelope.to_data(sink);
        }

        database::envelope envelope{};
    };
};

} // namespace table
} // namespace database
} // namespace libbitcoin

#endif
