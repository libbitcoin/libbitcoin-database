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
src_libbitcoin_database_la_CPPFLAGS = -I${srcdir}/include ${bitcoin_CPPFLAGS}
src_libbitcoin_database_la_LIBADD = ${bitcoin_LIBS}
src_libbitcoin_database_la_SOURCES = \
    src/data_base.cpp \
    src/settings.cpp \
    src/store.cpp \
    src/unspent_outputs.cpp \
    src/unspent_transaction.cpp \
    src/databases/block_database.cpp \
    src/databases/history_database.cpp \
    src/databases/spend_database.cpp \
    src/databases/stealth_database.cpp \
    src/databases/transaction_database.cpp \
    src/memory/accessor.cpp \
    src/memory/allocator.cpp \
    src/memory/memory_map.cpp \
    src/mman-win32/mman.c \
    src/mman-win32/mman.h \
    src/primitives/record_list.cpp \
    src/primitives/record_manager.cpp \
    src/primitives/record_multimap_iterable.cpp \
    src/primitives/record_multimap_iterator.cpp \
    src/primitives/slab_manager.cpp \
    src/result/block_result.cpp \
    src/result/transaction_result.cpp

# local: test/libbitcoin_database_test
#------------------------------------------------------------------------------
if WITH_TESTS

