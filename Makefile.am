###############################################################################
#  Copyright (c) 2014-2020 libbitcoin-database developers (see COPYING).
#
#         GENERATED SOURCE CODE, DO NOT EDIT EXCEPT EXPERIMENTALLY
#
###############################################################################

# Automake settings.
#==============================================================================
# Look for macros in the m4 subdirectory.
#------------------------------------------------------------------------------
ACLOCAL_AMFLAGS = -I m4


# Distribute, make and install products.
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

# src/libbitcoin-database.la => ${libdir}
#------------------------------------------------------------------------------
lib_LTLIBRARIES = src/libbitcoin-database.la
src_libbitcoin_database_la_CPPFLAGS = -I${srcdir}/include ${bitcoin_system_BUILD_CPPFLAGS}
src_libbitcoin_database_la_LIBADD = ${bitcoin_system_LIBS}
src_libbitcoin_database_la_SOURCES = \
    src/error.cpp \
    src/settings.cpp \
    src/file/rotator.cpp \
    src/file/utilities.cpp \
    src/locks/file_lock.cpp \
    src/locks/flush_lock.cpp \
    src/locks/interprocess_lock.cpp \
    src/memory/map.cpp \
    src/memory/mman-win32/mman.c \
    src/memory/mman-win32/mman.h

# local: test/libbitcoin-database-test
#------------------------------------------------------------------------------
if WITH_TESTS

TESTS = libbitcoin-database-test_runner.sh

check_PROGRAMS = test/libbitcoin-database-test
test_libbitcoin_database_test_CPPFLAGS = -I${srcdir}/include ${bitcoin_system_BUILD_CPPFLAGS}
test_libbitcoin_database_test_LDADD = src/libbitcoin-database.la ${boost_unit_test_framework_LIBS} ${bitcoin_system_LIBS}
test_libbitcoin_database_test_SOURCES = \
    test/error.cpp \
    test/main.cpp \
    test/settings.cpp \
    test/store.cpp \
    test/test.cpp \
    test/test.hpp \
    test/file/rotator.cpp \
    test/file/utilities.cpp \
    test/locks/file_lock.cpp \
    test/locks/flush_lock.cpp \
    test/locks/interprocess_lock.cpp \
    test/memory/accessor.cpp \
    test/memory/map.cpp \
    test/mocks/blocks.hpp \
    test/mocks/chunk_storage.cpp \
    test/mocks/chunk_storage.hpp \
    test/mocks/chunk_store.hpp \
    test/mocks/map_store.hpp \
    test/primitives/arraymap.cpp \
    test/primitives/hashmap.cpp \
    test/primitives/head.cpp \
    test/primitives/iterator.cpp \
    test/primitives/linkage.cpp \
    test/primitives/manager.cpp \
    test/query/archival.cpp \
    test/query/confirmation.cpp \
    test/query/initialization.cpp \
    test/query/optional.cpp \
    test/query/sizing.cpp \
    test/query/translation.cpp \
    test/query/validation.cpp \
    test/tables/archives/header.cpp \
    test/tables/archives/input.cpp \
    test/tables/archives/output.cpp \
    test/tables/archives/point.cpp \
    test/tables/archives/puts.cpp \
    test/tables/archives/transaction.cpp \
    test/tables/archives/txs.cpp \
    test/tables/caches/bootstrap.cpp \
    test/tables/caches/buffer.cpp \
    test/tables/caches/neutrino.cpp \
    test/tables/caches/validated_bk.cpp \
    test/tables/caches/validated_tx.cpp \
    test/tables/indexes/address.cpp \
    test/tables/indexes/height.cpp \
    test/tables/indexes/strong_tx.cpp

endif WITH_TESTS

# local: tools/initchain/initchain
#------------------------------------------------------------------------------
if WITH_TOOLS

noinst_PROGRAMS = tools/initchain/initchain
tools_initchain_initchain_CPPFLAGS = -I${srcdir}/include ${bitcoin_system_BUILD_CPPFLAGS}
tools_initchain_initchain_LDADD = src/libbitcoin-database.la ${bitcoin_system_LIBS}
tools_initchain_initchain_SOURCES = \
    tools/initchain/initchain.cpp

endif WITH_TOOLS

# files => ${includedir}/bitcoin
#------------------------------------------------------------------------------
include_bitcoindir = ${includedir}/bitcoin
include_bitcoin_HEADERS = \
    include/bitcoin/database.hpp

include_bitcoin_databasedir = ${includedir}/bitcoin/database
include_bitcoin_database_HEADERS = \
    include/bitcoin/database/boost.hpp \
    include/bitcoin/database/define.hpp \
    include/bitcoin/database/error.hpp \
    include/bitcoin/database/query.hpp \
    include/bitcoin/database/settings.hpp \
    include/bitcoin/database/store.hpp \
    include/bitcoin/database/version.hpp

include_bitcoin_database_filedir = ${includedir}/bitcoin/database/file
include_bitcoin_database_file_HEADERS = \
    include/bitcoin/database/file/file.hpp \
    include/bitcoin/database/file/rotator.hpp \
    include/bitcoin/database/file/utilities.hpp

