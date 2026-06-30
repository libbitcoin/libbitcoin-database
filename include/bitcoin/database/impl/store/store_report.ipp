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
#ifndef LIBBITCOIN_DATABASE_STORE_REPORT_IPP
#define LIBBITCOIN_DATABASE_STORE_REPORT_IPP

#include <bitcoin/database/define.hpp>

// TODO: there are now headers that increase in size, so these need to be
// included in faults and computations for disk full recovery, and in variable
// sizing reports.

namespace libbitcoin {
namespace database {

// public
TEMPLATE
void CLASS::report(const error_handler& handler) const NOEXCEPT
{
    const auto report = [&handler](const auto& file, table_t table) NOEXCEPT
    {
        auto ec = file.get_fault();
        if (!ec && to_bool(file.get_space()))
            ec = error::disk_full;

        handler(ec, table);
    };

    report(header_body_, table_t::header_body);
    report(input_body_, table_t::input_body);
    report(output_body_, table_t::output_body);
    report(point_body_, table_t::point_body);
    report(ins_body_, table_t::ins_body);
    report(outs_body_, table_t::outs_body);
    report(tx_body_, table_t::tx_body);
    report(txs_body_, table_t::txs_body);
    report(candidate_body_, table_t::candidate_body);
    report(confirmed_body_, table_t::confirmed_body);
    report(strong_tx_body_, table_t::strong_tx_body);
    report(ecdsa_body_, table_t::ecdsa_body);
    report(schnorr_body_, table_t::schnorr_body);
    report(silent_body_, table_t::silent_body);
    report(duplicate_body_, table_t::duplicate_body);
    report(prevalid_body_, table_t::prevalid_body);
    report(prevout_body_, table_t::prevout_body);
    report(validated_bk_body_, table_t::validated_bk_body);
    report(validated_tx_body_, table_t::validated_tx_body);
    report(address_body_, table_t::address_body);
    report(filter_bk_body_, table_t::filter_bk_body);
    report(filter_tx_body_, table_t::filter_tx_body);
}

// public
TEMPLATE
code CLASS::get_fault() const NOEXCEPT
{
    code ec{ error::success };
    if ((ec = header_body_.get_fault())) return ec;
    if ((ec = input_body_.get_fault())) return ec;
    if ((ec = output_body_.get_fault())) return ec;
    if ((ec = point_body_.get_fault())) return ec;
    if ((ec = ins_body_.get_fault())) return ec;
    if ((ec = outs_body_.get_fault())) return ec;
    if ((ec = tx_body_.get_fault())) return ec;
    if ((ec = txs_body_.get_fault())) return ec;
    if ((ec = candidate_body_.get_fault())) return ec;
    if ((ec = confirmed_body_.get_fault())) return ec;
    if ((ec = strong_tx_body_.get_fault())) return ec;
    if ((ec = ecdsa_body_.get_fault())) return ec;
    if ((ec = schnorr_body_.get_fault())) return ec;
    if ((ec = silent_body_.get_fault())) return ec;
    if ((ec = duplicate_body_.get_fault())) return ec;
    if ((ec = prevalid_body_.get_fault())) return ec;
    if ((ec = prevout_body_.get_fault())) return ec;
    if ((ec = validated_bk_body_.get_fault())) return ec;
    if ((ec = validated_tx_body_.get_fault())) return ec;
    if ((ec = address_body_.get_fault())) return ec;
    if ((ec = filter_bk_body_.get_fault())) return ec;
    if ((ec = filter_tx_body_.get_fault())) return ec;
    return ec;
}

// public
TEMPLATE
size_t CLASS::get_space() const NOEXCEPT
{
    size_t total{};
    const auto space = [&total](auto& storage) NOEXCEPT
    {
        total = system::ceilinged_add(total, storage.get_space());
    };

    space(header_body_);
    space(input_body_);
    space(output_body_);
    space(point_body_);
    space(ins_body_);
    space(outs_body_);
    space(tx_body_);
    space(txs_body_);
    space(candidate_body_);
    space(confirmed_body_);
    space(strong_tx_body_);
    space(ecdsa_body_);
    space(schnorr_body_);
    space(silent_body_);
    space(duplicate_body_);
    space(prevalid_body_);
    space(prevout_body_);
    space(validated_bk_body_);
    space(validated_tx_body_);
    space(address_body_);
    space(filter_bk_body_);
    space(filter_tx_body_);

    return total;
}

} // namespace database
} // namespace libbitcoin

#endif
