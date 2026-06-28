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
#ifndef LIBBITCOIN_DATABASE_MEMORY_MAPS_HPP
#define LIBBITCOIN_DATABASE_MEMORY_MAPS_HPP

#include <tuple>
#include <filesystem>
#include <bitcoin/database/define.hpp>

// Variadic forwarding with early termination.
#define DISPATCH_METHOD(method, ...) \
    code method() __VA_ARGS__ NOEXCEPT \
    { \
        code ec{}; \
        std::apply([&](auto&... column) \
        { \
            ((ec = ec ? ec : column.method()), ...); \
        }, files_); \
        return ec; \
    }

namespace libbitcoin {
namespace database {

/// Variadic template for defining an SoA aggregate table map.
template <typename Storage, typename... Columns>
class maps
{
public:
    using path = std::filesystem::path;
    static constexpr auto count = sizeof...(Columns);
    static constexpr auto stride = (Columns::width + ...);
    static constexpr auto widths = std::array{ Columns::width... };
    static constexpr auto sequence = std::make_index_sequence<count>{};
    static constexpr auto suffixes = std::array<std::string_view, count>
        { std::string_view{ Columns::suffix.data(), Columns::suffix.size() }... };

    template <size_t Index, if_lesser<Index, count> = true>
    Storage& file() NOEXCEPT { return std::get<Index>(files_); }

    template <size_t Index, if_lesser<Index, count> = true>
    const Storage& file() const NOEXCEPT { return std::get<Index>(files_); }

    maps(const path& base_path, size_t size, size_t rate,
        bool random_access) NOEXCEPT
      : maps{ base_path, size, rate, random_access, sequence }
    {
    }

    DISPATCH_METHOD(create, const)
    DISPATCH_METHOD(open)
    DISPATCH_METHOD(close)
    DISPATCH_METHOD(load)
    DISPATCH_METHOD(reload)
    DISPATCH_METHOD(flush)
    DISPATCH_METHOD(unload)
    DISPATCH_METHOD(get_fault, const)

    /// Macro not used because of accumulator.
    size_t get_space() const NOEXCEPT
    {
        size_t total{};
        std::apply([&](const auto&... column)
        {
            ((total += column.get_space()), ...);
        }, files_);
        return total;
    }

    /// Inject column filename extensions.
    code dump(const path& other_path) const NOEXCEPT
    {
        code ec{};
        std::apply([&](const auto&... column)
        {
            size_t index{};
            ((ec = ec ? ec : column.dump(to_subpath(other_path,
                suffixes[index++]))), ...);
        }, files_);
        return ec;
    }

protected:
    template <size_t... Index>
    maps(const path& base_path, size_t size, size_t rate, bool random_access,
        std::index_sequence<Index...>) NOEXCEPT
      : base_path_{ base_path },
        files_{ Storage
        {
            to_subpath(base_path, suffixes[Index]),
            size, ////system::ceilinged_multiply(size, widths[Index]) / stride,
            rate,
            random_access
        }... }
    {
    }

    path to_subpath(const path& base, const std::string_view& suffix) const
    {
        auto out = base;
        out.replace_extension();
        out += "_";
        out += suffix;
        out += base.extension();
        return out;
    }

    // These are thread safe.
    const path base_path_;
    std::array<Storage, count> files_;
};

} // namespace database
} // namespace libbitcoin

#undef DISPATCH_METHOD

#endif
