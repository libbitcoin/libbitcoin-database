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

template <typename Link, size_t Size = zero, typename Record = bool>
class arraymap
{
public:
    using link = typename Link;

    arraymap(storage& body) NOEXCEPT;

    /// Query interface.
    Record get(const link& link) const NOEXCEPT;
    bool insert(const Record& record) NOEXCEPT;

protected:
    /// Reader positioned at data (only data).
    reader_ptr at(const link& record) const NOEXCEPT;

    /// Reader positioned at data, size is one for records and bytes for slabs.
    writer_ptr push(const link& size=one) NOEXCEPT;

private:
    static constexpr size_t link_to_position(const Link& link) NOEXCEPT;
    static constexpr auto slab = is_zero(Size);

    // Thread safe.
    storage& body_;
};

} // namespace database
} // namespace libbitcoin

#define TEMPLATE \
template <typename Link, size_t Size, typename Record>
#define CLASS arraymap<Link, Size, Record>

#include <bitcoin/database/impl/primitives/arraymap.ipp>

#undef CLASS
#undef TEMPLATE

#endif
