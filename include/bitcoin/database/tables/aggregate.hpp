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
#ifndef LIBBITCOIN_DATABASE_AGGREGATE_HPP
#define LIBBITCOIN_DATABASE_AGGREGATE_HPP

#include <tuple>
#include <filesystem>
#include <bitcoin/database/define.hpp>

// Variadic forwarding macro with early termination.
#define AGGREGATE_FORWARD(method, ...) \
    code method() __VA_ARGS__ NOEXCEPT \
    { \
        code ec{ error::success }; \
        std::apply([&](auto&... column) \
        { \
            ((ec = ec ? ec : column.method()), ...); \
        }, columns_); \
        return ec; \
    }

namespace libbitcoin {
namespace database {

/// Table column.
/// ---------------------------------------------------------------------------

/// Variadic template for defining an SoA aggregate table file.
template <typename... Columns>
class table_columns
{
public:
    using path = std::filesystem::path;
    static constexpr std::array suffixes{ Columns::suffix... };

    table_columns(const path& base_path, size_t size, size_t rate,
        bool random_access) NOEXCEPT
      : base_path_(base_path),
        columns_
        (
            std::apply
            (
                [&]<std::size_t... Index>(std::index_sequence<Index...>)
                {
                    return std::make_tuple
                    (
                        typename Columns::type
                        {
                            to_subpath(base_path, suffixes[Index]),
                            size,
                            rate,
                            random_access
                        }...
                    );
                }, std::make_index_sequence<sizeof...(Columns)>{}
            )
        )
    {
    }

    AGGREGATE_FORWARD(get_fault, const)
    AGGREGATE_FORWARD(create, const)
    AGGREGATE_FORWARD(open)
    AGGREGATE_FORWARD(close)
    AGGREGATE_FORWARD(load)
    AGGREGATE_FORWARD(reload)
    AGGREGATE_FORWARD(flush)
    AGGREGATE_FORWARD(unload)

    /// Accumulate required space total (for disk full).
    size_t get_space() const NOEXCEPT
    {
        size_t total{};
        std::apply([&](const auto&... column)
        {
            ((total += column.get_space()), ...);
        }, columns_);
        return total;
    }

    /// Inject column filename extensions.
    code dump(const path& other_path) const NOEXCEPT
    {
        code ec{};
        constexpr std::array suffix{ Columns::suffix... };
        std::apply([&](const auto&... column)
        {
            size_t index{};
            ((ec = ec ? ec : column.dump(to_subpath(other_path,
                suffix[index++]))), ...);
        }, columns_);
        return ec;
    }

protected:
    path to_subpath(const path& base, const std::string_view& suffix) const
    {
        auto out = base;
        out.replace_extension();
        out += suffix;
        out += base.extension();
        return out;
    }

    // These are thread safe.
    const path base_path_;
    std::tuple<typename Columns::type...> columns_;
};

/// Concrete columns.
/// ---------------------------------------------------------------------------

constexpr char correlate[]  = "_correlate";
constexpr char digest[]     = "_digest";
constexpr char compressed[] = "_compressed";
constexpr char signature[]  = "_signature";
constexpr char xonly[]      = "_xonly";
constexpr char prefix[]     = "_prefix";

template <typename Type, const char* Suffix>
struct Column
{
    using type = Type;
    static constexpr const char* suffix = Suffix;
};

template <typename Storage>
using ecdsa_columns = table_columns
<
    Column<Storage, correlate>,
    Column<Storage, digest>,
    Column<Storage, compressed>,
    Column<Storage, signature>
>;

template <typename Storage>
class ecdsa_storage final
  : public ecdsa_columns<Storage>
{
public:
    ecdsa_storage(const std::filesystem::path& path, size_t size,
        size_t rate, bool random_access) NOEXCEPT
      : ecdsa_columns<Storage>(path, size, rate, random_access)
    {
    }

    Storage& correlate  = std::get<0>(this->columns_);
    Storage& digest     = std::get<1>(this->columns_);
    Storage& compressed = std::get<2>(this->columns_);
    Storage& signature  = std::get<3>(this->columns_);
};

template <typename Storage>
using schnorr_columns = table_columns
<
    Column<Storage, correlate>,
    Column<Storage, digest>,
    Column<Storage, xonly>,
    Column<Storage, signature>
>;

template <typename Storage>
class schnorr_storage final
  : public schnorr_columns<Storage>
{
public:
    schnorr_storage(const std::filesystem::path& path, size_t size,
        size_t rate, bool random_access) NOEXCEPT
      : schnorr_columns<Storage>(path, size, rate, random_access)
    {
    }

    Storage& correlate  = std::get<0>(this->columns_);
    Storage& digest     = std::get<1>(this->columns_);
    Storage& xonly      = std::get<2>(this->columns_);
    Storage& signature  = std::get<3>(this->columns_);
};

template <typename Storage>
using silent_columns = table_columns
<
    Column<Storage, correlate>,
    Column<Storage, prefix>,
    Column<Storage, compressed>
>;

template <typename Storage>
class silent_storage final
  : public silent_columns<Storage>
{
public:
    silent_storage(const std::filesystem::path& path, size_t size,
        size_t rate, bool random_access) NOEXCEPT
      : silent_columns<Storage>(path, size, rate, random_access)
    {
    }

    Storage& correlate  = std::get<0>(this->columns_);
    Storage& prefix     = std::get<1>(this->columns_);
    Storage& compressed = std::get<2>(this->columns_);
};

/// Table of columns.
/// ---------------------------------------------------------------------------

/// Variadic template for defining an SoA aggregate table.
template <typename Storage, typename... Columns>
class aggregate_table
{
public:
    aggregate_table(Storage& head, Storage& body) NOEXCEPT
      : columns_(std::make_tuple(typename Columns::type{ head, body }...))
    {
    }

