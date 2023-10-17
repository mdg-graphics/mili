#------------------------------------------------------------------------------
# Host-config file for the TOSS 3 machines at LLNL
#------------------------------------------------------------------------------
#
# This file provides CMake with paths / details for:
#  C/C++/Fortran:   Intel classic 2021.6.0
# 
#------------------------------------------------------------------------------

#---------------------------------------
# Compilers
#---------------------------------------

set( MILI_SYS_TYPE toss_4_x86_64_ib CACHE STRING "" )
set( MILI_COMPILER_FAMILY intel-classic  CACHE STRING "" )

set( MILI_COMPILER_VERSION 2021.6.0 )

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

set( CMAKE_C_COMPILER "${MILI_COMPILER_PREFIX}/icc" CACHE PATH "" )
set( CMAKE_CXX_COMPILER "${MILI_COMPILER_PREFIX}/icpc" CACHE PATH "" )
set( CMAKE_Fortran_COMPILER "${MILI_COMPILER_PREFIX}/ifort" CACHE PATH "" )

set( ENABLE_MILI TRUE CACHE BOOL "" )
set( ENABLE_TAURUS TRUE CACHE BOOL "" )
set( ENABLE_EPRINTF TRUE CACHE BOOL "" )
set( ENABLE_XMILICS TRUE CACHE BOOL "" )
set( ENABLE_UTILITIES TRUE CACHE BOOL "" )