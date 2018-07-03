###############################################################################
#  Copyright (c) 2014-2015 libbitcoin-database developers (see COPYING).
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
src_libbitcoin_database_la_CPPFLAGS = -I${srcdir}/include ${bitcoin_BUILD_CPPFLAGS}
src_libbitcoin_database_la_LIBADD = ${bitcoin_LIBS}
src_libbitcoin_database_la_SOURCES = \
    src/data_base.cpp \
    src/settings.cpp \
    src/store.cpp \
    src/unspent_outputs.cpp \
    src/unspent_transaction.cpp \
    src/verify.cpp \
    src/databases/address_database.cpp \
    src/databases/block_database.cpp \
    src/databases/transaction_database.cpp \
    src/memory/accessor.cpp \
    src/memory/file_storage.cpp \
    src/mman-win32/mman.c \
    src/mman-win32/mman.h \
    src/result/address_iterator.cpp \
    src/result/address_result.cpp \
    src/result/block_result.cpp \
    src/result/inpoint_iterator.cpp \
    src/result/transaction_iterator.cpp \
    src/result/transaction_result.cpp

# local: test/libbitcoin-database-test
#------------------------------------------------------------------------------
if WITH_TESTS

TESTS = libbitcoin-database-test_runner.sh

check_PROGRAMS = test/libbitcoin-database-test
test_libbitcoin_database_test_CPPFLAGS = -I${srcdir}/include ${bitcoin_BUILD_CPPFLAGS}
test_libbitcoin_database_test_LDADD = src/libbitcoin-database.la ${boost_unit_test_framework_LIBS} ${bitcoin_LIBS}
test_libbitcoin_database_test_SOURCES = \
    test/block_state.cpp \
    test/data_base.cpp \
    test/main.cpp \
    test/settings.cpp \
    test/store.cpp \
    test/unspent_outputs.cpp \
    test/unspent_transaction.cpp \
    test/databases/address_database.cpp \
    test/databases/block_database.cpp \
    test/databases/transaction_database.cpp \
    test/memory/accessor.cpp \
    test/memory/file_storage.cpp \
    test/primitives/hash_table.cpp \
    test/primitives/hash_table_header.cpp \
    test/primitives/hash_table_multimap.cpp \
    test/primitives/list.cpp \
    test/primitives/list_element.cpp \
    test/primitives/list_iterator.cpp \
    test/primitives/record_manager.cpp \
    test/primitives/slab_manager.cpp \
    test/result/address_iterator.cpp \
    test/result/address_result.cpp \
    test/result/block_result.cpp \
    test/result/transaction_iterator.cpp \
    test/result/transaction_result.cpp \
    test/utility/storage.cpp \
    test/utility/storage.hpp \
    test/utility/utility.cpp \
    test/utility/utility.hpp

endif WITH_TESTS

# local: tools/initchain/initchain
#------------------------------------------------------------------------------
if WITH_TOOLS

noinst_PROGRAMS = tools/initchain/initchain
tools_initchain_initchain_CPPFLAGS = -I${srcdir}/include ${bitcoin_BUILD_CPPFLAGS}
tools_initchain_initchain_LDADD = src/libbitcoin-database.la ${bitcoin_LIBS}
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
    include/bitcoin/database/block_state.hpp \
    include/bitcoin/database/data_base.hpp \
    include/bitcoin/database/define.hpp \
    include/bitcoin/database/settings.hpp \
    include/bitcoin/database/store.hpp \
    include/bitcoin/database/unspent_outputs.hpp \
    include/bitcoin/database/unspent_transaction.hpp \
    include/bitcoin/database/verify.hpp \
    include/bitcoin/database/version.hpp

include_bitcoin_database_databasesdir = ${includedir}/bitcoin/database/databases
include_bitcoin_database_databases_HEADERS = \
    include/bitcoin/database/databases/address_database.hpp \
    include/bitcoin/database/databases/block_database.hpp \
    include/bitcoin/database/databases/transaction_database.hpp

include_bitcoin_database_impldir = ${includedir}/bitcoin/database/impl
include_bitcoin_database_impl_HEADERS = \
    include/bitcoin/database/impl/hash_table.ipp \
    include/bitcoin/database/impl/hash_table_header.ipp \
    include/bitcoin/database/impl/hash_table_multimap.ipp \
    include/bitcoin/database/impl/list.ipp \
    include/bitcoin/database/impl/list_element.ipp \
    include/bitcoin/database/impl/list_iterator.ipp \
    include/bitcoin/database/impl/record_manager.ipp \
    include/bitcoin/database/impl/slab_manager.ipp

include_bitcoin_database_memorydir = ${includedir}/bitcoin/database/memory
include_bitcoin_database_memory_HEADERS = \
    include/bitcoin/database/memory/accessor.hpp \
    include/bitcoin/database/memory/file_storage.hpp \
    include/bitcoin/database/memory/memory.hpp \
    include/bitcoin/database/memory/storage.hpp

include_bitcoin_database_primitivesdir = ${includedir}/bitcoin/database/primitives
include_bitcoin_database_primitives_HEADERS = \
    include/bitcoin/database/primitives/hash_table.hpp \
    include/bitcoin/database/primitives/hash_table_header.hpp \
    include/bitcoin/database/primitives/hash_table_multimap.hpp \
    include/bitcoin/database/primitives/list.hpp \
    include/bitcoin/database/primitives/list_element.hpp \
    include/bitcoin/database/primitives/list_iterator.hpp \
    include/bitcoin/database/primitives/record_manager.hpp \
    include/bitcoin/database/primitives/slab_manager.hpp

include_bitcoin_database_resultdir = ${includedir}/bitcoin/database/result
include_bitcoin_database_result_HEADERS = \
    include/bitcoin/database/result/address_iterator.hpp \
    include/bitcoin/database/result/address_result.hpp \
    include/bitcoin/database/result/block_result.hpp \
    include/bitcoin/database/result/inpoint_iterator.hpp \
    include/bitcoin/database/result/transaction_iterator.hpp \
    include/bitcoin/database/result/transaction_result.hpp


# Custom make targets.
#==============================================================================
# make target: tools
#------------------------------------------------------------------------------
target_tools = \
    tools/initchain/initchain

tools: ${target_tools}

