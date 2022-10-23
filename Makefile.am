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
    test/storage.cpp \
    test/storage.hpp \
    test/test.cpp \
    test/test.hpp \
    test/elements/elements.cpp \
    test/locks/file_lock.cpp \
    test/locks/flush_lock.cpp \
    test/locks/interprocess_lock.cpp \
    test/memory/accessor.cpp \
    test/memory/map.cpp \
    test/primitives/element.cpp \
    test/primitives/iterable.cpp \
    test/primitives/iterator.cpp \
    test/primitives/manager.cpp

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

include_bitcoin_database_elementsdir = ${includedir}/bitcoin/database/elements
include_bitcoin_database_elements_HEADERS = \
    include/bitcoin/database/elements/elements.hpp \
    include/bitcoin/database/elements/keyed_record.hpp \
    include/bitcoin/database/elements/keyed_slab.hpp \
    include/bitcoin/database/elements/record.hpp \
    include/bitcoin/database/elements/slab.hpp

include_bitcoin_database_impl_elementsdir = ${includedir}/bitcoin/database/impl/elements
include_bitcoin_database_impl_elements_HEADERS = \
    include/bitcoin/database/impl/elements/keyed_record.ipp \
    include/bitcoin/database/impl/elements/keyed_slab.ipp \
    include/bitcoin/database/impl/elements/record.ipp \
    include/bitcoin/database/impl/elements/slab.ipp

include_bitcoin_database_impl_memorydir = ${includedir}/bitcoin/database/impl/memory
include_bitcoin_database_impl_memory_HEADERS = \
    include/bitcoin/database/impl/memory/accessor.ipp

include_bitcoin_database_impl_primitivesdir = ${includedir}/bitcoin/database/impl/primitives
include_bitcoin_database_impl_primitives_HEADERS = \
    include/bitcoin/database/impl/primitives/element.ipp \
    include/bitcoin/database/impl/primitives/iterable.ipp \
    include/bitcoin/database/impl/primitives/iterator.ipp \
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
    include/bitcoin/database/memory/storage.hpp

include_bitcoin_database_primitivesdir = ${includedir}/bitcoin/database/primitives
include_bitcoin_database_primitives_HEADERS = \
    include/bitcoin/database/primitives/element.hpp \
    include/bitcoin/database/primitives/iterable.hpp \
    include/bitcoin/database/primitives/iterator.hpp \
    include/bitcoin/database/primitives/manager.hpp \
    include/bitcoin/database/primitives/primitives.hpp


# Custom make targets.
#==============================================================================
# make target: tools
#------------------------------------------------------------------------------
target_tools = \
    tools/initchain/initchain

tools: ${target_tools}

