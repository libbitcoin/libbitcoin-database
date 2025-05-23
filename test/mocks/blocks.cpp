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
#include "../test.hpp"
#include "blocks.hpp"

namespace test {

// Setting block metadata on a shared instance creates test side effects.
// Chain objects such as blocks cannot be copied for side-effect-free metadata
// tests, since block copy takes shared pointer references. So create new test
// blocks for each metadata test.
block get_bogus_block()
{
    return block
    {
        header
        {
            0x31323334,
            system::null_hash,
            system::one_hash,
            0x41424344,
            0x51525354,
            0x61626364
        },
        transactions
        {
            transaction
            {
                0x01,
                inputs
                {
                    input
                    {
                        point{},
                        script{},
                        witness{},
                        0x02
                    },
                    input
                    {
                        point{},
                        script{},
                        witness{},
                        0x03
                    }
                },
                outputs
                {
                    output
                    {
                        0x04,
                        script{}
                    }
                },
                0x05
            },
            transaction
            {
                0x06,
                inputs
                {
                    input
                    {
                        point{},
                        script{},
                        witness{},
                        0x07
                    },
                    input
                    {
                        point{},
                        script{},
                        witness{},
                        0x08
                    }
                },
                outputs
                {
                    output
                    {
                        0x09,
                        script{}
                    }
                },
                0x0a
            },
            transaction
            {
                0x0b,
                inputs
                {
                    input
                    {
                        point{},
                        script{},
                        witness{},
                        0x0c
                    },
                    input
                    {
                        point{},
                        script{},
                        witness{},
                        0x0d
                    }
                },
                outputs
                {
                    output
                    {
                        0x0e,
                        script{}
                    }
                },
                0x0f
            }
        }
    };
}

} // namespace test
