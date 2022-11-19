///////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2014-2021 libbitcoin-database developers (see COPYING).
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
#include <bitcoin/database/query.hpp>
#include <bitcoin/database/settings.hpp>
#include <bitcoin/database/version.hpp>
#include <bitcoin/database/locks/file_lock.hpp>
#include <bitcoin/database/locks/flush_lock.hpp>
#include <bitcoin/database/locks/interprocess_lock.hpp>
#include <bitcoin/database/locks/locks.hpp>
#include <bitcoin/database/memory/accessor.hpp>
#include <bitcoin/database/memory/file.hpp>
#include <bitcoin/database/memory/finalizer.hpp>
#include <bitcoin/database/memory/map.hpp>
#include <bitcoin/database/memory/memory.hpp>
#include <bitcoin/database/memory/reader.hpp>
#include <bitcoin/database/memory/writer.hpp>
#include <bitcoin/database/memory/interfaces/memory.hpp>
#include <bitcoin/database/memory/interfaces/storage.hpp>
#include <bitcoin/database/primitives/arraymap.hpp>
#include <bitcoin/database/primitives/hashmap.hpp>
#include <bitcoin/database/primitives/head.hpp>
#include <bitcoin/database/primitives/iterator.hpp>
#include <bitcoin/database/primitives/linkage.hpp>
#include <bitcoin/database/primitives/manager.hpp>
#include <bitcoin/database/primitives/primitives.hpp>
#include <bitcoin/database/tables/schema.hpp>
#include <bitcoin/database/tables/store.hpp>
#include <bitcoin/database/tables/tables.hpp>
#include <bitcoin/database/tables/archives/header.hpp>
#include <bitcoin/database/tables/archives/input.hpp>
#include <bitcoin/database/tables/archives/output.hpp>
#include <bitcoin/database/tables/archives/point.hpp>
#include <bitcoin/database/tables/archives/puts.hpp>
#include <bitcoin/database/tables/archives/transaction.hpp>
#include <bitcoin/database/tables/archives/txs.hpp>
#include <bitcoin/database/tables/caches/bootstrap.hpp>
#include <bitcoin/database/tables/caches/buffer.hpp>
#include <bitcoin/database/tables/caches/neutrino.hpp>
#include <bitcoin/database/tables/caches/validated_block.hpp>
#include <bitcoin/database/tables/caches/validated_tx.hpp>
#include <bitcoin/database/tables/indexes/address.hpp>
#include <bitcoin/database/tables/indexes/candidate_height.hpp>
#include <bitcoin/database/tables/indexes/confirmed_block.hpp>
#include <bitcoin/database/tables/indexes/confirmed_height.hpp>
#include <bitcoin/database/tables/indexes/confirmed_tx.hpp>
#include <bitcoin/database/tables/indexes/input_tx.hpp>
#include <bitcoin/database/tables/indexes/output_tx.hpp>
#include <bitcoin/database/tables/indexes/spent_output.hpp>

#endif
