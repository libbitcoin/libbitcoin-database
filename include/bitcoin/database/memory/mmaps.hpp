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
#ifndef LIBBITCOIN_DATABASE_MEMORY_MMAPS_HPP
#define LIBBITCOIN_DATABASE_MEMORY_MMAPS_HPP

#include <algorithm>
#include <filesystem>
#include <utility>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/mmap.hpp>

namespace libbitcoin {
namespace database {

/// SoA aggregate body: one substitutable multi-column map (Storage) over N
/// column files, one shared allocation guard. Storage is the map family.
template <template <size_t...> class Storage, typename... Columns>
class mmaps
  : public Storage<Columns::width...>
{
public:
    using path = std::filesystem::path;
    using base = Storage<Columns::width...>;
    using paths = typename base::paths;

    static constexpr auto columns = sizeof...(Columns);
    static_assert(!is_zero(columns), "requires at least one column");
    static constexpr auto sequence = std::make_index_sequence<columns>{};
    static constexpr auto suffixes = std::array<std::string_view, columns>
    {
        std::string_view
        {
            Columns::suffix.data(),
            Columns::suffix.size()
        }...
    };

    mmaps(const path& base_path, size_t size, size_t rate,
        bool random_access, bool staged=false) NOEXCEPT
      : mmaps(base_path, size, rate, random_access, staged, sequence)
    {
    }

protected:
    template <size_t... Index>
    mmaps(const path& base_path, size_t size, size_t rate, bool random_access,
        bool staged, std::index_sequence<Index...>) NOEXCEPT
      : base(paths{ to_subpath(base_path, suffixes[Index])... }, size, rate,
          random_access, staged),
        base_path_{ base_path }
    {
    }

    static path to_subpath(const path& base,
        const std::string_view& suffix) NOEXCEPT
    {
        auto out = base;
        out.replace_extension();
        out += "_";
        out += suffix;
        out += base.extension();
        return out;
    }

    // This is thread safe.
    const path base_path_;
};

} // namespace database
} // namespace libbitcoin

#endif
