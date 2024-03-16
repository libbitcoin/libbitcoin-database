/**
 * Copyright (c) 2011-2023 libbitcoin developers (see AUTHORS)
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
#ifndef LIBBITCOIN_DATABASE_TEST_MOCKS_MAP_STORE_HPP
#define LIBBITCOIN_DATABASE_TEST_MOCKS_MAP_STORE_HPP

#include "../test.hpp"

namespace test {

// nop event handler.
const auto events = [](auto, auto) {};

// store<map> test accessor.
class map_store
  : public store<map>
{
public:
    using path = std::filesystem::path;
    using store<map>::store;

    // backup internals

    inline code backup_() NOEXCEPT
    {
        return backup(events);
    }

    inline code dump_(const std::filesystem::path& folder) NOEXCEPT
    {
        return dump(folder, events);
    }

    inline code restore_() NOEXCEPT
    {
        return restore(events);
    }

    inline const settings& configuration() const NOEXCEPT
    {
        return configuration_;
    }

    // Archives.

    inline const path& header_head_file() const NOEXCEPT
    {
        return header_head_.file();
    }

    inline const path& header_body_file() const NOEXCEPT
    {
        return header_body_.file();
    }

    inline const path& point_head_file() const NOEXCEPT
    {
        return point_head_.file();
    }

    inline const path& point_body_file() const NOEXCEPT
    {
        return point_body_.file();
    }

    inline const path& input_head_file() const NOEXCEPT
    {
        return input_head_.file();
    }

    inline const path& input_body_file() const NOEXCEPT
    {
        return input_body_.file();
    }

    inline const path& output_head_file() const NOEXCEPT
    {
        return output_head_.file();
    }

    inline const path& output_body_file() const NOEXCEPT
    {
        return output_body_.file();
    }

    inline const path& puts_head_file() const NOEXCEPT
    {
        return puts_head_.file();
    }

    inline const path& puts_body_file() const NOEXCEPT
    {
        return puts_body_.file();
    }

    inline const path& tx_head_file() const NOEXCEPT
    {
        return tx_head_.file();
    }

    inline const path& tx_body_file() const NOEXCEPT
    {
        return tx_body_.file();
    }

    inline const path& txs_head_file() const NOEXCEPT
    {
        return txs_head_.file();
    }

    inline const path& txs_body_file() const NOEXCEPT
    {
        return txs_body_.file();
    }

    // Indexes.

    inline const path& address_head_file() const NOEXCEPT
    {
        return address_head_.file();
    }

    inline const path& address_body_file() const NOEXCEPT
    {
        return address_body_.file();
    }

    inline const path& candidate_head_file() const NOEXCEPT
    {
        return candidate_head_.file();
    }

    inline const path& candidate_body_file() const NOEXCEPT
    {
        return candidate_body_.file();
    }

    inline const path& confirmed_head_file() const NOEXCEPT
    {
        return confirmed_head_.file();
    }

    inline const path& confirmed_body_file() const NOEXCEPT
    {
        return confirmed_body_.file();
    }

    inline const path& spend_head_file() const NOEXCEPT
    {
        return spend_head_.file();
    }

    inline const path& spend_body_file() const NOEXCEPT
    {
        return spend_body_.file();
    }

    inline const path& strong_tx_head_file() const NOEXCEPT
    {
        return strong_tx_head_.file();
    }

    inline const path& strong_tx_body_file() const NOEXCEPT
    {
        return strong_tx_body_.file();
    }

    // Caches.

    inline const path& bootstrap_head_file() const NOEXCEPT
    {
        return bootstrap_head_.file();
    }

    inline const path& bootstrap_body_file() const NOEXCEPT
    {
        return bootstrap_body_.file();
    }

    inline const path& buffer_head_file() const NOEXCEPT
    {
        return buffer_head_.file();
    }

    inline const path& buffer_body_file() const NOEXCEPT
    {
        return buffer_body_.file();
    }

    inline const path& neutrino_head_file() const NOEXCEPT
    {
        return neutrino_head_.file();
    }

    inline const path& neutrino_body_file() const NOEXCEPT
    {
        return neutrino_body_.file();
    }

    inline const path& validated_bk_head_file() const NOEXCEPT
    {
        return validated_bk_head_.file();
    }

    inline const path& validated_bk_body_file() const NOEXCEPT
    {
        return validated_bk_body_.file();
    }

    inline const path& validated_tx_head_file() const NOEXCEPT
    {
        return validated_tx_head_.file();
    }

    inline const path& validated_tx_body_file() const NOEXCEPT
    {
        return validated_tx_body_.file();
    }

    // Locks.

    inline const path& flush_lock_file() const NOEXCEPT
    {
        return flush_lock_.file();
    }

    inline const path& process_lock_file() const NOEXCEPT
    {
        return process_lock_.file();
    }

    inline std::shared_timed_mutex& transactor_mutex() NOEXCEPT
    {
        return transactor_mutex_;
    }
};

} // namespace test

#endif
