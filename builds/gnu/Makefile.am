###############################################################################
#  Copyright (c) 2014-2026 libbitcoin-database developers (see COPYING).
#
#         GENERATED SOURCE CODE, DO NOT EDIT EXCEPT EXPERIMENTALLY
#
###############################################################################

# Automake settings.
#==============================================================================
# Look for macros in the 'm4' subdirectory.
#------------------------------------------------------------------------------
ACLOCAL_AMFLAGS = -I m4

# Distribute data.
#==============================================================================
# files => ${pkgconfigdir}
#------------------------------------------------------------------------------
pkgconfig_DATA = \
    libbitcoin-database.pc

# files => ${docdir}
#------------------------------------------------------------------------------
doc_DATA = \
    AUTHORS \
    COPYING \
    ChangeLog \
    INSTALL \
    NEWS \
    README

# Libraries.
#==============================================================================
# Target library 'src/libbitcoin-database.la'
#------------------------------------------------------------------------------
lib_LTLIBRARIES = src/libbitcoin-database.la

src_libbitcoin_database_la_LIBS = src/libbitcoin-database.la

src_libbitcoin_database_la_CPPFLAGS = \
    -I${srcdir}/../../include \
    ${libbitcoin_system_BUILD_CPPFLAGS}

src_libbitcoin_database_la_LDFLAGS = \
    ${libbitcoin_system_LDFLAGS}

src_libbitcoin_database_la_LIBADD = \
    ${libbitcoin_system_LIBS}

src_libbitcoin_database_la_SOURCES = \
    ${srcdir}/../../src/define.cpp \
    ${srcdir}/../../src/error.cpp \
    ${srcdir}/../../src/settings.cpp \
    ${srcdir}/../../src/file/rotator.cpp \
    ${srcdir}/../../src/file/utilities.cpp \
    ${srcdir}/../../src/locks/file_lock.cpp \
    ${srcdir}/../../src/locks/flush_lock.cpp \
    ${srcdir}/../../src/locks/interprocess_lock.cpp \
    ${srcdir}/../../src/memory/mman.cpp \
    ${srcdir}/../../src/memory/utilities.cpp \
    ${srcdir}/../../src/types/history.cpp \
    ${srcdir}/../../src/types/multisig_view.cpp \
    ${srcdir}/../../src/types/unspent.cpp

include_bitcoindir = \
    ${includedir}/bitcoin

include_bitcoin_HEADERS = \
    ${srcdir}/../../include/bitcoin/database.hpp

include_bitcoin_databasedir = \
    ${includedir}/bitcoin/database

include_bitcoin_database_HEADERS = \
    ${srcdir}/../../include/bitcoin/database/boost.hpp \
    ${srcdir}/../../include/bitcoin/database/define.hpp \
    ${srcdir}/../../include/bitcoin/database/error.hpp \
    ${srcdir}/../../include/bitcoin/database/query.hpp \
    ${srcdir}/../../include/bitcoin/database/settings.hpp \
    ${srcdir}/../../include/bitcoin/database/store.hpp \
    ${srcdir}/../../include/bitcoin/database/version.hpp

include_bitcoin_database_filedir = \
    ${includedir}/bitcoin/database/file

include_bitcoin_database_file_HEADERS = \
    ${srcdir}/../../include/bitcoin/database/file/file.hpp \
    ${srcdir}/../../include/bitcoin/database/file/rotator.hpp \
    ${srcdir}/../../include/bitcoin/database/file/utilities.hpp

include_bitcoin_database_impl_memorydir = \
    ${includedir}/bitcoin/database/impl/memory

include_bitcoin_database_impl_memory_HEADERS = \
    ${srcdir}/../../include/bitcoin/database/impl/memory/accessor.ipp \
    ${srcdir}/../../include/bitcoin/database/impl/memory/mmap.ipp \
    ${srcdir}/../../include/bitcoin/database/impl/memory/mmap_dispatch.ipp \
    ${srcdir}/../../include/bitcoin/database/impl/memory/mmap_private.ipp \
    ${srcdir}/../../include/bitcoin/database/impl/memory/mmap_storage.ipp

