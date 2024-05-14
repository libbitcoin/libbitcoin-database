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
#ifndef LIBBITCOIN_DATABASE_QUERY_QUERY_IPP
#define LIBBITCOIN_DATABASE_QUERY_QUERY_IPP

#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>

// Error handling:
// Negative (false/empty/nullptr) return implies error or not found condition.
// Interpretation may depend on context. The presumption is of store integrity,
// as there is no validation (such as checksumming) of the store. Given that
// the blockchain is a cache of public data, this is a sufficient loss guard.
// If a validated link is passed and the result is negative, the integrity
// presumption implies that the result is negative - unless there are no such
// conditions. In that case the failure result implies loss of store integrity.
// Given the integrity presumption, such a possibility can be ignored. On the
// other hand, if a possible negative result from one call is cascaded to
// another call, the presumption on a final negative is that a negative result
// was propagated, or that a positive propagated, followed by a negative. In
// this case the original cause of the negative is presumed to be unimportant.
// Store integrity is not assured if the indexes are empty (no genesis block),
// and therefore assertions are provided for these limited situations, as
// error handling would be unnecessarily costly. The integrity presumption
// does not hold given faults in the underlying store interface. Consequently
// these are managed independently through the logging interface. Note that
// cascading failures from terminal to search key is an unnecessary perf hit.
// Note that expected stream invalidation may occur (index validation). Store
// read (paging) failures will result in an unhandled exception (termination).
// This can happen from drive disconnection (for example). Store write failures
// can result from memory remap failure, such as when the disk is full. These
// are caught and propagated to write failures and then query failures. Given
// a reliable disk, disk full is the only commonly-expected fault condition.
//
// Remapping:
// During any write operation, the store may be forced to unload and remap the
// underlying backing file of one or more table bodies (headers do not resize).
// This invalidates all memory pointers, but does not invalidate links.
// Pointers are short-lived and managed internally by tables. However iterators
// encapsulate pointers and therefore should be be used with caution. Holding
// an iterator on a table while also invoking a write on that table will result
// in deadlock in the case where the write invokes a remap (blocked by the
// iterator).
//
// Transactionality:
// The query interface uses the underlying store's transactor to provide an
// assurance that writes are consistent when the transactor is unlocked. This
// is provided so that store-wide archival may proceed with writes suspended at
// a point of consistency, while also allowing reads to proceed. Query callers
// should expect writes to be blocked during store hot backup.

namespace libbitcoin {
namespace database {

TEMPLATE
CLASS::query(Store& value) NOEXCEPT
  : store_(value)
{
}

TEMPLATE
bool CLASS::is_full() const NOEXCEPT
{
    return store_.is_error(error::disk_full);
}

TEMPLATE
bool CLASS::is_fault() const NOEXCEPT
{
    return !!store_.get_fault();
}

TEMPLATE
code CLASS::get_code() const NOEXCEPT
{
    return is_full() ? error::disk_full : store_.get_fault();
}


TEMPLATE
void CLASS::reset_full() NOEXCEPT
{
    // There is a possibility of clearing a non-disk-full condition here.
    if (is_full())
        store_.clear_errors();
}

TEMPLATE
code CLASS::snapshot(
    const typename Store::event_handler& handler) const NOEXCEPT
{
    return store_.snapshot(handler);
}

} // namespace database
} // namespace libbitcoin

#endif
