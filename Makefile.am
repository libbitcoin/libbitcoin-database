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

TESTS = libbitcoin_database_test_runner.sh

check_PROGRAMS = test/libbitcoin_database_test
test_libbitcoin_database_test_CPPFLAGS = -I${srcdir}/include ${bitcoin_CPPFLAGS}
test_libbitcoin_database_test_LDADD = src/libbitcoin-database.la ${boost_unit_test_framework_LIBS} ${bitcoin_LIBS}
test_libbitcoin_database_test_SOURCES = \
    test/block_database.cpp \
    test/data_base.cpp \
    test/hash_table.cpp \
    test/history_database.cpp \
    test/main.cpp \
    test/spend_database.cpp \
    test/structure.cpp \
    test/transaction_database.cpp

endif WITH_TESTS

# local: tools/block_db/block_db
#------------------------------------------------------------------------------
if WITH_TOOLS

noinst_PROGRAMS = tools/block_db/block_db
tools_block_db_block_db_CPPFLAGS = -I${srcdir}/include ${bitcoin_CPPFLAGS}
tools_block_db_block_db_LDADD = src/libbitcoin-database.la ${bitcoin_LIBS}
tools_block_db_block_db_SOURCES = \
    tools/block_db/block_db.cpp

endif WITH_TOOLS

# local: tools/count_records/count_records
#------------------------------------------------------------------------------
if WITH_TOOLS

noinst_PROGRAMS += tools/count_records/count_records
tools_count_records_count_records_CPPFLAGS = -I${srcdir}/include ${bitcoin_CPPFLAGS}
tools_count_records_count_records_LDADD = src/libbitcoin-database.la ${bitcoin_LIBS}
tools_count_records_count_records_SOURCES = \
    tools/count_records/count_records.cpp

endif WITH_TOOLS

# local: tools/history_db/history_db
#------------------------------------------------------------------------------
if WITH_TOOLS

noinst_PROGRAMS += tools/history_db/history_db
tools_history_db_history_db_CPPFLAGS = -I${srcdir}/include ${bitcoin_CPPFLAGS}
tools_history_db_history_db_LDADD = src/libbitcoin-database.la ${bitcoin_LIBS}
tools_history_db_history_db_SOURCES = \
    tools/history_db/history_db.cpp

endif WITH_TOOLS

# local: tools/initchain/initchain
#------------------------------------------------------------------------------
if WITH_TOOLS

noinst_PROGRAMS += tools/initchain/initchain
tools_initchain_initchain_CPPFLAGS = -I${srcdir}/include ${bitcoin_CPPFLAGS}
tools_initchain_initchain_LDADD = src/libbitcoin-database.la ${bitcoin_LIBS}
tools_initchain_initchain_SOURCES = \
    tools/initchain/initchain.cpp

endif WITH_TOOLS

# local: tools/mmr_add_row/mmr_add_row
#------------------------------------------------------------------------------
if WITH_TOOLS

noinst_PROGRAMS += tools/mmr_add_row/mmr_add_row
tools_mmr_add_row_mmr_add_row_CPPFLAGS = -I${srcdir}/include ${bitcoin_CPPFLAGS}
tools_mmr_add_row_mmr_add_row_LDADD = src/libbitcoin-database.la ${bitcoin_LIBS}
tools_mmr_add_row_mmr_add_row_SOURCES = \
    tools/mmr_add_row/mmr_add_row.cpp

endif WITH_TOOLS

# local: tools/mmr_create/mmr_create
#------------------------------------------------------------------------------
if WITH_TOOLS

noinst_PROGRAMS += tools/mmr_create/mmr_create
tools_mmr_create_mmr_create_CPPFLAGS = -I${srcdir}/include ${bitcoin_CPPFLAGS}
tools_mmr_create_mmr_create_LDADD = src/libbitcoin-database.la ${bitcoin_LIBS}
tools_mmr_create_mmr_create_SOURCES = \
    tools/mmr_create/mmr_create.cpp

endif WITH_TOOLS

# local: tools/mmr_delete_last_row/mmr_delete_last_row
#------------------------------------------------------------------------------
if WITH_TOOLS

noinst_PROGRAMS += tools/mmr_delete_last_row/mmr_delete_last_row
tools_mmr_delete_last_row_mmr_delete_last_row_CPPFLAGS = -I${srcdir}/include ${bitcoin_CPPFLAGS}
tools_mmr_delete_last_row_mmr_delete_last_row_LDADD = src/libbitcoin-database.la ${bitcoin_LIBS}
tools_mmr_delete_last_row_mmr_delete_last_row_SOURCES = \
    tools/mmr_delete_last_row/mmr_delete_last_row.cpp