include_bitcoin_database_impl_primitivesdir = \
    ${includedir}/bitcoin/database/impl/primitives

include_bitcoin_database_impl_primitives_HEADERS = \
    ${srcdir}/../../include/bitcoin/database/impl/primitives/arrayhead.ipp \
    ${srcdir}/../../include/bitcoin/database/impl/primitives/arraymap.ipp \
    ${srcdir}/../../include/bitcoin/database/impl/primitives/hashhead.ipp \
    ${srcdir}/../../include/bitcoin/database/impl/primitives/hashmap.ipp \
    ${srcdir}/../../include/bitcoin/database/impl/primitives/iterator.ipp \
    ${srcdir}/../../include/bitcoin/database/impl/primitives/keys.ipp \
    ${srcdir}/../../include/bitcoin/database/impl/primitives/linkage.ipp \
    ${srcdir}/../../include/bitcoin/database/impl/primitives/manager.ipp \
    ${srcdir}/../../include/bitcoin/database/impl/primitives/nohead.ipp \
    ${srcdir}/../../include/bitcoin/database/impl/primitives/nomap.ipp

include_bitcoin_database_impl_querydir = \
    ${includedir}/bitcoin/database/impl/query

include_bitcoin_database_impl_query_HEADERS = \
    ${srcdir}/../../include/bitcoin/database/impl/query/amounts.ipp \
    ${srcdir}/../../include/bitcoin/database/impl/query/confirmed.ipp \
    ${srcdir}/../../include/bitcoin/database/impl/query/extent.ipp \
    ${srcdir}/../../include/bitcoin/database/impl/query/fee_rate.ipp \
    ${srcdir}/../../include/bitcoin/database/impl/query/filters.ipp \
    ${srcdir}/../../include/bitcoin/database/impl/query/height.ipp \
    ${srcdir}/../../include/bitcoin/database/impl/query/initialize.ipp \
    ${srcdir}/../../include/bitcoin/database/impl/query/locator.ipp \
    ${srcdir}/../../include/bitcoin/database/impl/query/merkle.ipp \
    ${srcdir}/../../include/bitcoin/database/impl/query/properties_block.ipp \
    ${srcdir}/../../include/bitcoin/database/impl/query/properties_tx.ipp \
    ${srcdir}/../../include/bitcoin/database/impl/query/query.ipp \
    ${srcdir}/../../include/bitcoin/database/impl/query/sequences.ipp \
    ${srcdir}/../../include/bitcoin/database/impl/query/sizes.ipp

include_bitcoin_database_impl_query_addressdir = \
    ${includedir}/bitcoin/database/impl/query/address

include_bitcoin_database_impl_query_address_HEADERS = \
    ${srcdir}/../../include/bitcoin/database/impl/query/address/address_balance.ipp \
    ${srcdir}/../../include/bitcoin/database/impl/query/address/address_history.ipp \
    ${srcdir}/../../include/bitcoin/database/impl/query/address/address_outpoints.ipp \
    ${srcdir}/../../include/bitcoin/database/impl/query/address/address_unspent.ipp

include_bitcoin_database_impl_query_archivedir = \
    ${includedir}/bitcoin/database/impl/query/archive

include_bitcoin_database_impl_query_archive_HEADERS = \
    ${srcdir}/../../include/bitcoin/database/impl/query/archive/chain_reader.ipp \
    ${srcdir}/../../include/bitcoin/database/impl/query/archive/chain_writer.ipp \
    ${srcdir}/../../include/bitcoin/database/impl/query/archive/wire_reader.ipp \
    ${srcdir}/../../include/bitcoin/database/impl/query/archive/wire_writer.ipp

include_bitcoin_database_impl_query_consensusdir = \
    ${includedir}/bitcoin/database/impl/query/consensus

