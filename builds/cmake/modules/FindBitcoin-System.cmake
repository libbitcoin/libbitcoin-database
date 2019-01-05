###############################################################################
#  Copyright (c) 2014-2019 libbitcoin-protocol developers (see COPYING).
#
#         GENERATED SOURCE CODE, DO NOT EDIT EXCEPT EXPERIMENTALLY
#
###############################################################################
# FindBitcoin
#
# Use this module by invoking find_package with the form::
#
#   find_package( Bitcoin-System
#     [version]              # Minimum version
#     [REQUIRED]             # Fail with error if bitcoin is not found
#   )
#
#   Defines the following for use:
#
#   bitcoin_system_FOUND                         - true if headers and requested libraries were found
#   bitcoin_system_INCLUDE_DIRS                  - include directories for bitcoin-system libraries
#   bitcoin_system_LIBRARY_DIRS                  - link directories for bitcoin-system libraries
#   bitcoin_system_LIBRARIES                     - bitcoin-system libraries to be linked
#   bitcoin_system_PKG                           - bitcoin-system pkg-config package specification.
#

if (MSVC)
    if ( Bitcoin-System_FIND_REQUIRED )
        set( _bitcoin_system_MSG_STATUS "SEND_ERROR" )
    else ()
        set( _bitcoin_system_MSG_STATUS "STATUS" )
    endif()

    set( bitcoin_system_FOUND false )
    message( ${_bitcoin_system_MSG_STATUS} "MSVC environment detection for 'bitcoin-system' not currently supported." )
else ()
    # required
    if ( Bitcoin-System_FIND_REQUIRED )
        set( _bitcoin_system_REQUIRED "REQUIRED" )
    endif()

    # quiet
    if ( Bitcoin-System_FIND_QUIETLY )
        set( _bitcoin_system_QUIET "QUIET" )
    endif()

    # modulespec
    if ( Bitcoin-System_FIND_VERSION_COUNT EQUAL 0 )
        set( _bitcoin_system_MODULE_SPEC "libbitcoin-system" )
    else ()
        if ( Bitcoin-System_FIND_VERSION_EXACT )
            set( _bitcoin_system_MODULE_SPEC_OP "=" )
        else ()
            set( _bitcoin_system_MODULE_SPEC_OP ">=" )
        endif()

        set( _bitcoin_system_MODULE_SPEC "libbitcoin-system ${_bitcoin_system_MODULE_SPEC_OP} ${Bitcoin-System_FIND_VERSION}" )
    endif()

    pkg_check_modules( bitcoin_system ${_bitcoin_system_REQUIRED} ${_bitcoin_system_QUIET} "${_bitcoin_system_MODULE_SPEC}" )
    set( bitcoin_system_PKG "${_bitcoin_system_MODULE_SPEC}" )
endif()