endif WITH_TOOLS

# local: tools/mmr_lookup/mmr_lookup
#------------------------------------------------------------------------------
if WITH_TOOLS

noinst_PROGRAMS += tools/mmr_lookup/mmr_lookup
tools_mmr_lookup_mmr_lookup_CPPFLAGS = -I${srcdir}/include ${bitcoin_CPPFLAGS}
tools_mmr_lookup_mmr_lookup_LDADD = src/libbitcoin-database.la ${bitcoin_LIBS}
tools_mmr_lookup_mmr_lookup_SOURCES = \
    tools/mmr_lookup/mmr_lookup.cpp

endif WITH_TOOLS

# local: tools/read_htdb_record_value/read_htdb_record_value
#------------------------------------------------------------------------------
if WITH_TOOLS

noinst_PROGRAMS += tools/read_htdb_record_value/read_htdb_record_value
tools_read_htdb_record_value_read_htdb_record_value_CPPFLAGS = -I${srcdir}/include ${bitcoin_CPPFLAGS}
tools_read_htdb_record_value_read_htdb_record_value_LDADD = src/libbitcoin-database.la ${bitcoin_LIBS}
tools_read_htdb_record_value_read_htdb_record_value_SOURCES = \
    tools/read_htdb_record_value/read_htdb_record_value.cpp

endif WITH_TOOLS

# local: tools/read_htdb_slab_value/read_htdb_slab_value
#------------------------------------------------------------------------------
if WITH_TOOLS

noinst_PROGRAMS += tools/read_htdb_slab_value/read_htdb_slab_value
tools_read_htdb_slab_value_read_htdb_slab_value_CPPFLAGS = -I${srcdir}/include ${bitcoin_CPPFLAGS}
tools_read_htdb_slab_value_read_htdb_slab_value_LDADD = src/libbitcoin-database.la ${bitcoin_LIBS}
tools_read_htdb_slab_value_read_htdb_slab_value_SOURCES = \
    tools/read_htdb_slab_value/read_htdb_slab_value.cpp

endif WITH_TOOLS

# local: tools/show_array/show_array
#------------------------------------------------------------------------------
if WITH_TOOLS

noinst_PROGRAMS += tools/show_array/show_array
tools_show_array_show_array_CPPFLAGS = -I${srcdir}/include ${bitcoin_CPPFLAGS}
tools_show_array_show_array_LDADD = src/libbitcoin-database.la ${bitcoin_LIBS}
tools_show_array_show_array_SOURCES = \
    tools/show_array/show_array.cpp

endif WITH_TOOLS

# local: tools/show_linked_records/show_linked_records
#------------------------------------------------------------------------------
if WITH_TOOLS

noinst_PROGRAMS += tools/show_linked_records/show_linked_records
tools_show_linked_records_show_linked_records_CPPFLAGS = -I${srcdir}/include ${bitcoin_CPPFLAGS}
tools_show_linked_records_show_linked_records_LDADD = src/libbitcoin-database.la ${bitcoin_LIBS}
tools_show_linked_records_show_linked_records_SOURCES = \
    tools/show_linked_records/show_linked_records.cpp

endif WITH_TOOLS

# local: tools/show_records/show_records
#------------------------------------------------------------------------------
if WITH_TOOLS

noinst_PROGRAMS += tools/show_records/show_records
tools_show_records_show_records_CPPFLAGS = -I${srcdir}/include ${bitcoin_CPPFLAGS}
tools_show_records_show_records_LDADD = src/libbitcoin-database.la ${bitcoin_LIBS}
tools_show_records_show_records_SOURCES = \
    tools/show_records/show_records.cpp

endif WITH_TOOLS

# local: tools/spend_db/spend_db
#------------------------------------------------------------------------------
if WITH_TOOLS

noinst_PROGRAMS += tools/spend_db/spend_db
tools_spend_db_spend_db_CPPFLAGS = -I${srcdir}/include ${bitcoin_CPPFLAGS}
tools_spend_db_spend_db_LDADD = src/libbitcoin-database.la ${bitcoin_LIBS}
tools_spend_db_spend_db_SOURCES = \
    tools/spend_db/spend_db.cpp

endif WITH_TOOLS

# local: tools/stealth_db/stealth_db
#------------------------------------------------------------------------------
if WITH_TOOLS

