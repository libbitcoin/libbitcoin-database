/**
 * Copyright (c) 2011-2026 libbitcoin developers (see AUTHORS)
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
#ifndef LIBBITCOIN_DATABASE_TYPES_MULTISIG_VIEW_HPP
#define LIBBITCOIN_DATABASE_TYPES_MULTISIG_VIEW_HPP

#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

/// Non-owning writable view of a partial signature tuple.
/// m of n limited each to 4 bits (16) packed in upper/lower nibbles.
struct BCD_API multisig_view
{
    multisig_view(const system::ec_compressed& point,
        const system::ec_signature& signature, size_t m, size_t n) NOEXCEPT;

    void to_data(system::bytewriter& sink) const NOEXCEPT;

private:
    uint8_t pair_;
    const system::ec_compressed& point_;
    const system::ec_signature& signature_;
};

using multisig_views = std::vector<multisig_view>;

} // namespace database
} // namespace libbitcoin

#endif
