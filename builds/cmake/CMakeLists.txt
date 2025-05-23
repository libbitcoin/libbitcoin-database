###############################################################################
#  Copyright (c) 2014-2025 libbitcoin-database developers (see COPYING).
#
#         GENERATED SOURCE CODE, DO NOT EDIT EXCEPT EXPERIMENTALLY
#
###############################################################################

# libbitcoin-database project configuration.
#------------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.18 FATAL_ERROR)

project(libbitcoin-database LANGUAGES C CXX)

enable_testing()

list( APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/modules" )
include(CheckIncludeFiles)
include(CheckSymbolExists)
include(CheckCXXCompilerFlag)
include(CheckCXXSourceCompiles)
include(CheckLinkerFlag)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if (MSVC)
  set( CANONICAL_LIB_NAME libbitcoin-database )
else ()
  set( CANONICAL_LIB_NAME bitcoin-database )
  find_package( PkgConfig REQUIRED )

  set( prefix "${CMAKE_PREFIX_PATH}" )
  set( exec_prefix "\${prefix}" )
  set( libdir "\${exec_prefix}/lib" )
  set( includedir "\${exec_prefix}/include" )

  set( CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib" )

  set( PACKAGE_VERSION "4.0.0" )
  set( VERSION "${PACKAGE_VERSION}" )
endif ()

set( CMAKE_CXX_STANDARD 20 )
set( CMAKE_CXX_STANDARD_REQUIRED ON )

# Add compiler options
#------------------------------------------------------------------------------
# Warn on all stuff.
check_cxx_compiler_flag( "-Wall" HAS_FLAG_WALL )
if ( HAS_FLAG_WALL )
    add_compile_options( $<$<COMPILE_LANGUAGE:CXX>:-Wall> )
else()
    message( FATAL_ERROR "Compiler does not support -Wall" )
endif()

# Warn on extra stuff.
check_cxx_compiler_flag( "-Wextra" HAS_FLAG_WEXTRA )
if ( HAS_FLAG_WEXTRA )
    add_compile_options( $<$<COMPILE_LANGUAGE:CXX>:-Wextra> )
else()
    message( FATAL_ERROR "Compiler does not support -Wextra" )
endif()

# Disallow warning on style order of declarations.
check_cxx_compiler_flag( "-Wno-reorder" HAS_FLAG_WNO-REORDER )
if ( HAS_FLAG_WNO-REORDER )
    add_compile_options( $<$<COMPILE_LANGUAGE:CXX>:-Wno-reorder> )
else()
    message( FATAL_ERROR "Compiler does not support -Wno-reorder" )
endif()

# Suppress warning for incomplete field initialization.
check_cxx_compiler_flag( "-Wno-missing-field-initializers" HAS_FLAG_WNO-MISSING-FIELD-INITIALIZERS )
if ( HAS_FLAG_WNO-MISSING-FIELD-INITIALIZERS )
    add_compile_options( $<$<COMPILE_LANGUAGE:CXX>:-Wno-missing-field-initializers> )
else()
    message( FATAL_ERROR "Compiler does not support -Wno-missing-field-initializers" )
endif()

# Conform to style.
check_cxx_compiler_flag( "-Wno-missing-braces" HAS_FLAG_WNO-MISSING-BRACES )
if ( HAS_FLAG_WNO-MISSING-BRACES )
    add_compile_options( $<$<COMPILE_LANGUAGE:CXX>:-Wno-missing-braces> )
else()
    message( FATAL_ERROR "Compiler does not support -Wno-missing-braces" )
endif()

# Ignore comments within comments or commenting of backslash extended lines.
check_cxx_compiler_flag( "-Wno-comment" HAS_FLAG_WNO-COMMENT )
if ( HAS_FLAG_WNO-COMMENT )
    add_compile_options( $<$<COMPILE_LANGUAGE:CXX>:-Wno-comment> )
else()
    message( FATAL_ERROR "Compiler does not support -Wno-comment" )
endif()

# Suppress warning for copy of implicitly generated copy constructor.
check_cxx_compiler_flag( "-Wno-deprecated-copy" HAS_FLAG_WNO-DEPRECATED-COPY )
if ( HAS_FLAG_WNO-DEPRECATED-COPY )
    add_compile_options( $<$<COMPILE_LANGUAGE:CXX>:-Wno-deprecated-copy> )
else()
    message( FATAL_ERROR "Compiler does not support -Wno-deprecated-copy" )
endif()

# Conflict in stdlib under clang.
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    check_cxx_compiler_flag( "-Wno-mismatched-tags" HAS_FLAG_WNO-MISMATCHED-TAGS )
    if ( HAS_FLAG_WNO-MISMATCHED-TAGS )
        add_compile_options( $<$<COMPILE_LANGUAGE:CXX>:-Wno-mismatched-tags> )
    else()
        message( FATAL_ERROR "Compiler does not support -Wno-mismatched-tags" )
    endif()
endif()

# Implement -Dpkgconfigdir and output ${pkgconfigdir}.
#------------------------------------------------------------------------------
set( pkgconfigdir "${libdir}/pkgconfig" CACHE PATH "Path to pkgconfig directory." )

# Implement -Dwith-tests and declare with-tests.
#------------------------------------------------------------------------------
set( with-tests "yes" CACHE BOOL "Compile with unit tests." )

# Implement -Dwith-tools and declare with-tools.
#------------------------------------------------------------------------------
set( with-tools "yes" CACHE BOOL "Compile with tools." )

# Implement -Denable-ndebug and define NDEBUG.
#------------------------------------------------------------------------------
set( enable-ndebug "yes" CACHE BOOL "Compile without debug assertions." )

if (enable-ndebug)
    add_definitions( -DNDEBUG )
endif()

# Inherit -Denable-shared and define BOOST_ALL_DYN_LINK.
#------------------------------------------------------------------------------
if (BUILD_SHARED_LIBS)
    add_definitions( -DBOOST_ALL_DYN_LINK )
endif()

if (BUILD_SHARED_LIBS)
    set( Boost_USE_STATIC_LIBS "off" )
else()
    set( Boost_USE_STATIC_LIBS "on" )
endif()

# Find boost
#------------------------------------------------------------------------------
find_package( Boost 1.76.0 REQUIRED COMPONENTS
    unit_test_framework )

set( boost_unit_test_framework_LIBS "-lboost_unit_test_framework" )

set( Boost_LIBRARY_DIR $<IF:$<BOOL:${enable-ndebug}>,"${Boost_LIBRARY_DIR_RELEASE}","${Boost_LIBRARY_DIR_DEBUG}">)

set( boost_CPPFLAGS "-I${Boost_INCLUDE_DIR}" )
set( boost_LDFLAGS "-L${Boost_LIBRARY_DIR}" )

# Find bitcoin-system
#------------------------------------------------------------------------------
find_package( Bitcoin-System 4.0.0 REQUIRED )

# Define project common includes for build.
#------------------------------------------------------------------------------
if (BUILD_SHARED_LIBS)
    set( bitcoin_system_FOR_BUILD_INCLUDE_DIRS ${bitcoin_system_INCLUDE_DIRS} CACHE STRING "Placeholder" )
else()
    set( bitcoin_system_FOR_BUILD_INCLUDE_DIRS ${bitcoin_system_STATIC_INCLUDE_DIRS} CACHE STRING "Placeholder" )
endif()

# Define project common includes directories
#------------------------------------------------------------------------------
include_directories( SYSTEM
    ${bitcoin_system_FOR_BUILD_INCLUDE_DIRS} )

# Define project common library directories for build.
#------------------------------------------------------------------------------
if (BUILD_SHARED_LIBS)
    set( bitcoin_system_FOR_BUILD_LIBRARY_DIRS ${bitcoin_system_LIBRARY_DIRS} CACHE STRING "Placeholder" )
else()
    set( bitcoin_system_FOR_BUILD_LIBRARY_DIRS ${bitcoin_system_STATIC_LIBRARY_DIRS} CACHE STRING "Placeholder" )
endif()

# Define project common library directories
#------------------------------------------------------------------------------
link_directories(
    ${bitcoin_system_FOR_BUILD_LIBRARY_DIRS} )

# Define project common linker flags.
#------------------------------------------------------------------------------
check_linker_flag(CXX "-no_fixup_chains" "no_fixup_chains_FOUND")

if (no_fixup_chains_FOUND)
    add_link_options("-no_fixup_chains")
endif()

check_linker_flag(CXX "-fstack-protector" "fstack_protector_FOUND")

if (fstack_protector_FOUND)
    add_link_options("-fstack-protector")
endif()

check_linker_flag(CXX "-fstack-protector-all" "fstack_protector_all_FOUND")

if (fstack_protector_all_FOUND)
    add_link_options("-fstack-protector-all")
endif()


# Define common library usage for build.
#------------------------------------------------------------------------------
if (BUILD_SHARED_LIBS)
    set( bitcoin_system_FOR_BUILD_LIBRARIES ${bitcoin_system_LIBRARIES} CACHE STRING "Placeholder" )
else()
    set( bitcoin_system_FOR_BUILD_LIBRARIES ${bitcoin_system_STATIC_LIBRARIES} CACHE STRING "Placeholder" )
endif()

# Define project common libraries.
#------------------------------------------------------------------------------
link_libraries(
    ${bitcoin_system_FOR_BUILD_LIBRARIES} )

# Define ${CANONICAL_LIB_NAME} project.
#------------------------------------------------------------------------------
add_library( ${CANONICAL_LIB_NAME}
    "../../src/error.cpp"
    "../../src/settings.cpp"
    "../../src/file/rotator.cpp"
    "../../src/file/utilities.cpp"
    "../../src/locks/file_lock.cpp"
    "../../src/locks/flush_lock.cpp"
    "../../src/locks/interprocess_lock.cpp"
    "../../src/memory/map.cpp"
    "../../src/memory/utilities.cpp"
    "../../src/memory/mman-win32/mman.cpp"
    "../../src/memory/mman-win32/mman.hpp" )

# ${CANONICAL_LIB_NAME} project specific include directory normalization for build.
#------------------------------------------------------------------------------
if (BUILD_SHARED_LIBS)
    set( bitcoin_system_FOR_BUILD_INCLUDE_DIRS ${bitcoin_system_INCLUDE_DIRS} CACHE STRING "Placeholder" )
else()
    set( bitcoin_system_FOR_BUILD_INCLUDE_DIRS ${bitcoin_system_STATIC_INCLUDE_DIRS} CACHE STRING "Placeholder" )
endif()

# ${CANONICAL_LIB_NAME} project specific include directories.
#------------------------------------------------------------------------------
target_include_directories( ${CANONICAL_LIB_NAME} PRIVATE
    "../../include"
    ${bitcoin_system_FOR_BUILD_INCLUDE_DIRS} )

target_include_directories( ${CANONICAL_LIB_NAME} PUBLIC
"../../include" )

# ${CANONICAL_LIB_NAME} project specific libraries noramalization for build.
#------------------------------------------------------------------------------
if (BUILD_SHARED_LIBS)
    set( bitcoin_system_FOR_BUILD_LIBRARIES ${bitcoin_system_LIBRARIES} CACHE STRING "Placeholder" )
else()
    set( bitcoin_system_FOR_BUILD_LIBRARIES ${bitcoin_system_STATIC_LIBRARIES} CACHE STRING "Placeholder" )
endif()

# ${CANONICAL_LIB_NAME} project specific libraries/linker flags.
#------------------------------------------------------------------------------
target_link_libraries( ${CANONICAL_LIB_NAME}
    ${bitcoin_system_FOR_BUILD_LIBRARIES} )

# Define libbitcoin-database-test project.
#------------------------------------------------------------------------------
if (with-tests)
    add_executable( libbitcoin-database-test
        "../../test/error.cpp"
        "../../test/main.cpp"
        "../../test/settings.cpp"
        "../../test/store.cpp"
        "../../test/test.cpp"
        "../../test/test.hpp"
        "../../test/file/rotator.cpp"
        "../../test/file/utilities.cpp"
        "../../test/locks/file_lock.cpp"
        "../../test/locks/flush_lock.cpp"
        "../../test/locks/interprocess_lock.cpp"
        "../../test/memory/accessor.cpp"
        "../../test/memory/map.cpp"
        "../../test/memory/utilities.cpp"
        "../../test/mocks/blocks.cpp"
        "../../test/mocks/blocks.hpp"
        "../../test/mocks/chunk_storage.cpp"
        "../../test/mocks/chunk_storage.hpp"
        "../../test/mocks/chunk_store.hpp"
        "../../test/mocks/map_store.hpp"
        "../../test/primitives/arrayhead.cpp"
        "../../test/primitives/arraymap.cpp"
        "../../test/primitives/hashhead.cpp"
        "../../test/primitives/hashmap.cpp"
        "../../test/primitives/iterator.cpp"
        "../../test/primitives/keys.cpp"
        "../../test/primitives/linkage.cpp"
        "../../test/primitives/manager.cpp"
        "../../test/primitives/nomap.cpp"
        "../../test/query/archive_read.cpp"
        "../../test/query/archive_write.cpp"
        "../../test/query/confirm.cpp"
        "../../test/query/consensus.cpp"
        "../../test/query/context.cpp"
        "../../test/query/extent.cpp"
        "../../test/query/height.cpp"
        "../../test/query/initialize.cpp"
        "../../test/query/objects.cpp"
        "../../test/query/optional.cpp"
        "../../test/query/translate.cpp"
        "../../test/query/validate.cpp"
        "../../test/tables/archives/header.cpp"
        "../../test/tables/archives/input.cpp"
        "../../test/tables/archives/output.cpp"
        "../../test/tables/archives/point.cpp"
        "../../test/tables/archives/puts.cpp"
        "../../test/tables/archives/transaction.cpp"
        "../../test/tables/archives/txs.cpp"
        "../../test/tables/caches/duplicate.cpp"
        "../../test/tables/caches/prevout.cpp"
        "../../test/tables/caches/validated_bk.cpp"
        "../../test/tables/caches/validated_tx.cpp"
        "../../test/tables/indexes/height.cpp"
        "../../test/tables/indexes/strong_tx.cpp"
        "../../test/tables/optional/address.cpp"
        "../../test/tables/optional/filter_bk.cpp"
        "../../test/tables/optional/filter_tx.cpp" )

    add_test( NAME libbitcoin-database-test COMMAND libbitcoin-database-test
            --run_test=*
            --log_level=warning
            --show_progress=no
            --detect_memory_leak=0
            --report_level=no
            --build_info=yes )

#     libbitcoin-database-test project specific include directories.
#------------------------------------------------------------------------------
    target_include_directories( libbitcoin-database-test PRIVATE
        "../../include" )

#     libbitcoin-database-test project specific libraries/linker flags.
#------------------------------------------------------------------------------
    target_link_libraries( libbitcoin-database-test
        ${CANONICAL_LIB_NAME}
        ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY} )

endif()

# Define initchain project.
#------------------------------------------------------------------------------
if (with-tools)
    add_executable( initchain
        "../../tools/initchain/initchain.cpp" )

#     initchain project specific include directories.
#------------------------------------------------------------------------------
    target_include_directories( initchain PRIVATE
        "../../include" )

#     initchain project specific libraries/linker flags.
#------------------------------------------------------------------------------
    target_link_libraries( initchain
        ${CANONICAL_LIB_NAME} )

endif()

# Manage pkgconfig installation.
#------------------------------------------------------------------------------
configure_file(
  "../../libbitcoin-database.pc.in"
  "libbitcoin-database.pc" @ONLY )

install( FILES
    "${CMAKE_CURRENT_BINARY_DIR}/libbitcoin-database.pc"
    DESTINATION "${pkgconfigdir}" )

# Manage installation of docs.
#------------------------------------------------------------------------------
install( FILES
    "../../AUTHORS"
    "../../COPYING"
    "../../ChangeLog"
    "../../INSTALL"
    "../../NEWS"
    "../../README"
    DESTINATION share/doc/libbitcoin-database )

# Manage ${CANONICAL_LIB_NAME} installation.
#------------------------------------------------------------------------------
install( TARGETS ${CANONICAL_LIB_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    PUBLIC_HEADER DESTINATION include )

# Manage include installation.
#------------------------------------------------------------------------------
install( DIRECTORY "../../include/bitcoin"
    DESTINATION include )