    AGGREGATE_FORWARD(create)
    AGGREGATE_FORWARD(close)
    AGGREGATE_FORWARD(verify)
    AGGREGATE_FORWARD(restore)

    code backup(bool prune) NOEXCEPT
    {
        code ec{};
        std::apply([&](auto&... column)
        {
            ((ec = ec ? ec : column.backup(prune)), ...);
        }, columns_);
        return ec;
    }

protected:
    std::tuple<typename Columns::type...> columns_;
};

/// Concrete tables.
/// ---------------------------------------------------------------------------

template <typename Storage>
using ecdsa_table = aggregate_table
<
    Storage,
    Column<table::ecdsa_correlate,  correlate>,
    Column<table::ecdsa_digest,     digest>,
    Column<table::ecdsa_compressed, compressed>,
    Column<table::ecdsa_signature,  signature>
>;

template <typename Storage>
class ecdsa final
  : public ecdsa_table<Storage>
{
public:
    ecdsa(Storage& head, Storage& body) NOEXCEPT
      : ecdsa_table<Storage>(head, body)
    {
    }

    table::ecdsa_correlate&  correlate  = std::get<0>(this->columns_);
    table::ecdsa_digest&     digest     = std::get<1>(this->columns_);
    table::ecdsa_compressed& compressed = std::get<2>(this->columns_);
    table::ecdsa_signature&  signature  = std::get<3>(this->columns_);
};

template <typename Storage>
using schnorr_table = aggregate_table
<
    Storage,
    Column<table::schnorr_correlate, correlate>,
    Column<table::schnorr_digest,    digest>,
    Column<table::schnorr_xonly,     xonly>,
    Column<table::schnorr_signature, signature>
>;

template <typename Storage>
class schnorr final
  : public schnorr_table<Storage>
{
public:
    schnorr(Storage& head, Storage& body) NOEXCEPT
      : schnorr_table<Storage>(head, body)
    {
    }

    table::schnorr_correlate& correlate = std::get<0>(this->columns_);
    table::schnorr_digest&    digest    = std::get<1>(this->columns_);
    table::schnorr_xonly&     xonly     = std::get<2>(this->columns_);
    table::schnorr_signature& signature = std::get<3>(this->columns_);
};

template <typename Storage>
using silent_table = aggregate_table
<
    Storage,
    Column<table::silent_correlate,  correlate>,
    Column<table::silent_prefix,     prefix>,
    Column<table::silent_compressed, compressed>
>;

template <typename Storage>
class silent final
  : public silent_table<Storage>
{
public:
    silent(Storage& head, Storage& body) NOEXCEPT
      : silent_table<Storage>(head, body)
    {
    }

    table::silent_correlate&  correlate  = std::get<0>(this->columns_);
    table::silent_prefix&     prefix     = std::get<1>(this->columns_);
    table::silent_compressed& compressed = std::get<2>(this->columns_);
};

} // namespace database
} // namespace libbitcoin

#undef AGGREGATE_FORWARD

#endif