include_bitcoin_database_impl_query_consensus_HEADERS = \
    ${srcdir}/../../include/bitcoin/database/impl/query/consensus/consensus_block.ipp \
    ${srcdir}/../../include/bitcoin/database/impl/query/consensus/consensus_chain_state.ipp \
    ${srcdir}/../../include/bitcoin/database/impl/query/consensus/consensus_compact.ipp \
    ${srcdir}/../../include/bitcoin/database/impl/query/consensus/consensus_forks.ipp \
    ${srcdir}/../../include/bitcoin/database/impl/query/consensus/consensus_populate.ipp \
    ${srcdir}/../../include/bitcoin/database/impl/query/consensus/consensus_prevouts.ipp \
    ${srcdir}/../../include/bitcoin/database/impl/query/consensus/consensus_states.ipp \
    ${srcdir}/../../include/bitcoin/database/impl/query/consensus/consensus_strong.ipp \
    ${srcdir}/../../include/bitcoin/database/impl/query/consensus/consensus_transaction.ipp \
    ${srcdir}/../../include/bitcoin/database/impl/query/consensus/consensus_work.ipp

include_bitcoin_database_impl_query_navigatedir = \
    ${includedir}/bitcoin/database/impl/query/navigate

include_bitcoin_database_impl_query_navigate_HEADERS = \
    ${srcdir}/../../include/bitcoin/database/impl/query/navigate/navigate_arraymap.ipp \
    ${srcdir}/../../include/bitcoin/database/impl/query/navigate/navigate_forward.ipp \
    ${srcdir}/../../include/bitcoin/database/impl/query/navigate/navigate_hashmap.ipp \
    ${srcdir}/../../include/bitcoin/database/impl/query/navigate/navigate_natural.ipp \
    ${srcdir}/../../include/bitcoin/database/impl/query/navigate/navigate_reverse.ipp

include_bitcoin_database_impl_query_batchdir = \
    ${includedir}/bitcoin/database/impl/query/batch

include_bitcoin_database_impl_query_batch_HEADERS = \
    ${srcdir}/../../include/bitcoin/database/impl/query/batch/ecdsa.ipp \
    ${srcdir}/../../include/bitcoin/database/impl/query/batch/prevalid.ipp \
    ${srcdir}/../../include/bitcoin/database/impl/query/batch/schnorr.ipp \
    ${srcdir}/../../include/bitcoin/database/impl/query/batch/silent.ipp

include_bitcoin_database_impl_storedir = \
    ${includedir}/bitcoin/database/impl/store

include_bitcoin_database_impl_store_HEADERS = \
    ${srcdir}/../../include/bitcoin/database/impl/store/store.ipp \
    ${srcdir}/../../include/bitcoin/database/impl/store/store_backup.ipp \
    ${srcdir}/../../include/bitcoin/database/impl/store/store_close.ipp \
    ${srcdir}/../../include/bitcoin/database/impl/store/store_create.ipp \
    ${srcdir}/../../include/bitcoin/database/impl/store/store_dump.ipp \
    ${srcdir}/../../include/bitcoin/database/impl/store/store_events.ipp \
    ${srcdir}/../../include/bitcoin/database/impl/store/store_open.ipp \
    ${srcdir}/../../include/bitcoin/database/impl/store/store_open_load.ipp \
    ${srcdir}/../../include/bitcoin/database/impl/store/store_prune.ipp \
    ${srcdir}/../../include/bitcoin/database/impl/store/store_reload.ipp \
    ${srcdir}/../../include/bitcoin/database/impl/store/store_report.ipp \
    ${srcdir}/../../include/bitcoin/database/impl/store/store_restore.ipp \
    ${srcdir}/../../include/bitcoin/database/impl/store/store_snapshot.ipp \
    ${srcdir}/../../include/bitcoin/database/impl/store/store_tables.ipp \
    ${srcdir}/../../include/bitcoin/database/impl/store/store_unload_close.ipp

include_bitcoin_database_locksdir = \
    ${includedir}/bitcoin/database/locks

