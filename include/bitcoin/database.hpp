///////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2014-2025 libbitcoin-database developers (see COPYING).
//
//        GENERATED SOURCE CODE, DO NOT EDIT EXCEPT EXPERIMENTALLY
//
///////////////////////////////////////////////////////////////////////////////
#ifndef LIBBITCOIN_DATABASE_HPP
#define LIBBITCOIN_DATABASE_HPP

/**
 * API Users: Include only this header. Direct use of other headers is fragile
 * and unsupported as header organization is subject to change.
 *
 * Maintainers: Do not include this header internal to this library.
 */

#include <bitcoin/system.hpp>
#include <bitcoin/database/boost.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/error.hpp>
#include <bitcoin/database/query.hpp>
#include <bitcoin/database/settings.hpp>
#include <bitcoin/database/store.hpp>
#include <bitcoin/database/version.hpp>
#include <bitcoin/database/file/file.hpp>
#include <bitcoin/database/file/rotator.hpp>
#include <bitcoin/database/file/utilities.hpp>
#include <bitcoin/database/locks/file_lock.hpp>
#include <bitcoin/database/locks/flush_lock.hpp>
#include <bitcoin/database/locks/interprocess_lock.hpp>
#include <bitcoin/database/locks/locks.hpp>
#include <bitcoin/database/memory/accessor.hpp>
#include <bitcoin/database/memory/finalizer.hpp>
#include <bitcoin/database/memory/map.hpp>
#include <bitcoin/database/memory/memory.hpp>
#include <bitcoin/database/memory/reader.hpp>
#include <bitcoin/database/memory/simple_reader.hpp>
#include <bitcoin/database/memory/simple_writer.hpp>
#include <bitcoin/database/memory/streamers.hpp>
#include <bitcoin/database/memory/utilities.hpp>
#include <bitcoin/database/memory/interfaces/memory.hpp>
#include <bitcoin/database/memory/interfaces/storage.hpp>
#include <bitcoin/database/primitives/arrayhead.hpp>
#include <bitcoin/database/primitives/arraymap.hpp>
#include <bitcoin/database/primitives/hashhead.hpp>
#include <bitcoin/database/primitives/hashmap.hpp>
#include <bitcoin/database/primitives/iterator.hpp>
#include <bitcoin/database/primitives/keys.hpp>
#include <bitcoin/database/primitives/linkage.hpp>
#include <bitcoin/database/primitives/manager.hpp>
#include <bitcoin/database/primitives/nomap.hpp>
#include <bitcoin/database/primitives/primitives.hpp>
#include <bitcoin/database/primitives/sieve.hpp>
#include <bitcoin/database/tables/association.hpp>
#include <bitcoin/database/tables/associations.hpp>
#include <bitcoin/database/tables/context.hpp>
#include <bitcoin/database/tables/event.hpp>
#include <bitcoin/database/tables/keys.hpp>
#include <bitcoin/database/tables/names.hpp>
#include <bitcoin/database/tables/point_set.hpp>
#include <bitcoin/database/tables/schema.hpp>
#include <bitcoin/database/tables/states.hpp>
#include <bitcoin/database/tables/table.hpp>
#include <bitcoin/database/tables/tables.hpp>
#include <bitcoin/database/tables/archives/header.hpp>
#include <bitcoin/database/tables/archives/input.hpp>
#include <bitcoin/database/tables/archives/ins.hpp>
#include <bitcoin/database/tables/archives/output.hpp>
#include <bitcoin/database/tables/archives/outs.hpp>
#include <bitcoin/database/tables/archives/point.hpp>
#include <bitcoin/database/tables/archives/transaction.hpp>
#include <bitcoin/database/tables/archives/txs.hpp>
#include <bitcoin/database/tables/caches/duplicate.hpp>
#include <bitcoin/database/tables/caches/prevout.hpp>
#include <bitcoin/database/tables/caches/validated_bk.hpp>
#include <bitcoin/database/tables/caches/validated_tx.hpp>
#include <bitcoin/database/tables/indexes/height.hpp>
#include <bitcoin/database/tables/indexes/strong_tx.hpp>
#include <bitcoin/database/tables/optionals/address.hpp>
#include <bitcoin/database/tables/optionals/filter_bk.hpp>
#include <bitcoin/database/tables/optionals/filter_tx.hpp>

#endif
