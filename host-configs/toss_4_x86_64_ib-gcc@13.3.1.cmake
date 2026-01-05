#------------------------------------------------------------------------------
# Host-config file for the TOSS 4 machines at LLNL
#------------------------------------------------------------------------------
#
# This file provides CMake with paths / details for:
#  C/C++/Fortran:   GCC 13.3.1
#
#------------------------------------------------------------------------------

#---------------------------------------
# Compilers
#---------------------------------------

set( MILI_SYS_TYPE toss_4_x86_64_ib CACHE STRING "" )
set( MILI_COMPILER_FAMILY gcc CACHE STRING "" )

set( MILI_COMPILER_VERSION 13.3.1 CACHE STRING "" )

set( MILI_COMPILER_NAME ${MILI_COMPILER_FAMILY}-${MILI_COMPILER_VERSION} CACHE STRING "" )
set( MILI_PREFER_STATIC TRUE CACHE BOOL "" )

if( DEFINED ENV{SYS_TYPE} )
  set( ENV_SYS_TYPE $ENV{SYS_TYPE} )
  if( NOT "${ENV_SYS_TYPE}" STREQUAL "${MILI_SYS_TYPE}" )
    message( WARNING "SYS_TYPE environment variable set as '${ENV_SYS_TYPE}' for host-config file desgigned for '${MILI_SYS_TYPE}', attempting to config with '${ENV_SYS_TYPE}'!")
  endif( )
  set( MILI_SYS_TYPE ${ENV_SYS_TYPE} )
endif( )

set( MILI_COMPILER_PREFIX "/usr/tce/packages/${MILI_COMPILER_FAMILY}/${MILI_COMPILER_NAME}/bin" CACHE PATH "" )

set( CMAKE_C_COMPILER "${MILI_COMPILER_PREFIX}/gcc" CACHE PATH "" )
set( CMAKE_CXX_COMPILER "${MILI_COMPILER_PREFIX}/g++" CACHE PATH "" )
set( CMAKE_Fortran_COMPILER "${MILI_COMPILER_PREFIX}/gfortran" CACHE PATH "" )

#set(CMAKE_C_FLAGS "-mcmodel=medium -fPIC" CACHE PATH "")
#set(CMAKE_CXX_FLAGS "-mcmodel=medium -fPIC" CACHE PATH "")
#set(CMAKE_Fortran_FLAGS "-mcmodel=medium -fPIC -std=legacy" CACHE PATH "")
set(CMAKE_C_FLAGS "-fPIC" CACHE PATH "")
set(CMAKE_CXX_FLAGS "-fPIC" CACHE PATH "")
set(CMAKE_Fortran_FLAGS "-fPIC -std=legacy" CACHE PATH "")

set( DOD_BUILD FALSE CACHE BOOL "" )

set( ENABLE_MILI TRUE CACHE BOOL "Turn on/off building of Mili Library" )
set( ENABLE_TAURUS TRUE CACHE BOOL "Turn on/off building of Taurus Library" )
set( ENABLE_EPRINTF TRUE CACHE BOOL "Turn on/off building of Extended printf Library" )
set( ENABLE_XMILICS TRUE CACHE BOOL "Turn on/off building of Xmilics" )
set( ENABLE_UTILITIES TRUE CACHE BOOL "Turn on/off building of Mili Utilities (md, tipart, MiliReader, ti_strings, makemili_driver)" )