include_bitcoin_database_locks_HEADERS = \
    ${srcdir}/../../include/bitcoin/database/locks/file_lock.hpp \
    ${srcdir}/../../include/bitcoin/database/locks/flush_lock.hpp \
    ${srcdir}/../../include/bitcoin/database/locks/interprocess_lock.hpp \
    ${srcdir}/../../include/bitcoin/database/locks/locks.hpp

include_bitcoin_database_memorydir = \
    ${includedir}/bitcoin/database/memory

include_bitcoin_database_memory_HEADERS = \
    ${srcdir}/../../include/bitcoin/database/memory/accessor.hpp \
    ${srcdir}/../../include/bitcoin/database/memory/finalizer.hpp \
    ${srcdir}/../../include/bitcoin/database/memory/maps.hpp \
    ${srcdir}/../../include/bitcoin/database/memory/memory.hpp \
    ${srcdir}/../../include/bitcoin/database/memory/mman.hpp \
    ${srcdir}/../../include/bitcoin/database/memory/mmap.hpp \
    ${srcdir}/../../include/bitcoin/database/memory/reader.hpp \
    ${srcdir}/../../include/bitcoin/database/memory/streamers.hpp \
    ${srcdir}/../../include/bitcoin/database/memory/utilities.hpp

include_bitcoin_database_memory_interfacesdir = \
    ${includedir}/bitcoin/database/memory/interfaces

include_bitcoin_database_memory_interfaces_HEADERS = \
    ${srcdir}/../../include/bitcoin/database/memory/interfaces/memory.hpp \
    ${srcdir}/../../include/bitcoin/database/memory/interfaces/storage.hpp

include_bitcoin_database_primitivesdir = \
    ${includedir}/bitcoin/database/primitives

include_bitcoin_database_primitives_HEADERS = \
    ${srcdir}/../../include/bitcoin/database/primitives/arrayhead.hpp \
    ${srcdir}/../../include/bitcoin/database/primitives/arraymap.hpp \
    ${srcdir}/../../include/bitcoin/database/primitives/hashhead.hpp \
    ${srcdir}/../../include/bitcoin/database/primitives/hashmap.hpp \
    ${srcdir}/../../include/bitcoin/database/primitives/iterator.hpp \
    ${srcdir}/../../include/bitcoin/database/primitives/keys.hpp \
    ${srcdir}/../../include/bitcoin/database/primitives/linkage.hpp \
    ${srcdir}/../../include/bitcoin/database/primitives/manager.hpp \
    ${srcdir}/../../include/bitcoin/database/primitives/nohead.hpp \
    ${srcdir}/../../include/bitcoin/database/primitives/nomap.hpp \
    ${srcdir}/../../include/bitcoin/database/primitives/nomaps.hpp \
    ${srcdir}/../../include/bitcoin/database/primitives/primitives.hpp

include_bitcoin_database_tablesdir = \
    ${includedir}/bitcoin/database/tables

include_bitcoin_database_tables_HEADERS = \
    ${srcdir}/../../include/bitcoin/database/tables/context.hpp \
    ${srcdir}/../../include/bitcoin/database/tables/event.hpp \
    ${srcdir}/../../include/bitcoin/database/tables/names.hpp \
    ${srcdir}/../../include/bitcoin/database/tables/schema.hpp \
    ${srcdir}/../../include/bitcoin/database/tables/table.hpp \
    ${srcdir}/../../include/bitcoin/database/tables/tables.hpp

include_bitcoin_database_tables_archivesdir = \
    ${includedir}/bitcoin/database/tables/archives

include_bitcoin_database_tables_archives_HEADERS = \
    ${srcdir}/../../include/bitcoin/database/tables/archives/header.hpp \
    ${srcdir}/../../include/bitcoin/database/tables/archives/input.hpp \
    ${srcdir}/../../include/bitcoin/database/tables/archives/ins.hpp \
    ${srcdir}/../../include/bitcoin/database/tables/archives/output.hpp \
    ${srcdir}/../../include/bitcoin/database/tables/archives/outs.hpp \
    ${srcdir}/../../include/bitcoin/database/tables/archives/point.hpp \
    ${srcdir}/../../include/bitcoin/database/tables/archives/transaction.hpp \
    ${srcdir}/../../include/bitcoin/database/tables/archives/txs.hpp

