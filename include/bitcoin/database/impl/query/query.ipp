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
#ifndef LIBBITCOIN_DATABASE_QUERY_QUERY_IPP
#define LIBBITCOIN_DATABASE_QUERY_QUERY_IPP

#include <bitcoin/database/define.hpp>

// Error handling:
// Negative (false/empty/nullptr) return implies error or not found condition.
// Interpretation may depend on context. Presumption is of store integrity,
// as there is no validation (such as checksumming) of store. Given
// blockchain is a cache of public data, this is a sufficient loss guard. If
// a validated link is passed and result is negative, integrity presumption
// implies result is negative - unless there are no such conditions. In that
// case failure result implies loss of store integrity. Given integrity
// presumption, such a possibility can be ignored. On the other hand, if a
// possible negative result from one call is cascaded to another call,
// presumption on a final negative is that a negative result was propagated,
// or that a positive propagated, followed by a negative. In this case
// original cause of negative is presumed unimportant. Store integrity not
// assured if indexes are empty (no genesis block), so assertions are
// provided for these limited situations, as error handling would be
// unnecessarily costly. Integrity presumption does not hold given faults in
// underlying store interface, so these are managed independently through
// logging interface. Cascading failures from terminal to search key is an
// unnecessary perf hit. Expected stream invalidation may occur (index
// validation). Store read (paging) failures result in unhandled exception
// (termination), as from drive disconnection (for example). Store write
// failures can result from memory remap failure, such as when disk is full.
// These are caught and propagated to write failures and then query failures.
// Given reliable disk, disk full is only commonly-expected fault condition.
//
// Remapping:
// During any write operation, store may be forced to unload and remap
// underlying backing file of one or more table bodies (headers do not resize).
// This invalidates all memory pointers, but not links. Pointers are
// short-lived and managed internally by tables. However iterators
// encapsulate pointers and therefore should be used with caution.
// Holding an iterator on a table while also invoking a write on that
// table deadlocks where write invokes a remap (blocked by iterator).
//
// Transactionality:
// Query interface uses underlying store's transactor to assure writes are
// consistent when transactor is unlocked, so store-wide archival may proceed
// with writes suspended at a point of consistency, while also allowing reads to
// proceed. Query callers should expect writes blocked during store hot backup.

namespace libbitcoin {
namespace database {

TEMPLATE
CLASS::query(Store& store) NOEXCEPT
  : cold_reads_(system::sign_cast<ptrdiff_t>(store.cold_reads())),
    store_(store)
{
}

TEMPLATE
typename CLASS::transactor CLASS::get_transactor() const NOEXCEPT
{
    return store_.get_transactor();
}

TEMPLATE
code CLASS::get_code() const NOEXCEPT
{
    return is_full() ? error::disk_full : get_fault();
}

TEMPLATE
code CLASS::get_fault() const NOEXCEPT
{
    return store_.get_fault();
}

TEMPLATE
bool CLASS::is_fault() const NOEXCEPT
{
    return !!get_fault();
}

TEMPLATE
bool CLASS::is_full() const NOEXCEPT
{
    return to_bool(get_space()) && !is_fault();
}

TEMPLATE
bool CLASS::is_fault_locked() const NOEXCEPT
{
    return store_.is_fault_locked();
}

TEMPLATE
bool CLASS::set_fault_lock() NOEXCEPT
{
    return store_.set_fault_lock();
}

TEMPLATE
bool CLASS::is_coalesced() const NOEXCEPT
{
    return get_fork() == get_top_candidate();
}

TEMPLATE
size_t CLASS::get_space() const NOEXCEPT
{
    return store_.get_space();
}

TEMPLATE
code CLASS::reload(const typename Store::event_handler& handler) const NOEXCEPT
{
    return store_.reload(handler);
}

TEMPLATE
code CLASS::prune(const typename Store::event_handler& handler) const NOEXCEPT
{
    return store_.prune(handler);
}

TEMPLATE
code CLASS::snapshot(const typename Store::event_handler& handler) const NOEXCEPT
{
    return store_.snapshot(handler);
}

TEMPLATE
size_t CLASS::positive_search_count() const NOEXCEPT
{
    return store_.ins.positive_search_count();
}

TEMPLATE
size_t CLASS::negative_search_count() const NOEXCEPT
{
    return store_.ins.negative_search_count();
}

} // namespace database
} // namespace libbitcoin

#endif
