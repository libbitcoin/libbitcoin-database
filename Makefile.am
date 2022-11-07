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
    src/settings.cpp \
    src/locks/conditional_lock.cpp \
    src/locks/file_lock.cpp \
    src/locks/flush_lock.cpp \
    src/locks/interprocess_lock.cpp \
    src/locks/scope_lock.cpp \
    src/locks/sequential_lock.cpp \
    src/memory/file.cpp \
    src/memory/map.cpp \
    src/memory/mman-win32/mman.c \
    src/memory/mman-win32/mman.h \
    src/tables/archives/header.cpp \
    src/tables/archives/input.cpp \
    src/tables/archives/output.cpp \
    src/tables/archives/point.cpp \
    src/tables/archives/puts.cpp \
    src/tables/archives/transaction.cpp \
    src/tables/archives/txs.cpp \
    src/tables/caches/bootstrap.cpp \
    src/tables/caches/buffer.cpp \
    src/tables/caches/neutrino.cpp \
    src/tables/caches/validated_block.cpp \
    src/tables/caches/validated_tx.cpp \
    src/tables/indexes/address.cpp \
    src/tables/indexes/candidate_height.cpp \
    src/tables/indexes/confirmed_block.cpp \
    src/tables/indexes/confirmed_height.cpp \
    src/tables/indexes/confirmed_tx.cpp \
    src/tables/indexes/input_tx.cpp \
    src/tables/indexes/output_tx.cpp \
    src/tables/indexes/spent_output.cpp

# local: test/libbitcoin-database-test
#------------------------------------------------------------------------------
if WITH_TESTS

TESTS = libbitcoin-database-test_runner.sh

check_PROGRAMS = test/libbitcoin-database-test
test_libbitcoin_database_test_CPPFLAGS = -I${srcdir}/include ${bitcoin_system_BUILD_CPPFLAGS}
test_libbitcoin_database_test_LDADD = src/libbitcoin-database.la ${boost_unit_test_framework_LIBS} ${bitcoin_system_LIBS}
test_libbitcoin_database_test_SOURCES = \
    test/main.cpp \
    test/settings.cpp \
    test/storage.cpp \
    test/storage.hpp \
    test/test.cpp \
    test/test.hpp \
    test/locks/file_lock.cpp \
    test/locks/flush_lock.cpp \
    test/locks/interprocess_lock.cpp \
    test/memory/accessor.cpp \
    test/memory/file.cpp \
    test/memory/map.cpp \
    test/primitives/hashmap.cpp \
    test/primitives/head.cpp \
    test/primitives/iterator.cpp \
    test/primitives/linkage.cpp \
    test/primitives/manager.cpp \
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
    test/tables/caches/validated_block.cpp \
    test/tables/caches/validated_tx.cpp \
    test/tables/indexes/address.cpp \
    test/tables/indexes/candidate_height.cpp \
    test/tables/indexes/confirmed_block.cpp \
    test/tables/indexes/confirmed_height.cpp \
    test/tables/indexes/confirmed_tx.cpp \
    test/tables/indexes/input_tx.cpp \
    test/tables/indexes/output_tx.cpp \
    test/tables/indexes/spent_output.cpp

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
    include/bitcoin/database/settings.hpp \
    include/bitcoin/database/version.hpp

include_bitcoin_database_impl_memorydir = ${includedir}/bitcoin/database/impl/memory
include_bitcoin_database_impl_memory_HEADERS = \
    include/bitcoin/database/impl/memory/accessor.ipp

include_bitcoin_database_impl_primitivesdir = ${includedir}/bitcoin/database/impl/primitives
include_bitcoin_database_impl_primitives_HEADERS = \
    include/bitcoin/database/impl/primitives/hashmap.ipp \
    include/bitcoin/database/impl/primitives/head.ipp \
    include/bitcoin/database/impl/primitives/iterator.ipp \
    include/bitcoin/database/impl/primitives/linkage.ipp \
    include/bitcoin/database/impl/primitives/manager.ipp

include_bitcoin_database_locksdir = ${includedir}/bitcoin/database/locks
include_bitcoin_database_locks_HEADERS = \
    include/bitcoin/database/locks/conditional_lock.hpp \
    include/bitcoin/database/locks/file_lock.hpp \
    include/bitcoin/database/locks/flush_lock.hpp \
    include/bitcoin/database/locks/interprocess_lock.hpp \
    include/bitcoin/database/locks/locks.hpp \
    include/bitcoin/database/locks/scope_lock.hpp \
    include/bitcoin/database/locks/sequential_lock.hpp

include_bitcoin_database_memorydir = ${includedir}/bitcoin/database/memory
include_bitcoin_database_memory_HEADERS = \
    include/bitcoin/database/memory/accessor.hpp \
    include/bitcoin/database/memory/file.hpp \
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
    include/bitcoin/database/primitives/hashmap.hpp \
    include/bitcoin/database/primitives/head.hpp \
    include/bitcoin/database/primitives/iterator.hpp \
    include/bitcoin/database/primitives/linkage.hpp \
    include/bitcoin/database/primitives/manager.hpp \
    include/bitcoin/database/primitives/primitives.hpp

include_bitcoin_database_tablesdir = ${includedir}/bitcoin/database/tables
include_bitcoin_database_tables_HEADERS = \
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
    include/bitcoin/database/tables/caches/validated_block.hpp \
    include/bitcoin/database/tables/caches/validated_tx.hpp

include_bitcoin_database_tables_indexesdir = ${includedir}/bitcoin/database/tables/indexes
include_bitcoin_database_tables_indexes_HEADERS = \
    include/bitcoin/database/tables/indexes/address.hpp \
    include/bitcoin/database/tables/indexes/candidate_height.hpp \
    include/bitcoin/database/tables/indexes/confirmed_block.hpp \
    include/bitcoin/database/tables/indexes/confirmed_height.hpp \
    include/bitcoin/database/tables/indexes/confirmed_tx.hpp \
    include/bitcoin/database/tables/indexes/input_tx.hpp \
    include/bitcoin/database/tables/indexes/output_tx.hpp \
    include/bitcoin/database/tables/indexes/spent_output.hpp


# Custom make targets.
#==============================================================================
# make target: tools
#------------------------------------------------------------------------------
target_tools = \
    tools/initchain/initchain

tools: ${target_tools}

