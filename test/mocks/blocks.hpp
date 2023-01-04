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
#ifndef LIBBITCOIN_DATABASE_TEST_MOCKS_BLOCKS_HPP
#define LIBBITCOIN_DATABASE_TEST_MOCKS_BLOCKS_HPP

#include "../test.hpp"

namespace test {

// blockchain.info/rawblock/[block-hash]?format=hex
constexpr auto block1_hash = system::base16_hash(
    "00000000839a8e6886ab5951d76f411475428afc90947ee320161bbf18eb6048");
constexpr auto block1_data = system::base16_array(
    "010000006fe28c0ab6f1b372c1a6a246ae63f74f931e8365e15a089c68d61900"
    "00000000982051fd1e4ba744bbbe680e1fee14677ba1a3c3540bf7b1cdb606e8"
    "57233e0e61bc6649ffff001d01e3629901010000000100000000000000000000"
    "00000000000000000000000000000000000000000000ffffffff0704ffff001d"
    "0104ffffffff0100f2052a0100000043410496b538e853519c726a2c91e61ec1"
    "1600ae1390813a627c66fb8be7947be63c52da7589379515d4e0a604f8141781"
    "e62294721166bf621e73a82cbf2342c858eeac00000000");
constexpr auto block2_hash = system::base16_hash(
    "000000006a625f06636b8bb6ac7b960a8d03705d1ace08b1a19da3fdcc99ddbd");
constexpr auto block2_data = system::base16_array(
    "010000004860eb18bf1b1620e37e9490fc8a427514416fd75159ab86688e9a83"
    "00000000d5fdcc541e25de1c7a5addedf24858b8bb665c9f36ef744ee42c3160"
    "22c90f9bb0bc6649ffff001d08d2bd6101010000000100000000000000000000"
    "00000000000000000000000000000000000000000000ffffffff0704ffff001d"
    "010bffffffff0100f2052a010000004341047211a824f55b505228e4c3d5194c"
    "1fcfaa15a456abdf37f9b9d97a4040afc073dee6c89064984f03385237d92167"
    "c13e236446b417ab79a0fcae412ae3316b77ac00000000");
constexpr auto block3_hash = system::base16_hash(
    "0000000082b5015589a3fdf2d4baff403e6f0be035a5d9742c1cae6295464449");
constexpr auto block3_data = system::base16_array(
    "01000000bddd99ccfda39da1b108ce1a5d70038d0a967bacb68b6b63065f626a"
    "0000000044f672226090d85db9a9f2fbfe5f0f9609b387af7be5b7fbb7a1767c"
    "831c9e995dbe6649ffff001d05e0ed6d01010000000100000000000000000000"
    "00000000000000000000000000000000000000000000ffffffff0704ffff001d"
    "010effffffff0100f2052a0100000043410494b9d3e76c5b1629ecf97fff95d7"
    "a4bbdac87cc26099ada28066c6ff1eb9191223cd897194a08d0c2726c5747f1d"
    "b49e8cf90e75dc3e3550ae9b30086f3cd5aaac00000000");

constexpr hash_digest two_hash = system::from_uintx(uint256_t(two));
constexpr database::context context
{
    0x01020304, // flags
    0x11121314, // height
    0x21222324  // mtp
};

using namespace system::chain;
const auto genesis = system::settings{ selection::mainnet }.genesis_block;
const block block1{ test::block1_data, true };
const block block2{ test::block2_data, true };
const block block3{ test::block3_data, true };
const block block1a
{
    header
    {
        0x31323334,         // version
        genesis.hash(),     // previous_block_hash
        system::null_hash,  // merkle_root
        0x41424344,         // timestamp
        0x51525354,         // bits
        0x61626364          // nonce
    },
    transactions
    {
        // This first transaction is *not* a coinbase.
        transaction
        {
            0x2a,           // version
            inputs
            {
                input
                {
                    point{ system::one_hash, 0x18 },    // missing prevout
                    script{ { { opcode::op_return }, { opcode::pick } } },
                    witness{ "[242424]" },
                    0x2a    // sequence
                },
                input
                {
                    point{ system::one_hash, 0x2a },    // missing prevout
                    script{ { { opcode::op_return }, { opcode::roll } } },
                    witness{ "[313131]" },
                    0x18    // sequence
                },
                input
                {
                    point{ two_hash, 0x2b },            // missing prevout
                    script{ { { opcode::op_return }, { opcode::roll } } },
                    witness{ "[424242]" },
                    0x19    // sequence
                }
            },
            outputs
            {
                output
                {
                    0x18,   // value
                    script{ { { opcode::pick } } }
                },
                output
                {
                    0x2a,   // value
                    script{ { { opcode::roll } } }
                }
            },
            0x18            // locktime
        }
    }
};
const block block2a
{
    header
    {
        0x31323334,         // version
        block1a.hash(),     // previous_block_hash
        system::one_hash,   // merkle_root
        0x41424344,         // timestamp
        0x51525354,         // bits
        0x61626364          // nonce
    },
    transactions
    {
        // This first transaction is *not* a coinbase.
        transaction
        {
            0xa2,           // version
            inputs
            {
                input
                {
                    // existing prevout
                    point{ block1a.transactions_ptr()->front()->hash(false), 0x00 },
                    script{ { { opcode::checkmultisig }, { opcode::pick } } },
                    witness{ "[242424]" },
                    0xa2    // sequence
                },
                input
                {
                    // existing prevout
                    point{ block1a.transactions_ptr()->front()->hash(false), 0x01 },
                    script{ { { opcode::checkmultisig }, { opcode::roll } } },
                    witness{ "[313131]" },
                    0x81    // sequence
                }
            },
            outputs
            {
                output
                {
                    0x81,   // value
                    script{ { { opcode::pick } } }
                }
            },
            0x81            // locktime
        },
        transaction
        {
            0xa2,           // version
            inputs
            {
                input
                {
                    point{ system::one_hash, 0x20 },    // missing prevout
                    script{ { { opcode::checkmultisig }, { opcode::pick } } },
                    witness{ "[242424]" },
                    0xa2    // sequence
                },
                input
                {
                    point{ system::one_hash, 0x21 },    // missing prevout
                    script{ { { opcode::checkmultisig }, { opcode::roll } } },
                    witness{ "[313131]" },
                    0x81    // sequence
                }
            },
            outputs
            {
                output
                {
                    0x81,   // value
                    script{ { { opcode::pick } } }
                }
            },
            0x81            // locktime
        }
    }
};
const transaction tx4
{
    0xa5,           // version
    inputs
    {
        input
        {
            // existing prevout (double spend with block2a:tx0:0).
            point{ block1a.transactions_ptr()->front()->hash(false), 0x00 },
            script{ { { opcode::checkmultisig }, { opcode::pick } } },
            witness{ "[252525]" },
            0xa5    // sequence
        },
        input
        {
            // existing prevout (double spend with block2a:tx0:1).
            point{ block1a.transactions_ptr()->front()->hash(false), 0x01 },
            script{ { { opcode::checkmultisig }, { opcode::roll } } },
            witness{ "[353535]" },
            0x85    // sequence
        }
    },
    outputs
    {
        output
        {
            0x85,   // value
            script{ { { opcode::pick } } }
        }
    },
    0x85            // locktime
};
const transaction tx_spend_genesis
{
    0xa6,
    inputs
    {
        input
        {
            // Spend genesis.
            point{ genesis.transactions_ptr()->front()->hash(false), 0x00 },
            script{ { { opcode::checkmultisig }, { opcode::pick } } },
            witness{ "[262626]" },
            0xa6
        }
    },
    outputs
    {
        output
        {
            0x86,
            script{ { { opcode::pick } } }
        }
    },
    0x86
};
const block block3a
{
    header
    {
        0x31323334,         // version
        block2a.hash(),     // previous_block_hash
        system::one_hash,   // merkle_root
        0x41424344,         // timestamp
        0x51525354,         // bits
        0x61626364          // nonce
    },
    transactions
    {
        // This first transaction is *not* a coinbase.
        transaction
        {
            0xa3,           // version
            inputs
            {
                input
                {
                    // existing prevout
                    point{ block1a.transactions_ptr()->front()->hash(false), 0x01 },
                    script{ { { opcode::checkmultisig }, { opcode::size } } },
                    witness{ "[949494]" },
                    0xa3    // sequence
                },
                input
                {
                    // existing prevout
                    point{ block1a.transactions_ptr()->front()->hash(false), 0x00 },
                    script{ { { opcode::checkmultisig }, { opcode::size } } },
                    witness{ "[919191]" },
                    0x83    // sequence
                }
            },
            outputs
            {
                output
                {
                    0x83,   // value
                    script{ { { opcode::pick } } }
                }
            },
            0x83            // locktime
        }
    }
};
const block block1b
{
    header
    {
        0x31323334,         // version
        genesis.hash(),     // previous_block_hash
        system::one_hash,   // merkle_root
        0x41424344,         // timestamp
        0x51525354,         // bits
        0x61626364          // nonce
    },
    transactions
    {
        // This first transaction is a coinbase.
        transaction
        {
            0xb1,
            inputs
            {
                input
                {
                    point{},
                    script{ { { opcode::checkmultisig }, { opcode::size } } },
                    witness{},
                    0xb1
                }
            },
            outputs
            {
                output
                {
                    0xb1,
                    script{ { { opcode::pick } } }
                }
            },
            0xb1
        }
    }
};
const transaction tx2b
{
    transaction
    {
        0xb1,
        inputs
        {
            input
            {
                // Spends block1b coinbase.
                point{ block1b.transactions_ptr()->front()->hash(false), 0x00 },
                script{ { { opcode::checkmultisig }, { opcode::size } } },
                witness{},
                0xb1
            }
        },
        outputs
        {
            output
            {
                0xb1,
                script{ { { opcode::pick } } }
            }
        },
        0xb1
    }
};

} // namespace test

#endif
