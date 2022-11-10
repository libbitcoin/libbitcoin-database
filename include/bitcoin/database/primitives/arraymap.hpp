/**
/// Copyright (c) 2011-2022 libbitcoin developers (see AUTHORS)
 *
/// This file is part of libbitcoin.
 *
/// This program is free software: you can redistribute it and/or modify
/// it under the terms of the GNU Affero General Public License as published by
/// the Free Software Foundation, either version 3 of the License, or
/// (at your option) any later version.
 *
/// This program is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
/// GNU Affero General Public License for more details.
 *
/// You should have received a copy of the GNU Affero General Public License
/// along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef LIBBITCOIN_DATABASE_PRIMITIVES_ARRAY_HPP
#define LIBBITCOIN_DATABASE_PRIMITIVES_ARRAY_HPP

#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/primitives/linkage.hpp>
#include <bitcoin/database/primitives/manager.hpp>
#include <bitcoin/database/memory/memory.hpp>

namespace libbitcoin {
namespace database {

template <typename Link, typename Record>
class arraymap
{
public:
    arraymap(storage& body) NOEXCEPT;

    /// Query interface.
    Record get(const Link& link) const NOEXCEPT;
    bool insert(const Record& record) NOEXCEPT;

protected:
    /// Reader positioned at data (only data).
    reader_ptr at(const Link& link) const NOEXCEPT;

    /// Reader positioned at data, size is count for records, bytes for slabs.
    writer_ptr push(const Link& size=one) NOEXCEPT;

private:
    static constexpr auto is_slab = is_zero(Record::size);
    static constexpr size_t link_to_position(const Link& link) NOEXCEPT;

    // Thread safe.
    storage& body_;
};

} // namespace database
} // namespace libbitcoin

#define TEMPLATE template <typename Link, typename Record>
#define CLASS arraymap<Link, Record>

#include <bitcoin/database/impl/primitives/arraymap.ipp>

#undef CLASS
#undef TEMPLATE

#endif
