#------------------------------------------------------------------------------
# Default Host-config file
#------------------------------------------------------------------------------

# Check for CC and FC environment variables to set compilers
if( DEFINED ENV{CC} OR DEFINED CC  )
    if( DEFINED ENV{CC} )
        set( CMAKE_C_COMPILER $ENV{CC} CACHE PATH "" )
    else()
        set( CMAKE_C_COMPILER ${CC} CACHE PATH "" )
    endif()
endif()

if( DEFINED ENV{FC} OR DEFINED FC  )
    if( DEFINED ENV{FC} )
        set( CMAKE_Fortran_COMPILER $ENV{FC} CACHE PATH "" )
    else()
        set( CMAKE_Fortran_COMPILER ${FC} CACHE PATH "" )
    endif()
endif()

set( ENABLE_MILI TRUE CACHE BOOL "Turn on/off building of Mili Library" )
set( ENABLE_TAURUS TRUE CACHE BOOL "Turn on/off building of Taurus Library" )
set( ENABLE_EPRINTF TRUE CACHE BOOL "Turn on/off building of Extended printf Library" )
set( ENABLE_XMILICS TRUE CACHE BOOL "Turn on/off building of Xmilics" )
set( ENABLE_UTILITIES TRUE CACHE BOOL "Turn on/off building of Mili Utilities (md, tipart, MiliReader, ti_strings, makemili_driver)" )