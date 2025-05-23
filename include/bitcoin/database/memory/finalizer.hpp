/**
 * Copyright (c) 2011-2025 libbitcoin developers (see AUTHORS)
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
#ifndef LIBBITCOIN_DATABASE_MEMORY_FINALIZER_HPP
#define LIBBITCOIN_DATABASE_MEMORY_FINALIZER_HPP

#include <memory>
#include <utility>
#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

// This is single inheritance.
BC_PUSH_WARNING(DIAMOND_INHERITANCE)

/// A byte flipper with finalization extentions, that accepts an iostream.
template <typename IOStream = system::stream::flip::fast>
class finalizer_
  : public system::byte_flipper<IOStream>
{
public:
    DEFAULT_COPY_MOVE_DESTRUCT(finalizer_);

    using finalization = std::function<bool()>;

    finalizer_(IOStream& stream) NOEXCEPT
      : system::byte_flipper<IOStream>(stream)
    {
    }

    void set_finalizer(finalization&& functor) NOEXCEPT
    {
        finalize_ = std::move(functor);
    }

    // This is expected to have side effect on the stream buffer, specifically
    // setting the "next" pointer into beginning of the address space.
    bool finalize() NOEXCEPT
    {
        if (!*this) return false;

        // std::function does not allow for noexcept.
        BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
        return finalize_();
        BC_POP_WARNING()
    }

private:
    finalization finalize_;
};

/// A finalizing byte reader/writer that copies data from/to a memory_ptr.
using finalizer = finalizer_<>;

BC_POP_WARNING()

} // namespace database
} // namespace libbitcoin

#endif
