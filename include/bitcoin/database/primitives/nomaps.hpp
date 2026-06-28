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
#ifndef LIBBITCOIN_DATABASE_PRIMITIVES_NOMAPS_HPP
#define LIBBITCOIN_DATABASE_PRIMITIVES_NOMAPS_HPP

#include <algorithm>
#include <tuple>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/memory.hpp>

// Forwarding macro with early termination.
#define DISPATCH_METHOD(method) \
    bool method() NOEXCEPT \
    { \
        bool success{ true }; \
        std::apply([&](auto&... column) \
        { \
            ((success &= column.method()), ...); \
        }, tables_); \
        return success; \
    }

namespace libbitcoin {
namespace database {

/// Variadic template for defining an SoA aggregate table.
template <typename Storage, typename... Columns>
class nomaps
{
public:
    using files = maps<Storage, Columns...>;

    nomaps(files& heads, files& bodies) NOEXCEPT
      : nomaps{ heads, bodies, std::index_sequence_for<Columns...>{} }
    {
    }

    DISPATCH_METHOD(create)
    DISPATCH_METHOD(close)
    DISPATCH_METHOD(verify)
    DISPATCH_METHOD(restore)

    /// The smallest column length as integral.
    size_t count() const NOEXCEPT
    {
        return std::apply([](const auto&... column) NOEXCEPT
        {
            return std::min({ column.count().value... });
        }, tables_);
    }

    /// Macro not used because of accumulator.
    size_t body_size() const NOEXCEPT
    {
        size_t total{};
        std::apply([&](const auto&... column)
        {
            ((total += column.body_size()), ...);
        }, tables_);
        return total;
    }

    /// Macro not used because of parameter.
    bool backup(bool prune) NOEXCEPT
    {
        bool success{ true };
        std::apply([&](auto&... column)
        {
            ((success &= column.backup(prune)), ...);
        }, tables_);
        return success;
    }

protected:
    template <size_t... Index>
    nomaps(files& heads, files& bodies,
        std::index_sequence<Index...>) NOEXCEPT
      : tables_{ Columns
        {
            heads.template file<Index>(),
            bodies.template file<Index>()
        }...}
    {}

    std::tuple<Columns...> tables_;
};

} // namespace database
} // namespace libbitcoin

#undef DISPATCH_METHOD

#endif
