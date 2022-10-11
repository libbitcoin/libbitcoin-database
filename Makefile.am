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
    src/memory/accessor.cpp \
    src/memory/file_storage.cpp \
    src/mman-win32/mman.c \
    src/mman-win32/mman.h

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
    test/test.cpp \
    test/test.hpp \
    test/locks/file_lock.cpp \
    test/locks/flush_lock.cpp \
    test/locks/interprocess_lock.cpp \
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
    test/utility/storage.cpp \
    test/utility/storage.hpp \
    test/utility/utility.cpp \
    test/utility/utility.hpp

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

include_bitcoin_database_impl_primitivesdir = ${includedir}/bitcoin/database/impl/primitives
include_bitcoin_database_impl_primitives_HEADERS = \
    include/bitcoin/database/impl/primitives/element.ipp

include_bitcoin_database_impl_primitives_dir = ${includedir}/bitcoin/database/impl/primitives_
include_bitcoin_database_impl_primitives__HEADERS = \
    include/bitcoin/database/impl/primitives_/hash_table.ipp \
    include/bitcoin/database/impl/primitives_/hash_table_header.ipp \
    include/bitcoin/database/impl/primitives_/hash_table_multimap.ipp \
    include/bitcoin/database/impl/primitives_/list.ipp \
    include/bitcoin/database/impl/primitives_/list_element.ipp \
    include/bitcoin/database/impl/primitives_/list_iterator.ipp \
    include/bitcoin/database/impl/primitives_/record_manager.ipp \
    include/bitcoin/database/impl/primitives_/slab_manager.ipp

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
    include/bitcoin/database/memory/file_storage.hpp \
    include/bitcoin/database/memory/memory.hpp \
    include/bitcoin/database/memory/storage.hpp

include_bitcoin_database_primitivesdir = ${includedir}/bitcoin/database/primitives
include_bitcoin_database_primitives_HEADERS = \
    include/bitcoin/database/primitives/element.hpp \
    include/bitcoin/database/primitives/primitives.hpp

include_bitcoin_database_primitives_dir = ${includedir}/bitcoin/database/primitives_
include_bitcoin_database_primitives__HEADERS = \
    include/bitcoin/database/primitives_/hash_table.hpp \
    include/bitcoin/database/primitives_/hash_table_header.hpp \
    include/bitcoin/database/primitives_/hash_table_multimap.hpp \
    include/bitcoin/database/primitives_/list.hpp \
    include/bitcoin/database/primitives_/list_element.hpp \
    include/bitcoin/database/primitives_/list_iterator.hpp \
    include/bitcoin/database/primitives_/primitives.hpp \
    include/bitcoin/database/primitives_/record_manager.hpp \
    include/bitcoin/database/primitives_/slab_manager.hpp


# Custom make targets.
#==============================================================================
# make target: tools
#------------------------------------------------------------------------------
target_tools = \
    tools/initchain/initchain

tools: ${target_tools}