include_bitcoin_database_impldir = ${includedir}/bitcoin/database/impl
include_bitcoin_database_impl_HEADERS = \
    include/bitcoin/database/impl/store.ipp

include_bitcoin_database_impl_memorydir = ${includedir}/bitcoin/database/impl/memory
include_bitcoin_database_impl_memory_HEADERS = \
    include/bitcoin/database/impl/memory/accessor.ipp

include_bitcoin_database_impl_primitivesdir = ${includedir}/bitcoin/database/impl/primitives
include_bitcoin_database_impl_primitives_HEADERS = \
    include/bitcoin/database/impl/primitives/arraymap.ipp \
    include/bitcoin/database/impl/primitives/hashmap.ipp \
    include/bitcoin/database/impl/primitives/head.ipp \
    include/bitcoin/database/impl/primitives/iterator.ipp \
    include/bitcoin/database/impl/primitives/linkage.ipp \
    include/bitcoin/database/impl/primitives/manager.ipp

include_bitcoin_database_impl_querydir = ${includedir}/bitcoin/database/impl/query
include_bitcoin_database_impl_query_HEADERS = \
    include/bitcoin/database/impl/query/archive.ipp \
    include/bitcoin/database/impl/query/confirm.ipp \
    include/bitcoin/database/impl/query/context.ipp \
    include/bitcoin/database/impl/query/extent.ipp \
    include/bitcoin/database/impl/query/initialize.ipp \
    include/bitcoin/database/impl/query/optional.ipp \
    include/bitcoin/database/impl/query/query.ipp \
    include/bitcoin/database/impl/query/translate.ipp \
    include/bitcoin/database/impl/query/validate.ipp

include_bitcoin_database_locksdir = ${includedir}/bitcoin/database/locks
include_bitcoin_database_locks_HEADERS = \
    include/bitcoin/database/locks/file_lock.hpp \
    include/bitcoin/database/locks/flush_lock.hpp \
    include/bitcoin/database/locks/interprocess_lock.hpp \
    include/bitcoin/database/locks/locks.hpp

include_bitcoin_database_memorydir = ${includedir}/bitcoin/database/memory
include_bitcoin_database_memory_HEADERS = \
    include/bitcoin/database/memory/accessor.hpp \
    include/bitcoin/database/memory/finalizer.hpp \
    include/bitcoin/database/memory/map.hpp \
    include/bitcoin/database/memory/memory.hpp \
    include/bitcoin/database/memory/reader.hpp \
    include/bitcoin/database/memory/writer.hpp

include_bitcoin_database_memory_interfacesdir = ${includedir}/bitcoin/database/memory/interfaces
include_bitcoin_database_memory_interfaces_HEADERS = \
    include/bitcoin/database/memory/interfaces/memory.hpp \
    include/bitcoin/database/memory/interfaces/storage.hpp

include_bitcoin_database_primitivesdir = ${includedir}/bitcoin/database/primitives
include_bitcoin_database_primitives_HEADERS = \
    include/bitcoin/database/primitives/arraymap.hpp \
    include/bitcoin/database/primitives/hashmap.hpp \
    include/bitcoin/database/primitives/head.hpp \
    include/bitcoin/database/primitives/iterator.hpp \
    include/bitcoin/database/primitives/linkage.hpp \
    include/bitcoin/database/primitives/manager.hpp \
    include/bitcoin/database/primitives/primitives.hpp

include_bitcoin_database_tablesdir = ${includedir}/bitcoin/database/tables
include_bitcoin_database_tables_HEADERS = \
    include/bitcoin/database/tables/context.hpp \
    include/bitcoin/database/tables/schema.hpp \
    include/bitcoin/database/tables/tables.hpp

include_bitcoin_database_tables_archivesdir = ${includedir}/bitcoin/database/tables/archives
include_bitcoin_database_tables_archives_HEADERS = \
    include/bitcoin/database/tables/archives/header.hpp \
    include/bitcoin/database/tables/archives/input.hpp \
    include/bitcoin/database/tables/archives/output.hpp \
    include/bitcoin/database/tables/archives/point.hpp \
    include/bitcoin/database/tables/archives/puts.hpp \
    include/bitcoin/database/tables/archives/transaction.hpp \
    include/bitcoin/database/tables/archives/txs.hpp

include_bitcoin_database_tables_cachesdir = ${includedir}/bitcoin/database/tables/caches
include_bitcoin_database_tables_caches_HEADERS = \
    include/bitcoin/database/tables/caches/bootstrap.hpp \
    include/bitcoin/database/tables/caches/buffer.hpp \
    include/bitcoin/database/tables/caches/neutrino.hpp \
    include/bitcoin/database/tables/caches/validated_bk.hpp \
    include/bitcoin/database/tables/caches/validated_tx.hpp

include_bitcoin_database_tables_indexesdir = ${includedir}/bitcoin/database/tables/indexes
include_bitcoin_database_tables_indexes_HEADERS = \
    include/bitcoin/database/tables/indexes/address.hpp \
    include/bitcoin/database/tables/indexes/height.hpp \
    include/bitcoin/database/tables/indexes/strong_tx.hpp


# Custom make targets.
#==============================================================================
# make target: tools
#------------------------------------------------------------------------------
target_tools = \
    tools/initchain/initchain

tools: ${target_tools}

