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
#include <bitcoin/database/types/multisig_view.hpp>

#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

multisig_view::multisig_view(const system::ec_compressed& point,
    const system::ec_signature& signature, size_t m, size_t n) NOEXCEPT
  : point_(point),
    signature_(signature),
    pair_(system::pack_word<uint8_t>(m, n))
{
    BC_DEBUG_ONLY(constexpr auto half = system::power2(to_half(byte_bits));)
    BC_ASSERT(m < half && n < half);
}

void multisig_view::to_data(system::bytewriter& sink) const NOEXCEPT
{
    sink.write_bytes(point_);
    sink.write_bytes(signature_);
    sink.write_byte(pair_);
    BC_ASSERT(sink);
}

} // namespace database
} // namespace libbitcoin
