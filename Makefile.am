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
    src/block_database.cpp \
    src/data_base.cpp \
    src/history_database.cpp \
    src/settings.cpp \
    src/spend_database.cpp \
    src/stealth_database.cpp \
    src/transaction_database.cpp \
    src/disk/mmfile.cpp \
    src/mman-win32/mman.c \
    src/mman-win32/mman.h \
    src/record/linked_records.cpp \
    src/record/multimap_records.cpp \
    src/record/record_manager.cpp \
    src/slab/slab_manager.cpp

# local: test/libbitcoin_database_test
#------------------------------------------------------------------------------
if WITH_TESTS

TESTS = libbitcoin_database_test_runner.sh

check_PROGRAMS = test/libbitcoin_database_test
test_libbitcoin_database_test_CPPFLAGS = -I${srcdir}/include ${bitcoin_CPPFLAGS}
test_libbitcoin_database_test_LDADD = src/libbitcoin-database.la ${boost_unit_test_framework_LIBS} ${bitcoin_LIBS}
test_libbitcoin_database_test_SOURCES = \
    test/databases.cpp \
    test/disk_objs.cpp \
    test/htdb.cpp \
    test/interface.cpp \
    test/main.cpp

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
    include/bitcoin/database/block_database.hpp \
    include/bitcoin/database/data_base.hpp \
    include/bitcoin/database/define.hpp \
    include/bitcoin/database/history_database.hpp \
    include/bitcoin/database/pointer_array_source.hpp \
    include/bitcoin/database/settings.hpp \
    include/bitcoin/database/spend_database.hpp \
    include/bitcoin/database/stealth_database.hpp \
    include/bitcoin/database/transaction_database.hpp \
    include/bitcoin/database/version.hpp

include_bitcoin_database_diskdir = ${includedir}/bitcoin/database/disk
include_bitcoin_database_disk_HEADERS = \
    include/bitcoin/database/disk/accessor.hpp \
    include/bitcoin/database/disk/allocator.hpp \
    include/bitcoin/database/disk/disk_array.hpp \
    include/bitcoin/database/disk/mmfile.hpp

include_bitcoin_database_impldir = ${includedir}/bitcoin/database/impl
include_bitcoin_database_impl_HEADERS = \
    include/bitcoin/database/impl/disk_array.ipp \
    include/bitcoin/database/impl/htdb_record.ipp \
    include/bitcoin/database/impl/htdb_record_list_item.ipp \
    include/bitcoin/database/impl/htdb_slab.ipp \
    include/bitcoin/database/impl/htdb_slab_list_item.ipp \
    include/bitcoin/database/impl/multimap_records.ipp \
    include/bitcoin/database/impl/remainder.ipp

include_bitcoin_database_recorddir = ${includedir}/bitcoin/database/record
include_bitcoin_database_record_HEADERS = \
    include/bitcoin/database/record/htdb_record.hpp \
    include/bitcoin/database/record/linked_records.hpp \
    include/bitcoin/database/record/multimap_records.hpp \
    include/bitcoin/database/record/record_manager.hpp

include_bitcoin_database_slabdir = ${includedir}/bitcoin/database/slab
include_bitcoin_database_slab_HEADERS = \
    include/bitcoin/database/slab/htdb_slab.hpp \
    include/bitcoin/database/slab/slab_manager.hpp


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

