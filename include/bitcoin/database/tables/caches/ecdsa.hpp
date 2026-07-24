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
#ifndef LIBBITCOIN_DATABASE_TABLES_CACHES_ECDSA_HPP
#define LIBBITCOIN_DATABASE_TABLES_CACHES_ECDSA_HPP

#include <filesystem>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/primitives/primitives.hpp>
#include <bitcoin/database/tables/schema.hpp>

namespace libbitcoin {
namespace database {
namespace table {
    
/// ecdsa_digest is an array of ecdsa verification record signature hashes.
struct ecdsa_digest
  : public no_map<schema::ecdsa_digest>
{
    using no_map<schema::ecdsa_digest>::nomap;

    struct put_ref
      : public schema::ecdsa_digest
    {
        inline link count() const NOEXCEPT
        {
            return 1;
        }

        inline bool to_data(flipper& sink) const NOEXCEPT
        {
            sink.write_bytes(digest);
            BC_ASSERT(!sink || sink.get_write_position() == minrow);
            return sink;
        }

        const hash_digest& digest;
    };

    struct put_refs
      : public schema::ecdsa_digest
    {
        inline link count() const NOEXCEPT
        {
            return system::possible_narrow_cast<link::integer>(rows);
        }

        inline bool to_data(flipper& sink) const NOEXCEPT
        {
            // ecdsa multisig capture is limited to common signature hash.
            for (size_t row{}; row < rows; ++row)
                sink.write_bytes(digest);

            BC_ASSERT(!sink || sink.get_write_position() == count() * minrow);
            return sink;
        }

        const size_t rows{};
        const hash_digest& digest;
    };

    struct put_signatures
      : public schema::ecdsa_digest
    {
        inline link count() const NOEXCEPT
        {
            return system::possible_narrow_cast<link::integer>(sigs.rows());
        }

        inline bool to_data(flipper& sink) const NOEXCEPT
        {
            using namespace system;
            sigs.for_each([&](const hash_digest& digest,
                std::span<const ec_compressed> keys,
                std::span<const ec_signature> sigs) NOEXCEPT
            {
                // Group capture is limited to common signature hash.
                const auto rows = chain::multisig::rows(sigs.size(),
                    keys.size());

                for (size_t row{}; row < rows; ++row)
                    sink.write_bytes(digest);
            });

            BC_ASSERT(!sink || sink.get_write_position() == count() * minrow);
            return sink;
        }

        const system::chain::ecdsa_signatures& sigs;
    };
};

/// ecdsa_compressed is an array of ecdsa verification compressed public keys.
struct ecdsa_compressed
  : public no_map<schema::ecdsa_compressed>
{
    using no_map<schema::ecdsa_compressed>::nomap;

    struct put_ref
      : public schema::ecdsa_compressed
    {
        inline link count() const NOEXCEPT
        {
            return 1;
        }

        inline bool to_data(flipper& sink) const NOEXCEPT
        {
            sink.write_bytes(key);
            BC_ASSERT(!sink || sink.get_write_position() == minrow);
            return sink;
        }

        const system::ec_compressed& key;
    };

    struct put_refs
      : public schema::ecdsa_compressed
    {
        inline link count() const NOEXCEPT
        {
            return system::possible_narrow_cast<link::integer>(rows);
        }

        inline bool to_data(flipper& sink) const NOEXCEPT
        {
            using namespace system::chain;
            const auto m = sigs;
            const auto n = keys.size();
            BC_ASSERT(multisig::rows(m, n) == rows);

            const auto gap = (n - m);
            for (size_t sig{}; sig < m; ++sig)
                for (auto key = sig; key <= gap + sig; ++key)
                    sink.write_bytes(keys[key]);

            BC_ASSERT(!sink || sink.get_write_position() == count() * minrow);
            return sink;
        }

        const size_t rows{};
        const std::span<const system::ec_compressed> keys;
        const size_t sigs{};
    };

    struct put_signatures
      : public schema::ecdsa_compressed
    {
        inline link count() const NOEXCEPT
        {
            return system::possible_narrow_cast<link::integer>(sigs.rows());
        }

        inline bool to_data(flipper& sink) const NOEXCEPT
        {
            using namespace system;
            sigs.for_each([&](const hash_digest&,
                std::span<const ec_compressed> keys,
                std::span<const ec_signature> sigs) NOEXCEPT
            {
                const auto m = sigs.size();
                const auto gap = keys.size() - m;

                for (size_t sig{}; sig < m; ++sig)
                    for (auto key = sig; key <= gap + sig; ++key)
                        sink.write_bytes(keys[key]);
            });

            BC_ASSERT(!sink || sink.get_write_position() == count() * minrow);
            return sink;
        }

        const system::chain::ecdsa_signatures& sigs;
    };
};

/// ecdsa_signature is an array of ecdsa verification signatures.
struct ecdsa_signature
  : public no_map<schema::ecdsa_signature>
{
    using no_map<schema::ecdsa_signature>::nomap;

    struct put_ref
      : public schema::ecdsa_signature
    {
        inline link count() const NOEXCEPT
        {
            return 1;
        }

        inline bool to_data(flipper& sink) const NOEXCEPT
        {
            sink.write_bytes(signature);
            BC_ASSERT(!sink || sink.get_write_position() == minrow);
            return sink;
        }

        const system::ec_signature& signature;
    };

    struct put_refs
      : public schema::ecdsa_signature
    {
        inline link count() const NOEXCEPT
        {
            return system::possible_narrow_cast<link::integer>(rows);
        }

        inline bool to_data(flipper& sink) const NOEXCEPT
        {
            using namespace system::chain;
            const auto m = sigs.size();
            const auto n = keys;
            BC_ASSERT(multisig::rows(m, n) == rows);

            const auto gap = (n - m);
            for (size_t sig{}; sig < m; ++sig)
                for (auto key = sig; key <= gap + sig; ++key)
                    sink.write_bytes(sigs[sig]);

            BC_ASSERT(!sink || sink.get_write_position() == count() * minrow);
            return sink;
        }

        const size_t rows{};
        const size_t keys{};
        const std::span<const system::ec_signature> sigs;
    };

    struct put_signatures
      : public schema::ecdsa_signature
    {
        inline link count() const NOEXCEPT
        {
            return system::possible_narrow_cast<link::integer>(sigs.rows());
        }

        inline bool to_data(flipper& sink) const NOEXCEPT
        {
            using namespace system;
            sigs.for_each([&](const hash_digest&,
                std::span<const ec_compressed> keys,
                std::span<const ec_signature> sigs) NOEXCEPT
            {
                const auto m = sigs.size();
                const auto gap = keys.size() - m;

                for (size_t sig{}; sig < m; ++sig)
                    for (auto key = sig; key <= gap + sig; ++key)
                        sink.write_bytes(sigs[sig]);
            });

            BC_ASSERT(!sink || sink.get_write_position() == count() * minrow);
            return sink;
        }

        const system::chain::ecdsa_signatures& sigs;
    };
};

/// ecdsa_correlate is an array of ecdsa correlation records.
struct ecdsa_correlate
  : public no_map<schema::ecdsa_correlate>
{
    using hd = schema::header::link;
    using no_map<schema::ecdsa_correlate>::nomap;

    struct record
      : public schema::ecdsa_correlate
    {
        inline link count() const NOEXCEPT
        {
            return 1;
        }

        inline bool from_data(reader& source) NOEXCEPT
        {
            header_fk = source.read_little_endian<hd::integer, hd::size>();
            pair = source.read_byte();
            group = source.read_little_endian<uint16_t>();
            BC_ASSERT(!source || source.get_read_position() == minrow);
            return source;
        }

        hd::integer header_fk{};
        uint8_t pair{};
        uint16_t group{};
    };

    struct put_ref
      : public schema::ecdsa_correlate
    {
        inline link count() const NOEXCEPT
        {
            return 1;
        }

        inline bool to_data(flipper& sink) const NOEXCEPT
        {
            // 0 is required for single sig (0|0 -> 1 of 1).
            constexpr uint8_t pair = 0;
            sink.write_little_endian<hd::integer, hd::size>(header_fk);
            sink.write_byte(pair);
            sink.write_little_endian<uint16_t>(group);
            BC_ASSERT(!sink || sink.get_write_position() == minrow);
            return sink;
        }

        const hd::integer header_fk{};
        const uint16_t group{};
    };

    struct put_refs
      : public schema::ecdsa_correlate
    {
        inline link count() const NOEXCEPT
        {
            return system::possible_narrow_cast<link::integer>(rows);
        }

        inline bool to_data(flipper& sink) const NOEXCEPT
        {
            using namespace system::chain;
            const auto m = sigs;
            const auto n = keys;
            BC_ASSERT(multisig::rows(m, n) == rows);

            const auto gap = (n - m);
            for (size_t sig{}; sig < m; ++sig)
            {
                for (auto key = sig; key <= gap + sig; ++key)
                {
                    sink.write_little_endian<hd::integer, hd::size>(header_fk);
                    sink.write_byte(system::pack_word<uint8_t>(sig, key));
                    sink.write_little_endian<uint16_t>(group);
                }
            }

            BC_ASSERT(!sink || sink.get_write_position() == count() * minrow);
            return sink;
        }

        const size_t rows{};
        const hd::integer header_fk{};
        const size_t keys{};
        const size_t sigs{};
        const uint16_t group{};
    };

    struct put_signatures
      : public schema::ecdsa_correlate
    {
        inline link count() const NOEXCEPT
        {
            return system::possible_narrow_cast<link::integer>(sigs.rows());
        }

        inline bool to_data(flipper& sink) const NOEXCEPT
        {
            using namespace system;
            uint16_t group{};
            sigs.for_each([&](const hash_digest&,
                std::span<const ec_compressed> keys,
                std::span<const ec_signature> sigs) NOEXCEPT
            {
                const auto m = sigs.size();
                const auto gap = keys.size() - m;

                // Group id is the group capture ordinal (ecdsa_signatures).
                for (size_t sig{}; sig < m; ++sig)
                {
                    for (auto key = sig; key <= gap + sig; ++key)
                    {
                        sink.write_little_endian<hd::integer, hd::size>(
                            header_fk);
                        sink.write_byte(pack_word<uint8_t>(sig, key));
                        sink.write_little_endian<uint16_t>(group);
                    }
                }

                ++group;
            });

            BC_ASSERT(!sink || sink.get_write_position() == count() * minrow);
            return sink;
        }

        const hd::integer header_fk{};
        const system::chain::ecdsa_signatures& sigs;
    };
};

/// Aggregate (files)
/// ---------------------------------------------------------------------------

template <template <size_t...> class Storage>
using ecdsa_files = mmaps
<
    Storage,
    ecdsa_correlate,
    ecdsa_digest,
    ecdsa_compressed,
    ecdsa_signature
>;

template <template <size_t...> class Storage>
class ecdsa_storage
  : public ecdsa_files<Storage>
{
public:
    ecdsa_storage(const std::filesystem::path& path, size_t size,
        size_t rate, bool random_access, bool staged=false) NOEXCEPT
      : ecdsa_files<Storage>(path, size, rate, random_access, staged)
    {
    }
};

/// Aggregate (table)
/// ---------------------------------------------------------------------------

using ecdsa_table = nomaps
<
    ecdsa_correlate::link,
    ecdsa_correlate,
    ecdsa_digest,
    ecdsa_compressed,
    ecdsa_signature
>;

template <template <size_t...> class Storage>
class ecdsa
  : public ecdsa_table
{
public:
    ecdsa(database::storage& head, ecdsa_storage<Storage>& body) NOEXCEPT
      : ecdsa_table(head, body),
        correlate(*this),
        digest(*this),
        compressed(*this),
        signature(*this)
    {
    }

    column<ecdsa_table, 0> correlate;
    column<ecdsa_table, 1> digest;
    column<ecdsa_table, 2> compressed;
    column<ecdsa_table, 3> signature;
};

static_assert(sizeof(system::ecdsa::batch::correlate_t) ==
    ecdsa_correlate::width);

} // namespace table
} // namespace database
} // namespace libbitcoin

#endif