noinst_PROGRAMS += tools/stealth_db/stealth_db
tools_stealth_db_stealth_db_CPPFLAGS = -I${srcdir}/include ${bitcoin_CPPFLAGS}
tools_stealth_db_stealth_db_LDADD = src/libbitcoin-database.la ${bitcoin_LIBS}
tools_stealth_db_stealth_db_SOURCES = \
    tools/stealth_db/stealth_db.cpp

endif WITH_TOOLS

# local: tools/transaction_db/transaction_db
#------------------------------------------------------------------------------
if WITH_TOOLS

noinst_PROGRAMS += tools/transaction_db/transaction_db
tools_transaction_db_transaction_db_CPPFLAGS = -I${srcdir}/include ${bitcoin_CPPFLAGS}
tools_transaction_db_transaction_db_LDADD = src/libbitcoin-database.la ${bitcoin_LIBS}
tools_transaction_db_transaction_db_SOURCES = \
    tools/transaction_db/transaction_db.cpp

endif WITH_TOOLS

# files => ${includedir}/bitcoin
#------------------------------------------------------------------------------
include_bitcoindir = ${includedir}/bitcoin
include_bitcoin_HEADERS = \
    include/bitcoin/database.hpp

include_bitcoin_databasedir = ${includedir}/bitcoin/database
include_bitcoin_database_HEADERS = \
    include/bitcoin/database/data_base.hpp \
    include/bitcoin/database/define.hpp \
    include/bitcoin/database/settings.hpp \
    include/bitcoin/database/store.hpp \
    include/bitcoin/database/version.hpp

include_bitcoin_database_databasesdir = ${includedir}/bitcoin/database/databases
include_bitcoin_database_databases_HEADERS = \
    include/bitcoin/database/databases/block_database.hpp \
    include/bitcoin/database/databases/history_database.hpp \
    include/bitcoin/database/databases/spend_database.hpp \
    include/bitcoin/database/databases/stealth_database.hpp \
    include/bitcoin/database/databases/transaction_database.hpp

include_bitcoin_database_impldir = ${includedir}/bitcoin/database/impl
include_bitcoin_database_impl_HEADERS = \
    include/bitcoin/database/impl/hash_table_header.ipp \
    include/bitcoin/database/impl/record_hash_table.ipp \
    include/bitcoin/database/impl/record_multimap.ipp \
    include/bitcoin/database/impl/record_row.ipp \
    include/bitcoin/database/impl/remainder.ipp \
    include/bitcoin/database/impl/slab_hash_table.ipp \
    include/bitcoin/database/impl/slab_row.ipp

include_bitcoin_database_memorydir = ${includedir}/bitcoin/database/memory
include_bitcoin_database_memory_HEADERS = \
    include/bitcoin/database/memory/accessor.hpp \
    include/bitcoin/database/memory/allocator.hpp \
    include/bitcoin/database/memory/memory.hpp \
    include/bitcoin/database/memory/memory_map.hpp

include_bitcoin_database_primitivesdir = ${includedir}/bitcoin/database/primitives
include_bitcoin_database_primitives_HEADERS = \
    include/bitcoin/database/primitives/hash_table_header.hpp \
    include/bitcoin/database/primitives/record_hash_table.hpp \
    include/bitcoin/database/primitives/record_list.hpp \
    include/bitcoin/database/primitives/record_manager.hpp \
    include/bitcoin/database/primitives/record_multimap.hpp \
    include/bitcoin/database/primitives/record_multimap_iterable.hpp \
    include/bitcoin/database/primitives/record_multimap_iterator.hpp \
    include/bitcoin/database/primitives/slab_hash_table.hpp \
    include/bitcoin/database/primitives/slab_manager.hpp

include_bitcoin_database_resultdir = ${includedir}/bitcoin/database/result
include_bitcoin_database_result_HEADERS = \
    include/bitcoin/database/result/block_result.hpp \
    include/bitcoin/database/result/transaction_result.hpp


# Custom make targets.
#==============================================================================
# make target: tools
#------------------------------------------------------------------------------
target_tools = \
    tools/block_db/block_db \
    tools/count_records/count_records \
    tools/history_db/history_db \
    tools/initchain/initchain \
    tools/mmr_add_row/mmr_add_row \
    tools/mmr_create/mmr_create \
    tools/mmr_delete_last_row/mmr_delete_last_row \
    tools/mmr_lookup/mmr_lookup \
    tools/read_htdb_record_value/read_htdb_record_value \
    tools/read_htdb_slab_value/read_htdb_slab_value \
    tools/show_array/show_array \
    tools/show_linked_records/show_linked_records \
    tools/show_records/show_records \
    tools/spend_db/spend_db \
    tools/stealth_db/stealth_db \
    tools/transaction_db/transaction_db

tools: ${target_tools}