include_bitcoin_database_tables_cachesdir = \
    ${includedir}/bitcoin/database/tables/caches

include_bitcoin_database_tables_caches_HEADERS = \
    ${srcdir}/../../include/bitcoin/database/tables/caches/duplicate.hpp \
    ${srcdir}/../../include/bitcoin/database/tables/caches/ecdsa.hpp \
    ${srcdir}/../../include/bitcoin/database/tables/caches/prevalid.hpp \
    ${srcdir}/../../include/bitcoin/database/tables/caches/prevout.hpp \
    ${srcdir}/../../include/bitcoin/database/tables/caches/schnorr.hpp \
    ${srcdir}/../../include/bitcoin/database/tables/caches/silent.hpp \
    ${srcdir}/../../include/bitcoin/database/tables/caches/validated_bk.hpp \
    ${srcdir}/../../include/bitcoin/database/tables/caches/validated_tx.hpp

include_bitcoin_database_tables_indexesdir = \
    ${includedir}/bitcoin/database/tables/indexes

include_bitcoin_database_tables_indexes_HEADERS = \
    ${srcdir}/../../include/bitcoin/database/tables/indexes/height.hpp \
    ${srcdir}/../../include/bitcoin/database/tables/indexes/strong_tx.hpp

include_bitcoin_database_tables_optionalsdir = \
    ${includedir}/bitcoin/database/tables/optionals

include_bitcoin_database_tables_optionals_HEADERS = \
    ${srcdir}/../../include/bitcoin/database/tables/optionals/address.hpp \
    ${srcdir}/../../include/bitcoin/database/tables/optionals/filter_bk.hpp \
    ${srcdir}/../../include/bitcoin/database/tables/optionals/filter_tx.hpp

include_bitcoin_database_typesdir = \
    ${includedir}/bitcoin/database/types

include_bitcoin_database_types_HEADERS = \
    ${srcdir}/../../include/bitcoin/database/types/association.hpp \
    ${srcdir}/../../include/bitcoin/database/types/associations.hpp \
    ${srcdir}/../../include/bitcoin/database/types/block_state.hpp \
    ${srcdir}/../../include/bitcoin/database/types/fee_rate.hpp \
    ${srcdir}/../../include/bitcoin/database/types/header_state.hpp \
    ${srcdir}/../../include/bitcoin/database/types/history.hpp \
    ${srcdir}/../../include/bitcoin/database/types/multisig_view.hpp \
    ${srcdir}/../../include/bitcoin/database/types/point_set.hpp \
    ${srcdir}/../../include/bitcoin/database/types/position.hpp \
    ${srcdir}/../../include/bitcoin/database/types/span.hpp \
    ${srcdir}/../../include/bitcoin/database/types/tx_state.hpp \
    ${srcdir}/../../include/bitcoin/database/types/type.hpp \
    ${srcdir}/../../include/bitcoin/database/types/types.hpp \
    ${srcdir}/../../include/bitcoin/database/types/unspent.hpp

# Tests.
#==============================================================================
# Target test 'test/libbitcoin-database-test'
#------------------------------------------------------------------------------
if WITH_TESTS

check_PROGRAMS = test/libbitcoin-database-test

test_libbitcoin_database_test_CPPFLAGS = \
    ${boost_BUILD_CPPFLAGS} \
    ${boost_unit_test_framework_BUILD_CPPFLAGS} \
    ${src_libbitcoin_database_la_CPPFLAGS}

test_libbitcoin_database_test_LDFLAGS = \
    ${boost_LDFLAGS} \
    ${boost_unit_test_framework_LDFLAGS} \
    ${src_libbitcoin_database_la_LDFLAGS}

