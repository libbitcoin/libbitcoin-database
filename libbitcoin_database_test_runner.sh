#!/bin/sh
###############################################################################
#  Copyright (c) 2014-2015 libbitcoin-database developers (see COPYING).
#
#         GENERATED SOURCE CODE, DO NOT EDIT EXCEPT EXPERIMENTALLY
#
###############################################################################

# Define tests and options.
#==============================================================================
BOOST_UNIT_TEST_OPTIONS=\
"--run_test=database_tests,hash_table_tests,structure_tests,data_base_tests "\
"--show_progress=no "\
"--detect_memory_leak=0 "\
"--report_level=no "\
"--build_info=yes"


# Run tests.
#==============================================================================
./test/libbitcoin_database_test ${BOOST_UNIT_TEST_OPTIONS}
