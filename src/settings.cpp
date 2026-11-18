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
#include <bitcoin/database/settings.hpp>

#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

using namespace bc::system;

settings::settings() NOEXCEPT
{
    // archive

    header.rate = 1;
    txs.rate = 1;
    tx.rate = 1;
    ins.rate = 1;
    outs.rate = 1;
    input.rate = 1;
    output.rate = 1;

    // indexes

    candidate.rate = 1;
    confirmed.rate = 1;
    strong.rate = 1;
    scan.size = 0;
    scan.rate = 1;

    // caches

    ecdsa.size = 0;
    ecdsa.rate = 1;
    schnorr.size = 0;
    schnorr.rate = 1;
    silent.size = 0;
    silent.rate = 1;
    prevalid.size = 0;
    prevalid.rate = 1;
    presilent.size = 0;
    presilent.rate = 1;

    prevout.buckets = 0;
    prevout.size = 0;
    prevout.rate = 1;

    duplicate.buckets = 1024;
    duplicate.expected = 0;
    duplicate.size = 0;
    duplicate.rate = 1;

    state.rate = 1;

    // pool and spends (disabled by zero buckets)
    pool.expected = 10'000'000;
    pool.size = 0;
    pool.rate = 1;
    wtxid.expected = 10'000'000;
    wtxid.size = 0;
    wtxid.rate = 1;
    spends.size = 0;
    spends.rate = 1;

    // optional

    // also disabled by filter_tx
    filter_bk.buckets = 0;
    filter_bk.size = 0;
    filter_bk.rate = 1;

    // also disabled by filter_bk
    filter_tx.buckets = 0;
    filter_tx.size = 0;
    filter_tx.rate = 1;

    envelope.set(*this);
}

settings::settings(chain::selection context) NOEXCEPT
  : settings()
{
    network = context;
    path = std::string{ system::config::selection::to_word(context) };

    // expected are element counts, deriving filter k at create.
    // sizes are set to 1% of measured pruned body (tiny tables 100%).
    // strong buckets are set to expected tx.
    switch (context)
    {
        // Sized for the chain at 950,000 blocks.
        case chain::selection::mainnet:
        {
            header.expected = 962'953;
            header.size = 124'220'679;
            txs.buckets = 950'001;
            txs.size = 54'471'017;
            tx.expected = 1'359'871'695;
            tx.size = 870'317'885;
            ins.expected = 3'363'467'251;
            ins.size = 1'749'002'971;
            outs.expected = 3'741'929'086;
            outs.size = 336'773'618;
            input.size = 67'269'346;
            output.size = 1'278'023'220;
            candidate.buckets = 950'001;
            confirmed.buckets = 950'001;
            strong.buckets = 1'359'871'695;
            scan.buckets = 950'001;
            state.buckets = 950'001;
            break;
        }

        // Sized for the chain at 5,100,000 blocks.
        case chain::selection::testnet3:
        {
            header.expected = 5'158'746;
            header.size = 588'097'044;
            txs.buckets = 5'100'001;
            txs.size = 21'874'552;
            tx.expected = 536'643'888;
            tx.size = 343'452'088;
            ins.expected = 598'161'444;
            ins.size = 311'043'951;
            outs.expected = 1'166'697'559;
            outs.size = 105'002'780;
            input.size = 981'182'034;
            output.size = 600'719'803;
            candidate.buckets = 5'100'001;
            confirmed.buckets = 5'100'001;
            strong.buckets = 536'643'888;
            scan.buckets = 5'100'001;
            state.buckets = 5'100'001;
            break;
        }

        // Sized for the testnet4 chain at 150,000 blocks.
        case chain::selection::testnet4:
        case chain::selection::testnet5:
        {
            header.expected = 156'456;
            header.size = 17'835'984;
            txs.buckets = 150'001;
            txs.size = 604'424;
            tx.expected = 14'810'011;
            tx.size = 9'478'407;
            ins.expected = 33'127'580;
            ins.size = 17'226'342;
            outs.expected = 57'089'349;
            outs.size = 5'138'041;
            input.size = 87'852'864;
            output.size = 20'343'818;
            candidate.buckets = 150'001;
            confirmed.buckets = 150'001;
            strong.buckets = 14'810'011;
            scan.buckets = 150'001;
            state.buckets = 150'001;
            break;
        }

        // Sized for a small local chain.
        case chain::selection::regtest:
        {
            header.expected = 10'000;
            header.size = 0;
            txs.buckets = 10'001;
            txs.size = 0;
            tx.expected = 100'000;
            tx.size = 0;
            ins.expected = 200'000;
            ins.size = 0;
            outs.expected = 300'000;
            outs.size = 0;
            input.size = 0;
            output.size = 0;
            candidate.buckets = 10'001;
            confirmed.buckets = 10'001;
            strong.buckets = 100'000;
            scan.buckets = 10'001;
            state.buckets = 10'001;
            break;
        }

        // Chain height tables sized for the chain at 330,000 blocks.
        case chain::selection::signet:
        {
            header.expected = 330'000;
            header.size = 0;
            txs.buckets = 330'001;
            txs.size = 0;
            tx.expected = 35'000'000;
            tx.size = 0;
            ins.expected = 90'000'000;
            ins.size = 0;
            outs.expected = 175'000'000;
            outs.size = 0;
            input.size = 0;
            output.size = 0;
            candidate.buckets = 330'001;
            confirmed.buckets = 330'001;
            strong.buckets = 35'000'000;
            scan.buckets = 330'001;
            state.buckets = 330'001;
            break;
        }

        default:
        case chain::selection::none:
        {
        }
    }
}

void settings::initialize(const system::settings& bitcoin, bool limited_blocks,
    uint32_t silent_start) NOEXCEPT
{
    envelope = { bitcoin, *this, limited_blocks, silent_start };
}

} // namespace database
} // namespace libbitcoin