test_libbitcoin_database_test_LDADD = \
    ${boost_LIBS} \
    ${boost_unit_test_framework_LIBS} \
    ${src_libbitcoin_database_la_LIBS} \
    ${src_libbitcoin_database_la_LIBADD}

test_libbitcoin_database_test_SOURCES = \
    ${srcdir}/../../test/error.cpp \
    ${srcdir}/../../test/main.cpp \
    ${srcdir}/../../test/settings.cpp \
    ${srcdir}/../../test/test.cpp \
    ${srcdir}/../../test/file/rotator.cpp \
    ${srcdir}/../../test/file/utilities.cpp \
    ${srcdir}/../../test/locks/file_lock.cpp \
    ${srcdir}/../../test/locks/flush_lock.cpp \
    ${srcdir}/../../test/locks/interprocess_lock.cpp \
    ${srcdir}/../../test/memory/accessor.cpp \
    ${srcdir}/../../test/memory/mmap.cpp \
    ${srcdir}/../../test/memory/utilities.cpp \
    ${srcdir}/../../test/mocks/blocks.cpp \
    ${srcdir}/../../test/mocks/chunk_storage.cpp \
    ${srcdir}/../../test/primitives/arrayhead.cpp \
    ${srcdir}/../../test/primitives/arraymap.cpp \
    ${srcdir}/../../test/primitives/hashhead.cpp \
    ${srcdir}/../../test/primitives/hashmap.cpp \
    ${srcdir}/../../test/primitives/iterator.cpp \
    ${srcdir}/../../test/primitives/keys.cpp \
    ${srcdir}/../../test/primitives/linkage.cpp \
    ${srcdir}/../../test/primitives/manager.cpp \
    ${srcdir}/../../test/primitives/nohead.cpp \
    ${srcdir}/../../test/primitives/nomap.cpp \
    ${srcdir}/../../test/query/amounts.cpp \
    ${srcdir}/../../test/query/confirmed.cpp \
    ${srcdir}/../../test/query/extent.cpp \
    ${srcdir}/../../test/query/fee_rate.cpp \
    ${srcdir}/../../test/query/filters.cpp \
    ${srcdir}/../../test/query/height.cpp \
    ${srcdir}/../../test/query/initialize.cpp \
    ${srcdir}/../../test/query/locator.cpp \
    ${srcdir}/../../test/query/merkle.cpp \
    ${srcdir}/../../test/query/properties_block.cpp \
    ${srcdir}/../../test/query/properties_tx.cpp \
    ${srcdir}/../../test/query/sequences.cpp \
    ${srcdir}/../../test/query/sizes.cpp \
    ${srcdir}/../../test/query/address/address_balance.cpp \
    ${srcdir}/../../test/query/address/address_history.cpp \
    ${srcdir}/../../test/query/address/address_outpoints.cpp \
    ${srcdir}/../../test/query/address/address_unspent.cpp \
    ${srcdir}/../../test/query/archive/chain_reader.cpp \
    ${srcdir}/../../test/query/archive/chain_writer.cpp \
    ${srcdir}/../../test/query/archive/wire_reader.cpp \
    ${srcdir}/../../test/query/archive/wire_writer.cpp \
    ${srcdir}/../../test/query/consensus/consensus_block.cpp \
    ${srcdir}/../../test/query/consensus/consensus_chain_state.cpp \
    ${srcdir}/../../test/query/consensus/consensus_compact.cpp \
    ${srcdir}/../../test/query/consensus/consensus_forks.cpp \
    ${srcdir}/../../test/query/consensus/consensus_populate.cpp \
    ${srcdir}/../../test/query/consensus/consensus_prevouts.cpp \
    ${srcdir}/../../test/query/consensus/consensus_states.cpp \
    ${srcdir}/../../test/query/consensus/consensus_strong.cpp \
    ${srcdir}/../../test/query/consensus/consensus_transaction.cpp \
    ${srcdir}/../../test/query/consensus/consensus_work.cpp \
    ${srcdir}/../../test/query/navigate/navigate_arraymap.cpp \
    ${srcdir}/../../test/query/navigate/navigate_forward.cpp \
    ${srcdir}/../../test/query/navigate/navigate_hashmap.cpp \
    ${srcdir}/../../test/query/navigate/navigate_natural.cpp \
    ${srcdir}/../../test/query/navigate/navigate_reverse.cpp \
    ${srcdir}/../../test/query/batch/ecdsa.cpp \
    ${srcdir}/../../test/query/batch/prevalid.cpp \
    ${srcdir}/../../test/query/batch/schnorr.cpp \
    ${srcdir}/../../test/query/batch/silent.cpp \
    ${srcdir}/../../test/tables/archives/header.cpp \
    ${srcdir}/../../test/tables/archives/input.cpp \
    ${srcdir}/../../test/tables/archives/ins.cpp \
    ${srcdir}/../../test/tables/archives/output.cpp \
    ${srcdir}/../../test/tables/archives/outs.cpp \
    ${srcdir}/../../test/tables/archives/point.cpp \
    ${srcdir}/../../test/tables/archives/transaction.cpp \
    ${srcdir}/../../test/tables/archives/txs.cpp \
    ${srcdir}/../../test/tables/caches/duplicate.cpp \
    ${srcdir}/../../test/tables/caches/ecdsa.cpp \
    ${srcdir}/../../test/tables/caches/prevalid.cpp \
    ${srcdir}/../../test/tables/caches/prevout.cpp \
    ${srcdir}/../../test/tables/caches/schnorr.cpp \
    ${srcdir}/../../test/tables/caches/silent.cpp \
    ${srcdir}/../../test/tables/caches/validated_bk.cpp \
    ${srcdir}/../../test/tables/caches/validated_tx.cpp \
    ${srcdir}/../../test/tables/indexes/height.cpp \
    ${srcdir}/../../test/tables/indexes/strong_tx.cpp \
    ${srcdir}/../../test/tables/optional/address.cpp \
    ${srcdir}/../../test/tables/optional/filter_bk.cpp \
    ${srcdir}/../../test/tables/optional/filter_tx.cpp \
    ${srcdir}/../../test/types/history.cpp \
    ${srcdir}/../../test/types/span.cpp \
    ${srcdir}/../../test/types/unspent.cpp \
    ${srcdir}/../../test/store/store.cpp \
    ${srcdir}/../../test/store/store_backup.cpp \
    ${srcdir}/../../test/store/store_close.cpp \
    ${srcdir}/../../test/store/store_create.cpp \
    ${srcdir}/../../test/store/store_dump.cpp \
    ${srcdir}/../../test/store/store_events.cpp \
    ${srcdir}/../../test/store/store_open.cpp \
    ${srcdir}/../../test/store/store_open_load.cpp \
    ${srcdir}/../../test/store/store_prune.cpp \
    ${srcdir}/../../test/store/store_reload.cpp \
    ${srcdir}/../../test/store/store_report.cpp \
    ${srcdir}/../../test/store/store_restore.cpp \
    ${srcdir}/../../test/store/store_snapshot.cpp \
    ${srcdir}/../../test/store/store_tables.cpp \
    ${srcdir}/../../test/store/store_unload_close.cpp

TESTS = test_runner.sh

endif WITH_TESTS

# Binaries.
#==============================================================================
# Target binary 'tools/initchain/initchain'
#------------------------------------------------------------------------------
if WITH_TOOLS

noinst_PROGRAMS = tools/initchain/initchain

tools_initchain_initchain_CPPFLAGS = \
    ${src_libbitcoin_database_la_CPPFLAGS}

tools_initchain_initchain_LDFLAGS = \
    ${src_libbitcoin_database_la_LDFLAGS}

tools_initchain_initchain_LDADD = \
    ${src_libbitcoin_database_la_LIBS} \
    ${src_libbitcoin_database_la_LIBADD}

tools_initchain_initchain_SOURCES = \
    ${srcdir}/../../tools/initchain/initchain.cpp

target_tools = tools/initchain/initchain

tools: ${target_tools}

endif WITH_TOOLS
