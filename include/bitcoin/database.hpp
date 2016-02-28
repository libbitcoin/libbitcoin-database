///////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2014-2015 libbitcoin-database developers (see COPYING).
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

#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database/block_database.hpp>
#include <bitcoin/database/data_base.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/history_database.hpp>
#include <bitcoin/database/pointer_array_source.hpp>
#include <bitcoin/database/settings.hpp>
#include <bitcoin/database/spend_database.hpp>
#include <bitcoin/database/stealth_database.hpp>
#include <bitcoin/database/transaction_database.hpp>
#include <bitcoin/database/version.hpp>
#include <bitcoin/database/disk/accessor.hpp>
#include <bitcoin/database/disk/allocator.hpp>
#include <bitcoin/database/disk/disk_array.hpp>
#include <bitcoin/database/disk/mmfile.hpp>
#include <bitcoin/database/record/htdb_record.hpp>
#include <bitcoin/database/record/linked_records.hpp>
#include <bitcoin/database/record/multimap_records.hpp>
#include <bitcoin/database/record/record_manager.hpp>
#include <bitcoin/database/result/block_result.hpp>
#include <bitcoin/database/result/spend_result.hpp>
#include <bitcoin/database/result/transaction_result.hpp>
#include <bitcoin/database/slab/htdb_slab.hpp>
#include <bitcoin/database/slab/slab_manager.hpp>

#endif
