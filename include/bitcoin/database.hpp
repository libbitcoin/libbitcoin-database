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
#include <bitcoin/database/block_state.hpp>
#include <bitcoin/database/data_base.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/settings.hpp>
#include <bitcoin/database/store.hpp>
#include <bitcoin/database/unspent_outputs.hpp>
#include <bitcoin/database/unspent_transaction.hpp>
#include <bitcoin/database/verify.hpp>
#include <bitcoin/database/version.hpp>
#include <bitcoin/database/databases/address_database.hpp>
#include <bitcoin/database/databases/block_database.hpp>
#include <bitcoin/database/databases/transaction_database.hpp>
#include <bitcoin/database/memory/accessor.hpp>
#include <bitcoin/database/memory/file_storage.hpp>
#include <bitcoin/database/memory/memory.hpp>
#include <bitcoin/database/memory/storage.hpp>
#include <bitcoin/database/primitives/hash_table.hpp>
#include <bitcoin/database/primitives/hash_table_header.hpp>
#include <bitcoin/database/primitives/hash_table_multimap.hpp>
#include <bitcoin/database/primitives/list.hpp>
#include <bitcoin/database/primitives/list_element.hpp>
#include <bitcoin/database/primitives/list_iterator.hpp>
#include <bitcoin/database/primitives/record_manager.hpp>
#include <bitcoin/database/primitives/slab_manager.hpp>
#include <bitcoin/database/result/address_iterator.hpp>
#include <bitcoin/database/result/address_result.hpp>
#include <bitcoin/database/result/block_result.hpp>
#include <bitcoin/database/result/inpoint_iterator.hpp>
#include <bitcoin/database/result/transaction_iterator.hpp>
#include <bitcoin/database/result/transaction_result.hpp>

#endif
