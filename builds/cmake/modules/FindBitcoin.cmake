###############################################################################
#  Copyright (c) 2014-2018 libbitcoin-protocol developers (see COPYING).
#
#         GENERATED SOURCE CODE, DO NOT EDIT EXCEPT EXPERIMENTALLY
#
###############################################################################
# FindBitcoin
#
# Use this module by invoking find_package with the form::
#
#   find_package( Bitcoin
#     [version]              # Minimum version
#     [REQUIRED]             # Fail with error if bitcoin is not found
#   )
#
#   Defines the following for use:
#
#   bitcoin_FOUND                         - true if headers and requested libraries were found
#   bitcoin_INCLUDE_DIRS                  - include directories for bitcoin libraries
#   bitcoin_LIBRARY_DIRS                  - link directories for bitcoin libraries
#   bitcoin_LIBRARIES                     - bitcoin libraries to be linked
#   bitcoin_PKG                           - bitcoin pkg-config package specification.
#

if (MSVC)
    if ( Bitcoin_FIND_REQUIRED )
        set( _bitcoin_MSG_STATUS "SEND_ERROR" )
    else ()
        set( _bitcoin_MSG_STATUS "STATUS" )
    endif()

    set( bitcoin_FOUND false )
    message( ${_bitcoin_MSG_STATUS} "MSVC environment detection for 'bitcoin' not currently supported." )
else ()
    # required
    if ( Bitcoin_FIND_REQUIRED )
        set( _bitcoin_REQUIRED "REQUIRED" )
    endif()

    # quiet
    if ( Bitcoin_FIND_QUIETLY )
        set( _bitcoin_QUIET "QUIET" )
    endif()

    # modulespec
    if ( Bitcoin_FIND_VERSION_COUNT EQUAL 0 )
        set( _bitcoin_MODULE_SPEC "libbitcoin" )
    else ()
        if ( Bitcoin_FIND_VERSION_EXACT )
            set( _bitcoin_MODULE_SPEC_OP "=" )
        else ()
            set( _bitcoin_MODULE_SPEC_OP ">=" )
        endif()

        set( _bitcoin_MODULE_SPEC "libbitcoin ${_bitcoin_MODULE_SPEC_OP} ${Bitcoin_FIND_VERSION}" )
    endif()

    pkg_check_modules( bitcoin ${_bitcoin_REQUIRED} ${_bitcoin_QUIET} "${_bitcoin_MODULE_SPEC}" )
    set( bitcoin_PKG "${_bitcoin_MODULE_SPEC}" )
endif()
