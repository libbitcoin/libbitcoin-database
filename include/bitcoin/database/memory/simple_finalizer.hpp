/**
 * Copyright (c) 2011-2022 libbitcoin developers (see AUTHORS)
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
#ifndef LIBBITCOIN_DATABASE_MEMORY_SIMPLE_FINALIZER_HPP
#define LIBBITCOIN_DATABASE_MEMORY_SIMPLE_FINALIZER_HPP

#include <functional>
#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/simple_writer.hpp>

namespace libbitcoin {
namespace database {

class simple_finalizer
  : public simple_writer
{
public:
    using finalization = std::function<bool()>;

    using simple_writer::simple_writer;

    inline void set_finalizer(finalization&& functor) NOEXCEPT
    {
        finalize_ = std::move(functor);
    }

    // This is expected to have side effect on the stream buffer, specifically
    // setting the "next" pointer into beginning of the address space.
    inline bool finalize() NOEXCEPT
    {
        if (!*this)
            return false;

        // std::function does not allow for noexcept.
        BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
        return finalize_();
        BC_POP_WARNING()
    }

private:
    finalization finalize_;
};

} // namespace database
} // namespace libbitcoin

#endif
