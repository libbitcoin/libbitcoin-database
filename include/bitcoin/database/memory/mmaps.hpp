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
#ifndef LIBBITCOIN_DATABASE_MEMORY_MMAPS_HPP
#define LIBBITCOIN_DATABASE_MEMORY_MMAPS_HPP

#include <filesystem>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/mmap.hpp>

namespace libbitcoin {
namespace database {

/// SoA aggregate body or head: one substitutable multi-column map (Storage)
/// over N column files, one shared allocation guard. Storage is the map
/// family. A shared (unstaged) aggregate maps its files, as a head.
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

    mmaps(const path& base_path, const storage_settings& settings,
        bool random_access, bool staged=false, bool shared=false) NOEXCEPT
      : mmaps(base_path, settings, random_access, staged, shared, sequence)
    {
    }

    /// Dump each column's logical map to a new column file of path.
    code dump(const path& path) const NOEXCEPT override
    {
        return dump_columns(path, sequence);
    }

protected:
    template <size_t... Index>
    mmaps(const path& base_path, const storage_settings& settings,
        bool random_access, bool staged, bool shared,
        std::index_sequence<Index...>) NOEXCEPT
      : base(paths{ to_subpath(base_path, suffixes[Index])... }, settings,
          random_access, staged, shared),
        base_path_{ base_path }
    {
    }

    template <size_t... Index>
    code dump_columns(const path& path,
        std::index_sequence<Index...>) const NOEXCEPT
    {
        code ec{};
        const auto failed = ((ec = dump_column(to_subpath(path,
            suffixes[Index]), Index)) || ...);

        return failed ? ec : error::success;
    }

    code dump_column(const path& path, size_t column) const NOEXCEPT
    {
        const auto ptr = base::get_at(column);
        if (!ptr)
            return error::unloaded_file;

        return file::create_file_ex(path, ptr.begin(), ptr.size());
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
